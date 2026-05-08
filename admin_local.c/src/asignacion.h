/*
 * asignacion.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_ASIGNACION_H_
#define SRC_ASIGNACION_H_

#include "tipos_comunes.h"

typedef struct {
    int         id_asig;
    int         id_serv;
    int         id_u;
    int         id_t;
    RolServicio rol_servicio;
    char        observaciones[256];
    int         tiene_observaciones;
} AsignacionPersonal;

#endif /* SRC_ASIGNACION_H_ */
