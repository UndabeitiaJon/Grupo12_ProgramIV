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

//TRENES
int insertar_tren_db(Tren t);
void listar_trenes_db();
int modificar_estado_tren_db(int id_t, EstadoMantenimiento nuevo_estado);
int eliminar_tren_db(int id_t);
Tren obtener_tren_por_id(int id_t);
void buscar_tren_por_modelo(const char *modelo);
int modificar_tren_db(int id_t, const char *modelo, const char *num_serie,int anio, EstadoMantenimiento estado, const char *fecha_rev);
void listar_trenes_con_vagones(void); //

//ESTACIONES
int insertar_estacion_db(Estacion e);
void listar_estaciones_db();


//TRAYECTOS
int insertar_trayecto_db(Trayecto tr);
int cargar_trayectos_csv(const char *ruta_csv);
void listar_trayectos_db(void);
int modificar_trayecto_db(int id_tr, const char *hora_sal, const char *hora_ll, double precio, const char *dias);
int cambiar_estado_trayecto_db(int id_tr, EstadoTrayecto estado);
Trayecto obtener_trayecto_por_id(int id_tr);
/* Busca trayectos por origen/destino/fecha/clase */
int buscar_trayectos_db(int id_origen, int id_destino, const char *fecha, const char *clase);


#endif /* SRC_DB_MANAGER_H_ */
