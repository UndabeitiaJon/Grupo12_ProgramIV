/*
 * trenfe.c
 *
 *  Created on: 29 mar 2026
 *      Author:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "estructuras.h"


Usuario crearUsuario(char *nombre, char *apellido, char *dni,
                     char *email, char *telefono, char *contrasenia,
                     char *fechaNacimiento, RolUsuario rol) {

    Usuario u;
    memset(&u, 0, sizeof(u));
    u.id_u = 0;
    strncpy(u.nombre, nombre,sizeof(u.nombre)- 1);
    strncpy(u.apellido, apellido, sizeof(u.apellido)- 1);
    strncpy(u.dni, dni, sizeof(u.dni) - 1);
    strncpy(u.email, email, sizeof(u.email)- 1);
    strncpy(u.telf, telefono, sizeof(u.telf)- 1);
    strncpy(u.pass_hash, contrasenia, sizeof(u.pass_hash) - 1);
    strncpy(u.fecha_nac, fechaNacimiento, sizeof(u.fecha_nac) - 1);
    u.rol    = rol;
    u.activo = 1;  // activo por defecto al crearse

    // fecha de registro: la ponemos como la fecha actual (manual por ahora)
    strcpy(u.fecha_registro, "2026-03-29 10:00:00");

    return u;
}
