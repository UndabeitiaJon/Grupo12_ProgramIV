#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char db_path[256];
    char log_path[256];
    char admin_email[128];
    int  puerto_servidor;
    char host_servidor[128];
    //Se puede añadir en un futuro las tarifas de los descuentos y suplementos
} ConfigApp;

// Variable global accesible desde cualquier .c que incluya este .h
extern ConfigApp cfg;

int cargar_config(const char *ruta, ConfigApp *cfg);
void guardar_config(const char *ruta, ConfigApp *cfg); // viene bien tener guardar, para guardar los cambios
//que el adminitrador tenga que hacer.
void mostrar_config(const ConfigApp *cfg); // viene bien mostrar para enseñarle al adminitrador lo que ya hay
//para asi si tiene que cambiar algo pues saber que si y que no cambiar. Tambien viene bien para debugear
#endif
