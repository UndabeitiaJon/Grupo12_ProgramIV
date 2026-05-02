/*
 * pasajero.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef PASAJERO_H_
#define PASAJERO_H_

#include "tipos_comunes.h"

typedef struct {
    int           id_u;
    int           puntos_fidelidad;
    TipoDescuento tipo_descuento;
    char          num_tarjeta_fidelizacion[32];
    int           tiene_tarjeta;
    int           necesidad_asistencia_pmr;
} DatosPasajero;

#endif /* PASAJERO_H_ */
