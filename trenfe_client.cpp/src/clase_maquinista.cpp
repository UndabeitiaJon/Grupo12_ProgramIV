/*
 * clase_maquinista.cpp
 *
 *  Created on: 9 may 2026
 *      Author: e.aranoa
 */


#include <iostream>
#include <iomanip>
#include <limits>
#include "clase_maquinista.h"

Maquinista::Maquinista(int id, const std::string& nom, const std::string& ape,
                       const std::string& mail, Conexion& c)
    : UsuarioBase(id, nom, ape, mail, "MAQUINISTA", c)
{}

/* ══════════════════════════════════════════════
   MENÚ PRINCIPAL
   ══════════════════════════════════════════════ */

void Maquinista::mostrarMenuPrincipal() {
    int opcion = 0;
    do {
        mostrarCabecera(nombre, rol);
        std::cout << "  1. Ver mi cuadrante de servicios\n";
        std::cout << "  2. Marcar inicio de servicio\n";
        std::cout << "  3. Marcar fin de servicio\n";
        std::cout << "  4. Reportar retraso\n";
        std::cout << "  5. Mis datos\n";
        std::cout << "  6. Cambiar contraseña\n";
        std::cout << "  0. Cerrar sesión\n";
        std::cout << "  Opción: ";
        std::cin >> opcion;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (opcion) {
            case 1: menuCuadrante();  break;
            case 2: {
                menuCuadrante();
                std::string id;
                std::cout << "  ID servicio a iniciar: ";
                std::getline(std::cin, id);
                menuMarcarInicio(id);
                cuadranteCargado = false;
                break;
            }
            case 3: {
                menuCuadrante();
                std::string id;
                std::cout << "  ID servicio a finalizar: ";
                std::getline(std::cin, id);
                menuMarcarFin(id);
                cuadranteCargado = false;
                break;
            }
            case 4: {
                menuCuadrante();
                std::string id;
                std::cout << "  ID servicio: ";
                std::getline(std::cin, id);
                menuReportarRetraso(id);
                cuadranteCargado = false;  // forzar recarga para mostrar retraso actualizado
                break;
            }
            case 5: mostrarMisDatos();    break;
            case 6: cambiarContrasenia(); break;
            case 0:
                conn.enviar("LOGOUT");
                conn.recibir();
                break;
            default:
                std::cout << "  Opción no válida.\n";
        }
    } while (opcion != 0);
}

/* ══════════════════════════════════════════════
   CUADRANTE
   ══════════════════════════════════════════════ */

void Maquinista::mostrarCuadrante(const std::vector<std::string>& lista) {
    if (lista.empty()) {
        std::cout << "  No tienes servicios asignados.\n";
        return;
    }
    std::cout << "\n  " << std::left
              << std::setw(6)  << "ID"
              << std::setw(12) << "FECHA"
              << std::setw(18) << "ORIGEN"
              << std::setw(18) << "DESTINO"
              << std::setw(7)  << "SAL."
              << std::setw(7)  << "LLEGA"
              << std::setw(12) << "ESTADO"
              << std::setw(8)  << "RETRASO"
              << "\n";
    std::cout << "  " << std::string(88, '-') << "\n";

    for (const auto& linea : lista) {
        /* SERVICIO|id|fecha|origen|destino|h_sal|h_ll|estado|retraso_min */
        if (campo(linea, 0) != "SERVICIO") continue;
        std::cout << "  " << std::left
                  << std::setw(6)  << campo(linea, 1)
                  << std::setw(12) << campo(linea, 2)
                  << std::setw(18) << campo(linea, 3)
                  << std::setw(18) << campo(linea, 4)
                  << std::setw(7)  << campo(linea, 5)
                  << std::setw(7)  << campo(linea, 6)
                  << std::setw(12) << campo(linea, 7)
                  << std::setw(8)  << campo(linea, 8) + " min"
                  << "\n";
    }
}

void Maquinista::menuCuadrante() {
    if (!cuadranteCargado) {
        conn.enviar("CUADRANTE|" + std::to_string(id_u));
        cacheCuadrante  = conn.recibirLista();
        cuadranteCargado = true;
    }
    mostrarCuadrante(cacheCuadrante);
}

/* ══════════════════════════════════════════════
   INICIO / FIN DE SERVICIO
   ══════════════════════════════════════════════ */

void Maquinista::menuMarcarInicio(const std::string& id_serv) {
    conn.enviar("MARCAR_INICIO|" + id_serv);
    std::string resp = conn.recibir();
    if (campo(resp, 0) == "OK")
        std::cout << "  ✓ Servicio " << id_serv << " marcado como INICIADO.\n";
    else
        std::cout << "  ✗ Error: " << campo(resp, 1) << "\n";
}

void Maquinista::menuMarcarFin(const std::string& id_serv) {
    conn.enviar("MARCAR_FIN|" + id_serv);
    std::string resp = conn.recibir();
    if (campo(resp, 0) == "OK"){
    	std::cout << "  ✓ Servicio " << id_serv << " marcado como FINALIZADO.\n";
    }
    else{
    	std::cout << "  ✗ Error: " << campo(resp, 1) << "\n";
    }
}

/* ══════════════════════════════════════════════
   REPORTAR RETRASO
   ══════════════════════════════════════════════ */

void Maquinista::menuReportarRetraso(const std::string& id_serv) {
    std::string s_min, causa;
    std::cout << "  Minutos de retraso : ";
    std::getline(std::cin, s_min);
    std::cout << "  Causa              : ";
    std::getline(std::cin, causa);

    conn.enviar("REPORTAR_RETRASO|" + id_serv + "|" + s_min + "|" + causa);
    std::string resp = conn.recibir();
    if (campo(resp, 0) == "OK")
        std::cout << "  ✓ Retraso registrado.\n";
    else
        std::cout << "  ✗ Error: " << campo(resp, 1) << "\n";
}
