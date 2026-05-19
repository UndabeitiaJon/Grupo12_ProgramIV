/*
 * server_socket.h
 *
 *  Created on: 7 may 2026
 *      Author: e.aranoa
 */

/*
 * server_socket.h  –  Sistema TRENFE  –  Fase 2
 *
 * Abstracción de red para el servidor.
 * Compatible con Windows (Winsock2) y Linux/macOS (POSIX sockets).
 */

#ifndef SERVER_SOCKET_H_
#define SERVER_SOCKET_H_

//Portabilidad Windows / POSIX
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET  sock_t;
    #define SOCK_INVALIDO  INVALID_SOCKET
    #define SOCK_ERROR     SOCKET_ERROR
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int     sock_t;
    #define SOCK_INVALIDO  (-1)
    #define SOCK_ERROR     (-1)
#endif

// Tamaño máximo de un mensaje de protocolo (en bytes)
#define SOCK_BUF_MAX  8192

/* ── Inicialización / limpieza (solo relevante en Windows) ── */

/*
 * Inicializa la capa de red.
 * Llama a WSAStartup en Windows; no-op en Linux.
 * Devuelve 0 si OK, -1 si error.
 */
int  socket_inicializar(void);

/*
 * Limpia la capa de red.
 * Llama a WSACleanup en Windows; no-op en Linux.
 */
void socket_limpiar(void);

/* ── Servidor ── */

/*
 * Crea un socket TCP de escucha en el puerto indicado.
 * Devuelve el descriptor de socket, o SOCK_INVALIDO si falla.
 */
sock_t crear_socket_servidor(int puerto);

/*
 * Bloquea hasta que un cliente se conecta.
 * Rellena ip_cliente (buffer de al menos 46 bytes) con la IP del cliente.
 * Devuelve el descriptor del cliente, o SOCK_INVALIDO si falla.
 */
sock_t aceptar_cliente(sock_t servidor, char *ip_cliente, int ip_buf_len);

/* ── Utilidades comunes (cliente y servidor) ── */

/*
 * Cierra un socket de forma portable.
 */
void cerrar_socket(sock_t fd);

/*
 * Envía un mensaje al socket fd.
 * Añade automáticamente '\n' como terminador si no lo tiene.
 * Devuelve el nº de bytes enviados, o -1 si error.
 */
int  enviar_mensaje(sock_t fd, const char *msg);

/*
 * Recibe una línea completa (hasta '\n') desde el socket fd.
 * El '\n' se elimina del resultado. El buffer queda terminado en '\0'.
 * Devuelve el nº de bytes leídos (>0), 0 si el cliente cerró la conexión,
 * o -1 si hubo un error.
 */
int  recibir_mensaje(sock_t fd, char *buf, int max);

/*
 * Versión "segura" de enviar: formatea el mensaje y lo envía.
 * Ejemplo: enviar_fmt(fd, "OK|%d|%.2f", id_res, precio);
 * Devuelve el nº de bytes enviados, o -1 si error.
 */
int  enviar_fmt(sock_t fd, const char *fmt, ...);

#endif /* SERVER_SOCKET_H_ */
