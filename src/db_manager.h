/*
 * db_manager.h
 *
 *  Created on: 30 mar 2026
 *      Author: ander.lecue
 */

#ifndef SRC_DB_MANAGER_H_
#define SRC_DB_MANAGER_H_
#include "sqlite3.h"
#define DB_PATH "./data/trenfe.db"
#include "estructuras.h"

int init_database();
void importar_usuarios_csv(const char* ruta_csv);
int insertar_usuario_db(Usuario u);
const char* rol_a_texto(RolUsuario rol);
void listar_usuarios_db();

#endif /* SRC_DB_MANAGER_H_ */
