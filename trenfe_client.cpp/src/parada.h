/*
 * parada.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_PARADA_H_
#define SRC_PARADA_H_

typedef struct {
    int  id_parada;
    int  id_tr;
    int  id_est;
    int  orden;
    char hora_llegada[6];
    char hora_salida[6];
    int  anden;
    int  tiene_anden;
} ParadaIntermedia;

#endif /* SRC_PARADA_H_ */
