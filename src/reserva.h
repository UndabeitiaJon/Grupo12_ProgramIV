/*
 * reserva.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_RESERVA_H_
#define SRC_RESERVA_H_

#include "tipos_comunes.h"

typedef struct {
    int           id_res;
    int           id_u;
    int           id_tr;
    char          fecha_viaje[11];       /* "AAAA-MM-DD\0" */
    char          clase[4];              /* "B" o "T" */
    int           num_vagon;
    int           num_asiento;
    double        precio_base;
    double        descuento_pct;
    double        precio_final;
    EstadoReserva estado;
    char          codigo_validacion[64];
    char          fecha_reserva[11];     /* "AAAA-MM-DD\0" */
} Reserva;

#endif /* SRC_RESERVA_H_ */
