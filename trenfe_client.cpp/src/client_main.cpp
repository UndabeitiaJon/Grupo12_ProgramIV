/*
 * client_main.cpp
 *
 *  Created on: 8 may 2026
 *      Author: ander.lecue
 */

int main() {
    // 1. Cargar config del cliente
    ConfigCliente cfg = cargarConfigCliente("./data/client.cfg");

    // 2. Crear conexión
    Conexion conn(cfg.ip, cfg.puerto);
    if (!conn.conectar()) { /* error */ return 1; }

    // 3. Bucle login (hasta 3 intentos)
    UsuarioBase* usuario = nullptr;
    int intentos = 0;
    while (!usuario && intentos < 3) {
        usuario = login(conn);
        intentos++;
    }

    // 4. Menú polimórfico
    if (usuario) {
        usuario->mostrarMenuPrincipal(); // dispatch automático por rol
        delete usuario;
    }

    // 5. Desconexión
    conn.desconectar();
    return 0;
}


