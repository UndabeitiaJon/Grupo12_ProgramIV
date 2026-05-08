/*
 * main.c  –  Sistema TRENFE  –  Fase 1
 *
 * Punto de entrada.  Secuencia de arranque:
 *   1. Crea directorios data/ y logs/ si no existen
 *   2. Carga config.cfg (o usa valores por defecto)
 *   3. Inicializa BD SQLite con las 14 tablas
 *   4. Carga datos de prueba (INSERT OR IGNORE → idempotente)
 *   5. Registra arranque en log
 *   6. Lanza menú inicial (Login / Registro / Salir)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── compatibilidad Windows / Linux para mkdir ── */
#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(p) mkdir((p), 0755)
#endif

#include "estructuras.h"
#include "db_manager.h"
#include "config.h"
#include "logs.h"
#include "menus.h"


int main(void) {

    setvbuf(stdout, NULL, _IONBF, 0);

    MKDIR("./data");
    MKDIR("./logs");


    cargar_config("./data/config.cfg", &cfg);


    if (init_database() != 0) {
        fprintf(stderr, "ERROR CRITICO: no se pudo inicializar la BD.\n");
        return EXIT_FAILURE;
    }

    seed_database();


    log_evento(cfg.log_path, "SISTEMA", "ARRANQUE",
               "Administrador local TRENFE iniciado");


    menu_inicial();


    log_evento(cfg.log_path, "SISTEMA", "CIERRE",
               "Administrador local TRENFE detenido");

    return EXIT_SUCCESS;
}
