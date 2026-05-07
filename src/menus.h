/*
 * menus.h  –  Sistema TRENFE  –  Fase 1
 * Declaración de todos los menús del sistema por rol.
 */
#ifndef SRC_MENUS_H_
#define SRC_MENUS_H_

#include "estructuras.h"

/* ---- Menú entrada ---- */
void menu_inicial(void);
void menu_login(void);
void menu_registro_pasajero(void);

/* ---- Menú Admin (13 secciones) ---- */
void menu_principal_admin(int id_admin, const char *email);
void menu_gestion_trenes(int id_admin, const char *email);
void menu_gestion_trayectos(int id_admin, const char *email);
void menu_gestion_estaciones(int id_admin, const char *email);
void menu_gestion_personal(int id_admin, const char *email);
void menu_gestion_pasajeros(int id_admin, const char *email);
void menu_gestion_servicios(int id_admin, const char *email);
void menu_gestion_tarifas(int id_admin, const char *email);
void menu_importar_gtfs(int id_admin, const char *email);
void menu_incidencias(int id_admin, const char *email);
void menu_informes(int id_admin, const char *email);
void menu_logs(int id_admin, const char *email);
void menu_configuracion(int id_admin, const char *email);

/* ---- Menú Pasajero (5 secciones) ---- */
void menu_principal_pasajero(int id_u, const char *email);
void menu_buscar_trayecto(int id_u);
void menu_mis_reservas(int id_u);
void menu_puntos_fidelizacion(int id_u);
void menu_mis_datos_pasajero(int id_u, const char *email);

/* ---- Menú Maquinista (3 secciones) ---- */
void menu_principal_maquinista(int id_u, const char *email);
void menu_cuadrante_servicios(int id_u);
void menu_mis_datos_maquinista(int id_u, const char *email);

/* ---- Utilidades ---- */
void limpiar_pantalla(void);
void pausar(void);
int  leer_entero(const char *prompt);
double leer_double(const char *prompt);
void leer_cadena(const char *prompt, char *buf, int max);
static void limpiar_buffer_entrada(void);

#endif /* SRC_MENUS_H_ */
