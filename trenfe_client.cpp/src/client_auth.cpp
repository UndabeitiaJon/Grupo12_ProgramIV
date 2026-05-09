/*
 * client_auth.cpp
 *
 *  Created on: 8 may 2026
 *      Author: Grupo12
 */

/*
 * client_auth.cpp  -  Sistema TRENFE  -  Fase 2
 *
 * Implementa la función login() que:
 *   1. Pide email y contraseña al usuario
 *   2. Calcula el hash SHA-256 de la contraseña
 *   3. Envía LOGIN|email|hash al servidor
 *   4. Recibe AUTH_OK|id_u|rol|nombre  o  AUTH_FAIL|motivo
 *   5. Crea y devuelve el objeto de usuario correcto (Pasajero, etc.)
 *
 * Por ahora solo existe la clase Pasajero.
 * Maquinista y Administrador se añadirán en fases posteriores.
 */

#include <iostream>
#include <string>
#include <limits>
#include "clase_pasajero.h"
#include "conexion.h"

/* hash.h está en C, necesitamos extern "C" para enlazarlo desde C++ */
extern "C" {
#include "hash.h"
}

/* ── Función auxiliar: divide "campo1|campo2|..." y devuelve el campo Nº pos ── */
static std::string obtenerCampo(const std::string& linea, int pos) {
    int i = 0;
    size_t inicio = 0;

    while (inicio < linea.size()) {
        size_t fin = linea.find('|', inicio);
        if (fin == std::string::npos) fin = linea.size();

        if (i == pos) {
            return linea.substr(inicio, fin - inicio);
        }
        i++;
        inicio = fin + 1;
    }
    return "";
}

/* ── Función auxiliar: pide una cadena al usuario sin mostrarla ── */
static std::string pedirCadena(const std::string& prompt) {
    std::string valor;
    std::cout << prompt;
    std::getline(std::cin, valor);
    return valor;
}

/*
 * login()
 *
 * Pide credenciales, las envía al servidor y devuelve un puntero
 * al objeto de usuario correcto según el rol recibido.
 *
 * Devuelve nullptr si el login falla.
 *
 * El llamador es responsable de hacer delete al puntero devuelto.
 */
UsuarioBase* login(Conexion& conn) {

    /* 1. Pedir email y contraseña */
    std::string email = pedirCadena("  Email     : ");
    std::string pass  = pedirCadena("  Contraseña: ");

    if (email.empty() || pass.empty()) {
        std::cout << "  [ERROR] Email y contraseña no pueden estar vacíos.\n";
        return nullptr;
    }

    /* 2. Calcular SHA-256 de la contraseña */
    char hash[65];
    sha256_hex(pass.c_str(), hash);

    /* 3. Enviar comando de login al servidor */
    std::string cmd = "LOGIN|" + email + "|" + std::string(hash);
    if (!conn.enviar(cmd)) {
        std::cout << "  [ERROR] No se pudo enviar el login al servidor.\n";
        return nullptr;
    }

    /* 4. Recibir respuesta del servidor */
    std::string resp = conn.recibir();

    if (resp.empty()) {
        std::cout << "  [ERROR] Sin respuesta del servidor.\n";
        return nullptr;
    }

    /* 5. Parsear la respuesta */
    std::string tipo = obtenerCampo(resp, 0);

    /* ── LOGIN FALLIDO ── */
    if (tipo == "AUTH_FAIL") {
        std::string motivo = obtenerCampo(resp, 1);
        std::cout << "  [LOGIN FALLIDO] " << motivo << "\n";
        return nullptr;
    }

    /* ── LOGIN CORRECTO ── */
    /* Formato: AUTH_OK|id_u|rol|nombre */
    if (tipo != "AUTH_OK") {
        std::cout << "  [ERROR] Respuesta inesperada del servidor: " << resp << "\n";
        return nullptr;
    }

    int         id_u   = std::stoi(obtenerCampo(resp, 1));
    std::string rol    = obtenerCampo(resp, 2);
    std::string nombre = obtenerCampo(resp, 3);

    std::cout << "\n  Bienvenido/a, " << nombre << "! [" << rol << "]\n";

    /* 6. Crear el objeto correcto según el rol */

    if (rol == "PASAJERO") {
        /* Por ahora el apellido lo dejamos vacío; se obtiene con MIS_DATOS */
        return new Pasajero(id_u, nombre, "", email, conn);
    }

    if (rol == "MAQUINISTA") {
        /*
         * TODO (Prioridad 4): crear clase Maquinista y descomentarlo
         * return new Maquinista(id_u, nombre, "", email, conn);
         */
        std::cout << "  [INFO] Rol MAQUINISTA: menú en construcción.\n";
        std::cout << "  Por ahora mostramos el menú de pasajero como placeholder.\n";
        return new Pasajero(id_u, nombre, "", email, conn);
    }

    if (rol == "ADMIN") {
        /*
         * TODO (Prioridad 5): crear clase Administrador y descomentarlo
         * return new Administrador(id_u, nombre, "", email, conn);
         */
        std::cout << "  [INFO] Rol ADMIN: menú en construcción.\n";
        std::cout << "  Por ahora mostramos el menú de pasajero como placeholder.\n";
        return new Pasajero(id_u, nombre, "", email, conn);
    }

    std::cout << "  [ERROR] Rol desconocido: " << rol << "\n";
    return nullptr;
}
