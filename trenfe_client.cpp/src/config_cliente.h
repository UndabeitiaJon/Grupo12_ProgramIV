/*
 * config_cliente.h
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */
/*
 * config_cliente.h  -  Sistema TRENFE  -  Fase 2
 *
 * Configuración del cliente remoto.
 * Lee el fichero INI en data/client.cfg.
 */

#ifndef CONFIG_CLIENTE_H_
#define CONFIG_CLIENTE_H_

/* ── Estructura con todos los parámetros del cliente ── */
struct ConfigCliente {
    char ip[128];        /* IP o hostname del servidor    */
    int  puerto;         /* Puerto TCP del servidor        */
    char log_path[256];  /* Ruta del log del cliente       */
};

/*
 * Lee el fichero de configuración indicado por 'ruta' y devuelve
 * un ConfigCliente relleno. Si el fichero no existe o falta algún
 * campo se usan los valores por defecto (127.0.0.1 : 8080).
 */
ConfigCliente cargarConfigCliente(const char *ruta);

#endif /* CONFIG_CLIENTE_H_ */
