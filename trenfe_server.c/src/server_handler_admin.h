/*
 * server_handler_admin.h
 *
 *  Created on: 9 may 2026
 *      Author: e.aranoa
 */
/*
 * server_handler_admin.h  -  Sistema TRENFE  -  Fase 2
 *
 * Handlers exclusivos del rol ADMIN.
 * Se incluye desde server_handler.c.
 */

#ifndef SERVER_HANDLER_ADMIN_H_
#define SERVER_HANDLER_ADMIN_H_

#include "server_socket.h"

/* Cada función recibe el fd del cliente y los parámetros ya separados */

/* ── Trenes ── */
void hadmin_listar_trenes    (sock_t fd);
void hadmin_insertar_tren    (sock_t fd, char *param);
void hadmin_modificar_tren   (sock_t fd, char *param);
void hadmin_eliminar_tren    (sock_t fd, char *param);

/* ── Estaciones ── */
void hadmin_insertar_estacion (sock_t fd, char *param);
void hadmin_modificar_estacion(sock_t fd, char *param);

/* ── Trayectos ── */
void hadmin_insertar_trayecto (sock_t fd, char *param);
void hadmin_modificar_trayecto(sock_t fd, char *param);
void hadmin_estado_trayecto   (sock_t fd, char *param);

/* ── Usuarios y empleados ── */
void hadmin_listar_usuarios   (sock_t fd);
void hadmin_listar_empleados  (sock_t fd);
void hadmin_deshabilitar_user (sock_t fd, char *param);

/* ── Servicios operativos ── */
void hadmin_listar_servicios  (sock_t fd, char *param);
void hadmin_insertar_servicio (sock_t fd, char *param);
void hadmin_cancelar_servicio (sock_t fd, char *param);
void hadmin_asignar_empleado   (sock_t fd, char *param);

/* ── Incidencias ── */
void hadmin_listar_incidencias (sock_t fd, char *param);
void hadmin_insertar_incidencia(sock_t fd, char *param);
void hadmin_resolver_incidencia(sock_t fd, char *param, const char *email_admin);

/* ── Informes ── */
void hadmin_informe_ocupacion  (sock_t fd, char *param);
void hadmin_informe_ingresos   (sock_t fd, char *param);
void hadmin_informe_incidencias(sock_t fd, char *param);

/* ── Tarifas ── */
void hadmin_listar_tarifas    (sock_t fd);
void hadmin_mod_precio_base   (sock_t fd, char *param);
void hadmin_mod_coef_business (sock_t fd, char *param);   /* B-08 */
void hadmin_mod_exceso_kg     (sock_t fd, char *param);   /* B-09 */
void hadmin_mod_supl_bici     (sock_t fd, char *param);   /* B-10 */

/* ── Logs ── */
void hadmin_ver_logs(sock_t fd, char *param);

#endif /* SERVER_HANDLER_ADMIN_H_ */
