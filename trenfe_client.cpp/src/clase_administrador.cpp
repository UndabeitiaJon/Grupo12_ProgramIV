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
#include <algorithm>
#include <limits>
#include <sstream>
#include <vector>
#include <string>
#include "clase_administrador.h"
#include "usuario_base.h"   /* seleccionarEstacion() */

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

/* Si el usuario pulsa ENTER sin escribir nada, devuelve defaultVal */
static std::string pedirLineaConDefault(const std::string& label, const std::string& defaultVal) {
    std::string v;
    std::cout << "  " << label << " [" << defaultVal << "] (ENTER=mantener): ";
    std::getline(std::cin, v);
    if (v.empty()) return defaultVal;
    return v;
}

//MENÚ PRINCIPAL

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
            case 1:
            	menuGestionTrenes();
            break;
            case 2:
            	menuGestionEstaciones();
            break;
            case 3:
            	menuGestionTrayectos();
            break;
            case 4:
            	menuGestionUsuarios();
            break;
            case 5:
            	menuGestionServicios();
            break;
            case 6:
            	menuGestionIncidencias();
            break;
            case 7:
            	menuInformes();
            break;
            case 8:
            	menuLogs();
            break;
            case 9:
            	mostrarMisDatos();
            break;
            case 10:
            	cambiarContrasenia();
            break;
            case 0:
                conn.enviar("LOGOUT");
                conn.recibir();
                break;
            default: std::cout << "  Opción no válida.\n";
        }
    } while (op != 0);
}

//GESTIÓN DE TRENES


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
            // Listar los trenes disponibles
            conn.enviar("LISTAR_TRENES");
            std::vector<std::string> lista = conn.recibirLista();
            mostrarLista(lista);

            std::string id_t = pedirLinea("\n  ID tren a modificar: ");

            // Buscar datos actuales del tren en la lista
            std::string cur_modelo, cur_serie, cur_anio, cur_estado, cur_frev;
            for (const auto& fila : lista) {
                // Parsear campos separados por '|'
                std::vector<std::string> campos;
                std::stringstream ss(fila);
                std::string token;
                while (std::getline(ss, token, '|')) campos.push_back(token);
                if (campos.size() >= 7 && campos[1] == id_t) {
                    cur_modelo = campos[2];
                    cur_serie  = campos[3];
                    cur_anio   = campos[4];
                    cur_estado = campos[5];
                    cur_frev   = campos[6];
                    break;
                }
            }

            std::cout << "\n  ══ MODIFICAR TREN  (ENTER = mantener valor actual) ══\n";
            std::string modelo = pedirLineaConDefault("Modelo      ", cur_modelo);
            std::string serie  = pedirLineaConDefault("Num. serie  ", cur_serie);
            std::string anio   = pedirLineaConDefault("Año         ", cur_anio);
            std::string estado = pedirLineaConDefault("Estado (0=Operativo 1=Revision 2=Averia 3=Retirado)", cur_estado);
            std::string frev   = pedirLineaConDefault("Fecha rev.  (AAAA-MM-DD)", cur_frev);

            conn.enviar("MODIFICAR_TREN|"+id_t+"|"+modelo+"|"+serie+"|"+anio+"|"+estado+"|"+frev);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 4) {
            conn.enviar("LISTAR_TRENES");
            mostrarLista(conn.recibirLista());
            std::string id_t = pedirLinea("\n  ID tren a eliminar: ");
            conn.enviar("ELIMINAR_TREN|"+id_t);
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

// GESTIÓN DE ESTACIONES

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
            auto lista = conn.recibirLista();
            // Ordenar por nombre
            std::sort(lista.begin(), lista.end(), [](const std::string& a, const std::string& b){
                auto ca = a; auto cb = b;
                size_t p1a = ca.find('|'); if (p1a!=std::string::npos) p1a=ca.find('|',p1a+1);
                size_t p1b = cb.find('|'); if (p1b!=std::string::npos) p1b=cb.find('|',p1b+1);
                std::string na = (p1a!=std::string::npos) ? ca.substr(p1a+1,ca.find('|',p1a+1)-p1a-1) : ca;
                std::string nb = (p1b!=std::string::npos) ? cb.substr(p1b+1,cb.find('|',p1b+1)-p1b-1) : cb;
                return na < nb;
            });
            mostrarLista(lista);
        } else if (op == 2) {
            std::string nombre = pedirLinea("  Nombre     : ");
            std::string ciudad = pedirLinea("  Ciudad     : ");
            std::string prov   = pedirLinea("  Provincia  : ");
            std::string and_   = pedirLinea("  Andenes    : ");
            conn.enviar("INSERTAR_ESTACION|"+nombre+"|"+ciudad+"|"+prov+"|"+and_);
            std::cout << "  " << conn.recibir() << "\n";
        } else if (op == 3) {
            // Mostrar lista ordenada por ID antes de pedir el ID
            conn.enviar("LISTAR_ESTACIONES");
            std::vector<std::string> lista = conn.recibirLista();

            // Ordenar por ID
            std::sort(lista.begin(), lista.end(), [](const std::string& a, const std::string& b){
                int ia = 0, ib = 0;
                size_t p = a.find('|');
                if (p!=std::string::npos) { try { ia=std::stoi(a.substr(p+1)); } catch(...){} }
                p = b.find('|');
                if (p!=std::string::npos) { try { ib=std::stoi(b.substr(p+1)); } catch(...){} }
                return ia < ib;
            });

            std::cout << "\n  " << std::left
                      << std::setw(5)  << "ID"
                      << std::setw(30) << "NOMBRE"
                      << std::setw(20) << "CIUDAD"
                      << "PROVINCIA\n";
            std::cout << "  " << std::string(72, '-') << "\n";
            for (const auto& e : lista) {
                if (campo(e,0) != "ESTACION") continue;
                std::cout << "  " << std::left
                          << std::setw(5)  << campo(e,1)
                          << std::setw(30) << campo(e,2)
                          << std::setw(20) << campo(e,3)
                          << campo(e,4) << "\n";
            }
            std::cout << "\n";

            std::string id = pedirLinea("  ID estación a modificar (0=cancelar): ");
            if (id == "0" || id.empty()){
            	continue;
            }

            // Buscar datos actuales parseando con stringstream
            std::string cur_nom, cur_ciu, cur_prov, cur_and;
            for (const auto& e : lista) {
                std::vector<std::string> campos;
                std::stringstream ss(e);
                std::string token;
                while (std::getline(ss, token, '|')) campos.push_back(token);
                if (campos.size() >= 6 && campos[0] == "ESTACION" && campos[1] == id) {
                    cur_nom  = campos[2];
                    cur_ciu  = campos[3];
                    cur_prov = campos[4];
                    cur_and  = campos[5];
                    break;
                }
            }

            std::cout << "\n  ══ MODIFICAR ESTACIÓN  (ENTER = mantener valor actual) ══\n";
            std::string nom  = pedirLineaConDefault("Nombre    ", cur_nom);
            std::string ciu  = pedirLineaConDefault("Ciudad    ", cur_ciu);
            std::string prov = pedirLineaConDefault("Provincia ", cur_prov);
            std::string and_ = pedirLineaConDefault("Andenes   ", cur_and);
            conn.enviar("MODIFICAR_ESTACION|"+id+"|"+nom+"|"+ciu+"|"+prov+"|"+and_);
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

//GESTIÓN DE TRAYECTOS


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
            // Cargar estaciones y elegir origen/destino
            conn.enviar("LISTAR_ESTACIONES");
            auto estaciones = conn.recibirLista();

            std::cout << "\n  Selecciona estación ORIGEN:\n";
            std::string orig = seleccionarEstacion(estaciones, "origen");
            if (orig.empty()) {
            	std::cout << "  Cancelado.\n";
            	continue;
            }

            std::cout << "\n  Selecciona estación DESTINO:\n";
            std::string dest = seleccionarEstacion(estaciones, "destino");
            if (dest.empty()) {
            	std::cout << "  Cancelado.\n";
            	continue;
            }
            std::string h_sal  = pedirLinea("  Hora salida  (HH:MM)  : ");
            std::string h_ll   = pedirLinea("  Hora llegada (HH:MM)  : ");
            std::string dur    = pedirLinea("  Duración (min)        : ");
            std::string precio = pedirLinea("  Precio base           : ");
            std::string dias   = pedirLinea("  Días operación (LMXJVSD): ");

            /* ── 3. Listar trenes justo antes de pedir ID tren ───── */
            conn.enviar("LISTAR_TRENES");
            auto trenes = conn.recibirLista();
            if (trenes.empty()) {
                std::cout << "  (sin trenes — inserta un tren primero)\n"; continue;
            }
            std::cout << "\n  -- Trenes disponibles --\n";
            std::cout << "  " << std::left
                      << std::setw(5)  << "ID"
                      << std::setw(22) << "Modelo"
                      << std::setw(16) << "N.Serie"
                      << std::setw(6)  << "Año"
                      << "Estado\n";
            std::cout << "  " << std::string(62, '-') << "\n";
            for (const auto& t : trenes) {
                if (campo(t,0) != "TREN") continue;
                std::cout << "  " << std::setw(5)  << campo(t,1)
                          << std::setw(22) << campo(t,2)
                          << std::setw(16) << campo(t,3)
                          << std::setw(6)  << campo(t,4)
                          << campo(t,5) << "\n";
            }
            std::string id_t = pedirLinea("\n  ID tren: ");
            conn.enviar("INSERTAR_TRAYECTO|"+id_t+"|"+orig+"|"+dest+"|"+h_sal+"|"+h_ll+"|"+dur+"|"+precio+"|"+dias);
            std::cout << "  " << conn.recibir() << "\n";

        } else if (op == 3) {
            // Mostrar lista de trayectos
            conn.enviar("LISTAR_TRAYECTOS");
            std::vector<std::string> lista = conn.recibirLista();
            mostrarLista(lista);

            std::string id_tr = pedirLinea("\n  ID trayecto a modificar (0=cancelar): ");
            if (id_tr == "0" || id_tr.empty()) continue;

            // Buscar datos actuales
            std::string cur_hsal, cur_hll, cur_precio, cur_dias;
            for (const auto& t : lista) {
                std::vector<std::string> campos;
                std::stringstream ss(t);
                std::string token;
                while (std::getline(ss, token, '|')) campos.push_back(token);
                if (campos.size() >= 7 && campos[0] == "TRAYECTO" && campos[1] == id_tr) {
                    cur_hsal   = campos[4];   /* hora_salida  */
                    cur_hll    = campos[5];   /* hora_llegada */
                    cur_precio = campos[6];   /* precio_base  */
                    break;
                }
            }
            conn.enviar("DETALLE_TRAYECTO|" + id_tr);
            auto detalle = conn.recibirLista();

            for (const auto& d : detalle) {
                std::vector<std::string> dc;
                std::stringstream ss2(d);
                std::string tok;
                while (std::getline(ss2, tok, '|')) dc.push_back(tok);
                if (dc.size() >= 9 && dc[0] == "DETALLE") {
                    cur_dias = dc[8];
                    break;
                }
            }

            std::cout << "\n  ══ MODIFICAR TRAYECTO  (ENTER = mantener valor actual) ══\n";
            std::string h_sal  = pedirLineaConDefault("Hora salida  (HH:MM)", cur_hsal);
            std::string h_ll   = pedirLineaConDefault("Hora llegada (HH:MM)", cur_hll);
            std::string precio = pedirLineaConDefault("Precio base          ", cur_precio);
            std::string dias   = pedirLineaConDefault("Días operación       ", cur_dias);
            conn.enviar("MODIFICAR_TRAYECTO|"+id_tr+"|"+h_sal+"|"+h_ll+"|"+precio+"|"+dias);
            std::cout << "  " << conn.recibir() << "\n";

        } else if (op == 4) {
            conn.enviar("LISTAR_TRAYECTOS");
            mostrarLista(conn.recibirLista());

            std::string id_tr  = pedirLinea("\n  ID trayecto (0=cancelar): ");
            if (id_tr == "0" || id_tr.empty()){
            	continue;
            }
            std::string estado = pedirLinea("  Estado (ACTIVO/SUSPENDIDO/ELIMINADO): ");
            conn.enviar("ESTADO_TRAYECTO|"+id_tr+"|"+estado);
            std::cout << "  " << conn.recibir() << "\n";

        } else if (op == 5) {
            // Mostrar lista antes de pedir ID
            conn.enviar("LISTAR_TRAYECTOS");
            mostrarLista(conn.recibirLista());

            std::string id_tr  = pedirLinea("\n  ID trayecto (0=cancelar): ");
            if (id_tr == "0" || id_tr.empty()) {
            	continue;
            }
            std::string precio = pedirLinea("  Nuevo precio base: ");
            conn.enviar("MOD_PRECIO_BASE|"+id_tr+"|"+precio);
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

//GESTIÓN DE USUARIOS


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
            conn.enviar("LISTAR_USUARIOS");
            std::vector<std::string> lista = conn.recibirLista();
            std::cout << "\n  " << std::left
                      << std::setw(5)  << "ID"
                      << std::setw(16) << "NOMBRE"
                      << std::setw(16) << "APELLIDO"
                      << std::setw(26) << "EMAIL"
                      << std::setw(12) << "ROL"
                      << "ACTIVO\n";
            std::cout << "  " << std::string(78, '-') << "\n";
            for (const auto& u : lista) {
                if (campo(u,0) != "USUARIO") {
                	continue;
                }
                std::string activo_txt = (campo(u,6) == "1") ? "Sí" : "NO";
                std::cout << "  " << std::left
                          << std::setw(5)  << campo(u,1)
                          << std::setw(16) << campo(u,2)
                          << std::setw(16) << campo(u,3)
                          << std::setw(26) << campo(u,4)
                          << std::setw(12) << campo(u,5)
                          << activo_txt << "\n";
            }
            std::cout << "\n";

            std::string id = pedirLinea("  ID usuario a habilitar/deshabilitar (0=cancelar): ");
            if (id == "0" || id.empty()) {
            	continue;
            }
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
            // Mostrar trayectos disponibles para elegir ID
            conn.enviar("LISTAR_TRAYECTOS");
            auto trays = conn.recibirLista();
            std::cout << "\n  " << std::left
                      << std::setw(6)  << "ID"
                      << std::setw(26) << "ORIGEN"
                      << std::setw(26) << "DESTINO"
                      << std::setw(10) << "SALIDA"
                      << "LLEGADA\n";
            std::cout << "  " << std::string(74, '-') << "\n";
            for (const auto& t : trays) {
                if (campo(t,0) != "TRAYECTO") continue;
                std::cout << "  " << std::left
                          << std::setw(6)  << campo(t,1)
                          << std::setw(26) << campo(t,2)
                          << std::setw(26) << campo(t,3)
                          << std::setw(10) << campo(t,4)
                          << campo(t,5) << "\n";
            }
            std::cout << "\n";
            std::string id_tr = pedirLinea("  ID trayecto (0=cancelar): ");
            if (id_tr == "0" || id_tr.empty()) continue;
            std::string fecha = pedirLinea("  Fecha (AAAA-MM-DD): ");
            conn.enviar("INSERTAR_SERVICIO|"+id_tr+"|"+fecha);
            std::string resp_serv = conn.recibir();
            std::cout << "  " << resp_serv << "\n";

            // Si el servicio se ha creado, ofrecer asignar empleado
            if (campo(resp_serv, 0) == "OK") {
                std::string id_serv_nuevo = campo(resp_serv, 1);

                conn.enviar("LISTAR_EMPLEADOS");
                auto emps = conn.recibirLista();

                if (emps.empty()) {
                    std::cout << "  (No hay empleados registrados)\n";
                } else {
                    std::cout << "\n  -- Empleados disponibles --\n";
                    std::cout << "  " << std::left
                              << std::setw(5)  << "ID"
                              << std::setw(16) << "NOMBRE"
                              << std::setw(16) << "APELLIDO"
                              << std::setw(26) << "EMAIL"
                              << "ROL\n";
                    std::cout << "  " << std::string(70, '-') << "\n";
                    for (const auto& e : emps) {
                        if (campo(e,0) != "EMPLEADO") continue;
                        std::cout << "  " << std::left
                                  << std::setw(5)  << campo(e,1)
                                  << std::setw(16) << campo(e,2)
                                  << std::setw(16) << campo(e,3)
                                  << std::setw(26) << campo(e,4)
                                  << campo(e,5) << "\n";
                    }
                    std::cout << "\n";
                    std::string id_emp = pedirLinea("  ID empleado a asignar (0=omitir): ");
                    if (id_emp != "0" && !id_emp.empty()) {
                        std::string rol_serv = pedirLinea("  Rol en el servicio (MAQUINISTA/REVISOR/JEFE_TREN): ");
                        conn.enviar("ASIGNAR_EMPLEADO|"+id_serv_nuevo+"|"+id_emp+"|"+rol_serv);
                        std::string resp_asig = conn.recibir();
                        if (campo(resp_asig,0) == "OK")
                            std::cout << "  Empleado asignado al servicio " << id_serv_nuevo << ".\n";
                        else
                            std::cout << "  " << resp_asig << "\n";
                    }
                }
            }
        } else if (op == 4) {
            // Mostrar servicios antes de pedir cuál cancelar
            conn.enviar("LISTAR_SERVICIOS|");
            auto servs = conn.recibirLista();
            std::cout << "\n  " << std::left
                      << std::setw(6)  << "ID"
                      << std::setw(12) << "FECHA"
                      << std::setw(26) << "ORIGEN"
                      << std::setw(26) << "DESTINO"
                      << "ESTADO\n";
            std::cout << "  " << std::string(76, '-') << "\n";
            for (const auto& s : servs) {
                if (campo(s,0) != "SERVICIO") {
                	continue;
                }
                std::cout << "  " << std::left
                          << std::setw(6)  << campo(s,1)
                          << std::setw(12) << campo(s,2)
                          << std::setw(26) << campo(s,4)
                          << std::setw(26) << campo(s,5)
                          << campo(s,8) << "\n";
            }
            std::cout << "\n";
            std::string id = pedirLinea("  ID servicio a cancelar (0=cancelar): ");
            if (id == "0" || id.empty()){
            	continue;
            }
            conn.enviar("CANCELAR_SERVICIO|"+id);
            std::cout << "  " << conn.recibir() << "\n";
        }
    } while (op != 0);
}

//GESTIÓN DE INCIDENCIAS


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
            // Mostrar servicios disponibles para elegir ID
            conn.enviar("LISTAR_SERVICIOS|");
            auto servs = conn.recibirLista();
            std::cout << "\n  " << std::left
                      << std::setw(6)  << "ID"
                      << std::setw(12) << "FECHA"
                      << std::setw(26) << "ORIGEN"
                      << std::setw(26) << "DESTINO"
                      << "ESTADO\n";
            std::cout << "  " << std::string(76, '-') << "\n";
            for (const auto& s : servs) {
                if (campo(s,0) != "SERVICIO") {
                	continue;
                }
                std::cout << "  " << std::left
                          << std::setw(6)  << campo(s,1)
                          << std::setw(12) << campo(s,2)
                          << std::setw(26) << campo(s,4)
                          << std::setw(26) << campo(s,5)
                          << campo(s,8) << "\n";
            }
            std::cout << "\n";
            std::string id_serv = pedirLinea("  ID servicio (0=cancelar): ");
            if (id_serv == "0" || id_serv.empty()) {
            	continue;
            }
            std::string tipo    = pedirLinea("  Tipo (AVERIA/RETRASO/ACCIDENTE/OTRO): ");
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

//INFORMES


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
            conn.enviar("LISTAR_TRENES");
            auto trenes = conn.recibirLista();
            std::cout << "\n  " << std::left
                      << std::setw(5) << "ID" << std::setw(24) << "Modelo"
                      << std::setw(16) << "N.Serie" << "Estado\n";
            std::cout << "  " << std::string(58, '-') << "\n";
            for (const auto& t : trenes)
                if (campo(t,0) == "TREN")
                    std::cout << "  " << std::left
                              << std::setw(5)  << campo(t,1)
                              << std::setw(24) << campo(t,2)
                              << std::setw(16) << campo(t,3)
                              << campo(t,5) << "\n";

            std::string id_t = pedirLinea("\n  ID tren (0=cancelar): ");
            if (id_t == "0" || id_t.empty()) {
            	continue;
            }
            conn.enviar("INFORME_OCUPACION|"+id_t);
            auto lista = conn.recibirLista();

            std::cout << "\n  ============================================================\n";
            std::cout << "  INFORME DE OCUPACIÓN — TREN " << id_t << "\n";
            std::cout << "  ============================================================\n";
            if (lista.empty()) {
                std::cout << "  Sin datos para este tren.\n";
            } else {
                std::cout << "  " << std::left
                          << std::setw(6)  << "ID_TR"
                          << std::setw(28) << "ORIGEN"
                          << std::setw(28) << "DESTINO"
                          << std::setw(10) << "RESERVAS"
                          << "OCUP%\n";
                std::cout << "  " << std::string(78, '-') << "\n";
                /* OCUPACION|id_tr|origen|destino|reservas|pct */
                for (const auto& linea : lista) {
                    if (campo(linea,0) != "OCUPACION") {
                    	continue;
                    }
                    std::cout << "  " << std::left
                              << std::setw(6)  << campo(linea,1)
                              << std::setw(28) << campo(linea,2)
                              << std::setw(28) << campo(linea,3)
                              << std::setw(10) << campo(linea,4)
                              << campo(linea,5) << "%\n";
                }
            }

        } else if (op == 2) {
            // Mostrar trayectos antes de pedir ID
            conn.enviar("LISTAR_TRAYECTOS");
            auto trays = conn.recibirLista();
            std::cout << "\n  " << std::left
                      << std::setw(6)  << "ID"
                      << std::setw(28) << "ORIGEN"
                      << std::setw(28) << "DESTINO"
                      << std::setw(10) << "SALIDA"
                      << "LLEGADA\n";
            std::cout << "  " << std::string(78, '-') << "\n";
            for (const auto& t : trays) {
                if (campo(t,0) != "TRAYECTO") {
                	continue;
                }
                std::cout << "  " << std::left
                          << std::setw(6)  << campo(t,1)
                          << std::setw(28) << campo(t,2)
                          << std::setw(28) << campo(t,3)
                          << std::setw(10) << campo(t,4)
                          << campo(t,5) << "\n";
            }
            std::string id_tr = pedirLinea("\n  ID trayecto (0=cancelar): ");
            if (id_tr == "0" || id_tr.empty()) {
            	continue;
            }
            conn.enviar("INFORME_INGRESOS|"+id_tr);
            std::string resp = conn.recibir();
            if (campo(resp,0) == "INGRESOS") {
                std::cout << "\n  ============================================================\n";
                std::cout << "  INFORME DE INGRESOS — TRAYECTO " << campo(resp,1) << "\n";
                std::cout << "  ============================================================\n";
                std::cout << "  Ruta        : " << campo(resp,2) << " → " << campo(resp,3) << "\n";
                std::cout << "  Horario     : " << campo(resp,4) << " → " << campo(resp,5) << "\n";
                std::cout << "  Precio base : " << campo(resp,6) << " EUR\n";
                std::cout << "  Reservas    : " << campo(resp,7) << "\n";
                std::cout << "  Total       : " << campo(resp,8) << " EUR\n";
                std::cout << "  ============================================================\n";
            } else {
                std::cout << "  " << resp << "\n";
            }
        } else if (op == 3) {
            std::string f_ini = pedirLinea("  Fecha inicio (AAAA-MM-DD): ");
            std::string f_fin = pedirLinea("  Fecha fin    (AAAA-MM-DD): ");
            conn.enviar("INFORME_INCIDENCIAS|"+f_ini+"|"+f_fin);
            mostrarLista(conn.recibirLista());
        }
    } while (op != 0);
}

//LOGS


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
