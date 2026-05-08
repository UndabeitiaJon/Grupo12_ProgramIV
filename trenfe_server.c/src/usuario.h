/*
 * usuario.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef USUARIO_H_
#define USUARIO_H_

#include "tipos_comunes.h"

typedef struct {
    int        id_u;
    char       nombre[64];
    char       apellido[64];
    char       dni[16];
    char       email[128];
    char       telf[20];
    char       fecha_nac[11];        /* "AAAA-MM-DD\0" */
    char       pass_hash[256];
    RolUsuario rol;
    int        activo;
    char       fecha_registro[20];
} Usuario;

Usuario crearUsuario(char *nombre, char *apellido, char *dni,
                     char *email, char *telefono, char *contrasenia,
                     char *fechaNacimiento, RolUsuario rol);

#endif /* USUARIO_H_ */
