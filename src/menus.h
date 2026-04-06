/*
 * menus.h
 *
 *  Created on: 1 abr 2026
 *      Author: e.aranoa
 */

#ifndef MENUS_H
#define MENUS_H
//Menu entrada
void menu_login();
void menu_inicial();
void menu_registro_pasajero();


void comprobar_rol_usuario(char *email,Usuario user);
void menu_alta_usuario();
void limpiar_pantalla ();

//Menu de administrador
void menu_principal_administrador(const char *email_logueado,const Usuario user);

//Menu de pasajero
void menu_principal_pasajero(const Usuario user);
void menu_mis_reservas (const Usuario user);
void mis_reservas_activas (const Usuario user);
void menu_nuevas_reservas (const Usuario user);
void menu_comprar_billete (const Usuario user);
//Menu empleado
void menu_principal_empleado(const Usuario user);

#endif
