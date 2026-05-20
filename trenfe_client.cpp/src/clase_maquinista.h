/*
 * clase_maquinista.h
 *
 *  Created on: 9 may 2026
 *      Author: e.aranoa
 */

#ifndef CLASE_MAQUINISTA_H_
#define CLASE_MAQUINISTA_H_
#include <vector>
#include <string>
#include "usuario_base.h"

class Maquinista : public UsuarioBase {
private:
    /* Caché del cuadrante — se pide una sola vez por sesión */
    std::vector<std::string> cacheCuadrante;
    bool cuadranteCargado = false;

    void menuCuadrante();
    void menuReportarRetraso(const std::string& id_serv);
    void menuMarcarInicio(const std::string& id_serv);
    void menuMarcarFin(const std::string& id_serv);

    void mostrarCuadrante(const std::vector<std::string>& lista);

public:
    Maquinista(int id, const std::string& nombre, const std::string& apellido,
               const std::string& email, Conexion& conn);

    void mostrarMenuPrincipal() override;
};




#endif /* CLASE_MAQUINISTA_H_ */
