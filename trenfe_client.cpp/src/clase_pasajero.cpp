/*
 * clase_pasajero.cpp
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */

/*
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
        std::cout << "  1. Buscar trayecto y reservar\n";
        std::cout << "  2. Mis reservas\n";
        std::cout << "  3. Mis datos\n";
        std::cout << "  4. Cambiar contraseña\n";
        std::cout << "  0. Cerrar sesión\n";
        std::cout << "  Opción: ";
        std::cin  >> opcion;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (opcion) {
            case 1:
            	menuBuscarTrayecto();
            	break;
            case 2:
            	menuMisReservas();
            	break;
            case 3:
            	mostrarMisDatos();
            	break;
            case 4:
            	cambiarContrasenia();
            	break;
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
        std::cout << "\n  No hay trayectos disponibles para esa búsqueda.\n";
        return;
    }
    std::cout << "\n  "
              << std::left
              << std::setw(5)  << "ID"
              << std::setw(26) << "ORIGEN"
              << std::setw(26) << "DESTINO"
              << std::setw(10) << "SALIDA"
              << std::setw(10) << "LLEGADA"
              << std::setw(11) << "PRECIO"
              << "PLAZAS"
              << "\n";
    std::cout << "  " << std::string(90, '-') << "\n";

    for (const auto& linea : lista) {
        /* TRAYECTO|id|origen|destino|hora_sal|hora_ll|precio|libres|dias */
        if (campo(linea, 0) != "TRAYECTO") continue;

        std::string libres = campo(linea, 7);
        std::string libres_txt;
        if (libres.empty() || libres == "-1"){
        	libres_txt = "?";
        }else if(libres == "0"){
        	libres_txt = "AGOTADO";
        }else {
        	libres_txt = libres + " libres";
        }

        std::cout << "  "
                  << std::left
                  << std::setw(5)  << campo(linea, 1)
                  << std::setw(26) << campo(linea, 2)
                  << std::setw(26) << campo(linea, 3)
                  << std::setw(10) << campo(linea, 4)
                  << std::setw(10) << campo(linea, 5)
                  << std::setw(11) << (campo(linea, 6) + " EUR")
                  << libres_txt
                  << "\n";
    }
    std::cout << "\n";
}


std::string seleccionarEstacion(const std::vector<std::string>& cacheEstaciones,
                                const std::string& etiqueta)
{
    while (true) {
        std::string busqueda;
        std::cout << "\n  Ciudad " << etiqueta << " (Enter=cancelar): ";
        std::getline(std::cin, busqueda);
        if (busqueda.empty()) return "";

        /* Convertir búsqueda a minúsculas para comparación sin distinción */
        std::string busqLow = busqueda;
        for (auto& c : busqLow) c = (char)tolower((unsigned char)c);

        /* Filtrar estaciones cuya ciudad contenga el texto */
        struct Match { std::string id, nombre, ciudad, provincia; };
        std::vector<Match> coincidencias;

        for (const auto& e : cacheEstaciones) {
            if (campo(e, 0) != "ESTACION") continue;
            std::string ciudad = campo(e, 3);
            std::string ciudadLow = ciudad;
            for (auto& c : ciudadLow) c = (char)tolower((unsigned char)c);

            if (ciudadLow.find(busqLow) != std::string::npos)
                coincidencias.push_back({campo(e,1), campo(e,2), ciudad, campo(e,4)});
        }

        if (coincidencias.empty()) {
            std::cout << "  No se encontraron estaciones para \"" << busqueda
                      << "\". Inténtalo de nuevo.\n";
            continue;
        }

        /* Una sola coincidencia → selección automática */
        if (coincidencias.size() == 1) {
            std::cout << "  Estación seleccionada automáticamente: "
                      << coincidencias[0].nombre
                      << " (" << coincidencias[0].ciudad << ")\n";
            return coincidencias[0].id;
        }

        /* Varias coincidencias → mostrar tabla y pedir ID */
        std::cout << "\n  " << std::left
                  << std::setw(5)  << "ID"
                  << std::setw(32) << "ESTACION"
                  << std::setw(20) << "CIUDAD"
                  << "PROVINCIA\n";
        std::cout << "  " << std::string(72, '-') << "\n";
        for (const auto& m : coincidencias)
            std::cout << "  " << std::left
                      << std::setw(5)  << m.id
                      << std::setw(32) << m.nombre
                      << std::setw(20) << m.ciudad
                      << m.provincia << "\n";
        std::cout << "\n";

        std::cout << "  ID estación " << etiqueta << " (0 = nueva búsqueda): ";
        std::string s_id;
        std::getline(std::cin, s_id);
        if (s_id == "0" || s_id.empty()) continue;   /* volver a buscar */

        /* Verificar que el ID está en la lista mostrada */
        for (const auto& m : coincidencias)
            if (m.id == s_id) return s_id;

        std::cout << "  ID no encontrado en la lista. Inténtalo de nuevo.\n";
        /* No se sale del bucle: vuelve a pedir ciudad */
    }
}

void Pasajero::menuBuscarTrayecto() {
    cargarEstaciones();

    std::string s_orig = seleccionarEstacion(cacheEstaciones, "origen");
    if (s_orig.empty()) return;

    std::string s_dest = seleccionarEstacion(cacheEstaciones, "destino");
    if (s_dest.empty()) return;

    std::string fecha, clase;
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
    menuHacerReserva(s_id_tr, orig_nombre, dest_nombre, fecha, clase);
}

/* ══════════════════════════════════════════════
   HACER RESERVA
   ══════════════════════════════════════════════ */

void Pasajero::menuHacerReserva(const std::string& id_tr,
                                 const std::string& orig_nombre,
                                 const std::string& dest_nombre,
                                 const std::string& fecha,
                                 const std::string& clase) {

    std::cout << "\n  ╔══════════════════════════════════════════════╗\n";
    std::cout << "  ║  NUEVA RESERVA                               ║\n";
    std::cout << "  ╚══════════════════════════════════════════════╝\n";
    std::cout << "  Trayecto : " << orig_nombre << " → " << dest_nombre << "\n";
    std::cout << "  Fecha    : " << fecha << "   Clase: " << clase << "\n\n";

    /* ── 1. Vagones disponibles ── */
    conn.enviar("LISTAR_VAGONES|" + id_tr + "|" + fecha + "|" + clase);
    std::vector<std::string> vagones = conn.recibirLista();

    if (vagones.empty()) {
        std::cout << "  No hay vagones de clase " << clase << " para este trayecto.\n";
        return;
    }

    /* ── 2. Tabla de vagones ── */
    std::cout << "  Vagones disponibles:\n";
    std::cout << "  " << std::string(36, '-') << "\n";
    std::cout << "  " << std::left
              << std::setw(10) << "VAGON"
              << std::setw(14) << "CAPACIDAD"
              << "LIBRES\n";
    std::cout << "  " << std::string(36, '-') << "\n";

    std::vector<std::string> ids_vagones;
    for (const auto& v : vagones) {
        if (campo(v, 0) != "VAGON") continue;
        std::string num = campo(v, 1);
        std::string lib = campo(v, 3);
        std::string lib_txt = (lib == "0") ? "AGOTADO" : lib + " libres";
        std::cout << "  " << std::left
                  << std::setw(10) << ("  [" + num + "]")
                  << std::setw(14) << campo(v, 2)
                  << lib_txt << "\n";
        if (lib != "0") ids_vagones.push_back(num);
    }
    std::cout << "  " << std::string(36, '-') << "\n";

    if (ids_vagones.empty()) {
        std::cout << "\n  Todos los vagones están agotados para esta fecha.\n";
        return;
    }

    /* ── 3. Elegir vagón ── */
    std::string s_vagon;
    std::cout << "\n  Número de vagón (0 = cancelar): ";
    std::getline(std::cin, s_vagon);
    if (s_vagon == "0" || s_vagon.empty()) return;

    bool vagon_ok = false;
    for (const auto& id : ids_vagones)
        if (id == s_vagon) { vagon_ok = true; break; }
    if (!vagon_ok) {
        std::cout << "  Vagón no válido o sin plazas libres.\n";
        return;
    }

    /* ── 4. Mapa de asientos ── */
    conn.enviar("MAPA_VAGON|" + id_tr + "|" + fecha + "|" + s_vagon);
    std::vector<std::string> mapa = conn.recibirLista();

    int capacidad = 0;
    std::vector<int> ocupados_lista;
    for (const auto& linea : mapa) {
        std::string tipo = campo(linea, 0);
        if (tipo == "MAPA_INFO") {
            try { capacidad = std::stoi(campo(linea, 2)); } catch (...) {}
        } else if (tipo == "ASIENTO" && campo(linea, 2) == "1") {
            try { ocupados_lista.push_back(std::stoi(campo(linea, 1))); } catch (...) {}
        }
    }

    if (capacidad > 0) {
        bool ocup[101] = {};
        for (int a : ocupados_lista) if (a > 0 && a <= 100) ocup[a] = true;

        std::cout << "\n  MAPA DE ASIENTOS  –  Vagón " << s_vagon
                  << "  (capacidad " << capacidad << ")\n";
        std::cout << "   Número = libre  |[XX]| = ocupado\n";
        std::cout << "  +------+------+------+------+\n";
        std::cout << "  |  A   |  B   |  C   |  D   |\n";
        std::cout << "  +------+------+------+------+\n";

        int filas = (capacidad + 3) / 4;
        for (int f = 1; f <= filas; f++) {
            std::cout << "  |";
            for (int c = 0; c < 4; c++) {
                int asiento = (f - 1) * 4 + c + 1;
                if (asiento <= capacidad) {
                    if (ocup[asiento])
                        std::cout << std::setw(5) << ("[" + std::to_string(asiento) + "]") << " |";
                    else
                        std::cout << std::setw(5) << asiento << " |";
                } else {
                    std::cout << "      |";
                }
            }
            std::cout << " F" << std::setfill('0') << std::setw(2) << f
                      << std::setfill(' ') << "\n";
        }
        std::cout << "  +------+------+------+------+\n";
        std::cout << "  (A-F01 = asiento 1, B-F01 = asiento 2, ...)\n";
    }

    /* ── 5. Elegir asiento ── */
    std::string s_asiento;
    std::cout << "\n  Número de asiento (0 = cancelar): ";
    std::getline(std::cin, s_asiento);
    if (s_asiento == "0" || s_asiento.empty()) return;

    /* ── 6. Equipaje ── */
    std::string tipo_eq, peso_eq;
    std::cout << "  Equipaje (MANO / BODEGA / BICI / ESQUI, Enter = ninguno): ";
    std::getline(std::cin, tipo_eq);
    if (!tipo_eq.empty()) {
        std::cout << "  Peso equipaje (kg): ";
        std::getline(std::cin, peso_eq);
    }

    /* ── 7. Puntos de fidelidad ── */
    conn.enviar("MIS_PUNTOS");
    std::string resp_pts = conn.recibir();
    int puntos_disp = 0;
    try { puntos_disp = std::stoi(campo(resp_pts, 1)); } catch (...) {}

    int puntos_a_canjear = 0;
    if (puntos_disp > 0) {
        /* 100 puntos = 1 EUR de descuento */
        double descuento_max = puntos_disp / 100.0;
        std::cout << "\n  ★  Tienes " << puntos_disp << " puntos de fidelidad"
                  << " (equivalen a " << std::fixed << std::setprecision(2)
                  << descuento_max << " EUR de descuento).\n";
        std::cout << "  ¿Cuántos puntos quieres canjear? (0 = ninguno, máx "
                  << puntos_disp << "): ";
        std::string s_pts;
        std::getline(std::cin, s_pts);
        try { puntos_a_canjear = std::stoi(s_pts); } catch (...) {}
        if (puntos_a_canjear < 0)              puntos_a_canjear = 0;
        if (puntos_a_canjear > puntos_disp)    puntos_a_canjear = puntos_disp;
        if (puntos_a_canjear > 0)
            std::cout << "  → Se aplicarán " << puntos_a_canjear << " puntos ("
                      << std::fixed << std::setprecision(2)
                      << (puntos_a_canjear / 100.0) << " EUR de descuento).\n";
    }

    /* ── 8. Enviar HACER_RESERVA ── */
    /* Protocolo: HACER_RESERVA|id_tr|fecha|clase|vagon|asiento|tipo_eq|peso_eq|puntos */
    std::string cmd = "HACER_RESERVA|" + id_tr + "|" +
                      fecha + "|" + clase + "|" + s_vagon + "|" + s_asiento;
    if (!tipo_eq.empty())
        cmd += "|" + tipo_eq + "|" + (peso_eq.empty() ? "0" : peso_eq);
    else
        cmd += "|MANO|0";
    cmd += "|" + std::to_string(puntos_a_canjear);

    conn.enviar(cmd);
    std::string resp = conn.recibir();

    /* OK|id_res|precio_final|codigo_validacion|puntos_restantes */
    std::cout << "\n";
    if (campo(resp, 0) == "OK") {
        std::cout << "  ✓ Reserva confirmada!\n";
        std::cout << "  ID reserva        : " << campo(resp, 1) << "\n";
        std::cout << "  Precio final      : " << campo(resp, 2) << " EUR\n";
        std::cout << "  Código validación : " << campo(resp, 3) << "\n";
        if (!campo(resp, 4).empty())
            std::cout << "  Puntos restantes  : " << campo(resp, 4) << "\n";
    } else {
        std::cout << "  ✗ Error: " << campo(resp, 1) << "\n";
    }
    std::cout << "\n";
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
    conn.enviar("MIS_RESERVAS");   /* B-15: el servidor usa id_u de la sesion */
    std::vector<std::string> reservas = conn.recibirLista();
    mostrarReservas(reservas);

    if (reservas.empty()) return;

    std::string opcion;
    std::cout << "\n  Cancelar reserva (introduce ID, 0 = volver): ";
    std::getline(std::cin, opcion);
    if (opcion == "0" || opcion.empty()) return;

    conn.enviar("CANCELAR_RESERVA|" + opcion);   /* B-13: sin id_u */
    std::string resp = conn.recibir();
    if (campo(resp, 0) == "OK") {
        std::cout << "  Reserva cancelada correctamente.\n";
    } else {
        std::cout << "  Error: " << campo(resp, 1) << "\n";
    }
}


