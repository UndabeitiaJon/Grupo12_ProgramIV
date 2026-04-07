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
    c->coef_business    = 1.80;
    c->exceso_kg_precio = 12.0;
    c->suplemento_bici  = 30.0;
    c->menu_turista     = 8.0;
}

int cargar_config(const char *ruta, ConfigApp *c) {
    // valores por defecto por si falla el fichero
	set_defaults(c);


	FILE *f = fopen(ruta, "r");
	if (!f) {
	    printf("[CONFIG] No encontrado. Creando fichero con valores por defecto...\n");
	    guardar_config(ruta, c);
	    return 1;
	}
    char linea[512];
    char seccion[64] = "";
    while (fgets(linea, sizeof(linea), f)) {
            linea[strcspn(linea, "\r\n")] = 0;
            if (linea[0] == '#' || linea[0] == ';' || linea[0] == '\0') continue;
            if (linea[0] == '[') { sscanf(linea, "[%63[^]]", seccion); continue; }
            char *clave = strtok(linea, "=");
            char *valor = strtok(NULL,  "=");
            if (!clave || !valor) continue;
            while (*clave == ' ') clave++;
            while (*valor == ' ') valor++;
            char *fin = valor + strlen(valor) - 1;
            while (fin > valor && (*fin == ' ' || *fin == '\r')) *fin-- = 0;

            if      (!strcmp(clave,"ruta_bd"))           strcpy(c->db_path,       valor);
            else if (!strcmp(clave,"ruta_log_admin"))     strcpy(c->log_path,      valor);
            else if (!strcmp(clave,"ip"))                 strcpy(c->host_servidor, valor);
            else if (!strcmp(clave,"puerto"))             c->puerto_servidor  = atoi(valor);
            else if (!strcmp(clave,"usuario"))            strcpy(c->admin_email,   valor);
            else if (!strcmp(clave,"coef_business"))      c->coef_business    = atof(valor);
            else if (!strcmp(clave,"exceso_kg_precio"))   c->exceso_kg_precio = atof(valor);
            else if (!strcmp(clave,"suplemento_bici"))    c->suplemento_bici  = atof(valor);
            else if (!strcmp(clave,"menu_turista"))       c->menu_turista     = atof(valor);
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
    fprintf(f, "[tarifas]\ncoef_business = %.2f\nexceso_kg_precio = %.2f\n"
                   "suplemento_bici = %.2f\nmenu_turista = %.2f\n",
                c->coef_business, c->exceso_kg_precio, c->suplemento_bici, c->menu_turista);
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
	printf("  Coef. Business  : x%.2f\n", c->coef_business);
	printf("  Exceso kg       : %.2f EUR/kg\n", c->exceso_kg_precio);
	printf("  Supl. bici/esqui: %.2f EUR\n", c->suplemento_bici);
	printf("  Menu turista    : %.2f EUR\n", c->menu_turista);
	printf("=========================================\n");
}
