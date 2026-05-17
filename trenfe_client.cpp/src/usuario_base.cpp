/*
 * usuario_base.cpp
 *
 *  Created on: 8 may 2026
 *      Author: e.aranoa
 */

/*
 * usuario_base.cpp  -  Sistema TRENFE  -  Fase 2
 */

#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>
#include "usuario_base.h"

extern "C" {
#include "hash.h"
}

/* ══════════════════════════════════════════════
   HELPERS
   ══════════════════════════════════════════════ */

/* Divide 'linea' por '|' y devuelve el campo en posición 'pos' (0-based) */
std::string campo(const std::string& linea, int pos) {
    std::istringstream ss(linea);
    std::string tok;
    int i = 0;
    while (std::getline(ss, tok, '|')) {
        if (i == pos) return tok;
        i++;
    }
    return "";
}

void mostrarCabecera(const std::string& nombre, const std::string& rol) {
    std::cout << "\n========================================\n";
    std::cout << "  TRENFE  |  " << nombre << "  [" << rol << "]\n";
    std::cout << "========================================\n";
}

/* ══════════════════════════════════════════════
   CONSTRUCTOR
   ══════════════════════════════════════════════ */

UsuarioBase::UsuarioBase(int id, const std::string& nom, const std::string& ape,
                         const std::string& mail, const std::string& r, Conexion& c)
    : id_u(id), nombre(nom), apellido(ape), email(mail), rol(r), conn(c)
{}

/* ══════════════════════════════════════════════
   MIS DATOS
   ══════════════════════════════════════════════ */

void UsuarioBase::mostrarMisDatos() {
    conn.enviar("MIS_DATOS|" + std::to_string(id_u));
    std::string resp = conn.recibir();

    /* DATOS|nombre|apellido|dni|email|telf|fecha_nac|rol */
    if (campo(resp, 0) != "DATOS") {
        std::cout << "  Error al obtener datos: " << resp << "\n";
        return;
    }

    std::cout << "\n  -- Mis datos --\n";
    std::cout << "  Nombre   : " << campo(resp, 1) << " " << campo(resp, 2) << "\n";
    std::cout << "  DNI      : " << campo(resp, 3) << "\n";
    std::cout << "  Email    : " << campo(resp, 4) << "\n";
    std::cout << "  Teléfono : " << campo(resp, 5) << "\n";
    std::cout << "  Nac.     : " << campo(resp, 6) << "\n";
    std::cout << "  Rol      : " << campo(resp, 7) << "\n";
}

/* ══════════════════════════════════════════════
   CAMBIAR CONTRASEÑA
   ══════════════════════════════════════════════ */

void UsuarioBase::cambiarContrasenia() {
    std::string pass1, pass2;

    std::cout << "\n  Nueva contraseña     : ";
    std::getline(std::cin, pass1);
    std::cout << "  Confirmar contraseña : ";
    std::getline(std::cin, pass2);

    if (pass1 != pass2) {
        std::cout << "  Las contraseñas no coinciden.\n";
        return;
    }

    char hash[65];
    sha256_hex(pass1.c_str(), hash);

    conn.enviar("CAMBIAR_PASS|" + email + "|" + hash);
    std::string resp = conn.recibir();

    if (campo(resp, 0) == "OK") {
        std::cout << "  Contraseña actualizada correctamente.\n";
    } else {
        std::cout << "  Error: " << campo(resp, 1) << "\n";
    }
}


