/*
 * usuario_base.h
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */
/*
 * usuario_base.h  -  Sistema TRENFE  -  Fase 2
 *
 * Clase abstracta de la que heredan Pasajero, Maquinista y Administrador.
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
    std::string rol;        /* "PASAJERO", "MAQUINISTA", "ADMIN" */
    Conexion&   conn;

public:
    UsuarioBase(int id, const std::string& nombre, const std::string& apellido,
                const std::string& email, const std::string& rol, Conexion& conn);
    virtual ~UsuarioBase() = default;

    /* Cada rol implementa su propio menú principal */
    virtual void mostrarMenuPrincipal() = 0;

    /* Métodos comunes implementados en la clase base */
    void mostrarMisDatos();
    void cambiarContrasenia();

    /* Getters */
    int                getId()      const { return id_u;    }
    const std::string& getEmail()   const { return email;   }
    const std::string& getRol()     const { return rol;     }
    const std::string& getNombre()  const { return nombre;  }
};

/* ── Helpers de cadena usados por todas las subclases ── */

/* Divide una cadena por '|' y devuelve el campo en la posición indicada */
std::string campo(const std::string& linea, int pos);

/* Muestra una cabecera de menú con el nombre y rol del usuario */
void mostrarCabecera(const std::string& nombre, const std::string& rol);

#endif /* USUARIO_BASE_H_ */
