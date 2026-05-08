/*
 * conexion.h
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */

/*
 * conexion.h  -  Sistema TRENFE  -  Fase 2
 *
 * Clase que encapsula el socket TCP del cliente.
 * Compatible con Windows (Winsock2) y Linux/macOS (POSIX sockets).
 *
 * Uso básico:
 *   Conexion conn("127.0.0.1", 8080);
 *   if (!conn.conectar()) { ... error ... }
 *   conn.enviar("LOGIN|user@trenfe.com|abc123");
 *   std::string resp = conn.recibir();   // "AUTH_OK|5|PASAJERO|Juan"
 *   conn.desconectar();
 */

#ifndef CONEXION_H_
#define CONEXION_H_

#include <string>
#include <vector>

/* ── Portabilidad Windows / POSIX ── */
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET sock_cli_t;
    #define SOCK_CLI_INVALIDO  INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int sock_cli_t;
    #define SOCK_CLI_INVALIDO  (-1)
#endif

#include "protocolo.h"   /* FIN_LISTA, PROTO_BUF_MAX */

class Conexion {
private:
    sock_cli_t  fd;
    std::string ip;
    int         puerto;
    bool        conectado;

    /* Inicializa Winsock (solo Windows; no-op en Linux) */
    static bool winsockIniciado;
    static bool inicializarWinsock();
    static void limpiarWinsock();

public:
    Conexion(const std::string& ip, int puerto);
    ~Conexion();

    /* Abre la conexión TCP al servidor.
     * Devuelve true si OK, false si falla. */
    bool conectar();

    /* Cierra la conexión. */
    void desconectar();

    /* Envía una línea de texto al servidor (añade '\n' automáticamente).
     * Devuelve true si OK. */
    bool enviar(const std::string& msg);

    /* Lee una línea de respuesta del servidor (hasta '\n').
     * Devuelve la cadena sin el '\n', o "" si error/desconexión. */
    std::string recibir();

    /* Lee respuestas hasta recibir una línea que empiece por FIN_LISTA.
     * Devuelve un vector con todas las líneas de datos (sin FIN_LISTA). */
    std::vector<std::string> recibirLista();

    /* Consulta de estado */
    bool estaConectado() const { return conectado; }
    const std::string& getIp()  const { return ip;     }
    int                getPuerto() const { return puerto; }
};

#endif /* CONEXION_H_ */
