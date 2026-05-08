/*
 * clase_pasajero.cpp
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */

/*
 * clase_pasajero.cpp  -  Sistema TRENFE  -  Fase 2
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <limits>
#include "clase_pasajero.h"

/* ══════════════════════════════════════════════
   CONSTRUCTOR
   ══════════════════════════════════════════════ */

Pasajero::Pasajero(int id, const std::string& nom, const std::string& ape,
                   const std::string& mail, Conexion& c)
    : UsuarioBase(id, nom, ape, mail, "PASAJERO", c)
{}

/* ══════════════════════════════════════════════
   MENÚ PRINCIPAL
   ══════════════════════════════════════════════ */

void Pasajero::mostrarMenuPrincipal() {
    int opcion = 0;
    do {
        mostrarCabecera(nombre, rol);
        std::cout << "  1. Buscar trayecto\n";
        std::cout << "  2. Mis reservas\n";
        std::cout << "  3. Mis puntos de fidelidad\n";
        std::cout << "  4. Mis datos\n";
        std::cout << "  5. Cambiar contraseña\n";
        std::cout << "  0. Cerrar sesión\n";
        std::cout << "  Opción: ";
        std::cin  >> opcion;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (opcion) {
            case 1: menuBuscarTrayecto(); break;
            case 2: menuMisReservas();    break;
            case 3: menuPuntos();         break;
            case 4: mostrarMisDatos();    break;
            case 5: cambiarContrasenia(); break;
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
   CACHÉ DE ESTACIONES
   ══════════════════════════════════════════════ */

void Pasajero::cargarEstaciones() {
    if (estacionesCargadas) return;   /* ya en caché, no pedir al servidor */

    conn.enviar("LISTAR_ESTACIONES");
    cacheEstaciones = conn.recibirLista();
    estacionesCargadas = true;
}

/* ══════════════════════════════════════════════
   BUSCAR TRAYECTO
   ══════════════════════════════════════════════ */

void Pasajero::mostrarTrayectos(const std::vector<std::string>& lista) {
    if (lista.empty()) {
        std::cout << "  No hay trayectos disponibles.\n";
        return;
    }
    std::cout << "\n  " << std::left
              << std::setw(4)  << "ID"
              << std::setw(22) << "ORIGEN"
              << std::setw(22) << "DESTINO"
              << std::setw(7)  << "SAL."
              << std::setw(7)  << "LLEGA"
              << std::setw(10) << "PRECIO"
              << "\n";
    std::cout << "  " << std::string(72, '-') << "\n";

    for (const auto& linea : lista) {
        /* TRAYECTO|id|origen|destino|hora_sal|hora_ll|precio|estado */
        /* o con asientos libres: TRAYECTO|id|orig|dest|sal|ll|precio|libres|dias */
        std::string tipo = campo(linea, 0);
        if (tipo != "TRAYECTO") continue;
        std::cout << "  " << std::left
                  << std::setw(4)  << campo(linea, 1)
                  << std::setw(22) << campo(linea, 2)
                  << std::setw(22) << campo(linea, 3)
                  << std::setw(7)  << campo(linea, 4)
                  << std::setw(7)  << campo(linea, 5)
                  << std::setw(10) << campo(linea, 6)
                  << "\n";
    }
}

void Pasajero::menuBuscarTrayecto() {
    cargarEstaciones();   /* usa caché la segunda vez */

    /* Mostrar estaciones disponibles */
    std::cout << "\n  -- Estaciones disponibles --\n";
    for (const auto& e : cacheEstaciones) {
        /* ESTACION|id|nombre|ciudad|provincia|andenes|sala */
        if (campo(e, 0) == "ESTACION") {
            std::cout << "  [" << campo(e, 1) << "] "
                      << campo(e, 2) << " (" << campo(e, 3) << ")\n";
        }
    }

    std::string s_orig, s_dest, fecha, clase;
    std::cout << "\n  ID estación origen  : "; std::getline(std::cin, s_orig);
    std::cout << "  ID estación destino : "; std::getline(std::cin, s_dest);
    std::cout << "  Fecha (AAAA-MM-DD)  : "; std::getline(std::cin, fecha);
    std::cout << "  Clase (T/B)         : "; std::getline(std::cin, clase);
    if (clase != "B" && clase != "b") clase = "T";
    else clase = "B";

    conn.enviar("BUSCAR_TRAYECTO|" + s_orig + "|" + s_dest + "|" + fecha + "|" + clase);
    std::vector<std::string> trayectos = conn.recibirLista();
    cacheTrayectos = trayectos;
    trayectosCargados = true;

    mostrarTrayectos(trayectos);

    if (trayectos.empty()) return;

    std::string s_id_tr;
    std::cout << "\n  ID trayecto para reservar (0 = cancelar): ";
    std::getline(std::cin, s_id_tr);
    if (s_id_tr == "0" || s_id_tr.empty()) return;

    /* Buscar origen y destino del trayecto elegido */
    std::string orig_nombre, dest_nombre;
    for (const auto& t : trayectos) {
        if (campo(t, 1) == s_id_tr) {
            orig_nombre = campo(t, 2);
            dest_nombre = campo(t, 3);
            break;
        }
    }
    menuHacerReserva(s_id_tr, orig_nombre, dest_nombre);
}

/* ══════════════════════════════════════════════
   HACER RESERVA
   ══════════════════════════════════════════════ */

void Pasajero::menuHacerReserva(const std::string& id_tr,
                                 const std::string& id_origen,
                                 const std::string& id_destino) {
    (void)id_origen; (void)id_destino;

    std::string fecha, clase, s_vagon, s_asiento, tipo_eq, peso_eq;

    std::cout << "\n  -- Nueva reserva (trayecto " << id_tr << ") --\n";
    std::cout << "  Fecha de viaje (AAAA-MM-DD) : "; std::getline(std::cin, fecha);
    std::cout << "  Clase (T=Turista / B=Business): "; std::getline(std::cin, clase);
    if (clase != "B" && clase != "b") clase = "T"; else clase = "B";
    std::cout << "  Número de vagón             : "; std::getline(std::cin, s_vagon);
    std::cout << "  Número de asiento           : "; std::getline(std::cin, s_asiento);
    std::cout << "  Equipaje (MANO/BODEGA/BICI/ESQUI, Enter=ninguno): ";
    std::getline(std::cin, tipo_eq);
    if (!tipo_eq.empty()) {
        std::cout << "  Peso equipaje (kg)          : "; std::getline(std::cin, peso_eq);
    }

    std::string cmd = "HACER_RESERVA|" + std::to_string(id_u) + "|" + id_tr + "|" +
                      fecha + "|" + clase + "|" + s_vagon + "|" + s_asiento;
    if (!tipo_eq.empty()) cmd += "|" + tipo_eq + "|" + (peso_eq.empty() ? "0" : peso_eq);
    else                  cmd += "|MANO|0";

    conn.enviar(cmd);
    std::string resp = conn.recibir();

    /* OK|id_res|precio_final|codigo_validacion */
    if (campo(resp, 0) == "OK") {
        std::cout << "\n  ✓ Reserva confirmada!\n";
        std::cout << "  ID reserva        : " << campo(resp, 1) << "\n";
        std::cout << "  Precio final      : " << campo(resp, 2) << " €\n";
        std::cout << "  Código validación : " << campo(resp, 3) << "\n";
    } else {
        std::cout << "\n  ✗ Error: " << campo(resp, 1) << "\n";
    }
}

/* ══════════════════════════════════════════════
   MIS RESERVAS
   ══════════════════════════════════════════════ */

void Pasajero::mostrarReservas(const std::vector<std::string>& lista) {
    if (lista.empty()) {
        std::cout << "  No tienes reservas.\n";
        return;
    }
    std::cout << "\n  " << std::left
              << std::setw(6)  << "ID"
              << std::setw(18) << "ORIGEN"
              << std::setw(18) << "DESTINO"
              << std::setw(12) << "FECHA"
              << std::setw(6)  << "CLASE"
              << std::setw(9)  << "PRECIO"
              << std::setw(12) << "ESTADO"
              << "\n";
    std::cout << "  " << std::string(80, '-') << "\n";

    for (const auto& linea : lista) {
        /* RESERVA|id_res|orig|dest|fecha|clase|vagon|asiento|precio|estado|cod */
        if (campo(linea, 0) != "RESERVA") continue;
        std::cout << "  " << std::left
                  << std::setw(6)  << campo(linea, 1)
                  << std::setw(18) << campo(linea, 2)
                  << std::setw(18) << campo(linea, 3)
                  << std::setw(12) << campo(linea, 4)
                  << std::setw(6)  << campo(linea, 5)
                  << std::setw(9)  << campo(linea, 8)
                  << std::setw(12) << campo(linea, 9)
                  << "\n";
    }
}

void Pasajero::menuMisReservas() {
    conn.enviar("MIS_RESERVAS|" + std::to_string(id_u));
    std::vector<std::string> reservas = conn.recibirLista();
    mostrarReservas(reservas);

    if (reservas.empty()) return;

    std::string opcion;
    std::cout << "\n  Cancelar reserva (introduce ID, 0 = volver): ";
    std::getline(std::cin, opcion);
    if (opcion == "0" || opcion.empty()) return;

    conn.enviar("CANCELAR_RESERVA|" + opcion + "|" + std::to_string(id_u));
    std::string resp = conn.recibir();
    if (campo(resp, 0) == "OK") {
        std::cout << "  Reserva cancelada correctamente.\n";
    } else {
        std::cout << "  Error: " << campo(resp, 1) << "\n";
    }
}

/* ══════════════════════════════════════════════
   PUNTOS FIDELIDAD
   ══════════════════════════════════════════════ */

void Pasajero::menuPuntos() {
    conn.enviar("MIS_PUNTOS|" + std::to_string(id_u));
    std::string resp = conn.recibir();
    int puntos = std::stoi(campo(resp, 1).empty() ? "0" : campo(resp, 1));

    std::cout << "\n  Tienes " << puntos << " puntos de fidelidad.\n";
    std::cout << "  ¿Canjear puntos? (s/n): ";
    std::string op;
    std::getline(std::cin, op);
    if (op != "s" && op != "S") return;

    std::cout << "  Cantidad a canjear: ";
    std::string s_cant;
    std::getline(std::cin, s_cant);

    conn.enviar("CANJEAR_PUNTOS|" + std::to_string(id_u) + "|" + s_cant);
    resp = conn.recibir();
    if (campo(resp, 0) == "OK") {
        std::cout << "  Canje realizado. Puntos restantes: " << campo(resp, 1) << "\n";
    } else {
        std::cout << "  Error: " << campo(resp, 1) << "\n";
    }
}


