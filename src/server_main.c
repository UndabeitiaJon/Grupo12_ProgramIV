/*
 * server_main.c
 *
 *  Created on: 7 may 2026
 *      Author: e.aranoa
 */


/*
 * server_main.c  -  Sistema TRENFE  -  Fase 2
 *
 * Punto de entrada del servidor remoto.
 * Secuencia de arranque:
 *   1. Crea directorios data/ y logs/ si no existen
 *   2. Inicializa la capa de red (Winsock en Windows)
 *   3. Carga config.cfg (reutiliza config.c de Fase 1)
 *   4. Inicializa la BD SQLite (reutiliza db_manager.c de Fase 1)
 *   5. Registra arranque en log
 *   6. Crea socket de escucha
 *   7. Bucle principal: acepta un cliente, lo atiende, repite
 *   8. Limpieza al salir
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* ── Compatibilidad Windows / Linux para mkdir ── */
#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(p) _mkdir(p)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(p) mkdir((p), 0755)
#endif

/* ── Modulos reutilizados de Fase 1 ── */
#include "estructuras.h"
#include "db_manager.h"
#include "config.h"
#include "logs.h"

/* ── Modulos nuevos de Fase 2 ── */
#include "server_socket.h"
#include "server_handler.h"
#include "protocolo.h"

/* ── Socket global para poder cerrarlo en la señal SIGINT ── */
static sock_t g_fd_servidor = SOCK_INVALIDO;

/* ─────────────────────────────────────────────
   Manejador de CTRL+C — cierre limpio
   ───────────────────────────────────────────── */
static void manejador_sigint(int sig) {
    (void)sig;
    printf("\n[SERVIDOR] Señal de cierre recibida. Apagando...\n");
    if (g_fd_servidor != SOCK_INVALIDO) {
        cerrar_socket(g_fd_servidor);
        g_fd_servidor = SOCK_INVALIDO;
    }
    socket_limpiar();
    log_evento(cfg.log_path, "SISTEMA", "CIERRE", "Servidor TRENFE detenido por señal");
    exit(EXIT_SUCCESS);
}

/* ─────────────────────────────────────────────
   MAIN
   ───────────────────────────────────────────── */
int main(void) {

    setvbuf(stdout, NULL, _IONBF, 0);

    /* 1. Crear directorios necesarios */
    MKDIR("./data");
    MKDIR("./logs");

    /* 2. Inicializar capa de red */
    if (socket_inicializar() != 0) {
        fprintf(stderr, "[SERVIDOR] ERROR: no se pudo inicializar la red.\n");
        return EXIT_FAILURE;
    }

    /* 3. Cargar configuracion (reutiliza config.c de Fase 1) */
    cargar_config("./data/config.cfg", &cfg);

    /* 4. Inicializar base de datos (reutiliza db_manager.c de Fase 1) */
    if (init_database() != 0) {
        fprintf(stderr, "[SERVIDOR] ERROR CRITICO: no se pudo inicializar la BD.\n");
        socket_limpiar();
        return EXIT_FAILURE;
    }

    /* 5. Log de arranque */
    log_evento(cfg.log_path, "SISTEMA", "ARRANQUE",
               "Servidor remoto TRENFE iniciado");

    printf("\n");
    printf("========================================\n");
    printf("   SERVIDOR TRENFE  -  Fase 2          \n");
    printf("========================================\n");
    printf("  BD     : %s\n", cfg.db_path);
    printf("  Log    : %s\n", cfg.log_path);
    printf("  Puerto : %d\n", cfg.puerto_servidor);
    printf("========================================\n\n");

    /* 6. Registrar manejador CTRL+C */
    signal(SIGINT, manejador_sigint);

    /* 7. Crear socket de escucha */
    g_fd_servidor = crear_socket_servidor(cfg.puerto_servidor);
    if (g_fd_servidor == SOCK_INVALIDO) {
        fprintf(stderr, "[SERVIDOR] ERROR: no se pudo crear el socket.\n");
        log_evento(cfg.log_path, "SISTEMA", "ERROR",
                   "Fallo al crear socket de escucha");
        socket_limpiar();
        return EXIT_FAILURE;
    }

    printf("[SERVIDOR] Esperando conexiones en el puerto %d...\n\n",
           cfg.puerto_servidor);

    /* 8. Bucle principal — un cliente cada vez (sin hilos, según enunciado) */
    while (1) {
        char ip_cliente[46] = "";

        sock_t fd_cliente = aceptar_cliente(g_fd_servidor,
                                            ip_cliente, sizeof(ip_cliente));
        if (fd_cliente == SOCK_INVALIDO) {
            /* accept() puede fallar si llega SIGINT justo al cerrar */
            fprintf(stderr, "[SERVIDOR] accept() fallo. Reintentando...\n");
            continue;
        }

        printf("[SERVIDOR] Cliente conectado desde %s\n", ip_cliente);

        /* Mensaje de log */
        char msg_log[128];
        snprintf(msg_log, sizeof(msg_log),
                 "Cliente conectado desde %s", ip_cliente);
        log_evento(cfg.log_path, "SISTEMA", "CONEXION", msg_log);

        /* Atender al cliente — bloquea hasta que se desconecte */
        manejar_cliente(fd_cliente, ip_cliente);

        /* El cliente termino */
        cerrar_socket(fd_cliente);

        printf("[SERVIDOR] Cliente %s desconectado.\n\n", ip_cliente);

        snprintf(msg_log, sizeof(msg_log),
                 "Cliente desconectado desde %s", ip_cliente);
        log_evento(cfg.log_path, "SISTEMA", "DESCONEXION", msg_log);
    }

    /* 9. Limpieza (normalmente se llega por señal, no por aqui) */
    cerrar_socket(g_fd_servidor);
    socket_limpiar();
    log_evento(cfg.log_path, "SISTEMA", "CIERRE",
               "Servidor TRENFE detenido");

    return EXIT_SUCCESS;
}
