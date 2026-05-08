/*
 * server_handler.h
 *
 *  Created on: 8 may 2026
 *      Author: ander.lecue
 */

#ifndef TRENFE_SERVER_SRC_SERVER_HANDLER_H_
#define TRENFE_SERVER_SRC_SERVER_HANDLER_H_

#include "server_socket.h"

/*
 * manejar_cliente()
 *
 * Atiende a un cliente ya conectado hasta que se desconecta o manda LOGOUT.
 * Bloquea el hilo llamante mientras dure la sesión.
 *
 * fd         : descriptor de socket del cliente
 * ip_cliente : cadena con la IP del cliente (solo para logs)
 */
void manejar_cliente(sock_t fd, const char *ip_cliente);

#endif /* TRENFE_SERVER_SRC_SERVER_HANDLER_H_ */

