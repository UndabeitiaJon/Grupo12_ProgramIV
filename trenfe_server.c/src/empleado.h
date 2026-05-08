/*
 * empleado.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_EMPLEADO_H_
#define SRC_EMPLEADO_H_

#include "tipos_comunes.h"

typedef struct {
    int            id_u;
    char           num_empleado[32];
    char           num_ss[32];
    char           fecha_ingreso[11];
    char           rol_empleado[64];
    int            anios_exp;
    char           telf_empresa[20];
    EstadoEmpleado estado;
} DatosEmpleado;

#endif /* SRC_EMPLEADO_H_ */
