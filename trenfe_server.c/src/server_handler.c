/*
 * server_handler.c
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */
/*
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server_handler.h"
#include "server_socket.h"
#include "protocolo.h"
#include "config.h"

/* Comando añadido posterior al protocolo original */
#ifndef CMD_ASIGNAR_EMPLEADO
#define CMD_ASIGNAR_EMPLEADO "ASIGNAR_EMPLEADO"
#endif
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
    char        rol[16];
    char        ip[46];
} Sesion;

/* ══════════════════════════════════════════════════════════════
   HELPERS INTERNOS
   ══════════════════════════════════════════════════════════════ */

static sqlite3 *abrir_db(sock_t fd) {
    sqlite3 *db;
    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Base de datos no disponible");
        return NULL;
    }
    return db;
}

static const char *col_txt(sqlite3_stmt *s, int i) {
    const unsigned char *v = sqlite3_column_text(s, i);
    return v ? (const char *)v : "";
}

/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 2 — AUTENTICACIÓN
   ══════════════════════════════════════════════════════════════ */


static void handle_login(sock_t fd, char *param, Sesion *ses) {
    if (!param) {
        enviar_mensaje(fd, "AUTH_FAIL|Faltan credenciales");
        return;
    }

    char *email = strtok(param, SEP);
    char *pass_hash = strtok(NULL,  SEP);

    if (!email || !pass_hash) {
        enviar_mensaje(fd, "AUTH_FAIL|Formato incorrecto");
        return;
    }

    sqlite3 *db;
    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) {
        enviar_mensaje(fd, "AUTH_FAIL|Error interno de BD");
        return;
    }

    sqlite3_stmt *stmt;
    bool autenticado = false;

    const char *sql =
        "SELECT id_u, nombre, rol "
        "FROM USUARIOS "
        "WHERE email = ? AND pass_hash = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, email,     -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, pass_hash, -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int id_u  = sqlite3_column_int (stmt, 0);
            const char *nombre  = (const char*)sqlite3_column_text(stmt, 1);
            const char *rol_txt = (const char*)sqlite3_column_text(stmt, 2);

            const char *rol_str;
            if (rol_txt && strcmp(rol_txt, "ADMIN") == 0){
            	rol_str = "ADMIN";
            }
            else if (rol_txt && strcmp(rol_txt, "MAQUINISTA") == 0){
            	rol_str = "MAQUINISTA";
            }
            else if (rol_txt && strcmp(rol_txt, "EMPLEADO") == 0){
            	rol_str = "MAQUINISTA";
            }
            else{
            	rol_str = "PASAJERO";
            }

            ses->id_u = id_u;
            strncpy(ses->email, email,   sizeof(ses->email) - 1);
            strncpy(ses->rol,   rol_str, sizeof(ses->rol) - 1);

            enviar_fmt(fd, "AUTH_OK|%d|%s|%s", id_u, rol_str, nombre ? nombre : "");
            char msg[128];
            snprintf(msg, sizeof(msg),"Login correcto – rol=%s ip=%s", rol_str, ses->ip);
            log_evento(cfg.log_path, email, "AUTH_OK", msg);
            autenticado = true;
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);

    if (!autenticado){
        enviar_fmt(fd, "AUTH_FAIL|Credenciales incorrectas para %s", email);
    }
}


/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 3 — TRAYECTOS
   ══════════════════════════════════════════════════════════════ */


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


static void handle_buscar_trayecto(sock_t fd, char *param) {
    char *s_orig = strtok(param, SEP);
    char *s_dest = strtok(NULL,  SEP);
    char *fecha = strtok(NULL,  SEP);
    char *clase = strtok(NULL,  SEP);

    if (!s_orig || !s_dest || !fecha || !clase) {
        enviar_mensaje(fd, "ERROR|400|Formato: BUSCAR_TRAYECTO|id_orig|id_dest|fecha|clase");
        return;
    }

    int id_orig = atoi(s_orig);
    int id_dest = atoi(s_dest);


    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;

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
        sqlite3_stmt *s_vag;
        int libres_total = 0;
        if (sqlite3_prepare_v2(db,
            "SELECT numero_vagon FROM VAGONES WHERE id_tren ="
            " (SELECT id_t FROM TRAYECTOS WHERE id_tr=?) AND clase=?;",
            -1, &s_vag, NULL) == SQLITE_OK) {
            sqlite3_bind_int (s_vag, 1, id_tr);
            sqlite3_bind_text(s_vag, 2, clase, -1, SQLITE_STATIC);
            while (sqlite3_step(s_vag) == SQLITE_ROW) {
                int num_v = sqlite3_column_int(s_vag, 0);
                int libres_v = contar_asientos_libres(id_tr, fecha, num_v, clase);
                if (libres_v > 0) libres_total += libres_v;
            }
            sqlite3_finalize(s_vag);
        } else {

            libres_total = contar_asientos_libres(id_tr, fecha, 1, clase);
        }

        enviar_fmt(fd, "TRAYECTO|%d|%s|%s|%s|%s|%.2f|%d|%s",
                   id_tr,
                   col_txt(stmt, 1), col_txt(stmt, 2),
                   col_txt(stmt, 3), col_txt(stmt, 4),
                   precio, libres_total,
                   col_txt(stmt, 6));
        n++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}


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


static void handle_hacer_reserva(sock_t fd, char *param, Sesion *ses) {
    char *s_id_tr   = strtok(param, SEP);
    char *fecha  = strtok(NULL,  SEP);
    char *clase = strtok(NULL,  SEP);
    char *s_vagon = strtok(NULL,  SEP);
    char *s_asiento = strtok(NULL,  SEP);
    char *s_tipo_eq = strtok(NULL,  SEP);
    char *s_peso_eq = strtok(NULL,  SEP);
    char *s_puntos  = strtok(NULL,  SEP);   //campo opcional: puntos a canjear

    if (!s_id_tr || !fecha || !clase || !s_vagon || !s_asiento) {
        enviar_mensaje(fd, "ERROR|400|Faltan parametros en HACER_RESERVA");
        return;
    }

    int    id_u    = ses->id_u;
    int    id_tr   = atoi(s_id_tr);
    int    vagon   = atoi(s_vagon);
    int    asiento = atoi(s_asiento);
    int    puntos_canje = s_puntos ? atoi(s_puntos) : 0;


    if (!asiento_libre(id_tr, fecha, vagon, asiento)) {
        enviar_mensaje(fd, "ERROR|409|Asiento ya ocupado");
        return;
    }

    double sup_equipaje = 0.0;
    if (s_tipo_eq && s_peso_eq) {
        TipoEquipaje tipo_eq = EQUIPAJE_MANO;
        if (strcmp(s_tipo_eq, "BODEGA") == 0) tipo_eq = EQUIPAJE_BODEGA;
        else if (strcmp(s_tipo_eq, "BICI") == 0) tipo_eq = EQUIPAJE_BICI;
        else if (strcmp(s_tipo_eq, "ESQUI") == 0) tipo_eq = EQUIPAJE_ESQUI;
        sup_equipaje = calcular_suplemento_equipaje(tipo_eq, atof(s_peso_eq), clase);
    }

    // Descuento de tarjeta del pasajero
    TipoDescuento desc = obtener_descuento_usuario(id_u);

    /* Precio base para auditoría */
    Trayecto tr_base = obtener_trayecto_por_id(id_tr);
    double precio_base_trayecto = (tr_base.id_tr > 0) ? tr_base.precio_base : 0.0;

    /* Precio tras descuento de tarjeta + equipaje */
    double precio_final = calcular_precio_final(id_tr, clase, desc, sup_equipaje);

    // Aplicar canje de puntos (100 puntos = 1 EUR)
    if (puntos_canje > 0) {
        int puntos_actuales = obtener_puntos_fidelidad(id_u);
        if (puntos_canje > puntos_actuales)
            puntos_canje = puntos_actuales;

        double descuento_pts = puntos_canje / 100.0;
        if (descuento_pts > precio_final)
            descuento_pts = precio_final;

        precio_final -= descuento_pts;


        actualizar_puntos_fidelidad(id_u, -puntos_canje);
    }


    Reserva r;
    memset(&r, 0, sizeof(r));
    r.id_u  = id_u;
    r.id_tr = id_tr;
    r.num_vagon= vagon;
    r.num_asiento  = asiento;
    r.precio_base  = precio_base_trayecto;
    r.precio_final = precio_final;
    strncpy(r.clase, clase, sizeof(r.clase) - 1);
    strncpy(r.fecha_viaje, fecha, sizeof(r.fecha_viaje) - 1);
    generar_codigo_validacion(r.codigo_validacion, sizeof(r.codigo_validacion));

    int id_res = insertar_reserva_db(r);
    if (id_res <= 0) {
        //Si falló la inserción y ya restamos puntos, los devolvemos
        if (puntos_canje > 0) actualizar_puntos_fidelidad(id_u, puntos_canje);
        enviar_mensaje(fd, "ERROR|500|No se pudo crear la reserva");
        return;
    }


    if (s_tipo_eq && s_peso_eq && sup_equipaje > 0.0) {
        Equipaje eq;
        memset(&eq, 0, sizeof(eq));
        eq.id_res          = id_res;
        eq.peso_kg         = atof(s_peso_eq);
        eq.suplemento_pago = sup_equipaje;
        if (strcmp(s_tipo_eq, "BODEGA") == 0) {
        	eq.tipo = EQUIPAJE_BODEGA;
        }
        else if (strcmp(s_tipo_eq, "BICI") == 0){
        	eq.tipo = EQUIPAJE_BICI;
        }
        else if (strcmp(s_tipo_eq, "ESQUI") == 0){
        	eq.tipo = EQUIPAJE_ESQUI;
        }
        else {
        	eq.tipo = EQUIPAJE_MANO;
        }
        insertar_equipaje_db(eq);
    }

    // Puntos restantes tras la reserva
    int puntos_restantes = obtener_puntos_fidelidad(id_u);

    /* OK|id_res|precio_final|codigo_validacion|puntos_restantes */
    enviar_fmt(fd, "OK|%d|%.2f|%s|%d",
               id_res, precio_final, r.codigo_validacion, puntos_restantes);

    // Log
    char msg[160];
    snprintf(msg, sizeof(msg),
             "Reserva creada id_res=%d trayecto=%d fecha=%s puntos_canjeados=%d",
             id_res, id_tr, fecha, puntos_canje);
    log_evento(cfg.log_path, ses->email, "RESERVA", msg);
}


static void handle_cancelar_reserva(sock_t fd, char *param, Sesion *ses) {
    char *s_id_res = strtok(param, SEP);

    if (!s_id_res) {
        enviar_mensaje(fd, "ERROR|400|Formato: CANCELAR_RESERVA|id_res");
        return;
    }

    int id_res = atoi(s_id_res);
    int id_u   = ses->id_u;   /* B-13: id_u siempre de la sesion autenticada */

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


static void handle_mis_reservas(sock_t fd, Sesion *ses) {
    int id_u = ses->id_u;   /* B-15: id_u siempre de la sesion autenticada */

    sqlite3 *db = abrir_db(fd);
    if (!db){
    	return;
    }

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


static void handle_historial(sock_t fd, Sesion *ses) {
    int id_u = ses->id_u;

    sqlite3 *db = abrir_db(fd);
    if (!db) {
    	return;
    }

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
        "   AND (r.fecha_viaje < date('now') OR r.estado IN ('CANCELADA','COMPLETADA'))"
        " ORDER BY r.fecha_viaje DESC;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error en consulta de historial");
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

/* ══════════════════════════════════════════════════════════════
   PRIORIDAD 3 — PUNTOS FIDELIDAD
   ══════════════════════════════════════════════════════════════ */


static void handle_mis_puntos(sock_t fd, Sesion *ses) {
    int puntos = obtener_puntos_fidelidad(ses->id_u);
    enviar_fmt(fd, "OK|%d", puntos);
}


static void handle_canjear_puntos(sock_t fd, char *param, Sesion *ses) {
    char *s_cant = strtok(param, SEP);
    if (!s_cant) {
        enviar_mensaje(fd, "ERROR|400|Formato: CANJEAR_PUNTOS|cantidad");
        return;
    }
    int id_u = ses->id_u;   /* id_u siempre de la sesion autenticada */
    int cantidad = atoi(s_cant);

    int actuales = obtener_puntos_fidelidad(id_u);
    if (actuales < cantidad) {
        enviar_fmt(fd, "ERROR|402|Puntos insuficientes (tienes %d, pides %d)",actuales, cantidad);
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


static void handle_mis_datos(sock_t fd, char *param) {
    if (!param) { enviar_mensaje(fd, "ERROR|400|Falta id_u"); return; }
    Usuario u = obtener_usuario_por_id(atoi(param));
    if (u.id_u <= 0) {
        enviar_mensaje(fd, "ERROR|404|Usuario no encontrado");
        return;
    }
    const char *rol_txt = (u.rol == ROL_ADMIN)    ? "ADMIN"     :
                          (u.rol == ROL_EMPLEADO)  ? "MAQUINISTA": "PASAJERO";
    enviar_fmt(fd, "DATOS|%s|%s|%s|%s|%s|%s|%s", u.nombre, u.apellido, u.dni, u.email,
               u.telf, u.fecha_nac, rol_txt);
}


static void handle_cambiar_pass(sock_t fd, char *param, Sesion *ses) {
    char *email      = strtok(param, SEP);
    char *nueva_hash = strtok(NULL,  SEP);
    if (!email || !nueva_hash) {
        enviar_mensaje(fd, "ERROR|400|Formato: CAMBIAR_PASS|email|nueva_hash");
        return;
    }

    if (strcmp(ses->rol, "ADMIN") != 0 && strcmp(email, ses->email) != 0) {
        enviar_mensaje(fd, "ERROR|403|No puedes cambiar la contrasenia de otro usuario");
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


static void handle_marcar_inicio(sock_t fd, char *param, Sesion *ses) {
    if (!param) {
    	enviar_mensaje(fd, "ERROR|400|Falta id_serv");
    	return;
    }
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


static void handle_reportar_retraso(sock_t fd, char *param, Sesion *ses) {
    char *s_id_serv = strtok(param, SEP);
    char *s_min = strtok(NULL,  SEP);
    char *causa = strtok(NULL,  SEP);

    if (!s_id_serv || !s_min) {
        enviar_mensaje(fd, "ERROR|400|Formato: REPORTAR_RETRASO|id_serv|minutos|causa");
        return;
    }

    int id_serv = atoi(s_id_serv);
    int minutos = atoi(s_min);

    int rc = actualizar_retraso_servicio(id_serv, minutos, causa ? causa : "");
    if (rc == 0) {
        enviar_fmt(fd, "OK|Retraso de %d min registrado para servicio %d", minutos, id_serv);
        char msg[128];
        snprintf(msg, sizeof(msg), "Retraso id_serv=%d min=%d causa=%s", id_serv, minutos, causa ? causa : "");
        log_evento(cfg.log_path, ses->email, "RETRASO", msg);
    } else {
        enviar_fmt(fd, "ERROR|500|No se pudo registrar el retraso en servicio %d", id_serv);
    }
}




static void handle_registro(sock_t fd, char *param) {
    if (!param) {
        enviar_mensaje(fd, "ERROR|400|Formato: REGISTRO|nombre|apellido|dni|email|telf|fecha_nac|pass_hash");
        return;
    }
    char *nombre = strtok(param, SEP);
    char *apellido = strtok(NULL,  SEP);
    char *dni = strtok(NULL,  SEP);
    char *email = strtok(NULL,  SEP);
    char *telf = strtok(NULL,  SEP);
    char *fecha_nac = strtok(NULL,  SEP);
    char *pass_hash = strtok(NULL,  SEP);

    if (!nombre || !apellido || !dni || !email || !telf || !fecha_nac || !pass_hash) {
        enviar_mensaje(fd, "ERROR|400|Faltan campos en REGISTRO");
        return;
    }

    Usuario u;
    memset(&u, 0, sizeof(u));
    strncpy(u.nombre, nombre, sizeof(u.nombre) - 1);
    strncpy(u.apellido, apellido, sizeof(u.apellido) - 1);
    strncpy(u.dni, dni, sizeof(u.dni) - 1);
    strncpy(u.email, email, sizeof(u.email) - 1);
    strncpy(u.telf, telf, sizeof(u.telf) - 1);
    strncpy(u.fecha_nac, fecha_nac, sizeof(u.fecha_nac) - 1);
    strncpy(u.pass_hash, pass_hash, sizeof(u.pass_hash) - 1);
    u.rol    = ROL_PASAJERO;
    u.activo = 1;

    int id_u = insertar_usuario_db(u);
    if (id_u > 0) {
        enviar_fmt(fd, "OK|%d", id_u);
        log_evento(cfg.log_path, email, "REGISTRO", "Nuevo usuario registrado");
    } else {
        enviar_mensaje(fd, "ERROR|409|Email o DNI ya registrado");
    }
}




static void handle_listar_ciudades(sock_t fd) {
    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, "SELECT DISTINCT ciudad FROM ESTACIONES ORDER BY ciudad;", -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error al listar ciudades");
        sqlite3_close(db);
        return;
    }
    int n = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        enviar_fmt(fd, "CIUDAD|%s", col_txt(stmt, 0));
        n++;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

/* ══════════════════════════════════════════════════════════════
    DETALLE DE RESERVA
   ══════════════════════════════════════════════════════════════ */


static void handle_detalle_reserva(sock_t fd, char *param, Sesion *ses) {
    if (!param) {
        enviar_mensaje(fd, "ERROR|400|Formato: DETALLE_RESERVA|id_res");
        return;
    }
    int id_res = atoi(param);

    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT r.id_res, eo.nombre, ed.nombre, r.fecha_viaje,"
        " r.clase, r.num_vagon, r.num_asiento,"
        " r.precio_base, r.precio_final, r.estado, r.codigo_validacion,"
        " r.id_u"
        " FROM RESERVAS r"
        " JOIN TRAYECTOS t  ON r.id_tr = t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " WHERE r.id_res = ?;",
        -1, &stmt, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Error en consulta de reserva");
        sqlite3_close(db);
        return;
    }
    sqlite3_bind_int(stmt, 1, id_res);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int propietario = sqlite3_column_int(stmt, 11);
        /* Solo el dueño o un admin puede ver el detalle */
        if (propietario != ses->id_u && strcmp(ses->rol, "ADMIN") != 0) {
            enviar_mensaje(fd, "ERROR|403|Esta reserva no pertenece a tu cuenta");
        } else {
            enviar_fmt(fd, "DETALLE_RESERVA|%d|%s|%s|%s|%s|%d|%d|%.2f|%.2f|%s|%s",
                sqlite3_column_int(stmt, 0),
                col_txt(stmt, 1), col_txt(stmt, 2),
                col_txt(stmt, 3), col_txt(stmt, 4),
                sqlite3_column_int(stmt, 5),
                sqlite3_column_int(stmt, 6),
                sqlite3_column_double(stmt, 7),
                sqlite3_column_double(stmt, 8),
                col_txt(stmt, 9),
                col_txt(stmt, 10));
        }
    } else {
        enviar_fmt(fd, "ERROR|404|Reserva %d no encontrada", id_res);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}



static void handle_listar_vagones(sock_t fd, char *param) {
    char *s_id_tr = strtok(param, SEP);
    char *fecha   = strtok(NULL,  SEP);
    char *clase   = strtok(NULL,  SEP);

    if (!s_id_tr || !fecha || !clase) {
        enviar_mensaje(fd, "ERROR|400|Formato: LISTAR_VAGONES|id_tr|fecha|clase");
        return;
    }
    int id_tr = atoi(s_id_tr);

    Trayecto tr = obtener_trayecto_por_id(id_tr);
    if (tr.id_tr <= 0) {
        enviar_fmt(fd, "FIN_LISTA|0");
        return;
    }

    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    // Comprobar si el tren tiene vagones registrados
    sqlite3_stmt *chk;
    int tiene_vagones = 0;
    if (sqlite3_prepare_v2(db,
        "SELECT COUNT(*) FROM VAGONES WHERE id_tren=?;",
        -1, &chk, NULL) == SQLITE_OK) {
        sqlite3_bind_int(chk, 1, tr.id_t);
        if (sqlite3_step(chk) == SQLITE_ROW) {
            tiene_vagones = sqlite3_column_int(chk, 0);
        }
        sqlite3_finalize(chk);
    }

    // Si no tiene vagones, crearlos automaticamente (2 turista + 1 business)
    if (tiene_vagones == 0) {
        sqlite3_stmt *ins_s;
        if (sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO VAGONES(id_tren,numero_vagon,clase,capacidad_total)"
            " VALUES(?1,1,'T',50),(?1,2,'T',50),(?1,3,'B',30);",
            -1, &ins_s, NULL) == SQLITE_OK) {
            sqlite3_bind_int(ins_s, 1, tr.id_t);
            sqlite3_step(ins_s);
            sqlite3_finalize(ins_s);
        }
    }

    sqlite3_stmt *s;
    const char *sql =
        "SELECT v.numero_vagon, v.capacidad_total,"
        " v.capacidad_total - COALESCE("
        "  (SELECT COUNT(*) FROM RESERVAS r"
        "   WHERE r.id_tr=? AND r.fecha_viaje=?"
        "   AND r.num_vagon=v.numero_vagon"
        "   AND r.estado NOT IN ('CANCELADA'))"
        " ,0) AS libres"
        " FROM VAGONES v"
        " WHERE v.id_tren=? AND v.clase=?"
        " ORDER BY v.numero_vagon;";

    if (sqlite3_prepare_v2(db, sql, -1, &s, NULL) != SQLITE_OK) {
        enviar_fmt(fd, "ERROR|500|Consulta vagones fallida: %s", sqlite3_errmsg(db));
        sqlite3_close(db); return;
    }
    sqlite3_bind_int (s, 1, id_tr);
    sqlite3_bind_text(s, 2, fecha, -1, SQLITE_STATIC);
    sqlite3_bind_int (s, 3, tr.id_t);
    sqlite3_bind_text(s, 4, clase, -1, SQLITE_STATIC);

    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        enviar_fmt(fd, "VAGON|%d|%d|%d",
                   sqlite3_column_int(s, 0),
                   sqlite3_column_int(s, 1),
                   sqlite3_column_int(s, 2));
        n++;
    }
    sqlite3_finalize(s);
    sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}


static void handle_mapa_vagon(sock_t fd, char *param) {
    char *s_id_tr = strtok(param, SEP);
    char *fecha   = strtok(NULL,  SEP);
    char *s_num_v = strtok(NULL,  SEP);

    if (!s_id_tr || !fecha || !s_num_v) {
        enviar_mensaje(fd, "ERROR|400|Formato: MAPA_VAGON|id_tr|fecha|num_vagon");
        return;
    }
    int id_tr = atoi(s_id_tr);
    int num_v = atoi(s_num_v);

    Trayecto tr = obtener_trayecto_por_id(id_tr);
    if (tr.id_tr <= 0) {
        enviar_mensaje(fd, "ERROR|404|Trayecto no encontrado");
        return;
    }

    sqlite3 *db = abrir_db(fd);
    if (!db) return;

    // Capacidad del vagón
    sqlite3_stmt *s1;
    int capacidad = 0;
    if (sqlite3_prepare_v2(db,
        "SELECT capacidad_total FROM VAGONES WHERE id_tren=? AND numero_vagon=?;",
        -1, &s1, NULL) == SQLITE_OK) {
        sqlite3_bind_int(s1, 1, tr.id_t);
        sqlite3_bind_int(s1, 2, num_v);
        if (sqlite3_step(s1) == SQLITE_ROW)
            capacidad = sqlite3_column_int(s1, 0);
        sqlite3_finalize(s1);
    }
    if (capacidad == 0) {
        enviar_mensaje(fd, "ERROR|404|Vagon no encontrado o sin capacidad");
        sqlite3_close(db); return;
    }

    //Asientos ocupados
    int ocupado[MAX_ASIENTOS + 1];
    memset(ocupado, 0, sizeof(ocupado));
    sqlite3_stmt *s2;
    if (sqlite3_prepare_v2(db,
        "SELECT num_asiento FROM RESERVAS"
        "WHERE id_tr=? AND fecha_viaje=? AND num_vagon=?"
        "AND estado IN ('CONFIRMADA','PENDIENTE');",
        -1, &s2, NULL) == SQLITE_OK) {
        sqlite3_bind_int (s2, 1, id_tr);
        sqlite3_bind_text(s2, 2, fecha, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (s2, 3, num_v);
        while (sqlite3_step(s2) == SQLITE_ROW) {
            int a = sqlite3_column_int(s2, 0);
            if (a > 0 && a <= MAX_ASIENTOS) ocupado[a] = 1;
        }
        sqlite3_finalize(s2);
    }
    sqlite3_close(db);

    //Enviar mapa
    enviar_fmt(fd, "MAPA_INFO|%d|%d", num_v, capacidad);
    for (int i = 1; i <= capacidad; i++)
        enviar_fmt(fd, "ASIENTO|%d|%d", i, ocupado[i]);
    enviar_fmt(fd, "FIN_LISTA|%d", capacidad);
}

/* ══════════════════════════════════════════════════════════════
   BUCLE PRINCIPAL DE SESIÓN
   ══════════════════════════════════════════════════════════════ */

void manejar_cliente(sock_t fd, const char *ip_cliente) {
    Sesion ses;
    memset(&ses, 0, sizeof(ses));
    ses.fd = fd;
    ses.id_u = -1;
    strncpy(ses.ip, ip_cliente ? ip_cliente : "?", sizeof(ses.ip) - 1);

    char buf[PROTO_BUF_MAX];

    while (recibir_mensaje(fd, buf, sizeof(buf)) > 0) {

        // Parsear comando y resto del mensaje
        char *cmd= strtok(buf, SEP);
        char *param = strtok(NULL, "\n");

        if (!cmd || cmd[0] == '\0'){
        	continue;
        }

        if (strcmp(cmd, CMD_LOGIN) == 0) {
            handle_login(fd, param, &ses);
            //Traza post-login: si tuvo exito, ses.email ya esta relleno
            printf("[HANDLER] [%s] CMD=%s\n", ses.email[0] ? ses.email : "no-auth", cmd);
            continue;
        }
        if (strcmp(cmd, CMD_REGISTRO) == 0) {
            handle_registro(fd, param);
            printf("[HANDLER] [no-auth] CMD=%s\n", cmd);
            continue;
        }

        //Traza en consola del servidor
        printf("[HANDLER] [%s] CMD=%s\n",
               ses.email[0] ? ses.email : "no-auth", cmd);

        //el resto de comandos requieren autenticación
        if (ses.id_u < 0) {
            enviar_mensaje(fd, "ERROR|401|No autenticado");
            continue;
        }

        // LOGOUT
        if (strcmp(cmd, CMD_LOGOUT) == 0) {
            enviar_mensaje(fd, "OK|Sesion cerrada");
            log_evento(cfg.log_path, ses.email, "LOGOUT", "Cierre de sesion");
            printf("[HANDLER] %s cerro sesion. Esperando nuevo LOGIN...\n", ses.email);
            ses.id_u = -1;
            memset(ses.email, 0, sizeof(ses.email));
            memset(ses.rol,   0, sizeof(ses.rol));
            continue;
        }

        else if (strcmp(cmd, CMD_MIS_DATOS) == 0) handle_mis_datos(fd, param);
        else if (strcmp(cmd, CMD_CAMBIAR_PASS) == 0) handle_cambiar_pass(fd, param, &ses);
        else if (strcmp(cmd, CMD_LISTAR_TRAYECTOS) == 0) handle_listar_trayectos(fd);
        else if (strcmp(cmd, CMD_BUSCAR_TRAYECTO) == 0) handle_buscar_trayecto(fd, param);
        else if (strcmp(cmd, CMD_DETALLE_TRAYECTO) == 0) handle_detalle_trayecto(fd, param);
        else if (strcmp(cmd, CMD_LISTAR_ESTACIONES) == 0) handle_listar_estaciones(fd);
        else if (strcmp(cmd, CMD_LISTAR_CIUDADES) == 0) handle_listar_ciudades(fd);      /* B-07 */
        else if (strcmp(cmd, CMD_LISTAR_VAGONES) == 0) handle_listar_vagones(fd, param);
        else if (strcmp(cmd, CMD_MAPA_VAGON) == 0) handle_mapa_vagon(fd, param);
        else if (strcmp(cmd, CMD_HACER_RESERVA) == 0) handle_hacer_reserva(fd, param, &ses);
        else if (strcmp(cmd, CMD_CANCELAR_RESERVA) == 0) handle_cancelar_reserva(fd, param, &ses);
        else if (strcmp(cmd, CMD_MIS_RESERVAS) == 0) handle_mis_reservas(fd, &ses);
        else if (strcmp(cmd, CMD_HISTORIAL) == 0) handle_historial(fd, &ses);
        else if (strcmp(cmd, CMD_DETALLE_RESERVA) == 0) handle_detalle_reserva(fd, param, &ses);  /* B-06 */
        else if (strcmp(cmd, CMD_MIS_PUNTOS) == 0) handle_mis_puntos(fd, &ses);
        else if (strcmp(cmd, CMD_CANJEAR_PUNTOS) == 0) handle_canjear_puntos(fd, param, &ses);



        else if (strcmp(cmd, CMD_CUADRANTE) == 0 ||
                 strcmp(cmd, CMD_MARCAR_INICIO) == 0 ||
                 strcmp(cmd, CMD_MARCAR_FIN) == 0 ||
                 strcmp(cmd, CMD_REPORTAR_RETRASO) == 0) {

            if (strcmp(ses.rol, "MAQUINISTA") != 0 && strcmp(ses.rol, "ADMIN") != 0) {
                enviar_mensaje(fd, "ERROR|403|Acceso restringido a maquinistas");
            } else if (strcmp(cmd, CMD_CUADRANTE) == 0) handle_cuadrante(fd, param);
              else if (strcmp(cmd, CMD_MARCAR_INICIO) == 0) handle_marcar_inicio(fd, param, &ses);
              else if (strcmp(cmd, CMD_MARCAR_FIN) == 0) handle_marcar_fin(fd, param, &ses);
              else if (strcmp(cmd, CMD_REPORTAR_RETRASO) == 0) handle_reportar_retraso(fd, param, &ses);
        }



        else if (strcmp(ses.rol, "ADMIN") == 0) {
            if (strcmp(cmd, CMD_LISTAR_TRENES) == 0) hadmin_listar_trenes(fd);
            else if (strcmp(cmd, CMD_INSERTAR_TREN) == 0) hadmin_insertar_tren(fd, param);
            else if (strcmp(cmd, CMD_MODIFICAR_TREN) == 0) hadmin_modificar_tren(fd, param);
            else if (strcmp(cmd, CMD_ELIMINAR_TREN) == 0) hadmin_eliminar_tren(fd, param);
            else if (strcmp(cmd, CMD_LISTAR_USUARIOS) == 0) hadmin_listar_usuarios(fd);
            else if (strcmp(cmd, CMD_LISTAR_EMPLEADOS) == 0) hadmin_listar_empleados(fd);
            else if (strcmp(cmd, CMD_DESHABILITAR_USER) == 0) hadmin_deshabilitar_user(fd, param);
            else if (strcmp(cmd, CMD_INSERTAR_ESTACION) == 0) hadmin_insertar_estacion(fd, param);
            else if (strcmp(cmd, CMD_MODIFICAR_ESTACION) == 0) hadmin_modificar_estacion(fd, param);
            else if (strcmp(cmd, CMD_INSERTAR_TRAYECTO) == 0) hadmin_insertar_trayecto(fd, param);
            else if (strcmp(cmd, CMD_MODIFICAR_TRAYECTO) == 0) hadmin_modificar_trayecto(fd, param);
            else if (strcmp(cmd, CMD_ESTADO_TRAYECTO) == 0) hadmin_estado_trayecto(fd, param);
            else if (strcmp(cmd, CMD_LISTAR_SERVICIOS) == 0) hadmin_listar_servicios(fd, param);
            else if (strcmp(cmd, CMD_INSERTAR_SERVICIO) == 0) hadmin_insertar_servicio(fd, param);
            else if (strcmp(cmd, CMD_CANCELAR_SERVICIO) == 0) hadmin_cancelar_servicio(fd, param);
            else if (strcmp(cmd, CMD_ASIGNAR_EMPLEADO) == 0) hadmin_asignar_empleado(fd, param);
            else if (strcmp(cmd, CMD_LISTAR_INCIDENCIAS) == 0) hadmin_listar_incidencias(fd, param);
            else if (strcmp(cmd, CMD_INSERTAR_INCIDENCIA) == 0) hadmin_insertar_incidencia(fd, param);
            else if (strcmp(cmd, CMD_RESOLVER_INCIDENCIA) == 0) hadmin_resolver_incidencia(fd, param, ses.email);
            else if (strcmp(cmd, CMD_INFORME_OCUPACION) == 0) hadmin_informe_ocupacion(fd, param);
            else if (strcmp(cmd, CMD_INFORME_INGRESOS) == 0) hadmin_informe_ingresos(fd, param);
            else if (strcmp(cmd, CMD_INFORME_INCIDENCIAS) == 0) hadmin_informe_incidencias(fd, param);
            else if (strcmp(cmd, CMD_MOD_PRECIO_BASE) == 0) hadmin_mod_precio_base(fd, param);
            else if (strcmp(cmd, CMD_MOD_COEF_BUSINESS) == 0) hadmin_mod_coef_business(fd, param);
            else if (strcmp(cmd, CMD_MOD_EXCESO_KG) == 0) hadmin_mod_exceso_kg(fd, param);
            else if (strcmp(cmd, CMD_MOD_SUPL_BICI) == 0) hadmin_mod_supl_bici(fd, param);
            else if (strcmp(cmd, CMD_LISTAR_TARIFAS) == 0) hadmin_listar_tarifas(fd);
            else if (strcmp(cmd, CMD_VER_LOGS) == 0) hadmin_ver_logs(fd, param);
            else { enviar_fmt(fd, "ERROR|400|Comando admin desconocido: %s", cmd); }
        }



        else {
            enviar_fmt(fd, "ERROR|400|Comando desconocido o sin permiso: %s", cmd);
        }
    }

    // El cliente cerró la conexión o mandó LOGOUT
    printf("[HANDLER] Sesion de %s (%s) finalizada.\n",
           ses.email[0] ? ses.email : "no-auth",
           ses.ip);
}
