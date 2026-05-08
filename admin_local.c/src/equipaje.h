/*
 * equipaje.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_EQUIPAJE_H_
#define SRC_EQUIPAJE_H_

#include "tipos_comunes.h"

typedef struct {
    int          id_eq;
    int          id_res;
    TipoEquipaje tipo;
    double       peso_kg;
    char         dimensiones[64];
    double       exceso_kg;
    double       suplemento_pago;
    int          facturado;
} Equipaje;

#endif /* SRC_EQUIPAJE_H_ */
