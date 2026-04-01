#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char db_path[256];
    char log_path[256];
    char admin_email[128];
    int  puerto_servidor;
    char host_servidor[128];
} ConfigApp;

// Variable global accesible desde cualquier .c que incluya este .h
extern ConfigApp cfg;

int cargar_config(const char *ruta, ConfigApp *cfg);

#endif
