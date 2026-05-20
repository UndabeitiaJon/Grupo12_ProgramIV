/*
 * usuario_base.h
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */

#ifndef USUARIO_BASE_H_
#define USUARIO_BASE_H_

#include <string>
#include "conexion.h"

class UsuarioBase {
protected:
    int         id_u;
    std::string nombre;
    std::string apellido;
    std::string email;
    std::string rol;        //"PASAJERO", "MAQUINISTA", "ADMIN"
    Conexion&   conn;

public:
    UsuarioBase(int id, const std::string& nombre, const std::string& apellido,
                const std::string& email, const std::string& rol, Conexion& conn);
    virtual ~UsuarioBase() = default;

    // Cada rol implementa su propio menú principal
    virtual void mostrarMenuPrincipal() = 0;
    void mostrarMisDatos();
    void cambiarContrasenia();

    // Getters
    int                getId()      const { return id_u;    }
    const std::string& getEmail()   const { return email;   }
    const std::string& getRol()     const { return rol;     }
    const std::string& getNombre()  const { return nombre;  }
};


// Divide una cadena por '|' y devuelve el campo en la posición indicada
std::string campo(const std::string& linea, int pos);

void mostrarCabecera(const std::string& nombre, const std::string& rol);

std::string seleccionarEstacion(const std::vector<std::string>& cacheEstaciones,
                                const std::string& etiqueta);

#endif /* USUARIO_BASE_H_ */
