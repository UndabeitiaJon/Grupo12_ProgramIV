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
              << std::setw(8)  << "LIBRES"
              << "\n";
    std::cout << "  " << std::string(80, '-') << "\n";

    for (const auto& linea : lista) {
        /* TRAYECTO|id|origen|destino|hora_sal|hora_ll|precio|libres|dias */
        /*          1   2     3       4        5       6      7      8   */
        if (campo(linea, 0) != "TRAYECTO") continue;

        std::string libres = campo(linea, 7);
        std::string libres_txt = libres.empty() ? "-" :
                                 (libres == "0" ? "AGOTADO" : libres + " plz");

        std::cout << "  " << std::left
                  << std::setw(4)  << campo(linea, 1)
                  << std::setw(22) << campo(linea, 2)
                  << std::setw(22) << campo(linea, 3)
                  << std::setw(7)  << campo(linea, 4)
                  << std::setw(7)  << campo(linea, 5)
                  << std::setw(10) << (campo(linea, 6) + " €")
                  << std::setw(8)  << libres_txt
                  << "\n";
    }
}

/* ─────────────────────────────────────────────────────────────────────
   Helper: seleccionar estacion en tres pasos
   Equivalente a HashMap<Provincia, HashMap<Ciudad, List<Estacion>>>.
   Flujo:
     1. Muestra provincias numeradas.
     2. El usuario elige provincia.
     3. Muestra ciudades de esa provincia numeradas.
     4. El usuario elige ciudad.
     5. Muestra estaciones de esa ciudad con su ID real.
     6. El usuario introduce el ID de la estacion.
   Devuelve el ID de la estacion elegida, o "" si cancela.

   Formato del servidor: ESTACION|id|nombre|ciudad|provincia|andenes|sala
                         campo:    0   1      2      3         4
   ───────────────────────────────────────────────────────────────────── */
std::string seleccionarEstacion(const std::vector<std::string>& cacheEstaciones,
                                       const std::string& etiqueta)
{
    typedef std::pair<std::string, std::string>              ParEstacion;   /* {id, nombre} */
    typedef std::pair<std::string, std::vector<ParEstacion>> EntradaCiudad; /* {ciudad, estaciones} */
    typedef std::pair<std::string, std::vector<EntradaCiudad>> EntradaProv; /* {prov, ciudades} */

    /* ── 1. Construir mapa provincia -> ciudad -> estaciones ───────── */
    std::vector<EntradaProv> mapaProv;

    for (const auto& e : cacheEstaciones) {
        if (campo(e, 0) != "ESTACION") continue;
        std::string idEst    = campo(e, 1);
        std::string nomEst   = campo(e, 2);
        std::string ciudad   = campo(e, 3);
        std::string provincia = campo(e, 4);
        if (provincia.empty()) provincia = "Otras";
        if (ciudad.empty())    ciudad    = nomEst;

        /* Buscar o crear provincia */
        EntradaProv* prov = nullptr;
        for (auto& ep : mapaProv)
            if (ep.first == provincia) { prov = &ep; break; }
        if (!prov) {
            mapaProv.push_back({provincia, {}});
            prov = &mapaProv.back();
        }

        /* Buscar o crear ciudad dentro de la provincia */
        EntradaCiudad* ciu = nullptr;
        for (auto& ec : prov->second)
            if (ec.first == ciudad) { ciu = &ec; break; }
        if (!ciu) {
            prov->second.push_back({ciudad, {}});
            ciu = &prov->second.back();
        }

        ciu->second.push_back({idEst, nomEst});
    }

    /* ── 2. Mostrar provincias ─────────────────────────────────────── */
    std::cout << "\n  -- Provincias disponibles (" << etiqueta << ") --\n";
    std::cout << "  " << std::string(36, '-') << "\n";
    for (size_t i = 0; i < mapaProv.size(); ++i)
        std::cout << "  [" << (i + 1) << "] " << mapaProv[i].first << "\n";
    std::cout << "  " << std::string(36, '-') << "\n";

    std::cout << "  Numero de provincia (0 = cancelar): ";
    std::string s_np;
    std::getline(std::cin, s_np);
    int np = 0;
    try { np = std::stoi(s_np); } catch (...) { np = 0; }
    if (np <= 0 || np > (int)mapaProv.size()) return "";

    const std::vector<EntradaCiudad>& ciudades = mapaProv[np - 1].second;
    const std::string& provElegida             = mapaProv[np - 1].first;

    /* ── 3. Mostrar ciudades de esa provincia ──────────────────────── */
    std::cout << "\n  -- Ciudades en " << provElegida << " --\n";
    std::cout << "  " << std::string(36, '-') << "\n";
    for (size_t i = 0; i < ciudades.size(); ++i)
        std::cout << "  [" << (i + 1) << "] " << ciudades[i].first << "\n";
    std::cout << "  " << std::string(36, '-') << "\n";

    std::cout << "  Numero de ciudad (0 = cancelar): ";
    std::string s_nc;
    std::getline(std::cin, s_nc);
    int nc = 0;
    try { nc = std::stoi(s_nc); } catch (...) { nc = 0; }
    if (nc <= 0 || nc > (int)ciudades.size()) return "";

    const std::vector<ParEstacion>& estaciones = ciudades[nc - 1].second;
    const std::string& ciudadElegida           = ciudades[nc - 1].first;

    /* ── 4. Mostrar estaciones de esa ciudad con numeración ──────── */
    std::cout << "\n  -- Estaciones en " << ciudadElegida << " --\n";
    std::cout << "  " << std::string(36, '-') << "\n";
    for (size_t i = 0; i < estaciones.size(); ++i)
        std::cout << "  [" << (i + 1) << "] " << estaciones[i].second
                  << "  (id: " << estaciones[i].first << ")\n";
    std::cout << "  " << std::string(36, '-') << "\n";

    /* ── 5. El usuario elige por número; devolvemos el id_est real ─ */
    std::string s_ne;
    std::cout << "  Número " << etiqueta << " (0 = cancelar): ";
    std::getline(std::cin, s_ne);
    int ne = 0;
    try { ne = std::stoi(s_ne); } catch (...) { ne = 0; }
    if (ne <= 0 || ne > (int)estaciones.size()) return "";
    return estaciones[ne - 1].first;   /* id_est real de la BD */
}

void Pasajero::menuBuscarTrayecto() {
    cargarEstaciones();   /* usa caché la segunda vez */

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
    /* B-20: fecha y clase ya fueron preguntadas en menuBuscarTrayecto, no se vuelven a pedir */

    std::string s_vagon, s_asiento, tipo_eq, peso_eq;

    std::cout << "\n  -- Nueva reserva: " << orig_nombre << " → " << dest_nombre
              << " (" << fecha << ", clase " << clase << ") --\n";
    std::cout << "  Número de vagón             : "; std::getline(std::cin, s_vagon);
    std::cout << "  Número de asiento           : "; std::getline(std::cin, s_asiento);
    std::cout << "  Equipaje (MANO/BODEGA/BICI/ESQUI, Enter=ninguno): ";
    std::getline(std::cin, tipo_eq);
    if (!tipo_eq.empty()) {
        std::cout << "  Peso equipaje (kg)          : "; std::getline(std::cin, peso_eq);
    }

    /* B-13: el servidor obtiene id_u de la sesión, no lo enviamos desde el cliente */
    std::string cmd = "HACER_RESERVA|" + id_tr + "|" +
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

/* ══════════════════════════════════════════════
   PUNTOS FIDELIDAD
   ══════════════════════════════════════════════ */

void Pasajero::menuPuntos() {
    conn.enviar("MIS_PUNTOS");   /* B-15: sin id_u */
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

    conn.enviar("CANJEAR_PUNTOS|" + s_cant);   /* B-14: sin id_u */
    resp = conn.recibir();
    if (campo(resp, 0) == "OK") {
        std::cout << "  Canje realizado. Puntos restantes: " << campo(resp, 1) << "\n";
    } else {
        std::cout << "  Error: " << campo(resp, 1) << "\n";
    }
}
