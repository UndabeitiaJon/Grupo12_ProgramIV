/*
 * conexion.cpp
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */
/*
 *
 *
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "conexion.h"

/* ── Inicialización estática ── */
bool Conexion::winsockIniciado = false;

bool Conexion::inicializarWinsock() {
#ifdef _WIN32
    if (winsockIniciado) return true;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[CONEXION] WSAStartup fallo: %d\n", WSAGetLastError());
        return false;
    }
    winsockIniciado = true;
#endif
    return true;
}

void Conexion::limpiarWinsock() {
#ifdef _WIN32
    if (winsockIniciado) {
        WSACleanup();
        winsockIniciado = false;
    }
#endif
}

/* ══════════════════════════════════════════════
   CONSTRUCTOR / DESTRUCTOR
   ══════════════════════════════════════════════ */

Conexion::Conexion(const std::string& ip, int puerto)
    : fd(SOCK_CLI_INVALIDO), ip(ip), puerto(puerto), conectado(false)
{
    inicializarWinsock();
}

Conexion::~Conexion() {
    desconectar();
}

/* ══════════════════════════════════════════════
   CONECTAR
   ══════════════════════════════════════════════ */

bool Conexion::conectar() {
    if (conectado) return true;

    /* 1. Crear socket TCP */
    fd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (fd == INVALID_SOCKET) {
        fprintf(stderr, "[CONEXION] socket() fallo: %d\n", WSAGetLastError());
        return false;
    }
#else
    if (fd < 0) {
        perror("[CONEXION] socket()");
        return false;
    }
#endif

    /* 2. Rellenar dirección del servidor */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((unsigned short)puerto);

    /* inet_addr soporta tanto IP numérica como hostname simple */
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "[CONEXION] IP invalida: %s\n", ip.c_str());
        desconectar();
        return false;
    }

    /* 3. Conectar */
#ifdef _WIN32
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[CONEXION] connect() fallo: %d\n", WSAGetLastError());
        desconectar();
        return false;
    }
#else
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[CONEXION] connect()");
        desconectar();
        return false;
    }
#endif

    conectado = true;
    printf("[CONEXION] Conectado al servidor %s:%d\n", ip.c_str(), puerto);
    return true;
}

/* ══════════════════════════════════════════════
   DESCONECTAR
   ══════════════════════════════════════════════ */

void Conexion::desconectar() {
    if (fd != SOCK_CLI_INVALIDO) {
#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
        fd = SOCK_CLI_INVALIDO;
    }
    conectado = false;
}

/* ══════════════════════════════════════════════
   ENVIAR
   ══════════════════════════════════════════════ */

bool Conexion::enviar(const std::string& msg) {
    if (!conectado) return false;

    /* Garantizar que el mensaje termina en '\n' */
    std::string linea = msg;
    if (linea.empty() || linea.back() != '\n') {
        linea += '\n';
    }

    const char *buf = linea.c_str();
    int total = (int)linea.size();
    int enviado = 0;

    while (enviado < total) {
#ifdef _WIN32
        int ret = send(fd, buf + enviado, total - enviado, 0);
        if (ret == SOCKET_ERROR) {
            fprintf(stderr, "[CONEXION] send() fallo: %d\n", WSAGetLastError());
            conectado = false;
            return false;
        }
#else
        int ret = (int)send(fd, buf + enviado, (size_t)(total - enviado), 0);
        if (ret < 0) {
            perror("[CONEXION] send()");
            conectado = false;
            return false;
        }
#endif
        enviado += ret;
    }
    return true;
}

/* ══════════════════════════════════════════════
   RECIBIR (una línea)
   ══════════════════════════════════════════════ */

std::string Conexion::recibir() {
    if (!conectado) return "";

    std::string resultado;
    char c;
    while (resultado.size() < PROTO_BUF_MAX - 1) {
#ifdef _WIN32
        int ret = recv(fd, &c, 1, 0);
        if (ret == 0)            { conectado = false; break; }
        if (ret == SOCKET_ERROR) { conectado = false; break; }
#else
        int ret = (int)recv(fd, &c, 1, 0);
        if (ret == 0) { conectado = false; break; }
        if (ret < 0)  { conectado = false; break; }
#endif
        if (c == '\n') break;
        if (c == '\r') continue;
        resultado += c;
    }
    return resultado;
}
/* ══════════════════════════════════════════════
   RECIBIR LISTA (hasta FIN_LISTA)
   ══════════════════════════════════════════════ */

std::vector<std::string> Conexion::recibirLista() {
    std::vector<std::string> lista;

    while (conectado) {
        std::string linea = recibir();
        if (linea.empty() && !conectado) break;


        if (linea.rfind("FIN_LISTA", 0) == 0) break;


        if (linea.rfind("ERROR", 0) == 0) {
            fprintf(stderr, "[CONEXION] Error del servidor: %s\n", linea.c_str());
            break;
        }

        lista.push_back(linea);
    }

    return lista;
}



