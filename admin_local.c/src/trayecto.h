/*
 * trayecto.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_TRAYECTO_H_
#define SRC_TRAYECTO_H_

#include "tipos_comunes.h"

typedef struct {
    int            id_tr;
    int            id_t;
    int            id_est_origen;
    int            id_est_destino;
    char           hora_salida[6];
    char           hora_llegada[6];
    int            duracion_min;
    double         precio_base;
    char           dias_operacion[8];
    EstadoTrayecto estado;
} Trayecto;

#endif /* SRC_TRAYECTO_H_ */
