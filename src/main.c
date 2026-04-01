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
#include <direct.h>
#include "estructuras.h"
#include "db_manager.h"
#include "config.h"
#include "logs.h"
#include "menus.h"

int main(void) {
    printf("=== BIENVENIDO A TRENFE ===\n");

    _mkdir("./data");
    _mkdir("./logs");

    cargar_config("./data/config.cfg", &cfg);

    if (init_database() != 0) {
        printf("Error: no se pudo inicializar la base de datos.\n");
        return EXIT_FAILURE;
    }

    log_evento(cfg.log_path, NULL, "INICIO", "Sistema arrancado");

    int opcion;
    do {
        printf("\n1. Iniciar sesion\n");
        printf("0. Salir\n");
        printf("Opcion: ");
        scanf("%d", &opcion);

        switch(opcion) {
            case 1:
                menu_login();
                break;
            case 0:
                log_evento(cfg.log_path, NULL, "FIN", "Sistema detenido");
                printf("Hasta luego.\n");
                break;
            default:
                printf("Opcion no valida.\n");
        }
    } while(opcion != 0);

    return EXIT_SUCCESS;
}
