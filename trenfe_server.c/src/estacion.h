/*
 * estacion.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_ESTACION_H_
#define SRC_ESTACION_H_

typedef struct {
    int    id_est;
    char   nombre[128];
    char   codigo_gtfs[16];
    char   ciudad[64];
    char   provincia[64];
    double latitud;
    double longitud;
    int    num_andenes;
    int    tiene_sala_club;
} Estacion;

#endif /* SRC_ESTACION_H_ */
