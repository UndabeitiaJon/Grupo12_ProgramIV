/*
 * clase_administrador.cpp
 *
 *  Created on: 9 may 2026
 *      Author: e.aranoa
 */


/*
 * clase_administrador.cpp  -  Sistema TRENFE  -  Fase 2
 */

#include <iostream>
#include <iomanip>
#include <limits>
#include <vector>
#include <string>
#include "clase_administrador.h"

Administrador::Administrador(int id, const std::string& nom, const std::string& ape,
                             const std::string& mail, Conexion& c)
    : UsuarioBase(id, nom, ape, mail, "ADMIN", c)
{}

/* Imprime una lista de líneas recibidas del servidor */
void Administrador::mostrarLista(const std::vector<std::string>& lista) {
    if (lista.empty()) { std::cout << "  (sin resultados)\n"; return; }
    for (const auto& l : lista)
        std::cout << "  " << l << "\n";
}

static std::string pedirLinea(const std::string& prompt) {
    std::string v;
    std::cout << prompt;
    std::getline(std::cin, v);
    return v;
}

/* ══════════════════════════════════════════════
   MENÚ PRINCIPAL
   ══════════════════════════════════════════════ */

void Administrador::mostrarMenuPrincipal() {
    int op = 0;
    do {
        mostrarCabecera(nombre, rol);
        std::cout << "  1.  Gestión de trenes\n";
        std::cout << "  2.  Gestión de estaciones\n";
        std::cout << "  3.  Gestión de trayectos\n";
        std::cout << "  4.  Gestión de usuarios / empleados\n";
        std::cout << "  5.  Gestión de servicios\n";
        std::cout << "  6.  Gestión de incidencias\n";
        std::cout << "  7.  Informes\n";
        std::cout << "  8.  Ver logs\n";
        std::cout << "  9.  Mis datos\n";
        std::cout << "  10. Cambiar contraseña\n";
        std::cout << "  0.  Cerrar sesión\n";
        std::cout << "  Opción: ";
        std::cin >> op;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (op) {
            case 1:  menuGestionTrenes();      break;
            case 2:  menuGestionEstaciones();  break;
            case 3:  menuGestionTrayectos();   break;
            case 4:  menuGestionUsuarios();    break;
            case 5:  menuGestionServicios();   break;
            case 6:  menuGestionIncidencias(); break;
            case 7:  menuInformes();           break;
            case 8:  menuLogs();               break;
            case 9:  mostrarMisDatos();        break;
            case 10: cambiarContrasenia();     break;
            case 0:
                conn.enviar("LOGOUT");
                conn.recibir();
                break;
            default: std::cout << "  Opción no válida.\n";
        }
    } while (op != 0);
}

/* ══════════════════════════════════════════════
   GESTIÓN DE TRENES
   ══════════════════════════════════════════════ */

void Administrador::menuGestionTrenes() {
    int op = 0;
    do {
        std::cout << "\n  -- Trenes --\n";
        std::cout << "  1. Listar trenes\n";
        std::cout << "  2. Insertar tren\n";
        std::cout << "  3. Modificar tren\n";
        std::cout << "  4. Eliminar tren\n";
        std::cout << "  0. Volver\n";
        std::cout << "  Opción: ";
        std::cin >> op;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (op == 1) {
            conn.enviar("LISTAR_TRENES");
            mostrarLista(conn.recibirLista());
        } else if (op == 2) {
            std::string modelo  = pedirLinea("  Modelo        : ");
            std::string serie   = pedirLinea("  Num. serie    : ");
            std::string anio    = pedirLinea("  Año fabricac. : ");
            std::string estado  = pedirLinea("  Estado (OPERATIVO/MANTENIMIENTO): ");
            std::string frev    = pedirLinea("  Fecha últ. revisión (AAAA-MM-DD): ");
            conn.enviar("INSERTAR_TREN|"+modelo+"|"+serie+"|"+anio+"|"+estado+"|"+frev);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 3) {
            std::string id_t   = pedirLinea("  ID tren       : ");
            std::string modelo = pedirLinea("  Nuevo modelo  : ");
            std::string serie  = pedirLinea("  Nueva serie   : ");
            std::string anio   = pedirLinea("  Nuevo año     : ");
            std::string estado = pedirLinea("  Nuevo estado  : ");
            std::string frev   = pedirLinea("  Nueva rev.    : ");
            conn.enviar("MODIFICAR_TREN|"+id_t+"|"+modelo+"|"+serie+"|"+anio+"|"+estado+"|"+frev);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 4) {
            std::string id_t = pedirLinea("  ID tren a eliminar: ");
            conn.enviar("ELIMINAR_TREN|"+id_t);
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

/* ══════════════════════════════════════════════
   GESTIÓN DE ESTACIONES
   ══════════════════════════════════════════════ */

void Administrador::menuGestionEstaciones() {
    int op = 0;
    do {
        std::cout << "\n  -- Estaciones --\n";
        std::cout << "  1. Listar estaciones\n";
        std::cout << "  2. Insertar estación\n";
        std::cout << "  3. Modificar estación\n";
        std::cout << "  0. Volver\n";
        std::cout << "  Opción: ";
        std::cin >> op;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (op == 1) {
            conn.enviar("LISTAR_ESTACIONES");
            mostrarLista(conn.recibirLista());
        } else if (op == 2) {
            std::string nombre = pedirLinea("  Nombre     : ");
            std::string ciudad = pedirLinea("  Ciudad     : ");
            std::string prov   = pedirLinea("  Provincia  : ");
            std::string and_   = pedirLinea("  Andenes    : ");
            conn.enviar("INSERTAR_ESTACION|"+nombre+"|"+ciudad+"|"+prov+"|"+and_);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 3) {
            std::string id   = pedirLinea("  ID estación : ");
            std::string nom  = pedirLinea("  Nuevo nombre: ");
            std::string ciu  = pedirLinea("  Nueva ciudad: ");
            std::string prov = pedirLinea("  Nueva prov. : ");
            std::string and_ = pedirLinea("  Andenes     : ");
            conn.enviar("MODIFICAR_ESTACION|"+id+"|"+nom+"|"+ciu+"|"+prov+"|"+and_);
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

/* ══════════════════════════════════════════════
   GESTIÓN DE TRAYECTOS
   ══════════════════════════════════════════════ */

void Administrador::menuGestionTrayectos() {
    int op = 0;
    do {
        std::cout << "\n  -- Trayectos --\n";
        std::cout << "  1. Listar trayectos\n";
        std::cout << "  2. Insertar trayecto\n";
        std::cout << "  3. Modificar trayecto\n";
        std::cout << "  4. Cambiar estado trayecto\n";
        std::cout << "  5. Modificar precio base\n";
        std::cout << "  0. Volver\n";
        std::cout << "  Opción: ";
        std::cin >> op;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (op == 1) {
            conn.enviar("LISTAR_TRAYECTOS");
            mostrarLista(conn.recibirLista());
        } else if (op == 2) {
            std::string id_t   = pedirLinea("  ID tren       : ");
            std::string orig   = pedirLinea("  ID est. origen : ");
            std::string dest   = pedirLinea("  ID est. destino: ");
            std::string h_sal  = pedirLinea("  Hora salida (HH:MM): ");
            std::string h_ll   = pedirLinea("  Hora llegada  : ");
            std::string dur    = pedirLinea("  Duración (min): ");
            std::string precio = pedirLinea("  Precio base   : ");
            std::string dias   = pedirLinea("  Días (LMXJVSD): ");
            conn.enviar("INSERTAR_TRAYECTO|"+id_t+"|"+orig+"|"+dest+"|"+h_sal+"|"+h_ll+"|"+dur+"|"+precio+"|"+dias);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 3) {
            std::string id_tr  = pedirLinea("  ID trayecto   : ");
            std::string h_sal  = pedirLinea("  Nueva h. salida: ");
            std::string h_ll   = pedirLinea("  Nueva h. llegada: ");
            std::string precio = pedirLinea("  Nuevo precio  : ");
            std::string dias   = pedirLinea("  Nuevos días   : ");
            conn.enviar("MODIFICAR_TRAYECTO|"+id_tr+"|"+h_sal+"|"+h_ll+"|"+precio+"|"+dias);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 4) {
            std::string id_tr  = pedirLinea("  ID trayecto: ");
            std::string estado = pedirLinea("  Estado (ACTIVO/SUSPENDIDO/ELIMINADO): ");
            conn.enviar("ESTADO_TRAYECTO|"+id_tr+"|"+estado);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 5) {
            std::string id_tr  = pedirLinea("  ID trayecto: ");
            std::string precio = pedirLinea("  Nuevo precio base: ");
            conn.enviar("MOD_PRECIO_BASE|"+id_tr+"|"+precio);
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

/* ══════════════════════════════════════════════
   GESTIÓN DE USUARIOS
   ══════════════════════════════════════════════ */

void Administrador::menuGestionUsuarios() {
    int op = 0;
    do {
        std::cout << "\n  -- Usuarios y Empleados --\n";
        std::cout << "  1. Listar todos los usuarios\n";
        std::cout << "  2. Listar empleados\n";
        std::cout << "  3. Habilitar / deshabilitar usuario\n";
        std::cout << "  0. Volver\n";
        std::cout << "  Opción: ";
        std::cin >> op;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (op == 1) {
            conn.enviar("LISTAR_USUARIOS");
            mostrarLista(conn.recibirLista());
        } else if (op == 2) {
            conn.enviar("LISTAR_EMPLEADOS");
            mostrarLista(conn.recibirLista());
        } else if (op == 3) {
            std::string id = pedirLinea("  ID usuario: ");
            conn.enviar("DESHABILITAR_USER|"+id);
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

/* ══════════════════════════════════════════════
   GESTIÓN DE SERVICIOS
   ══════════════════════════════════════════════ */

void Administrador::menuGestionServicios() {
    int op = 0;
    do {
        std::cout << "\n  -- Servicios --\n";
        std::cout << "  1. Listar servicios (todos)\n";
        std::cout << "  2. Listar servicios por fecha\n";
        std::cout << "  3. Crear servicio\n";
        std::cout << "  4. Cancelar servicio\n";
        std::cout << "  0. Volver\n";
        std::cout << "  Opción: ";
        std::cin >> op;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (op == 1) {
            conn.enviar("LISTAR_SERVICIOS|");
            mostrarLista(conn.recibirLista());
        } else if (op == 2) {
            std::string fecha = pedirLinea("  Fecha (AAAA-MM-DD): ");
            conn.enviar("LISTAR_SERVICIOS|"+fecha);
            mostrarLista(conn.recibirLista());
        } else if (op == 3) {
            std::string id_tr = pedirLinea("  ID trayecto: ");
            std::string fecha = pedirLinea("  Fecha (AAAA-MM-DD): ");
            conn.enviar("INSERTAR_SERVICIO|"+id_tr+"|"+fecha);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 4) {
            std::string id = pedirLinea("  ID servicio a cancelar: ");
            conn.enviar("CANCELAR_SERVICIO|"+id);
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

/* ══════════════════════════════════════════════
   GESTIÓN DE INCIDENCIAS
   ══════════════════════════════════════════════ */

void Administrador::menuGestionIncidencias() {
    int op = 0;
    do {
        std::cout << "\n  -- Incidencias --\n";
        std::cout << "  1. Listar abiertas\n";
        std::cout << "  2. Listar todas\n";
        std::cout << "  3. Insertar incidencia\n";
        std::cout << "  4. Resolver incidencia\n";
        std::cout << "  0. Volver\n";
        std::cout << "  Opción: ";
        std::cin >> op;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (op == 1) {
            conn.enviar("LISTAR_INCIDENCIAS|ABIERTA");
            mostrarLista(conn.recibirLista());
        } else if (op == 2) {
            conn.enviar("LISTAR_INCIDENCIAS|TODAS");
            mostrarLista(conn.recibirLista());
        } else if (op == 3) {
            std::string id_serv = pedirLinea("  ID servicio   : ");
            std::string tipo    = pedirLinea("  Tipo          : ");
            std::string desc    = pedirLinea("  Descripción   : ");
            std::string prior   = pedirLinea("  Prioridad (ALTA/MEDIA/BAJA): ");
            conn.enviar("INSERTAR_INCIDENCIA|"+id_serv+"|"+tipo+"|"+desc+"|"+prior);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 4) {
            std::string id = pedirLinea("  ID incidencia: ");
            conn.enviar("RESOLVER_INCIDENCIA|"+id+"|"+std::to_string(id_u));
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

/* ══════════════════════════════════════════════
   INFORMES
   ══════════════════════════════════════════════ */

void Administrador::menuInformes() {
    int op = 0;
    do {
        std::cout << "\n  -- Informes --\n";
        std::cout << "  1. Ocupación por tren\n";
        std::cout << "  2. Ingresos por trayecto\n";
        std::cout << "  3. Incidencias por período\n";
        std::cout << "  0. Volver\n";
        std::cout << "  Opción: ";
        std::cin >> op;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (op == 1) {
            std::string id_t = pedirLinea("  ID tren: ");
            conn.enviar("INFORME_OCUPACION|"+id_t);
            mostrarLista(conn.recibirLista());
        } else if (op == 2) {
            std::string id_tr = pedirLinea("  ID trayecto: ");
            conn.enviar("INFORME_INGRESOS|"+id_tr);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 3) {
            std::string f_ini = pedirLinea("  Fecha inicio (AAAA-MM-DD): ");
            std::string f_fin = pedirLinea("  Fecha fin    (AAAA-MM-DD): ");
            conn.enviar("INFORME_INCIDENCIAS|"+f_ini+"|"+f_fin);
            mostrarLista(conn.recibirLista());
        }
    } while (op != 0);
}

/* ══════════════════════════════════════════════
   LOGS
   ══════════════════════════════════════════════ */

void Administrador::menuLogs() {
    std::cout << "\n  Filtro de fecha (AAAA-MM-DD, Enter=todos): ";
    std::string fecha;
    std::getline(std::cin, fecha);
    std::cout << "  Filtro de usuario (Enter=todos): ";
    std::string usr;
    std::getline(std::cin, usr);

    conn.enviar("VER_LOGS|"+fecha+"|"+usr);
    auto lista = conn.recibirLista();
    std::cout << "\n";
    mostrarLista(lista);
    std::cout << "\n  (" << lista.size() << " entradas)\n";
}

