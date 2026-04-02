/*
 * trenes.h
 *
 *  Created on: 3 abr 2026
 *      Author: e.aranoa
 */

#ifndef TRENES_H
#define TRENES_H
#include "estructuras.h"

// CRUD base
int insertar_tren_db(Tren t);
void listar_trenes_db();
int modificar_estado_tren_db(int id_t, EstadoMantenimiento nuevo_estado);
int eliminar_tren_db(int id_t);
Tren obtener_tren_por_id(int id_t);
void buscar_tren_por_modelo(const char *modelo);

// Menus
void menu_gestion_trenes();

// Helpers
const char* estado_tren_a_texto(EstadoMantenimiento estado);
EstadoMantenimiento texto_a_estado_tren(const char *texto);

#endif
