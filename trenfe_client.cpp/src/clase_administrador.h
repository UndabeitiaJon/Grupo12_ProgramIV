/*
 * clase_administrador.h
 *
 *  Created on: 9 may 2026
 *      Author: e.aranoa
 */

#ifndef SRC_CLASE_ADMINISTRADOR_H_
#define SRC_CLASE_ADMINISTRADOR_H_


#include <string>
#include "usuario_base.h"

class Administrador : public UsuarioBase {
private:
    void menuGestionTrenes();
    void menuGestionEstaciones();
    void menuGestionTrayectos();
    void menuGestionUsuarios();
    void menuGestionServicios();
    void menuGestionIncidencias();
    void menuInformes();
    void menuLogs();

    void mostrarLista(const std::vector<std::string>& lista);

public:
    Administrador(int id, const std::string& nombre, const std::string& apellido,
                  const std::string& email, Conexion& conn);

    void mostrarMenuPrincipal() override;
};


#endif /* SRC_CLASE_ADMINISTRADOR_H_ */
