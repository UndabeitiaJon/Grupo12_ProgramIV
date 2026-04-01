/*
 ============================================================================
 Name        : Ekain Aranoa
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

#include "estructuras.h"
#include "db_manager.h"

int main(void) {
	printf("BIENVENIDO A TRENFE");

	return EXIT_SUCCESS;
}



void menu_alta_usuario() {
    // AQUI ESTAN LOS CORCHETES, SI NO ESTAN, EXPLOTA
    char nombre[64], apellido[64], dni[16], email[128], telf[20], pass[256], fecha_nac[11];
    int rol_opcion;
    RolUsuario rol;

    printf("\n=== ALTA DE NUEVO USUARIO ===\n");
    fflush(stdout);
    #define CLEAN_BUFFER while(getchar() != '\n')

    printf("Nombre: ");
    scanf("%63s", nombre); CLEAN_BUFFER;

    printf("Apellido: ");
    scanf("%63s", apellido); CLEAN_BUFFER;

    printf("DNI: ");
    scanf("%15s", dni); CLEAN_BUFFER;

    printf("Email: ");
    scanf("%127s", email); CLEAN_BUFFER;

    printf("Telefono: ");
    scanf("%19s", telf); CLEAN_BUFFER;

    printf("Fecha Nacimiento (AAAA-MM-DD): ");
    scanf("%10s", fecha_nac); CLEAN_BUFFER;

    printf("Contrasena temporal: ");
    scanf("%255s", pass); CLEAN_BUFFER;

    printf("Seleccione el Rol (0: Pasajero, 1: Maquinista, 2: Admin): ");
    scanf("%d", &rol_opcion); CLEAN_BUFFER;

    if (rol_opcion == 2) rol = ROL_ADMIN;
    else if (rol_opcion == 1) rol = ROL_EMPLEADO;
    else rol = ROL_PASAJERO;

    Usuario nuevo = crearUsuario(nombre, apellido, dni, email, telf, pass, fecha_nac, rol);

    printf("\nGuardando en la base de datos...\n");
    if (insertar_usuario_db(nuevo) == 0) {
        printf("¡Alta completada con éxito!\n");
    } else {
        printf("Error al dar de alta el usuario.\n");
    }
}
