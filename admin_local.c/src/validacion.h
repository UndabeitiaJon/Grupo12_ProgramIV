/*
 * validacion.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */

#ifndef SRC_VALIDACION_H_
#define SRC_VALIDACION_H_

// DNI: 8 digitos seguidos de 1 letra mayuscula (ej: 12345678A)
int validar_dni(const char *dni);

// Email: debe contener '@' y '.', longitud 6-127
int validar_email(const char *email);

// Telefono: 9 digitos numericos
int validar_telefono(const char *telf);

// Fecha formato AAAA-MM-DD con valores de mes y dia coherentes
int validar_fecha(const char *fecha);

// Hora formato HH:MM con valores coherentes (00:00 - 23:59)
int validar_hora(const char *hora);

// Contraseña: minimo 6 caracteres
int validar_contrasenia(const char *pass);

// Nombre y/o apellido: al menos 2 caracteres, solo letras y espacios
int validar_nombre(const char *nombre);

// Precio: valor numerico positivo
int validar_precio(double precio);

// Duracion en minutos: valor entero positivo
int validar_duracion(int minutos);


#endif /* SRC_VALIDACION_H_ */
