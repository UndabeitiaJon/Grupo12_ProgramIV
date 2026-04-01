#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

// Aquí se define la variable global
ConfigApp cfg;

int cargar_config(const char *ruta, ConfigApp *cfg) {
    // valores por defecto por si falla el fichero
    strcpy(cfg->db_path, "./data/trenfe.db");
    strcpy(cfg->log_path, "./logs/trenfe.log");
    strcpy(cfg->admin_email, "admin@trenfe.com");
    cfg->puerto_servidor = 8080;
    strcpy(cfg->host_servidor, "127.0.0.1");

    FILE *f = fopen(ruta, "r");
    if (!f) {
        printf("Config no encontrada, usando valores por defecto.\n");
        return 1;
    }

    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {
        linea[strcspn(linea, "\n")] = 0;
        char *clave = strtok(linea, "=");
        char *valor = strtok(NULL, "=");
        if (!clave || !valor) continue;

        if (strcmp(clave, "db_path") == 0)
            strcpy(cfg->db_path, valor);
        else if (strcmp(clave, "log_path") == 0)
            strcpy(cfg->log_path, valor);
        else if (strcmp(clave, "admin_email") == 0)
            strcpy(cfg->admin_email, valor);
        else if (strcmp(clave, "puerto_servidor") == 0)
            cfg->puerto_servidor = atoi(valor);
        else if (strcmp(clave, "host_servidor") == 0)
            strcpy(cfg->host_servidor, valor);
    }
    fclose(f);
    return 0;
}
