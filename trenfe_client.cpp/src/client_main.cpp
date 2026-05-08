/*
 * client_main.cpp
 *
 *  Created on: 8 may 2026
 *      Author: ander.lecue
 */

/*
 * client_main.cpp  -  Sistema TRENFE  -  Fase 2
 */

#include <iostream>
#include "config_cliente.h"
#include "conexion.h"
#include "usuario_base.h"   // ← lo crearemos en el siguiente paso

// Declarada en client_auth.cpp (siguiente paso)
UsuarioBase* login(Conexion& conn);

int main() {
    // 1. Cargar config del cliente
    ConfigCliente cfg = cargarConfigCliente("./data/client.cfg");

    // 2. Crear conexión
    Conexion conn(cfg.ip, cfg.puerto);
    if (!conn.conectar()) { return 1; }

    // 3. Bucle login (hasta 3 intentos)
    UsuarioBase* usuario = nullptr;
    int intentos = 0;
    while (!usuario && intentos < 3) {
        usuario = login(conn);
        intentos++;
    }

    // 4. Menú polimórfico
    if (usuario) {
        usuario->mostrarMenuPrincipal();
        delete usuario;
    }

    // 5. Desconexión
    conn.desconectar();
    return 0;
}


