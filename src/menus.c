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

void menu_principal_pasajero(const Usuario user);
void menu_principal_empleado(const Usuario user);
void menu_principal_administrador(const char *email_logueado, const Usuario user);
void menu_alta_usuario();


//COMPROBAR EL ROL DEL USUARIO PARA ABRIR UN MENU PRINCIPAL U OTRO
void comprobar_rol_usuario(char *email,Usuario user){
	RolUsuario rol = obtener_rol_usuario(email);
	switch (rol) {
		case ROL_PASAJERO:
			printf("ENTRANDO EN MENU PASAJERO");
			menu_principal_pasajero(user);
			break;
		case ROL_EMPLEADO:
			menu_principal_empleado(user);
			break;
		case ROL_ADMIN:
			menu_principal_administrador(email, user);
			break;
		default:
		    printf("Rol desconocido.\n");
		    break;
	}
}
//MENU PRINCIPAL DE ADMIN
void menu_principal_administrador(const char *email_logueado, const Usuario user) {
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
//MENU REGISTRO
void menu_alta_usuario(Usuario user) {
    char nombre[64], apellido[64], dni[16], email[128], telf[20], pass[256], fecha_nac[11];
    int rol_opcion;
    RolUsuario rol = ROL_PASAJERO;

    printf("\n=== ALTA DE NUEVO USUARIO ===\n");

    printf("Nombre: ");
    scanf("%63s", nombre);
    printf("Apellido: ");
    scanf("%63s", apellido);
    printf("DNI: ");
    scanf("%15s", dni);
    //COMPROBAR QUE EL EMAIL NO ESTE REGISTRADO YA EN LA BD

    do {
    	printf("Email: ");
    	scanf("%127s", email);
	} while (comprobar_usuario_registrado(email));

    printf("Telefono: ");
    scanf("%19s", telf);
    printf("Fecha Nacimiento (AAAA-MM-DD): ");
    scanf("%10s", fecha_nac);
    printf("Contrasena: ");
    scanf("%255s", pass);

    if (user.rol == ROL_ADMIN){
    	printf("Rol (0: Pasajero, 1: Maquinista, 2: Admin): ");
    	scanf("%d", &rol_opcion);

    	if (rol_opcion == 2) rol = ROL_ADMIN;
    	else if (rol_opcion == 1) rol = ROL_EMPLEADO;
   	    else rol = ROL_PASAJERO;
    }



    Usuario nuevo = crearUsuario(nombre, apellido, dni, email, telf, pass, fecha_nac, rol);

    if (insertar_usuario_db(nuevo) == 0) {
        log_evento(cfg.log_path, email, "INSERT", "Usuario creado correctamente");
        printf("Alta completada con exito.\n");
    } else {
        printf("Error al dar de alta el usuario.\n");
    }
}
//Funcion para limpiar la terminal -- NO FUNCIONA ; )
void limpiar_pantalla(){
	#ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}
//MENU DE PASAJEROS
void menu_principal_pasajero(const Usuario user) {
    int opcion;
    limpiar_pantalla();
    do {
        printf("\n=== BIENVENIDO/A %s ===\n", user.nombre);
        printf("1. Gestionar mis reservas\n");
        printf("2. Gestionar nuevas reservas\n");
        printf("3. Mi Trenfe\n");
        printf("0. Cerrar sesion\n");
        printf("Opcion: ");
        scanf("%d", &opcion);

        switch(opcion) {
            case 1: printf("Mis reservas (proximamente)\n"); break;
            case 2: printf("Nueva reserva (proximamente)\n"); break;
            case 3: printf("Mi Trenfe (proximamente)\n"); break;
            case 0:
                log_evento(cfg.log_path, user.email, "LOGOUT", "Sesion cerrada");
                printf("Hasta luego %s.\n", user.nombre);
                break;
            default: printf("Opcion no valida.\n");
        }
    } while(opcion != 0);
}

void menu_principal_empleado(const Usuario user) {
    int opcion;
    limpiar_pantalla();
    do {
        printf("\n=== BIENVENIDO/A %s ===\n", user.nombre);
        printf("1. Mis datos\n");
        printf("2. Servicios asignados\n");
        printf("0. Cerrar sesion\n");
        printf("Opcion: ");
        scanf("%d", &opcion);

        switch(opcion) {
            case 1: printf("Mis datos (proximamente)\n"); break;
            case 2: printf("Servicios (proximamente)\n"); break;
            case 0:
                log_evento(cfg.log_path, user.email, "LOGOUT", "Sesion cerrada");
                printf("Hasta luego %s.\n", user.nombre);
                break;
            default: printf("Opcion no valida.\n");
        }
    } while(opcion != 0);
}

void menu_login() {
    char email[128], pass[256];
    int intentos = 3;
    limpiar_pantalla();
    printf("\n=== INICIO DE SESION ===\n");
    while (intentos-- > 0) {
        printf("Email: ");
        scanf("%127s", email);
        printf("Contrasena: ");
        scanf("%255s", pass);

        if (verificar_usuario(email, pass)) {
            log_evento(cfg.log_path, email, "LOGIN", "Inicio de sesion correcto");
            //EXTRAEMOS EL USUARIO DE LA BD
            Usuario user = obtener_usuario(email);
            comprobar_rol_usuario(email, user);
            return;
        }
        printf("Credenciales incorrectas. Intentos restantes: %d\n", intentos);
    }
    log_evento(cfg.log_path, email, "LOGIN_FAIL", "Demasiados intentos fallidos");
    printf("Demasiados intentos fallidos.\n");
}
