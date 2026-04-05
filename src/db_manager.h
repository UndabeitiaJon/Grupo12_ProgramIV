/*
 * db_manager.h
 *
 *  Created on: 30 mar 2026
 *      Author: ander.lecue
 */

#ifndef SRC_DB_MANAGER_H_
#define SRC_DB_MANAGER_H_
#include "sqlite3.h"
#include "config.h"
#include "estructuras.h"
#include <stdbool.h>

int init_database();
void seed_database(void);

//Usuarios
// los que tienen // es que falta de implementar
const char* rol_a_texto(RolUsuario rol);
RolUsuario obtener_rol_usuario(const char *email); //aqui podemos ver si es mejor en base al email o dni o asi
int obtener_id_usuario(const char *email); //aqui podemos ver si es mejor en base al email o dni o asi
//Funcion para obtener un usuario de la bd
Usuario obtener_usuario(const char *email);
int insertar_usuario_db(Usuario u);
void listar_usuarios_db();
void listar_empleados_db();
int modificar_usuario_db(int id_u, const char *campo, const char *valor);
int deshabilitar_usuario_db(int id_u);
void buscar_usuario_db(const char *dni_o_nombre);
bool verificar_usuario(char *email, char *contraseniaIntroducida);
//Funcion para comprobar si el usuario existe en la bbdd
bool comprobar_usuario_registrado(char *email);
//Funcion para comprobar la contraseña del usuario
bool comprobar_contrasenia (char *email, char *contraseniaIntroducida);
int cambiar_contrasenia_db(const char *email, const char *nueva_pass);
void importar_usuarios_csv(const char* ruta_csv);
Usuario obtener_usuario_por_id(int id_u);
Usuario obtener_usuario_por_email(const char *email);


/* ============================================================
 *  DATOS PASAJERO (puntos fidelidad, descuento)
 * ============================================================ */
int  obtener_puntos_fidelidad(int id_u);
int  actualizar_puntos_fidelidad(int id_u, int puntos);
void listar_historial_puntos(int id_u);
TipoDescuento obtener_descuento_usuario(int id_u);
int  actualizar_descuento_usuario(int id_u, TipoDescuento tipo);

//TRENES
int insertar_tren_db(Tren t);
void listar_trenes_db();
int modificar_estado_tren_db(int id_t, EstadoMantenimiento nuevo_estado);
int eliminar_tren_db(int id_t);
Tren obtener_tren_por_id(int id_t);
void buscar_tren_por_modelo(const char *modelo);
int modificar_tren_db(int id_t, const char *modelo, const char *num_serie,int anio, EstadoMantenimiento estado, const char *fecha_rev);
//void listar_trenes_con_vagones(void);

//VAGONES
int  insertar_vagon_db(Vagon v);
void listar_vagones_tren(int id_tren);
int  contar_asientos_libres(int id_tr, const char *fecha_viaje, int num_vagon, const char *clase);
/* Devuelve mapa de asientos (1=libre, 0=ocupado) para un vagón */
void mostrar_mapa_asientos(int id_tr, const char *fecha_viaje, int num_vagon);
int  modificar_estacion_db(int id_est, const char *nombre, const char *ciudad, const char *provincia, int num_andenes);
Estacion  obtener_estacion_por_id(int id_est);

//ESTACIONES
int insertar_estacion_db(Estacion e);
void listar_estaciones_db();
int modificar_estacion_db(int id_est, const char *nombre, const char *ciudad, const char *provincia, int num_andenes);
int toggle_sala_club_db(int id_est);
Estacion  obtener_estacion_por_id(int id_est);


//TRAYECTOS
int insertar_trayecto_db(Trayecto tr);
int cargar_trayectos_csv(const char *ruta_csv);
void listar_trayectos_db(void);
int modificar_trayecto_db(int id_tr, const char *hora_sal, const char *hora_ll, double precio, const char *dias);
int cambiar_estado_trayecto_db(int id_tr, EstadoTrayecto estado);
Trayecto obtener_trayecto_por_id(int id_tr);
/* Busca trayectos por origen/destino/fecha/clase */
int buscar_trayectos_db(int id_origen, int id_destino, const char *fecha, const char *clase);


//PARADAS INTERMEDIAS
int insertar_parada_db(ParadaIntermedia p);
void listar_paradas_trayecto(int id_tr);
int eliminar_parada_db(int id_parada);

//RESERVAS
int insertar_reserva_db(Reserva r);
void listar_reservas_usuario(int id_u);
void listar_reservas_activas_usuario(int id_u);
void listar_historial_usuario(int id_u);
int cancelar_reserva_db(int id_res, int id_u);
Reserva obtener_reserva_por_id(int id_res);
/* Genera código de validación único */
void generar_codigo_validacion(char *buf, int len);
/* Calcula precio final según clase, descuento y suplementos */
double calcular_precio_final(int id_tr, const char *clase, TipoDescuento desc, double suplementos_extra);
/* Comprueba si un asiento está libre en esa fecha/trayecto */
bool asiento_libre(int id_tr, const char *fecha, int vagon, int asiento);

//EQUIPAJE
int insertar_equipaje_db(Equipaje eq);
void listar_equipaje_reserva(int id_res);
double calcular_suplemento_equipaje(TipoEquipaje tipo, double peso_kg,const char *clase);

//SERVICIOS OPERATIVOS
int insertar_servicio_db(ServicioOperativo s);
void listar_servicios_db(const char *filtro_fecha, int filtro_tren);
int cancelar_servicio_db(int id_serv);
int marcar_inicio_servicio(int id_serv);
int marcar_fin_servicio(int id_serv);
int actualizar_retraso_servicio(int id_serv, int minutos, const char *causa);
ServicioOperativo obtener_servicio_por_id(int id_serv);
/* Lista servicios asignados a un maquinista */
void listar_servicios_maquinista(int id_u);

//ASIGNACION DE PERSONAL
int insertar_asignacion_db(AsignacionPersonal a);
void listar_asignaciones_servicio(int id_serv);
int eliminar_asignacion_db(int id_asig);

//INCIDENCIAS
int insertar_incidencia_db(Incidencia inc);
void listar_incidencias_db(EstadoIncidencia filtro_estado, int todas);
int resolver_incidencia_db(int id_inc, int id_u_resuelve);
Incidencia obtener_incidencia_por_id(int id_inc);
void ver_detalle_incidencia(int id_inc);

//TARIFAS
void listar_tarifas_db(void);
int modificar_precio_base_trayecto(int id_tr, double nuevo_precio);
int modificar_coef_business_db(int id_tr, double coef);
int modificar_suplemento_bici_db(double precio);
int modificar_exceso_kg_db(double precio_por_kg);

//INFORMES
void informe_ocupacion_tren(int id_tren);
void informe_ingresos_trayecto(int id_tr);
void informe_incidencias_periodo(const char *fecha_ini, const char *fecha_fin);
void informe_empleados_activos(void);

//LOGS
void consultar_logs_db(const char *filtro_fecha, const char *filtro_usuario,const char *filtro_nivel);

//IMPORTAR
int importar_gtfs(const char *ruta_directorio);
void resumen_ultima_importacion(void);

#endif /* SRC_DB_MANAGER_H_ */
