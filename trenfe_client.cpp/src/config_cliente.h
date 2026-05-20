/*
 * config_cliente.h
 *
 *  Created on: 8 may 2026
 *      Author:
 */


#ifndef CONFIG_CLIENTE_H_
#define CONFIG_CLIENTE_H_

//Estructura con todos los parámetros del cliente
struct ConfigCliente {
    char ip[128];        // ip del servidor
    int  puerto;         // Puerto TCP del servidor
    char log_path[256];  // Ruta del log del cliente
};


ConfigCliente cargarConfigCliente(const char *ruta);

#endif /* CONFIG_CLIENTE_H_ */
