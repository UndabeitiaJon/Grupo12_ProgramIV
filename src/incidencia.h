/*
 * incidencia.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_INCIDENCIA_H_
#define SRC_INCIDENCIA_H_

#include "tipos_comunes.h"

typedef struct {
    int                 id_inc;
    int                 id_serv;
    int                 id_u_reporta;
    TipoIncidencia      tipo;
    char                descripcion[512];
    PrioridadIncidencia prioridad;
    EstadoIncidencia    estado;
    char                fecha_reporte[11];     /* "AAAA-MM-DD\0" */
    char                fecha_resolucion[11];  /* "AAAA-MM-DD\0" */
    int                 tiene_fecha_resolucion;/* 0 si no resuelta */
    int                 id_u_resuelve;
    int                 tiene_resolutor;       /* 0 si no asignado */
} Incidencia;

#endif /* SRC_INCIDENCIA_H_ */
