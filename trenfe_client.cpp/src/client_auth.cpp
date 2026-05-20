/*
 * client_auth.cpp
 *
 *  Created on: 8 may 2026
 *      Author: Grupo12
 */





#include <iostream>
#include <string>
#include <limits>
#include "clase_pasajero.h"
#include "clase_maquinista.h"
#include "clase_administrador.h"
#include "conexion.h"

extern "C" {
#include "hash.h"
#include "validacion.h"
}

/* ──────────────────────────────────────────
   Utilidades internas
   ────────────────────────────────────────── */

static std::string obtenerCampo(const std::string& linea, int pos) {
    int i = 0;
    size_t inicio = 0;
    while (inicio < linea.size()) {
        size_t fin = linea.find('|', inicio);
        if (fin == std::string::npos) fin = linea.size();
        if (i == pos) return linea.substr(inicio, fin - inicio);
        i++;
        inicio = fin + 1;
    }
    return "";
}

static std::string pedirCadena(const std::string& prompt) {
    std::string v;
    std::cout << prompt;
    std::getline(std::cin, v);
    return v;
}


static std::string pedirCampoValido(const std::string& prompt,
                                    int (*validador)(const char*)) {
    std::string valor;
    do {
        valor = pedirCadena(prompt);
    } while (!validador(valor.c_str()));
    return valor;
}

/* ──────────────────────────────────────────
   login()
   ────────────────────────────────────────── */


UsuarioBase* login(Conexion& conn) {
    std::string email = pedirCadena("  Email     : ");
    std::string pass  = pedirCadena("  Contraseña: ");

    if (email.empty() || pass.empty()) {
        std::cout << "  [ERROR] Email y contraseña no pueden estar vacíos.\n";
        return nullptr;
    }

    char hash[65];
    sha256_hex(pass.c_str(), hash);

    if (!conn.enviar("LOGIN|" + email + "|" + std::string(hash))) {
        std::cout << "  [ERROR] No se pudo contactar con el servidor.\n";
        return nullptr;
    }

    std::string resp = conn.recibir();
    if (resp.empty()) {
        std::cout << "  [ERROR] Sin respuesta del servidor.\n";
        return nullptr;
    }

    std::string tipo = obtenerCampo(resp, 0);

    if (tipo == "AUTH_FAIL") {
        std::cout << "  [LOGIN FALLIDO] " << obtenerCampo(resp, 1) << "\n";
        return nullptr;
    }

    if (tipo != "AUTH_OK") {
        std::cout << "  [ERROR] Respuesta inesperada: " << resp << "\n";
        return nullptr;
    }

    /* AUTH_OK|id_u|rol|nombre */
    int         id_u   = std::stoi(obtenerCampo(resp, 1));
    std::string rol    = obtenerCampo(resp, 2);
    std::string nombre = obtenerCampo(resp, 3);

    std::cout << "\n  Bienvenido/a, " << nombre << "! [" << rol << "]\n";

    if (rol == "PASAJERO")
        return new Pasajero(id_u, nombre, "", email, conn);

    if (rol == "MAQUINISTA")
        return new Maquinista(id_u, nombre, "", email, conn);

    if (rol == "ADMIN")
        return new Administrador(id_u, nombre, "", email, conn);

    std::cout << "  [ERROR] Rol desconocido: " << rol << "\n";
    return nullptr;
}

/* ──────────────────────────────────────────
   registrar()
   ────────────────────────────────────────── */


bool registrar(Conexion& conn) {
    std::cout << "\n  ─────────────────────────────────────\n";
    std::cout << "        REGISTRO DE NUEVO USUARIO\n";
    std::cout << "  ─────────────────────────────────────\n";

    // Nombre
    std::string nombre = pedirCampoValido(
        "  Nombre            : ", validar_nombre);

    // Apellido(s)
    std::string apellido = pedirCampoValido(
        "  Apellido(s)        : ", validar_nombre);

    //DNI
    std::string dni = pedirCampoValido(
        "  DNI/NIE (ej: 12345678A): ", validar_dni);

    //Email
    std::string email = pedirCampoValido(
        "  Email              : ", validar_email);

    //Teléfono
    std::string telf = pedirCampoValido(
        "  Teléfono (9 dígitos): ", validar_telefono);

    // Fecha de nacimiento
    std::string fecha_nac = pedirCampoValido(
        "  Fecha nac (AAAA-MM-DD): ", validar_fecha);

    //Contraseña con confirmación
    std::string pass1, pass2;
    int intentos = 0;

    do {
        if (intentos > 0)
            std::cout << "  Las contraseñas no coinciden. Inténtalo de nuevo.\n";

        // Pedir contraseña hasta que cumpla la longitud mínima
        do {
            pass1 = pedirCadena("  Contraseña (mín. 6 caracteres): ");
        } while (!validar_contrasenia(pass1.c_str()));

        pass2 = pedirCadena("  Confirmar contraseña           : ");
        intentos++;

    } while (pass1 != pass2 && intentos < 3);

    if (pass1 != pass2) {
        std::cout << "  [ERROR] Las contraseñas no coinciden tras 3 intentos.\n";
        return false;
    }

    //Hashear contraseña antes de enviar
    char hash[65];
    sha256_hex(pass1.c_str(), hash);

    //Enviar al servidor
    std::string cmd = "REGISTRO|" + nombre    + "|" + apellido  + "|" +
                                    dni        + "|" + email     + "|" +
                                    telf       + "|" + fecha_nac + "|" +
                                    std::string(hash);

    if (!conn.enviar(cmd)) {
        std::cout << "  [ERROR] No se pudo contactar con el servidor.\n";
        return false;
    }

    std::string resp = conn.recibir();
    std::string tipo = obtenerCampo(resp, 0);

    if (tipo == "OK") {
        std::cout << "\n  ✓ Cuenta creada correctamente.\n";
        std::cout << "  Ya puedes iniciar sesión con tu email y contraseña.\n\n";
        return true;
    } else {
        // ERROR|409|Email o DNI ya registrado
        std::cout << "\n  [ERROR] " << obtenerCampo(resp, 2) << "\n\n";
        return false;
    }
}
