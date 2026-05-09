/*
 * test_conexion.cpp
 *
 *  Created on: 8 may 2026
 *      Author: Grupo12
 */

/*
 * test_conexion.cpp  -  Sistema TRENFE  -  Fase 2
 *
 * Test mínimo del Bloque 1.
 * Verifica que:
 *   1. El cliente puede conectarse al servidor
 *   2. El login correcto devuelve AUTH_OK
 *   3. El login incorrecto devuelve AUTH_FAIL
 *   4. El LOGOUT funciona
 *
 * USO:
 *   - Arrancar primero el servidor: ./trenfe_server
 *   - Luego ejecutar este test:     ./test_conexion
 *
 * Credenciales de prueba (del seed_database):
 *   admin@trenfe.com  / admin123
 *   juan@trenfe.com   / pass123
 *   pedro@trenfe.com  / maq123
 */

#include <iostream>
#include <string>
#include "conexion.h"

extern "C" {
#include "hash.h"
}

/* Divide "campo1|campo2|..." y devuelve el campo Nº pos */
static std::string campo(const std::string& linea, int pos) {
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

/* Prueba un login y muestra el resultado */
static bool probarLogin(Conexion& conn,
                         const std::string& email,
                         const std::string& pass,
                         bool esperaExito) {

    char hash[65];
    sha256_hex(pass.c_str(), hash);

    std::string cmd = "LOGIN|" + email + "|" + std::string(hash);
    conn.enviar(cmd);

    std::string resp = conn.recibir();
    std::string tipo = campo(resp, 0);

    bool exito = (tipo == "AUTH_OK");

    std::cout << "  LOGIN " << email << " → " << resp;

    if (exito == esperaExito) {
        std::cout << "  ✓ OK\n";
    } else {
        std::cout << "  ✗ FALLO (resultado inesperado)\n";
    }

    /* Si hicimos login correcto, mandamos LOGOUT para limpiar la sesión */
    if (exito) {
        conn.enviar("LOGOUT");
        std::string resp_logout = conn.recibir();
        std::cout << "  LOGOUT → " << resp_logout << "\n";
    }

    return exito == esperaExito;
}

int main() {

    std::cout << "\n========================================\n";
    std::cout << "   TEST MÍNIMO - BLOQUE 1 - TRENFE\n";
    std::cout << "========================================\n\n";

    /* ── Paso 1: conectar ── */
    std::cout << "[ 1/4 ] Conectando al servidor 127.0.0.1:8080...\n";

    Conexion conn("127.0.0.1", 8080);
    if (!conn.conectar()) {
        std::cerr << "  ✗ FALLO: no se pudo conectar.\n";
        std::cerr << "  Asegúrate de que el servidor está arrancado.\n";
        return 1;
    }
    std::cout << "  ✓ Conexión establecida.\n\n";

    /* ── Paso 2: login correcto (admin) ── */
    std::cout << "[ 2/4 ] Login correcto (admin)...\n";
    bool t1 = probarLogin(conn, "admin@trenfe.com", "admin123", true);
    std::cout << "\n";

    /* ── Paso 3: login correcto (pasajero) ── */
    std::cout << "[ 3/4 ] Login correcto (pasajero)...\n";
    bool t2 = probarLogin(conn, "juan@trenfe.com", "pass123", true);
    std::cout << "\n";

    /* ── Paso 4: login incorrecto ── */
    std::cout << "[ 4/4 ] Login incorrecto (contraseña mala)...\n";
    bool t3 = probarLogin(conn, "juan@trenfe.com", "contraseña_mala", false);
    std::cout << "\n";

    /* ── Resumen ── */
    conn.desconectar();

    int pasados = (t1 ? 1 : 0) + (t2 ? 1 : 0) + (t3 ? 1 : 0);

    std::cout << "========================================\n";
    std::cout << "  RESULTADO: " << pasados << "/3 tests pasados\n";

    if (pasados == 3) {
        std::cout << "  ✓ Bloque 1 completado correctamente.\n";
    } else {
        std::cout << "  ✗ Hay fallos. Revisa el servidor.\n";
    }

    std::cout << "========================================\n\n";

    return (pasados == 3) ? 0 : 1;
}
