/*
 * client_main.cpp
 *
 *  Created on:
 *      Author:
 */



#include <iostream>
#include <string>
#include "config_cliente.h"
#include "conexion.h"
#include "usuario_base.h"

/* Cambia a 1 para ejecutar el test de conexión */
#define MODO_TEST 0


/* login() y registrar() están implementadas en client_auth.cpp */
UsuarioBase* login(Conexion& conn);
bool registrar(Conexion& conn);

extern "C" {
#include "hash.h"
}

/* ══════════════════════════════════════════
   FUNCIONES DEL TEST
   ══════════════════════════════════════════ */

static std::string campoCampo(const std::string& linea, int pos) {
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

static bool probarLogin(Conexion& conn,
                        const std::string& email,
                        const std::string& pass,
                        bool esperaExito) {
    char hash[65];
    sha256_hex(pass.c_str(), hash);

    conn.enviar("LOGIN|" + email + "|" + std::string(hash));
    std::string resp = conn.recibir();
    std::string tipo = campoCampo(resp, 0);
    bool exito = (tipo == "AUTH_OK");

    std::cout << "  " << email << " -> " << resp << "\n";

    if (exito == esperaExito) {
        std::cout << "  OK - Resultado correcto\n\n";
    } else {
        std::cout << "  FALLO - Resultado inesperado\n\n";
    }

    if (exito) {
        conn.enviar("LOGOUT");
        std::string r = conn.recibir();
        std::cout << "  LOGOUT -> " << r << "\n\n";
    }

    return exito == esperaExito;
}

static int ejecutarTest() {
    std::cout << "\n========================================\n";
    std::cout << "   TEST BLOQUE 1 - TRENFE\n";
    std::cout << "========================================\n\n";

    std::cout << "[ 1/4 ] Conectando a 127.0.0.1:8080...\n";
    Conexion conn("127.0.0.1", 8080);
    if (!conn.conectar()) {
        std::cout << "  FALLO: No se pudo conectar. Arranca el servidor primero.\n";
        return 1;
    }
    std::cout << "  OK - Conexion establecida.\n\n";

    std::cout << "[ 2/4 ] Login correcto (admin):\n";
    bool t1 = probarLogin(conn, "admin@trenfe.com", "admin123", true);

    std::cout << "[ 3/4 ] Login correcto (pasajero):\n";
    bool t2 = probarLogin(conn, "juan@trenfe.com", "pass123", true);

    std::cout << "[ 4/4 ] Login incorrecto:\n";
    bool t3 = probarLogin(conn, "juan@trenfe.com", "mala_clave", false);

    conn.desconectar();

    int ok = (t1?1:0) + (t2?1:0) + (t3?1:0);
    std::cout << "========================================\n";
    std::cout << "  RESULTADO: " << ok << "/3 tests pasados\n";
    if (ok == 3)
        std::cout << "  Bloque 1 completado correctamente.\n";
    else
        std::cout << "  Hay fallos. Revisa el servidor.\n";
    std::cout << "========================================\n\n";

    return (ok == 3) ? 0 : 1;
}

/* ══════════════════════════════════════════
   MAIN
   ══════════════════════════════════════════ */

int main() {

#if MODO_TEST
    return ejecutarTest();

#else
    std::cout << "\n========================================\n";
    std::cout << "   TRENFE  -  Cliente Remoto  -  Fase 2\n";
    std::cout << "========================================\n\n";

    ConfigCliente cfg = cargarConfigCliente("./data/client.cfg");

    Conexion conn(cfg.ip, cfg.puerto);
    std::cout << "  Conectando con " << cfg.ip << ":" << cfg.puerto << "...\n";

    if (!conn.conectar()) {
        std::cerr << "  [ERROR] No se pudo conectar al servidor.\n";
        return 1;
    }
    std::cout << "  Conexion establecida.\n\n";

    UsuarioBase* usuario = nullptr;
    int intentosFallidos = 0;

    while (usuario == nullptr) {
        //Menú de acceso
        std::cout << "  ----------------------------------------\n";
        std::cout << "   1. Iniciar sesión\n";
        std::cout << "   2. Registrarse\n";
        std::cout << "   0. Salir\n";
        std::cout << "  ----------------------------------------\n";
        std::cout << "  Opción: ";

        std::string opStr;
        std::getline(std::cin, opStr);
        int opcion = -1;
        if (!opStr.empty() && opStr[0] >= '0' && opStr[0] <= '9')
            opcion = opStr[0] - '0';

        if (opcion == 0) {
            std::cout << "\n  Hasta luego.\n\n";
            conn.desconectar();
            return 0;
        }

        if (opcion == 2) {
            // Registro: si tiene éxito el usuario puede hacer login a continuación
            registrar(conn);
            continue;
        }

        if (opcion != 1) {
            std::cout << "  [ERROR] Opción no válida.\n\n";
            continue;
        }

        // Login
        if (intentosFallidos > 0)
            std::cout << "\n  Intento " << (intentosFallidos + 1) << " de 3\n";

        std::cout << "  --- INICIO DE SESIÓN ---\n";
        usuario = login(conn);

        if (usuario == nullptr) {
            intentosFallidos++;
            if (intentosFallidos >= 3) {
                std::cout << "\n  Demasiados intentos fallidos. Cerrando.\n";
                conn.desconectar();
                return 1;
            }
        }
    }

    usuario->mostrarMenuPrincipal();

    delete usuario;
    conn.desconectar();
    std::cout << "\n  Hasta luego.\n\n";
    return 0;

#endif
}
