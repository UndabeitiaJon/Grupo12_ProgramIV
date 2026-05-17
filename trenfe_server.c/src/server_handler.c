/*
 * server_handler.c
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */
/*
 * server_handler.c  -  Sistema TRENFE  -  Fase 2
 *
 * Implementa el bucle de sesión y todos los comandos del protocolo.
 *
 * Estrategia de acceso a datos:
 *   - Para comandos que devuelven un único registro se reutilizan las
 *     funciones de db_manager (obtener_usuario_por_email, insertar_reserva_db…).
 *   - Para comandos que listan filas se abre SQLite directamente con
 *     cfg.db_path y se envían las filas una a una por el socket, terminando
 *     con FIN_LISTA. Así no hay que tocar db_manager.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server_handler.h"
#include "server_socket.h"
#include "protocolo.h"
#include "config.h"
#include "logs.h"
#include "db_manager.h"
#include "sqlite3.h"
#include "server_handler_admin.h"
#include "tipos_comunes.h"
#include "usuario.h"
#include "trayecto.h"
#include "reserva.h"
#include "estacion.h"
#include "servicio.h"

/* ══════════════════════════════════════════════════════════════
   ESTADO DE SESIÓN  (local a cada llamada de manejar_cliente)
   ══════════════════════════════════════════════════════════════ */
typedef struct {
    sock_t      fd;
    int         id_u;
    char        email[128];
    char        rol[16];       /* "PASAJERO", "MAQUINISTA", "ADMIN" */
    char        ip[46];
} Sesion;

/* ══════════════════════════════════════════════════════════════
   HELPERS INTERNOS
   ══════════════════════════════════════════════════════════════ */

/* Abre la BD del servidor y devuelve el puntero, o NULL si falla. */
static sqlite3 *abrir_db(sock_t fd) {
    sqlite3 *db;
    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Base de datos no disponible");
        return NULL;
    }
    return db;
}

/* Devuelve el texto de la columna o "" si es NULL. */
static const char *col_txt(sqlite3_stmt *s, int i) {
    const unsigned char *v = sqlite3_column_text(s, i);
    return v ? (const char *)v : "";
}

/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 2 — AUTENTICACIÓN
   ══════════════════════════════════════════════════════════════ */

/*
 * handle_login()
 *
 * Recibe "LOGIN|email|pass_hash" ya parseado (param apunta a "email|pass_hash").
 * Responde:
 *   AUTH_OK|id_u|rol|nombre
 *   AUTH_FAIL|mensaje
 */
static void handle_login(sock_t fd, char *param, Sesion *ses) {
    if (!param) {
        enviar_mensaje(fd, "AUTH_FAIL|Faltan credenciales");
        return;
    }

    /* Separar email y pass_hash */
    char *email     = strtok(param, SEP);
    char *pass_hash = strtok(NULL,  SEP);

    if (!email || !pass_hash) {
        enviar_mensaje(fd, "AUTH_FAIL|Formato incorrecto (LOGIN|email|pass_hash)");
        return;
    }

    /* verificar_usuario compara el hash directamente */
    if (!verificar_usuario(email, pass_hash)) {
        enviar_fmt(fd, "AUTH_FAIL|Credenciales incorrectas para %s", email);
        return;
    }

    /* Obtener datos del usuario */
    Usuario u = obtener_usuario_por_email(email);
    if (u.id_u <= 0) {
        enviar_mensaje(fd, "AUTH_FAIL|Usuario no encontrado");
        return;
    }

    /* Guardar estado de sesión */
    ses->id_u = u.id_u;
    strncpy(ses->email, u.email, sizeof(ses->email) - 1);
    strncpy(ses->rol,   u.rol == ROL_ADMIN     ? "ADMIN"     :
                        u.rol == ROL_EMPLEADO  ? "MAQUINISTA": "PASAJERO",
            sizeof(ses->rol) - 1);

    enviar_fmt(fd, "AUTH_OK|%d|%s|%s", u.id_u, ses->rol, u.nombre);

    /* Log */
    char msg[256];
    snprintf(msg, sizeof(msg), "Login correcto – rol=%s ip=%s", ses->rol, ses->ip);
    log_evento(cfg.log_path, email, "AUTH_OK", msg);
}

/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 3 — TRAYECTOS
   ══════════════════════════════════════════════════════════════ */

/*
 * handle_listar_trayectos()
 *
 * Envía una fila por trayecto activo:
 *   TRAYECTO|id|origen|destino|hora_sal|hora_ll|precio|estado
 * Termina con FIN_LISTA|n
 */
static void handle_listar_trayectos(sock_t fd) {
    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT t.id_tr, eo.nombre, ed.nombre,"
        "       t.hora_salida, t.hora_llegada, t.precio_base, t.estado"
        " FROM TRAYECTOS t"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " WHERE t.estado = 'ACTIVO'"
        " ORDER BY t.id_tr;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error en consulta de trayectos");
        sqlite3_close(db);
        return;
    }

    int n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        enviar_fmt(fd, "TRAYECTO|%d|%s|%s|%s|%s|%.2f|%s",
                   sqlite3_column_int(stmt, 0),
                   col_txt(stmt, 1), col_txt(stmt, 2),
                   col_txt(stmt, 3), col_txt(stmt, 4),
                   sqlite3_column_double(stmt, 5),
                   col_txt(stmt, 6));
        n++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

/*
 * handle_buscar_trayecto()
 *
 * param: "id_origen|id_destino|fecha|clase"
 * Envía los trayectos que operan en esa fecha y tienen asientos libres.
 */
static void handle_buscar_trayecto(sock_t fd, char *param) {
    char *s_orig  = strtok(param, SEP);
    char *s_dest  = strtok(NULL,  SEP);
    char *fecha   = strtok(NULL,  SEP);
    char *clase   = strtok(NULL,  SEP);

    if (!s_orig || !s_dest || !fecha || !clase) {
        enviar_mensaje(fd, "ERROR|400|Formato: BUSCAR_TRAYECTO|id_orig|id_dest|fecha|clase");
        return;
    }

    int id_orig = atoi(s_orig);
    int id_dest = atoi(s_dest);

    /* Obtener el día de la semana de la fecha pedida (0=L…6=D) */
    /* Usamos SQLite para no depender de mktime en Windows */
    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    /* dias_operacion es p.ej. "LMXJVSD"; strftime %w: 0=dom,1=lun…6=sab */
    const char *sql =
        "SELECT t.id_tr, eo.nombre, ed.nombre,"
        "       t.hora_salida, t.hora_llegada, t.precio_base, t.dias_operacion"
        " FROM TRAYECTOS t"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " WHERE t.id_est_origen  = ?1"
        "   AND t.id_est_destino = ?2"
        "   AND t.estado = 'ACTIVO'"
        " ORDER BY t.hora_salida;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error en busqueda de trayectos");
        sqlite3_close(db);
        return;
    }
    sqlite3_bind_int(stmt, 1, id_orig);
    sqlite3_bind_int(stmt, 2, id_dest);

    int n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int    id_tr  = sqlite3_column_int(stmt, 0);
        double precio = sqlite3_column_double(stmt, 5);
        int    libres = contar_asientos_libres(id_tr, fecha, 1, clase);

        enviar_fmt(fd, "TRAYECTO|%d|%s|%s|%s|%s|%.2f|%d|%s",
                   id_tr,
                   col_txt(stmt, 1), col_txt(stmt, 2),
                   col_txt(stmt, 3), col_txt(stmt, 4),
                   precio, libres,
                   col_txt(stmt, 6));
        n++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

/*
 * handle_detalle_trayecto()
 *
 * param: "id_tr"
 * Responde con los datos completos del trayecto y sus paradas intermedias.
 */
static void handle_detalle_trayecto(sock_t fd, char *param) {
    if (!param) {
        enviar_mensaje(fd, "ERROR|400|Falta id_tr");
        return;
    }
    int id_tr = atoi(param);
    Trayecto tr = obtener_trayecto_por_id(id_tr);
    if (tr.id_tr <= 0) {
        enviar_fmt(fd, "ERROR|404|Trayecto %d no encontrado", id_tr);
        return;
    }

    Estacion orig = obtener_estacion_por_id(tr.id_est_origen);
    Estacion dest = obtener_estacion_por_id(tr.id_est_destino);

    enviar_fmt(fd, "DETALLE|%d|%s|%s|%s|%s|%d|%.2f|%s",
               tr.id_tr,
               orig.nombre, dest.nombre,
               tr.hora_salida, tr.hora_llegada,
               tr.duracion_min, tr.precio_base,
               tr.dias_operacion);

    /* Paradas intermedias */
    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT p.orden, e.nombre, p.hora_llegada, p.hora_salida"
        " FROM PARADAS_INTERMEDIAS p"
        " JOIN ESTACIONES e ON p.id_est = e.id_est"
        " WHERE p.id_tr = ?"
        " ORDER BY p.orden;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id_tr);
        int np = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            enviar_fmt(fd, "PARADA|%d|%s|%s|%s",
                       sqlite3_column_int(stmt, 0),
                       col_txt(stmt, 1),
                       col_txt(stmt, 2),
                       col_txt(stmt, 3));
            np++;
        }
        sqlite3_finalize(stmt);
        enviar_fmt(fd, "FIN_LISTA|%d", np);
    } else {
        enviar_mensaje(fd, "FIN_LISTA|0");
    }
    sqlite3_close(db);
}

/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 3 — ESTACIONES
   ══════════════════════════════════════════════════════════════ */

/*
 * handle_listar_estaciones()
 *
 * Envía todas las estaciones:
 *   ESTACION|id|nombre|ciudad|provincia|andenes|sala_club
 */
static void handle_listar_estaciones(sock_t fd) {
    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT id_est, nombre, ciudad, provincia, num_andenes, tiene_sala_club"
        " FROM ESTACIONES ORDER BY ciudad, nombre;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error en consulta de estaciones");
        sqlite3_close(db);
        return;
    }

    int n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        enviar_fmt(fd, "ESTACION|%d|%s|%s|%s|%d|%d",
                   sqlite3_column_int(stmt, 0),
                   col_txt(stmt, 1), col_txt(stmt, 2), col_txt(stmt, 3),
                   sqlite3_column_int(stmt, 4),
                   sqlite3_column_int(stmt, 5));
        n++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 3 — RESERVAS (Pasajero)
   ══════════════════════════════════════════════════════════════ */

/*
 * handle_hacer_reserva()
 *
 * param: "id_u|id_tr|fecha|clase|vagon|asiento|tipo_eq|peso_eq"
 * Responde: OK|id_res|precio_final|codigo_validacion
 */
static void handle_hacer_reserva(sock_t fd, char *param, Sesion *ses) {
    char *s_id_u    = strtok(param, SEP);
    char *s_id_tr   = strtok(NULL,  SEP);
    char *fecha     = strtok(NULL,  SEP);
    char *clase     = strtok(NULL,  SEP);
    char *s_vagon   = strtok(NULL,  SEP);
    char *s_asiento = strtok(NULL,  SEP);
    char *s_tipo_eq = strtok(NULL,  SEP);
    char *s_peso_eq = strtok(NULL,  SEP);

    if (!s_id_u || !s_id_tr || !fecha || !clase || !s_vagon || !s_asiento) {
        enviar_mensaje(fd, "ERROR|400|Faltan parametros en HACER_RESERVA");
        return;
    }

    int    id_u    = atoi(s_id_u);
    int    id_tr   = atoi(s_id_tr);
    int    vagon   = atoi(s_vagon);
    int    asiento = atoi(s_asiento);

    /* Validar que el asiento esté libre */
    if (!asiento_libre(id_tr, fecha, vagon, asiento)) {
        enviar_mensaje(fd, "ERROR|409|Asiento ya ocupado");
        return;
    }

    /* Calcular suplemento de equipaje */
    double sup_equipaje = 0.0;
    if (s_tipo_eq && s_peso_eq) {
        TipoEquipaje tipo_eq = EQUIPAJE_MANO;
        if (strcmp(s_tipo_eq, "BODEGA") == 0) tipo_eq = EQUIPAJE_BODEGA;
        else if (strcmp(s_tipo_eq, "BICI") == 0) tipo_eq = EQUIPAJE_BICI;
        else if (strcmp(s_tipo_eq, "ESQUI") == 0) tipo_eq = EQUIPAJE_ESQUI;
        sup_equipaje = calcular_suplemento_equipaje(tipo_eq, atof(s_peso_eq), clase);
    }

    /* Obtener descuento del pasajero */
    TipoDescuento desc = obtener_descuento_usuario(id_u);

    /* Calcular precio final */
    double precio_final = calcular_precio_final(id_tr, clase, desc, sup_equipaje);

    /* Rellenar struct Reserva */
    Reserva r;
    memset(&r, 0, sizeof(r));
    r.id_u        = id_u;
    r.id_tr       = id_tr;
    r.num_vagon   = vagon;
    r.num_asiento = asiento;
    r.precio_base = precio_final;   /* calcular_precio_final ya aplica descuento */
    r.precio_final = precio_final;
    strncpy(r.clase,       clase, sizeof(r.clase) - 1);
    strncpy(r.fecha_viaje, fecha, sizeof(r.fecha_viaje) - 1);
    generar_codigo_validacion(r.codigo_validacion, sizeof(r.codigo_validacion));

    int id_res = insertar_reserva_db(r);
    if (id_res <= 0) {
        enviar_mensaje(fd, "ERROR|500|No se pudo crear la reserva");
        return;
    }

    /* Insertar equipaje si lo hay */
    if (s_tipo_eq && s_peso_eq && sup_equipaje > 0.0) {
        Equipaje eq;
        memset(&eq, 0, sizeof(eq));
        eq.id_res     = id_res;
        eq.peso_kg    = atof(s_peso_eq);
        eq.suplemento_pago = sup_equipaje;
        if (strcmp(s_tipo_eq, "BODEGA") == 0)      eq.tipo = EQUIPAJE_BODEGA;
        else if (strcmp(s_tipo_eq, "BICI") == 0)   eq.tipo = EQUIPAJE_BICI;
        else if (strcmp(s_tipo_eq, "ESQUI") == 0)  eq.tipo = EQUIPAJE_ESQUI;
        else                                         eq.tipo = EQUIPAJE_MANO;
        insertar_equipaje_db(eq);
    }

    enviar_fmt(fd, "OK|%d|%.2f|%s", id_res, precio_final, r.codigo_validacion);

    /* Log */
    char msg[128];
    snprintf(msg, sizeof(msg), "Reserva creada id_res=%d trayecto=%d fecha=%s",
             id_res, id_tr, fecha);
    log_evento(cfg.log_path, ses->email, "RESERVA", msg);
}

/*
 * handle_cancelar_reserva()
 *
 * param: "id_res|id_u"
 */
static void handle_cancelar_reserva(sock_t fd, char *param, Sesion *ses) {
    char *s_id_res = strtok(param, SEP);
    char *s_id_u   = strtok(NULL,  SEP);

    if (!s_id_res || !s_id_u) {
        enviar_mensaje(fd, "ERROR|400|Formato: CANCELAR_RESERVA|id_res|id_u");
        return;
    }

    int id_res = atoi(s_id_res);
    int id_u   = atoi(s_id_u);

    int rc = cancelar_reserva_db(id_res, id_u);
    if (rc == 0) {
        enviar_fmt(fd, "OK|Reserva %d cancelada", id_res);
        char msg[64];
        snprintf(msg, sizeof(msg), "Reserva cancelada id_res=%d", id_res);
        log_evento(cfg.log_path, ses->email, "CANCELACION", msg);
    } else {
        enviar_mensaje(fd, "ERROR|404|Reserva no encontrada o no pertenece al usuario");
    }
}

/*
 * handle_mis_reservas()
 *
 * param: "id_u"
 * Envía una línea por reserva:
 *   RESERVA|id_res|origen|destino|fecha|clase|vagon|asiento|precio|estado|cod
 */
static void handle_mis_reservas(sock_t fd, char *param) {
    if (!param) {
        enviar_mensaje(fd, "ERROR|400|Falta id_u");
        return;
    }
    int id_u = atoi(param);

    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT r.id_res, eo.nombre, ed.nombre, r.fecha_viaje,"
        "       r.clase, r.num_vagon, r.num_asiento, r.precio_final,"
        "       r.estado, r.codigo_validacion"
        " FROM RESERVAS r"
        " JOIN TRAYECTOS t  ON r.id_tr = t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " WHERE r.id_u = ?"
        " ORDER BY r.fecha_viaje DESC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error en consulta de reservas");
        sqlite3_close(db);
        return;
    }
    sqlite3_bind_int(stmt, 1, id_u);

    int n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        enviar_fmt(fd, "RESERVA|%d|%s|%s|%s|%s|%d|%d|%.2f|%s|%s",
                   sqlite3_column_int(stmt, 0),
                   col_txt(stmt, 1), col_txt(stmt, 2),
                   col_txt(stmt, 3), col_txt(stmt, 4),
                   sqlite3_column_int(stmt, 5),
                   sqlite3_column_int(stmt, 6),
                   sqlite3_column_double(stmt, 7),
                   col_txt(stmt, 8),
                   col_txt(stmt, 9));
        n++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

/*
 * handle_historial()
 *
 * param: "id_u"
 * Igual que mis_reservas pero incluye también las canceladas/completadas.
 */
static void handle_historial(sock_t fd, char *param) {
    /* Reutilizamos la misma query — mis_reservas ya devuelve todas las reservas */
    handle_mis_reservas(fd, param);
}

/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 3 — PUNTOS FIDELIDAD
   ══════════════════════════════════════════════════════════════ */

/*
 * handle_mis_puntos()
 *
 * param: "id_u"
 * Responde: OK|puntos
 */
static void handle_mis_puntos(sock_t fd, char *param) {
    if (!param) { enviar_mensaje(fd, "ERROR|400|Falta id_u"); return; }
    int puntos = obtener_puntos_fidelidad(atoi(param));
    enviar_fmt(fd, "OK|%d", puntos);
}

/*
 * handle_canjear_puntos()
 *
 * param: "id_u|cantidad"
 */
static void handle_canjear_puntos(sock_t fd, char *param, Sesion *ses) {
    char *s_id_u = strtok(param, SEP);
    char *s_cant = strtok(NULL,  SEP);
    if (!s_id_u || !s_cant) {
        enviar_mensaje(fd, "ERROR|400|Formato: CANJEAR_PUNTOS|id_u|cantidad");
        return;
    }
    int id_u    = atoi(s_id_u);
    int cantidad = atoi(s_cant);

    int actuales = obtener_puntos_fidelidad(id_u);
    if (actuales < cantidad) {
        enviar_fmt(fd, "ERROR|402|Puntos insuficientes (tienes %d, pides %d)",
                   actuales, cantidad);
        return;
    }

    int rc = actualizar_puntos_fidelidad(id_u, -cantidad);
    if (rc == 0) {
        enviar_fmt(fd, "OK|%d", actuales - cantidad);
        char msg[64];
        snprintf(msg, sizeof(msg), "Canje de %d puntos", cantidad);
        log_evento(cfg.log_path, ses->email, "CANJE_PUNTOS", msg);
    } else {
        enviar_mensaje(fd, "ERROR|500|No se pudieron canjear los puntos");
    }
}

/* ══════════════════════════════════════════════════════════════
   DATOS PERSONALES
   ══════════════════════════════════════════════════════════════ */

/*
 * handle_mis_datos()
 *
 * param: "id_u"
 * Responde: DATOS|nombre|apellido|dni|email|telf|fecha_nac|rol
 */
static void handle_mis_datos(sock_t fd, char *param) {
    if (!param) { enviar_mensaje(fd, "ERROR|400|Falta id_u"); return; }
    Usuario u = obtener_usuario_por_id(atoi(param));
    if (u.id_u <= 0) {
        enviar_mensaje(fd, "ERROR|404|Usuario no encontrado");
        return;
    }
    const char *rol_txt = (u.rol == ROL_ADMIN)    ? "ADMIN"     :
                          (u.rol == ROL_EMPLEADO)  ? "MAQUINISTA": "PASAJERO";
    enviar_fmt(fd, "DATOS|%s|%s|%s|%s|%s|%s|%s",
               u.nombre, u.apellido, u.dni, u.email,
               u.telf, u.fecha_nac, rol_txt);
}

/*
 * handle_cambiar_pass()
 *
 * param: "email|nueva_pass_hash"
 */
static void handle_cambiar_pass(sock_t fd, char *param, Sesion *ses) {
    char *email      = strtok(param, SEP);
    char *nueva_hash = strtok(NULL,  SEP);
    if (!email || !nueva_hash) {
        enviar_mensaje(fd, "ERROR|400|Formato: CAMBIAR_PASS|email|nueva_hash");
        return;
    }
    int rc = cambiar_contrasenia_db(email, nueva_hash);
    if (rc == 0) {
        enviar_mensaje(fd, "OK|Contrasenia actualizada");
        log_evento(cfg.log_path, ses->email, "CAMBIO_PASS", "Cambio de contrasenia");
    } else {
        enviar_mensaje(fd, "ERROR|500|No se pudo cambiar la contrasenia");
    }
}

/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 4 — MAQUINISTA
   ══════════════════════════════════════════════════════════════ */

/*
 * handle_cuadrante()
 *
 * param: "id_u"
 * Envía los servicios asignados al maquinista:
 *   SERVICIO|id_serv|fecha|origen|destino|hora_sal|hora_ll|estado|retraso_min
 */
static void handle_cuadrante(sock_t fd, char *param) {
    if (!param) { enviar_mensaje(fd, "ERROR|400|Falta id_u"); return; }
    int id_u = atoi(param);

    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT so.id_serv, so.fecha, eo.nombre, ed.nombre,"
        "       t.hora_salida, t.hora_llegada, so.estado_serv, so.minutos_retraso"
        " FROM ASIGNACION_PERSONAL ap"
        " JOIN SERVICIOS_OPERATIVOS so ON ap.id_serv = so.id_serv"
        " JOIN TRAYECTOS t  ON so.id_tr = t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " WHERE ap.id_u = ?"
        " ORDER BY so.fecha, t.hora_salida;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error en consulta de cuadrante");
        sqlite3_close(db);
        return;
    }
    sqlite3_bind_int(stmt, 1, id_u);

    int n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        enviar_fmt(fd, "SERVICIO|%d|%s|%s|%s|%s|%s|%s|%d",
                   sqlite3_column_int(stmt, 0),
                   col_txt(stmt, 1),
                   col_txt(stmt, 2), col_txt(stmt, 3),
                   col_txt(stmt, 4), col_txt(stmt, 5),
                   col_txt(stmt, 6),
                   sqlite3_column_int(stmt, 7));
        n++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

/*
 * handle_marcar_inicio()
 *
 * param: "id_serv"
 */
static void handle_marcar_inicio(sock_t fd, char *param, Sesion *ses) {
    if (!param) { enviar_mensaje(fd, "ERROR|400|Falta id_serv"); return; }
    int id_serv = atoi(param);
    int rc = marcar_inicio_servicio(id_serv);
    if (rc == 0) {
        enviar_fmt(fd, "OK|Servicio %d iniciado", id_serv);
        char msg[64];
        snprintf(msg, sizeof(msg), "Inicio de servicio id_serv=%d", id_serv);
        log_evento(cfg.log_path, ses->email, "INICIO_SERVICIO", msg);
    } else {
        enviar_fmt(fd, "ERROR|500|No se pudo marcar inicio del servicio %d", id_serv);
    }
}

/*
 * handle_marcar_fin()
 *
 * param: "id_serv"
 */
static void handle_marcar_fin(sock_t fd, char *param, Sesion *ses) {
    if (!param) { enviar_mensaje(fd, "ERROR|400|Falta id_serv"); return; }
    int id_serv = atoi(param);
    int rc = marcar_fin_servicio(id_serv);
    if (rc == 0) {
        enviar_fmt(fd, "OK|Servicio %d finalizado", id_serv);
        char msg[64];
        snprintf(msg, sizeof(msg), "Fin de servicio id_serv=%d", id_serv);
        log_evento(cfg.log_path, ses->email, "FIN_SERVICIO", msg);
    } else {
        enviar_fmt(fd, "ERROR|500|No se pudo marcar fin del servicio %d", id_serv);
    }
}

/*
 * handle_reportar_retraso()
 *
 * param: "id_serv|minutos|causa"
 */
static void handle_reportar_retraso(sock_t fd, char *param, Sesion *ses) {
    char *s_id_serv = strtok(param, SEP);
    char *s_min     = strtok(NULL,  SEP);
    char *causa     = strtok(NULL,  SEP);

    if (!s_id_serv || !s_min) {
        enviar_mensaje(fd, "ERROR|400|Formato: REPORTAR_RETRASO|id_serv|minutos|causa");
        return;
    }

    int id_serv = atoi(s_id_serv);
    int minutos = atoi(s_min);

    int rc = actualizar_retraso_servicio(id_serv, minutos, causa ? causa : "");
    if (rc == 0) {
        enviar_fmt(fd, "OK|Retraso de %d min registrado para servicio %d",
                   minutos, id_serv);
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "Retraso id_serv=%d min=%d causa=%s",
                 id_serv, minutos, causa ? causa : "");
        log_evento(cfg.log_path, ses->email, "RETRASO", msg);
    } else {
        enviar_fmt(fd, "ERROR|500|No se pudo registrar el retraso en servicio %d", id_serv);
    }
}

/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 5 — ADMIN (stubs ampliables)
   ══════════════════════════════════════════════════════════════ */

static void handle_listar_trenes(sock_t fd) {
    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT id_t, nombre_modelo, num_serie, anio_fab, estado_mant"
        " FROM TRENES ORDER BY id_t;",
        -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error en consulta de trenes");
        sqlite3_close(db);
        return;
    }
    int n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        enviar_fmt(fd, "TREN|%d|%s|%s|%d|%s",
                   sqlite3_column_int(stmt, 0),
                   col_txt(stmt, 1), col_txt(stmt, 2),
                   sqlite3_column_int(stmt, 3),
                   col_txt(stmt, 4));
        n++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

static void handle_listar_usuarios(sock_t fd) {
    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT id_u, nombre, apellido, email, rol, activo"
        " FROM USUARIOS ORDER BY id_u;",
        -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error en consulta de usuarios");
        sqlite3_close(db);
        return;
    }
    int n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        enviar_fmt(fd, "USUARIO|%d|%s|%s|%s|%s|%d",
                   sqlite3_column_int(stmt, 0),
                   col_txt(stmt, 1), col_txt(stmt, 2),
                   col_txt(stmt, 3), col_txt(stmt, 4),
                   sqlite3_column_int(stmt, 5));
        n++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

/* ══════════════════════════════════════════════════════════════
   BUCLE PRINCIPAL DE SESIÓN
   ══════════════════════════════════════════════════════════════ */

void manejar_cliente(sock_t fd, const char *ip_cliente) {
    Sesion ses;
    memset(&ses, 0, sizeof(ses));
    ses.fd    = fd;
    ses.id_u  = -1;
    strncpy(ses.ip, ip_cliente ? ip_cliente : "?", sizeof(ses.ip) - 1);

    char buf[PROTO_BUF_MAX];

    while (recibir_mensaje(fd, buf, sizeof(buf)) > 0) {

        /* Parsear comando y resto del mensaje */
        char *cmd   = strtok(buf, SEP);
        char *param = strtok(NULL, "\n");   /* todo lo que sigue al primer | */

        if (!cmd || cmd[0] == '\0') continue;

        /* ── LOGIN (único comando permitido sin sesión activa) ── */
        if (strcmp(cmd, CMD_LOGIN) == 0) {
            handle_login(fd, param, &ses);
            continue;
        }

        /* ── Guardia: el resto de comandos requieren autenticación ── */
        if (ses.id_u < 0) {
            enviar_mensaje(fd, "ERROR|401|No autenticado");
            continue;
        }

        /* ── LOGOUT ── */
        if (strcmp(cmd, CMD_LOGOUT) == 0) {
            enviar_mensaje(fd, "OK|Sesion cerrada");
            log_evento(cfg.log_path, ses.email, "LOGOUT", "Cierre de sesion");
            printf("[HANDLER] %s cerro sesion. Esperando nuevo LOGIN...\n", ses.email);
            ses.id_u = -1;
            memset(ses.email, 0, sizeof(ses.email));
            memset(ses.rol,   0, sizeof(ses.rol));
            continue;
        }

        /* ════ COMANDOS COMUNES (todos los roles) ════ */

        else if (strcmp(cmd, CMD_MIS_DATOS)    == 0) handle_mis_datos(fd, param);
        else if (strcmp(cmd, CMD_CAMBIAR_PASS) == 0) handle_cambiar_pass(fd, param, &ses);

        /* ════ TRAYECTOS Y ESTACIONES ════ */

        else if (strcmp(cmd, CMD_LISTAR_TRAYECTOS)  == 0) handle_listar_trayectos(fd);
        else if (strcmp(cmd, CMD_BUSCAR_TRAYECTO)   == 0) handle_buscar_trayecto(fd, param);
        else if (strcmp(cmd, CMD_DETALLE_TRAYECTO)  == 0) handle_detalle_trayecto(fd, param);
        else if (strcmp(cmd, CMD_LISTAR_ESTACIONES) == 0) handle_listar_estaciones(fd);

        /* ════ RESERVAS (Pasajero) ════ */

        else if (strcmp(cmd, CMD_HACER_RESERVA)    == 0) handle_hacer_reserva(fd, param, &ses);
        else if (strcmp(cmd, CMD_CANCELAR_RESERVA) == 0) handle_cancelar_reserva(fd, param, &ses);
        else if (strcmp(cmd, CMD_MIS_RESERVAS)     == 0) handle_mis_reservas(fd, param);
        else if (strcmp(cmd, CMD_HISTORIAL)        == 0) handle_historial(fd, param);

        /* ════ PUNTOS FIDELIDAD ════ */

        else if (strcmp(cmd, CMD_MIS_PUNTOS)     == 0) handle_mis_puntos(fd, param);
        else if (strcmp(cmd, CMD_CANJEAR_PUNTOS) == 0) handle_canjear_puntos(fd, param, &ses);

        /* ════ MAQUINISTA ════ */

        else if (strcmp(cmd, CMD_CUADRANTE)        == 0) handle_cuadrante(fd, param);
        else if (strcmp(cmd, CMD_MARCAR_INICIO)    == 0) handle_marcar_inicio(fd, param, &ses);
        else if (strcmp(cmd, CMD_MARCAR_FIN)       == 0) handle_marcar_fin(fd, param, &ses);
        else if (strcmp(cmd, CMD_REPORTAR_RETRASO) == 0) handle_reportar_retraso(fd, param, &ses);


        /* ════ ADMIN — acceso restringido ════ */

                else if (strcmp(ses.rol, "ADMIN") == 0) {
                    if      (strcmp(cmd, CMD_LISTAR_TRENES)       == 0) hadmin_listar_trenes(fd);
                    else if (strcmp(cmd, CMD_INSERTAR_TREN)       == 0) hadmin_insertar_tren(fd, param);
                    else if (strcmp(cmd, CMD_MODIFICAR_TREN)      == 0) hadmin_modificar_tren(fd, param);
                    else if (strcmp(cmd, CMD_ELIMINAR_TREN)       == 0) hadmin_eliminar_tren(fd, param);
                    else if (strcmp(cmd, CMD_INSERTAR_ESTACION)   == 0) hadmin_insertar_estacion(fd, param);
                    else if (strcmp(cmd, CMD_MODIFICAR_ESTACION)  == 0) hadmin_modificar_estacion(fd, param);
                    else if (strcmp(cmd, CMD_INSERTAR_TRAYECTO)   == 0) hadmin_insertar_trayecto(fd, param);
                    else if (strcmp(cmd, CMD_MODIFICAR_TRAYECTO)  == 0) hadmin_modificar_trayecto(fd, param);
                    else if (strcmp(cmd, CMD_ESTADO_TRAYECTO)     == 0) hadmin_estado_trayecto(fd, param);
                    else if (strcmp(cmd, CMD_LISTAR_USUARIOS)     == 0) hadmin_listar_usuarios(fd);
                    else if (strcmp(cmd, CMD_LISTAR_EMPLEADOS)    == 0) hadmin_listar_empleados(fd);
                    else if (strcmp(cmd, CMD_DESHABILITAR_USER)   == 0) hadmin_deshabilitar_user(fd, param);
                    else if (strcmp(cmd, CMD_LISTAR_SERVICIOS)    == 0) hadmin_listar_servicios(fd, param);
                    else if (strcmp(cmd, CMD_INSERTAR_SERVICIO)   == 0) hadmin_insertar_servicio(fd, param);
                    else if (strcmp(cmd, CMD_CANCELAR_SERVICIO)   == 0) hadmin_cancelar_servicio(fd, param);
                    else if (strcmp(cmd, CMD_LISTAR_INCIDENCIAS)  == 0) hadmin_listar_incidencias(fd, param);
                    else if (strcmp(cmd, CMD_INSERTAR_INCIDENCIA) == 0) hadmin_insertar_incidencia(fd, param);
                    else if (strcmp(cmd, CMD_RESOLVER_INCIDENCIA) == 0) hadmin_resolver_incidencia(fd, param, ses.email);
                    else if (strcmp(cmd, CMD_INFORME_OCUPACION)   == 0) hadmin_informe_ocupacion(fd, param);
                    else if (strcmp(cmd, CMD_INFORME_INGRESOS)    == 0) hadmin_informe_ingresos(fd, param);
                    else if (strcmp(cmd, CMD_INFORME_INCIDENCIAS) == 0) hadmin_informe_incidencias(fd, param);
                    else if (strcmp(cmd, CMD_MOD_PRECIO_BASE)     == 0) hadmin_mod_precio_base(fd, param);
                    else if (strcmp(cmd, CMD_LISTAR_TARIFAS)      == 0) hadmin_listar_tarifas(fd);
                    else if (strcmp(cmd, CMD_VER_LOGS)            == 0) hadmin_ver_logs(fd, param);
                    else { enviar_fmt(fd, "ERROR|400|Comando admin desconocido: %s", cmd); }
                }

        /* ════ Comando desconocido ════ */

        else {
            enviar_fmt(fd, "ERROR|400|Comando desconocido: %s", cmd);
        }
    }

    /* El cliente cerró la conexión o mandó LOGOUT */
    printf("[HANDLER] Sesion de %s (%s) finalizada.\n",
           ses.email[0] ? ses.email : "no-auth",
           ses.ip);
}

