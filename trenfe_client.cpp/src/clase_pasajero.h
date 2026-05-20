/*
 * clase_pasajero.h
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */

<<<<<<< HEAD
/*
 */
=======

>>>>>>> branch 'main' of git@github.com:UndabeitiaJon/Grupo12_ProgramIV.git

#ifndef CLASE_PASAJERO_H_
#define CLASE_PASAJERO_H_

#include <vector>
#include <string>
#include "usuario_base.h"

class Pasajero : public UsuarioBase {
private:
    // Evita peticiones repetidas al servidor
    std::vector<std::string> cacheEstaciones;
    std::vector<std::string> cacheTrayectos;
    bool estacionesCargadas = false;
    bool trayectosCargados  = false;

    // Sub-menús
    void menuBuscarTrayecto();
    void menuHacerReserva(const std::string& id_tr,
                          const std::string& orig_nombre,
                          const std::string& dest_nombre,
                          const std::string& fecha,
                          const std::string& clase);
    void menuMisReservas();

    // Helpers de presentación
    void mostrarTrayectos(const std::vector<std::string>& lista);
    void mostrarReservas (const std::vector<std::string>& lista);
    void cargarEstaciones();

public:
    Pasajero(int id, const std::string& nombre, const std::string& apellido,
             const std::string& email, Conexion& conn);

    void mostrarMenuPrincipal() override;
};

#endif /* CLASE_PASAJERO_H_ */
