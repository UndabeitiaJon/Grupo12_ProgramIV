/*
 * log_operativo.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_LOG_OPERATIVO_H_
#define SRC_LOG_OPERATIVO_H_

#include "tipos_comunes.h"

typedef struct {
    int      id_log;
    char     timestamp[20];
    int      id_u;
    int      tiene_usuario;
    char     accion[128];
    char     entidad_afectada[64];
    int      id_entidad;
    char     ip_origen[46];
    NivelLog nivel;
    char     detalle_json[1024];
    int      tiene_detalle;
} LogOperativo;

#endif /* SRC_LOG_OPERATIVO_H_ */
