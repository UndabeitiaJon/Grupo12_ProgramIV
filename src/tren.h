/*
 * tren.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_TREN_H_
#define SRC_TREN_H_

#include "tipos_comunes.h"

typedef struct {
    int                 id_t;
    char                nombre_modelo[64];
    char                num_serie[32];
    int                 anio_fab;
    EstadoMantenimiento estado_mant;
    char                fecha_ultima_revision[11]; /* "AAAA-MM-DD\0" */
    int                 tiene_revision;            /* 0 si NULL en BD */
} Tren;


#endif /* SRC_TREN_H_ */
