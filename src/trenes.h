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


// Menus
void menu_gestion_trenes();
void menu_anadir_tren();
// Helpers
const char* estado_tren_a_texto(EstadoMantenimiento estado);
EstadoMantenimiento texto_a_estado_tren(const char *texto);

#endif
