/*
 ============================================================================
 Name        :
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

#ifdef _WIN32
    #include <direct.h>
    #define crear_directorio(p) _mkdir(p)
#else
    #include <sys/stat.h>
    #define crear_directorio(p) mkdir(p, 0755)
#endif

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("=========================================\n");
    printf("     BIENVENIDO AL SISTEMA TRENFE\n");
    printf("=========================================\n");

    crear_directorio("./data");
    crear_directorio("./logs");

    cargar_config("./data/config.cfg", &cfg);

    if (init_database() != 0) {
        printf("Error: no se pudo inicializar la base de datos.\n");
        return EXIT_FAILURE;
    }

    seed_database();

    log_evento(cfg.log_path, NULL, "INICIO", "Sistema arrancado");

    int opcion;

    do {
        limpiar_pantalla();
        printf("\n-----------------------------------------\n");
        printf("     BIENVENIDO AL SISTEMA TRENFE\n");
        printf("-----------------------------------------\n");
        printf("  1. Iniciar sesion\n");
        printf("  2. Registrar\n");
        printf("  0. Salir\n");
        printf("-----------------------------------------\n");
        printf("Opcion: ");
        scanf("%d", &opcion);

        switch(opcion) {
            case 1: limpiar_pantalla(); menu_login(); break;
            case 2: limpiar_pantalla(); menu_alta_usuario(NULL); break;
            case 0:
                log_evento(cfg.log_path, NULL, "FIN", "Sistema detenido");
                printf("Hasta luego.\n");
                break;
            default: printf("Opcion no valida.\n"); break;
        }
    } while(opcion != 0);


    return EXIT_SUCCESS;
}
