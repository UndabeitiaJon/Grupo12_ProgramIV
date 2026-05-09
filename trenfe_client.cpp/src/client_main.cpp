/*
 * client_main.cpp
 *
 *  Created on: 8 may 2026
 *      Author: Grupo12
 */

/*
 * client_main.cpp  -  Sistema TRENFE  -  Fase 2
 *
 * Punto de entrada del cliente remoto.
 * Secuencia:
 *   1. Cargar config del cliente (data/client.cfg)
 *   2. Crear conexión TCP con el servidor
 *   3. Bucle de login (hasta 3 intentos)
 *   4. Lanzar menú polimórfico según el rol del usuario
 *   5. Desconectar
 */

#include <iostream>
#include "config_cliente.h"
#include "conexion.h"
#include "usuario_base.h"

/*
 * login() está implementada en client_auth.cpp.
 * La declaramos aquí para que el compilador la conozca.
 */
UsuarioBase* login(Conexion& conn);

int main() {

    std::cout << "\n========================================\n";
    std::cout << "   TRENFE  -  Cliente Remoto  -  Fase 2\n";
    std::cout << "========================================\n\n";

    /* 1. Cargar configuración del cliente */
    ConfigCliente cfg = cargarConfigCliente("./data/client.cfg");

    /* 2. Crear y abrir conexión TCP con el servidor */
    Conexion conn(cfg.ip, cfg.puerto);

    std::cout << "  Conectando con " << cfg.ip << ":" << cfg.puerto << "...\n";

    if (!conn.conectar()) {
        std::cerr << "  [ERROR] No se pudo conectar al servidor.\n";
        std::cerr << "  Asegúrate de que el servidor está arrancado.\n";
        return 1;
    }

    std::cout << "  Conexión establecida.\n\n";

    /* 3. Bucle de login — máximo 3 intentos */
    UsuarioBase* usuario = nullptr;
    int intentos = 0;
    const int MAX_INTENTOS = 3;

    while (usuario == nullptr && intentos < MAX_INTENTOS) {

        if (intentos > 0) {
            std::cout << "\n  Intento " << (intentos + 1)
                      << " de " << MAX_INTENTOS << "\n";
        }

        std::cout << "  --- INICIO DE SESIÓN ---\n";
        usuario = login(conn);
        intentos++;
    }

    if (usuario == nullptr) {
        std::cout << "\n  Demasiados intentos fallidos. Cerrando.\n";
        conn.desconectar();
        return 1;
    }

    /* 4. Lanzar el menú según el rol (polimorfismo) */
    usuario->mostrarMenuPrincipal();

    /* 5. Liberar memoria y desconectar */
    delete usuario;
    conn.desconectar();

    std::cout << "\n  Hasta luego.\n\n";
    return 0;
}
