/*
 * servicio.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_SERVICIO_H_
#define SRC_SERVICIO_H_

#include "tipos_comunes.h"

typedef struct {
    int            id_serv;
    int            id_tr;
    char           fecha[11];
    EstadoServicio estado_serv;
    char           hora_inicio_real[6];
    int            tiene_hora_inicio;
    char           hora_fin_real[6];
    int            tiene_hora_fin;
    int            minutos_retraso;
    char           causa_retraso[256];
    int            tiene_causa;
} ServicioOperativo;

#endif /* SRC_SERVICIO_H_ */
