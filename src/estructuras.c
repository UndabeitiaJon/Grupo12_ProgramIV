/*
 * trenfe.c
 *
 *  Created on: 29 mar 2026
 *      Author: jon.undabeitia (Unai Erdoiza seguro que no)
 */

#include <stdio.h>
#include <stdlib.h>
#include "estructuras.h"


Usuario crearUsuario(char *nombre, char *apellido, char *dni,
                     char *email, char *telefono, char *contrasenia,
                     Date fechaNacimiento, Rol rol) {
    static int contador = 0;

    Usuario u;
    u.id = contador++;
    strcpy(u.nombre, nombre);
    strcpy(u.apellido, apellido);
    strcpy(u.dni, dni);
    strcpy(u.email, email);
    strcpy(u.telefono, telefono);
    strcpy(u.contrasenia, contrasenia);
    u.fechaNacimiento = fechaNacimiento;
    u.rol    = rol;
    u.activo = 1;  // activo por defecto al crearse

    // fecha de registro: la ponemos como la fecha actual (manual por ahora)
    u.fechaRegistro = (Date){29, 3, 2025};

    return u;
}
