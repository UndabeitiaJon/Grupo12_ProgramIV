/*
 * vagon.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_VAGON_H_
#define SRC_VAGON_H_

typedef struct {
    int  id_vagon;
    int  id_tren;
    int  numero_vagon;
    char clase[4];       /* "T"=Turista, "B"=Business, "P"=PMR */
    int  capacidad_total;
    int  vagon_PMR;      /* 0=no, 1=si */
} Vagon;


#endif /* SRC_VAGON_H_ */
