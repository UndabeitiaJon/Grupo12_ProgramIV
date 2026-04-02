/*
 * menus.c
 *
 *  Created on: 1 abr 2026
 *      Author: e.aranoa
 */
#include <stdio.h>
#include <stdlib.h>
#include "estructuras.h"
#include "db_manager.h"
#include "config.h"
#include "logs.h"

void menu_principal(char *email_logueado);
void menu_alta_usuario();

void menu_login() {
    char email[128], pass[256];
    int intentos = 3;

    printf("\n=== INICIO DE SESION ===\n");
    while (intentos-- > 0) {
        printf("Email: ");
        scanf("%127s", email);
        printf("Contrasena: ");
        scanf("%255s", pass);

        if (verificar_usuario(email, pass)) {
            log_evento(cfg.log_path, email, "LOGIN", "Inicio de sesion correcto");
            menu_principal(email);
            return;
        }
        printf("Credenciales incorrectas. Intentos restantes: %d\n", intentos);
    }
    log_evento(cfg.log_path, email, "LOGIN_FAIL", "Demasiados intentos fallidos");
    printf("Demasiados intentos fallidos.\n");
}

void menu_principal(char *email_logueado) {
    int opcion;
    do {
        printf("\n=== MENU PRINCIPAL ===\n");
        printf("1. Gestion de usuarios\n");
        printf("2. Gestion de trenes\n");
        printf("3. Gestion de trayectos\n");
        printf("4. Gestion de reservas\n");
        printf("5. Importar usuarios desde CSV\n");
        printf("0. Cerrar sesion\n");
        printf("Opcion: ");
        scanf("%d", &opcion);

        switch(opcion) {
            case 1: menu_alta_usuario(); break;
            case 5:
                importar_usuarios_csv("./data/usuarios.csv");
                break;
            case 0:
                log_evento(cfg.log_path, email_logueado, "LOGOUT", "Sesion cerrada");
                printf("Sesion cerrada.\n");
                break;
            default: printf("Opcion no valida.\n");
        }
    } while(opcion != 0);
}

void menu_alta_usuario() {
    char nombre[64], apellido[64], dni[16], email[128], telf[20], pass[256], fecha_nac[11];
    int rol_opcion;
    RolUsuario rol;

    printf("\n=== ALTA DE NUEVO USUARIO ===\n");

    printf("Nombre: ");
    scanf("%63s", nombre);
    printf("Apellido: ");
    scanf("%63s", apellido);
    printf("DNI: ");
    scanf("%15s", dni);
    printf("Email: ");
    scanf("%127s", email);
    printf("Telefono: ");
    scanf("%19s", telf);
    printf("Fecha Nacimiento (AAAA-MM-DD): ");
    scanf("%10s", fecha_nac);
    printf("Contrasena: ");
    scanf("%255s", pass);
    printf("Rol (0: Pasajero, 1: Maquinista, 2: Admin): ");
    scanf("%d", &rol_opcion);

    if (rol_opcion == 2) rol = ROL_ADMIN;
    else if (rol_opcion == 1) rol = ROL_EMPLEADO;
    else rol = ROL_PASAJERO;

    Usuario nuevo = crearUsuario(nombre, apellido, dni, email, telf, pass, fecha_nac, rol);

    if (insertar_usuario_db(nuevo) == 0) {
        log_evento(cfg.log_path, email, "INSERT", "Usuario creado correctamente");
        printf("Alta completada con exito.\n");
    } else {
        printf("Error al dar de alta el usuario.\n");
    }
}
void limpiar_pantalla(){
	#ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

