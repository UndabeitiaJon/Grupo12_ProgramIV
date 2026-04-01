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
void importar_usuarios_csv(const char* ruta_csv);
int insertar_usuario_db(Usuario u);
const char* rol_a_texto(RolUsuario rol);
void listar_usuarios_db();

bool verificar_usuario(char *email, char *contraseniaIntroducida);
//Funcion para comprobar si el usuario existe en la bbdd
bool comprobar_usuario_registrado(char *email);
//Funcion para comprobar la contraseña del usuario
bool comprobar_contrasenia (char *email, char *contraseniaIntroducida);
#endif /* SRC_DB_MANAGER_H_ */
