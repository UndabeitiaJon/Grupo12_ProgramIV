/*
 * server_handler_admin.c
 *
 *  Created on: 9 may 2026
 *      Author: e.aranoa
 */

/*
 * server_handler_admin.cpp
 *
 *  Created on: 9 may 2026
 *      Author: e.aranoa
 */


/*
 * server_handler_admin.c  -  Sistema TRENFE  -  Fase 2
 *
 * Handlers exclusivos del rol ADMIN.
 * Cada función abre y cierra la BD por sí misma para ser independiente.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server_handler_admin.h"
#include "server_socket.h"
#include "protocolo.h"
#include "config.h"
#include "logs.h"
#include "db_manager.h"
#include "sqlite3.h"

/* ── Helpers locales ── */

static sqlite3 *abrir_db_admin(sock_t fd) {
    sqlite3 *db;
    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Base de datos no disponible");
        return NULL;
    }
    return db;
}

static const char *ctxt(sqlite3_stmt *s, int i) {
    const unsigned char *v = sqlite3_column_text(s, i);
    return v ? (const char *)v : "";
}

/* ══════════════════════════════════════════════
   TRENES
   ══════════════════════════════════════════════ */

void hadmin_listar_trenes(sock_t fd) {
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    if (sqlite3_prepare_v2(db,
        "SELECT id_t, nombre_modelo, num_serie, anio_fab, estado_mant, fecha_ultima_revision"
        " FROM TRENES ORDER BY id_t;", -1, &s, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Consulta de trenes fallida");
        sqlite3_close(db); return;
    }
    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        enviar_fmt(fd, "TREN|%d|%s|%s|%d|%s|%s",
            sqlite3_column_int(s,0), ctxt(s,1), ctxt(s,2),
            sqlite3_column_int(s,3), ctxt(s,4), ctxt(s,5));
        n++;
    }
    sqlite3_finalize(s); sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

void hadmin_insertar_tren(sock_t fd, char *param) {
    /* param: "modelo|serie|anio|estado|fecha_rev" */
    char *modelo    = strtok(param, "|");
    char *serie     = strtok(NULL,  "|");
    char *s_anio    = strtok(NULL,  "|");
    char *estado    = strtok(NULL,  "|");
    char *fecha_rev = strtok(NULL,  "|");

    if (!modelo || !serie || !s_anio || !estado || !fecha_rev) {
        enviar_mensaje(fd, "ERROR|400|Formato: INSERTAR_TREN|modelo|serie|anio|estado|fecha_rev");
        return;
    }

    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "INSERT INTO TRENES(nombre_modelo,num_serie,anio_fab,estado_mant,fecha_ultima_revision)"
        " VALUES(?,?,?,?,?);", -1, &s, NULL);
    sqlite3_bind_text(s,1,modelo,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,2,serie,-1,SQLITE_STATIC);
    sqlite3_bind_int (s,3,atoi(s_anio));
    sqlite3_bind_text(s,4,estado,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,5,fecha_rev,-1,SQLITE_STATIC);

    if (sqlite3_step(s) == SQLITE_DONE) {
        int id = (int)sqlite3_last_insert_rowid(db);
        enviar_fmt(fd, "OK|%d", id);
    } else {
        enviar_mensaje(fd, "ERROR|500|No se pudo insertar el tren");
    }
    sqlite3_finalize(s); sqlite3_close(db);
}

void hadmin_modificar_tren(sock_t fd, char *param) {
    /* param: "id_t|modelo|serie|anio|estado|fecha_rev" */
    char *s_id      = strtok(param, "|");
    char *modelo    = strtok(NULL,  "|");
    char *serie     = strtok(NULL,  "|");
    char *s_anio    = strtok(NULL,  "|");
    char *estado    = strtok(NULL,  "|");
    char *fecha_rev = strtok(NULL,  "|");

    if (!s_id || !modelo || !serie || !s_anio || !estado || !fecha_rev) {
        enviar_mensaje(fd, "ERROR|400|Faltan parametros en MODIFICAR_TREN");
        return;
    }

    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "UPDATE TRENES SET nombre_modelo=?,num_serie=?,anio_fab=?,"
        "estado_mant=?,fecha_ultima_revision=? WHERE id_t=?;", -1, &s, NULL);
    sqlite3_bind_text(s,1,modelo,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,2,serie,-1,SQLITE_STATIC);
    sqlite3_bind_int (s,3,atoi(s_anio));
    sqlite3_bind_text(s,4,estado,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,5,fecha_rev,-1,SQLITE_STATIC);
    sqlite3_bind_int (s,6,atoi(s_id));

    if (sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0)
        enviar_mensaje(fd, "OK|Tren actualizado");
    else
        enviar_mensaje(fd, "ERROR|404|Tren no encontrado");
    sqlite3_finalize(s); sqlite3_close(db);
}

void hadmin_eliminar_tren(sock_t fd, char *param) {
    if (!param) { enviar_mensaje(fd,"ERROR|400|Falta id_t"); return; }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db, "DELETE FROM TRENES WHERE id_t=?;", -1, &s, NULL);
    sqlite3_bind_int(s, 1, atoi(param));
    if (sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0)
        enviar_fmt(fd, "OK|Tren %s eliminado", param);
    else
        enviar_mensaje(fd, "ERROR|404|Tren no encontrado o tiene servicios activos");
    sqlite3_finalize(s); sqlite3_close(db);
}

/* ══════════════════════════════════════════════
   ESTACIONES
   ══════════════════════════════════════════════ */

void hadmin_insertar_estacion(sock_t fd, char *param) {
    /* param: "nombre|ciudad|provincia|andenes" */
    char *nombre    = strtok(param, "|");
    char *ciudad    = strtok(NULL,  "|");
    char *provincia = strtok(NULL,  "|");
    char *s_andenes = strtok(NULL,  "|");

    if (!nombre || !ciudad || !provincia || !s_andenes) {
        enviar_mensaje(fd, "ERROR|400|Formato: INSERTAR_ESTACION|nombre|ciudad|provincia|andenes");
        return;
    }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "INSERT INTO ESTACIONES(nombre,ciudad,provincia,num_andenes) VALUES(?,?,?,?);",
        -1, &s, NULL);
    sqlite3_bind_text(s,1,nombre,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,2,ciudad,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,3,provincia,-1,SQLITE_STATIC);
    sqlite3_bind_int (s,4,atoi(s_andenes));

    if (sqlite3_step(s) == SQLITE_DONE)
        enviar_fmt(fd, "OK|%d", (int)sqlite3_last_insert_rowid(db));
    else
        enviar_mensaje(fd, "ERROR|500|No se pudo insertar la estacion");
    sqlite3_finalize(s); sqlite3_close(db);
}

void hadmin_modificar_estacion(sock_t fd, char *param) {
    /* param: "id_est|nombre|ciudad|provincia|andenes" */
    char *s_id      = strtok(param, "|");
    char *nombre    = strtok(NULL,  "|");
    char *ciudad    = strtok(NULL,  "|");
    char *provincia = strtok(NULL,  "|");
    char *s_andenes = strtok(NULL,  "|");

    if (!s_id || !nombre || !ciudad || !provincia || !s_andenes) {
        enviar_mensaje(fd, "ERROR|400|Faltan parametros en MODIFICAR_ESTACION");
        return;
    }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "UPDATE ESTACIONES SET nombre=?,ciudad=?,provincia=?,num_andenes=? WHERE id_est=?;",
        -1, &s, NULL);
    sqlite3_bind_text(s,1,nombre,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,2,ciudad,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,3,provincia,-1,SQLITE_STATIC);
    sqlite3_bind_int (s,4,atoi(s_andenes));
    sqlite3_bind_int (s,5,atoi(s_id));

    if (sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0)
        enviar_mensaje(fd, "OK|Estacion actualizada");
    else
        enviar_mensaje(fd, "ERROR|404|Estacion no encontrada");
    sqlite3_finalize(s); sqlite3_close(db);
}

/* ══════════════════════════════════════════════
   TRAYECTOS
   ══════════════════════════════════════════════ */

void hadmin_insertar_trayecto(sock_t fd, char *param) {
    /* param: "id_t|id_orig|id_dest|h_sal|h_ll|dur|precio|dias" */
    char *s_id_t   = strtok(param, "|");
    char *s_orig   = strtok(NULL,  "|");
    char *s_dest   = strtok(NULL,  "|");
    char *h_sal    = strtok(NULL,  "|");
    char *h_ll     = strtok(NULL,  "|");
    char *s_dur    = strtok(NULL,  "|");
    char *s_precio = strtok(NULL,  "|");
    char *dias     = strtok(NULL,  "|");

    if (!s_id_t || !s_orig || !s_dest || !h_sal || !h_ll || !s_dur || !s_precio || !dias) {
        enviar_mensaje(fd, "ERROR|400|Faltan parametros en INSERTAR_TRAYECTO");
        return;
    }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "INSERT INTO TRAYECTOS(id_t,id_est_origen,id_est_destino,hora_salida,"
        "hora_llegada,duracion_min,precio_base,dias_operacion,estado)"
        " VALUES(?,?,?,?,?,?,?,?,'ACTIVO');", -1, &s, NULL);
    sqlite3_bind_int   (s,1,atoi(s_id_t));
    sqlite3_bind_int   (s,2,atoi(s_orig));
    sqlite3_bind_int   (s,3,atoi(s_dest));
    sqlite3_bind_text  (s,4,h_sal,-1,SQLITE_STATIC);
    sqlite3_bind_text  (s,5,h_ll,-1,SQLITE_STATIC);
    sqlite3_bind_int   (s,6,atoi(s_dur));
    sqlite3_bind_double(s,7,atof(s_precio));
    sqlite3_bind_text  (s,8,dias,-1,SQLITE_STATIC);

    if (sqlite3_step(s) == SQLITE_DONE)
        enviar_fmt(fd, "OK|%d", (int)sqlite3_last_insert_rowid(db));
    else
        enviar_mensaje(fd, "ERROR|500|No se pudo insertar el trayecto");
    sqlite3_finalize(s); sqlite3_close(db);
}

void hadmin_modificar_trayecto(sock_t fd, char *param) {
    /* param: "id_tr|h_sal|h_ll|precio|dias" */
    char *s_id_tr  = strtok(param, "|");
    char *h_sal    = strtok(NULL,  "|");
    char *h_ll     = strtok(NULL,  "|");
    char *s_precio = strtok(NULL,  "|");
    char *dias     = strtok(NULL,  "|");

    if (!s_id_tr || !h_sal || !h_ll || !s_precio || !dias) {
        enviar_mensaje(fd, "ERROR|400|Faltan parametros en MODIFICAR_TRAYECTO");
        return;
    }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "UPDATE TRAYECTOS SET hora_salida=?,hora_llegada=?,"
        "precio_base=?,dias_operacion=? WHERE id_tr=?;", -1, &s, NULL);
    sqlite3_bind_text  (s,1,h_sal,-1,SQLITE_STATIC);
    sqlite3_bind_text  (s,2,h_ll,-1,SQLITE_STATIC);
    sqlite3_bind_double(s,3,atof(s_precio));
    sqlite3_bind_text  (s,4,dias,-1,SQLITE_STATIC);
    sqlite3_bind_int   (s,5,atoi(s_id_tr));

    if (sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0)
        enviar_mensaje(fd, "OK|Trayecto actualizado");
    else
        enviar_mensaje(fd, "ERROR|404|Trayecto no encontrado");
    sqlite3_finalize(s); sqlite3_close(db);
}

void hadmin_estado_trayecto(sock_t fd, char *param) {
    /* param: "id_tr|estado"  (ACTIVO / SUSPENDIDO / ELIMINADO) */
    char *s_id_tr = strtok(param, "|");
    char *estado  = strtok(NULL,  "|");
    if (!s_id_tr || !estado) {
        enviar_mensaje(fd, "ERROR|400|Formato: ESTADO_TRAYECTO|id_tr|estado");
        return;
    }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "UPDATE TRAYECTOS SET estado=? WHERE id_tr=?;", -1, &s, NULL);
    sqlite3_bind_text(s,1,estado,-1,SQLITE_STATIC);
    sqlite3_bind_int (s,2,atoi(s_id_tr));

    if (sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0)
        enviar_fmt(fd, "OK|Estado del trayecto %s actualizado a %s", s_id_tr, estado);
    else
        enviar_mensaje(fd, "ERROR|404|Trayecto no encontrado");
    sqlite3_finalize(s); sqlite3_close(db);
}

/* ══════════════════════════════════════════════
   USUARIOS Y EMPLEADOS
   ══════════════════════════════════════════════ */

void hadmin_listar_usuarios(sock_t fd) {
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "SELECT id_u, nombre, apellido, email, rol, activo FROM USUARIOS ORDER BY id_u;",
        -1, &s, NULL);
    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        enviar_fmt(fd, "USUARIO|%d|%s|%s|%s|%s|%d",
            sqlite3_column_int(s,0), ctxt(s,1), ctxt(s,2),
            ctxt(s,3), ctxt(s,4), sqlite3_column_int(s,5));
        n++;
    }
    sqlite3_finalize(s); sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

void hadmin_listar_empleados(sock_t fd) {
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "SELECT de.id_u, u.nombre, u.apellido, u.email, de.rol_empleado, de.estado"
        " FROM DATOS_EMPLEADO de JOIN USUARIOS u ON de.id_u = u.id_u ORDER BY de.id_u;",
        -1, &s, NULL);
    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        enviar_fmt(fd, "EMPLEADO|%d|%s|%s|%s|%s|%s",
            sqlite3_column_int(s,0), ctxt(s,1), ctxt(s,2),
            ctxt(s,3), ctxt(s,4), ctxt(s,5));
        n++;
    }
    sqlite3_finalize(s); sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

void hadmin_deshabilitar_user(sock_t fd, char *param) {
    if (!param) { enviar_mensaje(fd, "ERROR|400|Falta id_u"); return; }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    /* Alternamos: si activo=1 lo deshabilitamos, si activo=0 lo habilitamos */
    sqlite3_prepare_v2(db,
        "UPDATE USUARIOS SET activo = CASE WHEN activo=1 THEN 0 ELSE 1 END WHERE id_u=?;",
        -1, &s, NULL);
    sqlite3_bind_int(s, 1, atoi(param));
    if (sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0)
        enviar_fmt(fd, "OK|Estado del usuario %s actualizado", param);
    else
        enviar_mensaje(fd, "ERROR|404|Usuario no encontrado");
    sqlite3_finalize(s); sqlite3_close(db);
}

/* ══════════════════════════════════════════════
   SERVICIOS OPERATIVOS
   ══════════════════════════════════════════════ */

void hadmin_listar_servicios(sock_t fd, char *param) {
    /* param opcional: "fecha" */
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    const char *sql_con =
        "SELECT so.id_serv, so.fecha, t.id_t, eo.nombre, ed.nombre,"
        "       t.hora_salida, t.hora_llegada, so.estado_serv, so.minutos_retraso"
        " FROM SERVICIOS_OPERATIVOS so"
        " JOIN TRAYECTOS t  ON so.id_tr = t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " WHERE so.fecha = ? ORDER BY t.hora_salida;";
    const char *sql_sin =
        "SELECT so.id_serv, so.fecha, t.id_t, eo.nombre, ed.nombre,"
        "       t.hora_salida, t.hora_llegada, so.estado_serv, so.minutos_retraso"
        " FROM SERVICIOS_OPERATIVOS so"
        " JOIN TRAYECTOS t  ON so.id_tr = t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " ORDER BY so.fecha, t.hora_salida;";

    if (param && param[0] != '\0') {
        sqlite3_prepare_v2(db, sql_con, -1, &s, NULL);
        sqlite3_bind_text(s, 1, param, -1, SQLITE_STATIC);
    } else {
        sqlite3_prepare_v2(db, sql_sin, -1, &s, NULL);
    }

    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        enviar_fmt(fd, "SERVICIO|%d|%s|%d|%s|%s|%s|%s|%s|%d",
            sqlite3_column_int(s,0), ctxt(s,1), sqlite3_column_int(s,2),
            ctxt(s,3), ctxt(s,4), ctxt(s,5), ctxt(s,6), ctxt(s,7),
            sqlite3_column_int(s,8));
        n++;
    }
    sqlite3_finalize(s); sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

void hadmin_insertar_servicio(sock_t fd, char *param) {
    /* param: "id_tr|fecha" */
    char *s_id_tr = strtok(param, "|");
    char *fecha   = strtok(NULL,  "|");
    if (!s_id_tr || !fecha) {
        enviar_mensaje(fd, "ERROR|400|Formato: INSERTAR_SERVICIO|id_tr|fecha");
        return;
    }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "INSERT INTO SERVICIOS_OPERATIVOS(id_tr,fecha,estado_serv,minutos_retraso)"
        " VALUES(?,?,'PROGRAMADO',0);", -1, &s, NULL);
    sqlite3_bind_int (s,1,atoi(s_id_tr));
    sqlite3_bind_text(s,2,fecha,-1,SQLITE_STATIC);

    if (sqlite3_step(s) == SQLITE_DONE)
        enviar_fmt(fd, "OK|%d", (int)sqlite3_last_insert_rowid(db));
    else
        enviar_mensaje(fd, "ERROR|500|No se pudo crear el servicio");
    sqlite3_finalize(s); sqlite3_close(db);
}

void hadmin_cancelar_servicio(sock_t fd, char *param) {
    if (!param) { enviar_mensaje(fd, "ERROR|400|Falta id_serv"); return; }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "UPDATE SERVICIOS_OPERATIVOS SET estado_serv='CANCELADO' WHERE id_serv=?;",
        -1, &s, NULL);
    sqlite3_bind_int(s, 1, atoi(param));
    if (sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0)
        enviar_fmt(fd, "OK|Servicio %s cancelado", param);
    else
        enviar_mensaje(fd, "ERROR|404|Servicio no encontrado");
    sqlite3_finalize(s); sqlite3_close(db);
}

/* ══════════════════════════════════════════════
   INCIDENCIAS
   ══════════════════════════════════════════════ */

void hadmin_listar_incidencias(sock_t fd, char *param) {
    /* param opcional: estado (ABIERTA / RESUELTA / TODAS) */
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    int filtrar = (param && param[0] != '\0' && strcmp(param,"TODAS") != 0);

    if (filtrar) {
        sqlite3_prepare_v2(db,
            "SELECT id_inc, id_serv, tipo_incidencia, prioridad, estado, descripcion"
            " FROM INCIDENCIAS WHERE estado=? ORDER BY prioridad DESC, id_inc;",
            -1, &s, NULL);
        sqlite3_bind_text(s, 1, param, -1, SQLITE_STATIC);
    } else {
        sqlite3_prepare_v2(db,
            "SELECT id_inc, id_serv, tipo_incidencia, prioridad, estado, descripcion"
            " FROM INCIDENCIAS ORDER BY prioridad DESC, id_inc;",
            -1, &s, NULL);
    }

    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        enviar_fmt(fd, "INCIDENCIA|%d|%d|%s|%s|%s|%s",
            sqlite3_column_int(s,0), sqlite3_column_int(s,1),
            ctxt(s,2), ctxt(s,3), ctxt(s,4), ctxt(s,5));
        n++;
    }
    sqlite3_finalize(s); sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

void hadmin_insertar_incidencia(sock_t fd, char *param) {
    /* param: "id_serv|tipo|descripcion|prioridad" */
    char *s_id_serv = strtok(param, "|");
    char *tipo      = strtok(NULL,  "|");
    char *desc      = strtok(NULL,  "|");
    char *prioridad = strtok(NULL,  "|");

    if (!s_id_serv || !tipo || !desc || !prioridad) {
        enviar_mensaje(fd, "ERROR|400|Faltan parametros en INSERTAR_INCIDENCIA");
        return;
    }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "INSERT INTO INCIDENCIAS(id_serv,tipo_incidencia,descripcion,prioridad,estado)"
        " VALUES(?,?,?,?,'ABIERTA');", -1, &s, NULL);
    sqlite3_bind_int (s,1,atoi(s_id_serv));
    sqlite3_bind_text(s,2,tipo,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,3,desc,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,4,prioridad,-1,SQLITE_STATIC);

    if (sqlite3_step(s) == SQLITE_DONE)
        enviar_fmt(fd, "OK|%d", (int)sqlite3_last_insert_rowid(db));
    else
        enviar_mensaje(fd, "ERROR|500|No se pudo insertar la incidencia");
    sqlite3_finalize(s); sqlite3_close(db);
}

void hadmin_resolver_incidencia(sock_t fd, char *param, const char *email_admin) {
    /* param: "id_inc|id_u_resolvio" */
    char *s_id_inc = strtok(param, "|");
    char *s_id_u   = strtok(NULL,  "|");
    if (!s_id_inc) { enviar_mensaje(fd,"ERROR|400|Falta id_inc"); return; }

    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "UPDATE INCIDENCIAS SET estado='RESUELTA' WHERE id_inc=?;",
        -1, &s, NULL);
    sqlite3_bind_int(s, 1, atoi(s_id_inc));

    if (sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0) {
        enviar_fmt(fd, "OK|Incidencia %s resuelta", s_id_inc);
        char msg[64];
        snprintf(msg, sizeof(msg), "Incidencia resuelta id=%s", s_id_inc);
        log_evento(cfg.log_path, email_admin, "RESOLUCION_INC", msg);
    } else {
        enviar_mensaje(fd, "ERROR|404|Incidencia no encontrada");
    }
    sqlite3_finalize(s); sqlite3_close(db);
    (void)s_id_u;
}

/* ══════════════════════════════════════════════
   INFORMES
   ══════════════════════════════════════════════ */

void hadmin_informe_ocupacion(sock_t fd, char *param) {
    /* param: "id_t" */
<<<<<<< HEAD
    if (!param || param[0] == '\0') {
        enviar_mensaje(fd, "ERROR|400|Formato: INFORME_OCUPACION|id_t");
        return;
    }
    int id_t = atoi(param);
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;

    /* Misma lógica que informe_ocupacion_tren() del admin local:
       reservas CONFIRMADAS + porcentaje respecto a la capacidad total del tren */
    if (sqlite3_prepare_v2(db,
        "SELECT t.id_tr, eo.nombre, ed.nombre,"
        "       COUNT(r.id_res) AS reservas,"
        "       ROUND(COUNT(r.id_res)*100.0 /"
        "         COALESCE((SELECT SUM(v.capacidad_total) FROM VAGONES v WHERE v.id_tren=t.id_t),100),1) AS pct"
        " FROM TRAYECTOS t"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " LEFT JOIN RESERVAS r ON r.id_tr = t.id_tr AND r.estado='CONFIRMADA'"
        " WHERE t.id_t = ?"
        " GROUP BY t.id_tr ORDER BY t.id_tr;",
        -1, &s, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Consulta de ocupacion fallida");
        sqlite3_close(db); return;
    }
    sqlite3_bind_int(s, 1, id_t);
    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        /* OCUPACION|id_tr|origen|destino|reservas|pct */
        enviar_fmt(fd, "OCUPACION|%d|%s|%s|%d|%.1f",
            sqlite3_column_int(s,0),
            ctxt(s,1), ctxt(s,2),
            sqlite3_column_int(s,3),
            sqlite3_column_double(s,4));
=======
    if (!param) { enviar_mensaje(fd,"ERROR|400|Falta id_t"); return; }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    /* Cuenta reservas ACTIVAS por trayecto del tren dado */
    sqlite3_prepare_v2(db,
        "SELECT t.id_tr, eo.nombre, ed.nombre, COUNT(r.id_res) AS reservas"
        " FROM TRAYECTOS t"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " LEFT JOIN RESERVAS r ON r.id_tr = t.id_tr AND r.estado='ACTIVA'"
        " WHERE t.id_t = ?"
        " GROUP BY t.id_tr ORDER BY t.id_tr;",
        -1, &s, NULL);
    sqlite3_bind_int(s, 1, atoi(param));
    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        enviar_fmt(fd, "OCUPACION|%d|%s|%s|%d",
            sqlite3_column_int(s,0), ctxt(s,1), ctxt(s,2),
            sqlite3_column_int(s,3));
>>>>>>> branch 'main' of https://github.com/UndabeitiaJon/Grupo12_ProgramIV.git
        n++;
    }
    sqlite3_finalize(s); sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

void hadmin_informe_ingresos(sock_t fd, char *param) {
    /* param: "id_tr" */
    if (!param) { enviar_mensaje(fd,"ERROR|400|Falta id_tr"); return; }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "SELECT COUNT(id_res), SUM(precio_final)"
        " FROM RESERVAS WHERE id_tr=? AND estado='ACTIVA';",
        -1, &s, NULL);
    sqlite3_bind_int(s, 1, atoi(param));
    if (sqlite3_step(s) == SQLITE_ROW)
        enviar_fmt(fd, "INGRESOS|%d|%.2f",
            sqlite3_column_int(s,0), sqlite3_column_double(s,1));
    else
        enviar_mensaje(fd, "INGRESOS|0|0.00");
    sqlite3_finalize(s); sqlite3_close(db);
}

void hadmin_informe_incidencias(sock_t fd, char *param) {
    /* param: "f_ini|f_fin" */
    char *f_ini = strtok(param, "|");
    char *f_fin = strtok(NULL,  "|");
    if (!f_ini || !f_fin) {
        enviar_mensaje(fd, "ERROR|400|Formato: INFORME_INCIDENCIAS|f_ini|f_fin");
        return;
    }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "SELECT i.id_inc, so.fecha, i.tipo_incidencia, i.prioridad, i.estado"
        " FROM INCIDENCIAS i"
        " JOIN SERVICIOS_OPERATIVOS so ON i.id_serv = so.id_serv"
        " WHERE so.fecha BETWEEN ? AND ?"
        " ORDER BY so.fecha, i.prioridad DESC;",
        -1, &s, NULL);
    sqlite3_bind_text(s,1,f_ini,-1,SQLITE_STATIC);
    sqlite3_bind_text(s,2,f_fin,-1,SQLITE_STATIC);
    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        enviar_fmt(fd, "INCIDENCIA|%d|%s|%s|%s|%s",
            sqlite3_column_int(s,0), ctxt(s,1), ctxt(s,2), ctxt(s,3), ctxt(s,4));
        n++;
    }
    sqlite3_finalize(s); sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

/* ══════════════════════════════════════════════
   TARIFAS
   ══════════════════════════════════════════════ */

void hadmin_listar_tarifas(sock_t fd) {
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    /* Unimos TARIFAS con TRAYECTOS para mostrar id_tr y precio_base del trayecto */
    if (sqlite3_prepare_v2(db,
        "SELECT ta.id_tr, t.hora_salida, t.hora_llegada,"
        "       ta.precio_base, ta.coef_turista, ta.coef_business,"
        "       ta.suplemento_bici, ta.exceso_kg_precio"
        " FROM TARIFAS ta JOIN TRAYECTOS t ON ta.id_tr = t.id_tr"
        " ORDER BY ta.id_tr;",
        -1, &s, NULL) != SQLITE_OK) {
        enviar_mensaje(fd, "ERROR|500|Consulta de tarifas fallida");
        sqlite3_close(db); return;
    }
    int n = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        enviar_fmt(fd, "TARIFA|%d|%s|%s|%.2f|%.2f|%.2f|%.2f|%.2f",
            sqlite3_column_int(s,0),
            ctxt(s,1), ctxt(s,2),
            sqlite3_column_double(s,3), sqlite3_column_double(s,4),
            sqlite3_column_double(s,5), sqlite3_column_double(s,6),
            sqlite3_column_double(s,7));
        n++;
    }
    sqlite3_finalize(s); sqlite3_close(db);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}

/* B-08: handler para CMD_MOD_COEF_BUSINESS
 * param: "id_tr|coef"
 */
void hadmin_mod_coef_business(sock_t fd, char *param) {
    char *s_id_tr = strtok(param, "|");
    char *s_coef  = strtok(NULL,  "|");
    if (!s_id_tr || !s_coef) {
        enviar_mensaje(fd, "ERROR|400|Formato: MOD_COEF_BUSINESS|id_tr|coef");
        return;
    }
    int rc = modificar_coef_business_db(atoi(s_id_tr), atof(s_coef));
    if (rc == 0)
        enviar_fmt(fd, "OK|Coef. business del trayecto %s actualizado a %s", s_id_tr, s_coef);
    else
        enviar_mensaje(fd, "ERROR|404|Trayecto no encontrado en TARIFAS");
}

/* B-09: handler para CMD_MOD_EXCESO_KG
 * param: "precio_por_kg"
 */
void hadmin_mod_exceso_kg(sock_t fd, char *param) {
    if (!param || param[0] == '\0') {
        enviar_mensaje(fd, "ERROR|400|Formato: MOD_EXCESO_KG|precio_por_kg");
        return;
    }
    int rc = modificar_exceso_kg_db(atof(param));
    if (rc == 0)
        enviar_fmt(fd, "OK|Precio exceso kg actualizado a %s eur/kg", param);
    else
        enviar_mensaje(fd, "ERROR|500|No se pudo actualizar el precio de exceso de kg");
}

/* B-10: handler para CMD_MOD_SUPL_BICI
 * param: "precio"
 */
void hadmin_mod_supl_bici(sock_t fd, char *param) {
    if (!param || param[0] == '\0') {
        enviar_mensaje(fd, "ERROR|400|Formato: MOD_SUPL_BICI|precio");
        return;
    }
    int rc = modificar_suplemento_bici_db(atof(param));
    if (rc == 0)
        enviar_fmt(fd, "OK|Suplemento bicicleta actualizado a %s eur", param);
    else
        enviar_mensaje(fd, "ERROR|500|No se pudo actualizar el suplemento de bicicleta");
}

void hadmin_mod_precio_base(sock_t fd, char *param) {
    /* param: "id_tr|precio" */
    char *s_id_tr  = strtok(param, "|");
    char *s_precio = strtok(NULL,  "|");
    if (!s_id_tr || !s_precio) {
        enviar_mensaje(fd, "ERROR|400|Formato: MOD_PRECIO_BASE|id_tr|precio");
        return;
    }
    sqlite3 *db = abrir_db_admin(fd); if (!db) return;
    sqlite3_stmt *s;
    sqlite3_prepare_v2(db,
        "UPDATE TRAYECTOS SET precio_base=? WHERE id_tr=?;", -1, &s, NULL);
    sqlite3_bind_double(s,1,atof(s_precio));
    sqlite3_bind_int   (s,2,atoi(s_id_tr));
    if (sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0)
        enviar_fmt(fd, "OK|Precio de trayecto %s actualizado a %s", s_id_tr, s_precio);
    else
        enviar_mensaje(fd, "ERROR|404|Trayecto no encontrado");
    sqlite3_finalize(s); sqlite3_close(db);
}

/* ══════════════════════════════════════════════
   LOGS
   ══════════════════════════════════════════════ */

void hadmin_ver_logs(sock_t fd, char *param) {
    /* param opcional: "fecha|usuario|nivel" */
    /* Leemos las últimas 200 líneas del fichero de log del servidor */
    FILE *f = fopen(cfg.log_path, "r");
    if (!f) {
        enviar_mensaje(fd, "ERROR|404|Fichero de log no encontrado");
        return;
    }

    /* Filtro simple por cadena de usuario si se pasó */
    char filtro_usr[128] = "";
    char filtro_fecha[12] = "";
    if (param && param[0]) {
        char tmp[256];
        strncpy(tmp, param, sizeof(tmp)-1);
        char *f_fecha = strtok(tmp, "|");
        char *f_usr   = strtok(NULL, "|");
        if (f_fecha) strncpy(filtro_fecha, f_fecha, sizeof(filtro_fecha)-1);
        if (f_usr)   strncpy(filtro_usr,   f_usr,   sizeof(filtro_usr)-1);
    }

    char linea[1024];
    int n = 0;
    while (fgets(linea, sizeof(linea), f)) {
        /* Eliminar salto de línea */
        int len = (int)strlen(linea);
        if (len > 0 && linea[len-1] == '\n') linea[len-1] = '\0';

        /* Aplicar filtros si los hay */
        if (filtro_fecha[0] && !strstr(linea, filtro_fecha)) continue;
        if (filtro_usr[0]   && !strstr(linea, filtro_usr))   continue;

        enviar_fmt(fd, "LOG|%s", linea);
        n++;
    }
    fclose(f);
    enviar_fmt(fd, "FIN_LISTA|%d", n);
}
