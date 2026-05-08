/*
 * server_socket.c
 *
 *  Created on: 7 may 2026
 *      Author: e.aranoa
 */


/*
 * server_socket.c  –  Sistema TRENFE  –  Fase 2
 *
 * Implementación portable de las utilidades de red.
 * Compatible con Windows (Winsock2) y Linux/macOS (POSIX sockets).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "server_socket.h"

#ifndef _WIN32
#include <errno.h>
#endif

/* ── Helper: mensaje de error portable ──
 * En Windows los sockets usan WSAGetLastError(), no errno.
 * En Linux/macOS se usa strerror(errno) directamente.         */
static void sock_perror(const char *ctx) {
#ifdef _WIN32
    fprintf(stderr, "[SOCKET] %s: WSA error %d\n", ctx, WSAGetLastError());
#else
    fprintf(stderr, "[SOCKET] %s: %s\n", ctx, strerror(errno));
#endif
}

/* ─────────────────────────────────────────────
   BLOQUE 1 – Inicialización / limpieza
   ───────────────────────────────────────────── */

int socket_inicializar(void) {
#ifdef _WIN32
    WSADATA wsa;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (ret != 0) {
        fprintf(stderr, "[SOCKET] WSAStartup fallo con codigo %d\n", ret);
        return -1;
    }
    printf("[SOCKET] Winsock2 inicializado correctamente.\n");
#endif
    return 0;
}

void socket_limpiar(void) {
#ifdef _WIN32
    WSACleanup();
    printf("[SOCKET] Winsock2 limpiado.\n");
#endif
}

/* ─────────────────────────────────────────────
   BLOQUE 2 – Creación del socket de escucha
   ───────────────────────────────────────────── */

sock_t crear_socket_servidor(int puerto) {
    sock_t fd;
    struct sockaddr_in addr;
    int opt = 1;

    /* 1. Crear socket TCP */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == SOCK_INVALIDO) {
        sock_perror("socket()");
        return SOCK_INVALIDO;
    }

    /* 2. SO_REUSEADDR → permite relanzar el servidor sin esperar TIME_WAIT */
#ifdef _WIN32
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   (const char *)&opt, sizeof(opt)) == SOCK_ERROR) {
#else
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
#endif
        sock_perror("setsockopt()");
        cerrar_socket(fd);
        return SOCK_INVALIDO;
    }

    /* 3. Bind al puerto en todas las interfaces */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)puerto);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == SOCK_ERROR) {
        fprintf(stderr, "[SOCKET] bind() fallo en puerto %d\n", puerto);
        sock_perror("bind()");
        cerrar_socket(fd);
        return SOCK_INVALIDO;
    }

    /* 4. Listen – cola de hasta 10 conexiones pendientes */
    if (listen(fd, 10) == SOCK_ERROR) {
        sock_perror("listen()");
        cerrar_socket(fd);
        return SOCK_INVALIDO;
    }

    printf("[SOCKET] Servidor escuchando en puerto %d.\n", puerto);
    return fd;
}

/* ─────────────────────────────────────────────
   BLOQUE 3 – Aceptar cliente
   ───────────────────────────────────────────── */

sock_t aceptar_cliente(sock_t servidor, char *ip_cliente, int ip_buf_len) {
    struct sockaddr_in addr_cliente;
#ifdef _WIN32
    int addr_len = sizeof(addr_cliente);
#else
    socklen_t addr_len = sizeof(addr_cliente);
#endif

    sock_t fd_cliente = accept(servidor,
                               (struct sockaddr *)&addr_cliente,
                               &addr_len);

    if (fd_cliente == SOCK_INVALIDO) {
        sock_perror("accept()");
        return SOCK_INVALIDO;
    }

    /* Extraer IP del cliente en formato legible */
    if (ip_cliente && ip_buf_len > 0) {
        const char *ip = inet_ntoa(addr_cliente.sin_addr);
        strncpy(ip_cliente, ip ? ip : "desconocida", ip_buf_len - 1);
        ip_cliente[ip_buf_len - 1] = '\0';
    }

    return fd_cliente;
}

/* ─────────────────────────────────────────────
   BLOQUE 4 – Cerrar socket
   ───────────────────────────────────────────── */

void cerrar_socket(sock_t fd) {
    if (fd == SOCK_INVALIDO) return;
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

/* ─────────────────────────────────────────────
   BLOQUE 5 – Enviar mensaje
   ───────────────────────────────────────────── */

int enviar_mensaje(sock_t fd, const char *msg) {
    if (!msg) return -1;

    char buf[SOCK_BUF_MAX];
    int  len = (int)strlen(msg);

    /* Copiar mensaje al buffer */
    if (len >= SOCK_BUF_MAX - 1) {
        fprintf(stderr, "[SOCKET] Mensaje demasiado largo (%d bytes), truncado.\n", len);
        len = SOCK_BUF_MAX - 2;
    }
    memcpy(buf, msg, len);

    /* Garantizar que termina en '\n' */
    if (len == 0 || buf[len - 1] != '\n') {
        buf[len]     = '\n';
        buf[len + 1] = '\0';
        len++;
    } else {
        buf[len] = '\0';
    }

    /* Envío completo (loop por si send() devuelve menos bytes) */
    int enviado = 0;
    while (enviado < len) {
#ifdef _WIN32
        int ret = send(fd, buf + enviado, len - enviado, 0);
        if (ret == SOCKET_ERROR) {
#else
        int ret = (int)send(fd, buf + enviado, (size_t)(len - enviado), 0);
        if (ret < 0) {
#endif
            sock_perror("send()");
            return -1;
        }
        enviado += ret;
    }
    return enviado;
}

/* ─────────────────────────────────────────────
   BLOQUE 6 – Recibir mensaje (hasta '\n')
   ───────────────────────────────────────────── */

int recibir_mensaje(sock_t fd, char *buf, int max) {
    if (!buf || max <= 0) return -1;

    int total = 0;
    char c;

    while (total < max - 1) {
#ifdef _WIN32
        int ret = recv(fd, &c, 1, 0);
        if (ret == 0)            return 0;   /* cliente cerró conexión */
        if (ret == SOCKET_ERROR) {
#else
        int ret = (int)recv(fd, &c, 1, 0);
        if (ret == 0)  return 0;             /* cliente cerró conexión */
        if (ret < 0) {
#endif
            sock_perror("recv()");
            return -1;
        }

        if (c == '\n') break;               /* fin de mensaje */
        if (c == '\r') continue;            /* ignorar CR en Windows */

        buf[total++] = c;
    }

    buf[total] = '\0';
    return total;
}

/* ─────────────────────────────────────────────
   BLOQUE 7 – enviar_fmt (formato printf-style)
   ───────────────────────────────────────────── */

int enviar_fmt(sock_t fd, const char *fmt, ...) {
    char buf[SOCK_BUF_MAX];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    return enviar_mensaje(fd, buf);
}
