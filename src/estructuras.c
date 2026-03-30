/*
 * trenfe.c
 *
 *  Created on: 29 mar 2026
 *      Author: jon.undabeitia (Unai Erdoiza seguro que no)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "estructuras.h"


Usuario crearUsuario(char *nombre, char *apellido, char *dni,
                     char *email, char *telefono, char *contrasenia,
                     char *fechaNacimiento, RolUsuario rol) {
    static int contador = 0;

    Usuario u;
    u.id_u = contador++;
    strcpy(u.nombre, nombre);
    strcpy(u.apellido, apellido);
    strcpy(u.dni, dni);
    strcpy(u.email, email);
    strcpy(u.telf, telefono);
    strcpy(u.pass_hash, contrasenia);
    strcpy(u.fecha_nac, fechaNacimiento);
    u.rol    = rol;
    u.activo = 1;  // activo por defecto al crearse

    // fecha de registro: la ponemos como la fecha actual (manual por ahora)
    strcpy(u.fecha_registro, "2026-03-29 10:00:00");

    return u;
}
