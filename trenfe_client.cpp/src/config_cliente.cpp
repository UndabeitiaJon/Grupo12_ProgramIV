/*
 * config_cliente.cpp
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */



/*
 * config_cliente.cpp  -  Sistema TRENFE  -  Fase 2
 *
 * Lee data/client.cfg con formato INI sencillo:
 *
 *   [server]
 *   ip     = 127.0.0.1
 *   puerto = 8080
 *
 *   [logs]
 *   ruta_log_cliente = ./logs/client.log
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "config_cliente.h"

/* Elimina espacios y el salto de línea al final de una cadena */
static void trim(char *s) {
    if (!s) return;
    /* Trailing */
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r' ||
                       s[len-1] == ' '  || s[len-1] == '\t')) {
        s[--len] = '\0';
    }
    /* Leading */
    char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

ConfigCliente cargarConfigCliente(const char *ruta) {
    /* Valores por defecto */
    ConfigCliente cfg;
    strncpy(cfg.ip,       "127.0.0.1",        sizeof(cfg.ip)       - 1);
    cfg.puerto = 8080;
    strncpy(cfg.log_path, "./logs/client.log", sizeof(cfg.log_path) - 1);

    FILE *f = fopen(ruta, "r");
    if (!f) {
        fprintf(stderr, "[CONFIG] No se encontro %s, usando valores por defecto.\n", ruta);
        return cfg;
    }

    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {
        trim(linea);

        /* Ignorar líneas vacías y comentarios */
        if (linea[0] == '\0' || linea[0] == '#' || linea[0] == '[') continue;

        /* Separar clave = valor */
        char *eq = strchr(linea, '=');
        if (!eq) continue;

        *eq = '\0';
        char *clave = linea;
        char *valor = eq + 1;
        trim(clave);
        trim(valor);

        if (strcmp(clave, "ip") == 0) {
            strncpy(cfg.ip, valor, sizeof(cfg.ip) - 1);
        } else if (strcmp(clave, "puerto") == 0) {
            cfg.puerto = atoi(valor);
        } else if (strcmp(clave, "ruta_log_cliente") == 0) {
            strncpy(cfg.log_path, valor, sizeof(cfg.log_path) - 1);
        }
    }

    fclose(f);

    printf("[CONFIG] Servidor: %s:%d  |  Log: %s\n",
           cfg.ip, cfg.puerto, cfg.log_path);
    return cfg;
}
