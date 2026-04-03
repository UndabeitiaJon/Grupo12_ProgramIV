		#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

// Aquí se define la variable global
ConfigApp cfg;

static void set_defaults(ConfigApp *c) {
    strcpy(c->db_path,"./data/trenfe.db");
    strcpy(c->log_path, "./logs/trenfe.log");
    strcpy(c->admin_email,"admin@trenfe.com");
    c->puerto_servidor  = 8080;
    strcpy(c->host_servidor,"127.0.0.1");
}

int cargar_config(const char *ruta, ConfigApp *cfg) {
    // valores por defecto por si falla el fichero
	set_defaults(cfg);

    FILE *f = fopen(ruta, "r");
    if (!f) {
        printf("Config no encontrada, usando valores por defecto.\n");
        return 1;
    }

    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {
        linea[strcspn(linea, "\n")] = 0;
        if (linea[0] == '#' || linea[0] == ';' || linea[0] == '\0'){
        	continue;
        }
        char *clave = strtok(linea, "=");
        char *valor = strtok(NULL, "=");
        if (!clave || !valor) continue;
        while (*clave == ' ') clave++;
        while (*valor == ' ') valor++;

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
    printf("[CONFIG] Configuracion cargada desde %s\n", ruta);
    return 0;
}
void guardar_config(const char *ruta, ConfigApp *c) {
    FILE *f = fopen(ruta, "w");
    if (!f){
    	printf("No se pudo guardar config en %s\n", ruta);
    	return;
    }
    fprintf(f,"[database]\n");
    fprintf(f,"ruta_bd = %s\n\n", c->db_path);
    fprintf(f,"[server]\n");
    fprintf(f,"ip = %s\n", c->host_servidor);
    fprintf(f,"puerto = %d\n\n", c->puerto_servidor);
    fprintf(f,"[admin]\n");
    fprintf(f,"usuario = %s\n\n", c->admin_email);
    fprintf(f,"[logs]\n");
    fprintf(f,"ruta_log_admin = %s\n\n", c->log_path);
    fclose(f);
    printf("Configuracion guardada en %s\n", ruta);
}

void mostrar_config(const ConfigApp *c){
	printf("\n=========================================\n");
	printf("  CONFIGURACION ACTUAL DEL SISTEMA\n");
	printf("=========================================\n");
	printf("Base de datos: %s\n", c->db_path);
	printf("Log fichero: %s\n", c->log_path);
	printf("Admin email: %s\n", c->admin_email);
	printf("Host servidor: %s\n", c->host_servidor);
	printf("Puerto: %d\n", c->puerto_servidor);
	printf("-----------------------------------------\n");
}
