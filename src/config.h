#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char db_path[256];
    char log_path[256];
    char admin_email[128];
    int  puerto_servidor;
    char host_servidor[128];
    double coef_business;      /* default 1.80 */
    double exceso_kg_precio;   /* default 12.0 */
    double suplemento_bici;    /* default 30.0 */
    double menu_turista;
} ConfigApp;

// Variable global accesible desde cualquier .c que incluya este .h
extern ConfigApp cfg;

int cargar_config(const char *ruta, ConfigApp *cfg);
void guardar_config(const char *ruta, ConfigApp *cfg); // viene bien tener guardar, para guardar los cambios
//que el adminitrador tenga que hacer.
void mostrar_config(const ConfigApp *cfg); // viene bien mostrar para enseñarle al adminitrador lo que ya hay
//para asi si tiene que cambiar algo pues saber que si y que no cambiar. Tambien viene bien para debugear
#endif
