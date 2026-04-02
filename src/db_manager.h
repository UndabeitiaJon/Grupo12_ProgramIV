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
void listar_empleados_db(void); //
int modificar_usuario_db(int id_u, const char *campo, const char *valor); //
int deshabilitar_usuario_db(int id_u); //
void buscar_usuario_db(const char *dni_o_nombre);//
bool verificar_usuario(char *email, char *contraseniaIntroducida);
//Funcion para comprobar si el usuario existe en la bbdd
bool comprobar_usuario_registrado(char *email);
//Funcion para comprobar la contraseña del usuario
bool comprobar_contrasenia (char *email, char *contraseniaIntroducida);
int cambiar_contrasenia_db(const char *email, const char *nueva_pass); //
void importar_usuarios_csv(const char* ruta_csv);
Usuario obtener_usuario_por_id(int id_u); //
Usuario obtener_usuario_por_email(const char *email); //
#endif /* SRC_DB_MANAGER_H_ */
