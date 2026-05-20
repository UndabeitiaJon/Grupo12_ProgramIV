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

/*
 * client_auth.cpp  -  Sistema TRENFE  -  Fase 2
 *
 * Función login(): pide credenciales, las envía al servidor
 * y devuelve el objeto de usuario correcto según el rol.
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
}

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

/*
 * login()
 *
 * Devuelve puntero al objeto de usuario correcto, o nullptr si falla.
 * El llamador debe hacer delete sobre el puntero.
 */
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
bool registrar(Conexion& conn) {
    std::cout << "\n  ─────────────────────────────────────\n";
    std::cout << "        REGISTRO DE NUEVO USUARIO\n";
    std::cout << "  ─────────────────────────────────────\n";

    std::string nombre    = pedirCadena("  Nombre           : ");
    std::string apellido  = pedirCadena("  Apellido(s)       : ");
    std::string dni       = pedirCadena("  DNI/NIE           : ");
    std::string email     = pedirCadena("  Email             : ");
    std::string telf      = pedirCadena("  Teléfono          : ");
    std::string fecha_nac = pedirCadena("  Fecha nac (AAAA-MM-DD): ");

    /* Pedir contraseña con confirmación */
    std::string pass1, pass2;
    int intentos = 0;
    do {
        if (intentos > 0)
            std::cout << "  Las contraseñas no coinciden. Inténtalo de nuevo.\n";
        pass1 = pedirCadena("  Contraseña        : ");
        pass2 = pedirCadena("  Confirmar contraseña: ");
        intentos++;
    } while (pass1 != pass2 && intentos < 3);

    if (pass1 != pass2) {
        std::cout << "  [ERROR] Las contraseñas no coinciden tras 3 intentos.\n";
        return false;
    }
    if (pass1.size() < 6) {
        std::cout << "  [ERROR] La contraseña debe tener al menos 6 caracteres.\n";
        return false;
    }

    /* Validación local básica */
    if (nombre.empty() || apellido.empty() || dni.empty() ||
        email.empty() || telf.empty() || fecha_nac.empty()) {
        std::cout << "  [ERROR] Todos los campos son obligatorios.\n";
        return false;
    }
    if (email.find('@') == std::string::npos) {
        std::cout << "  [ERROR] El email no tiene formato válido.\n";
        return false;
    }

    /* Hashear contraseña antes de enviar */
    char hash[65];
    sha256_hex(pass1.c_str(), hash);

    /* Enviar al servidor */
    std::string cmd = "REGISTRO|" + nombre   + "|" + apellido  + "|" +
                                    dni       + "|" + email     + "|" +
                                    telf      + "|" + fecha_nac + "|" +
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
        /* ERROR|409|Email o DNI ya registrado */
        std::cout << "\n  [ERROR] " << obtenerCampo(resp, 2) << "\n\n";
        return false;
    }
}
