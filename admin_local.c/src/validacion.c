/*
 * validacion.c
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "validacion.h"

int validar_dni(const char *dni) {
    if (!dni || strlen(dni) != 9) {
        printf("  [ERROR] DNI invalido: debe tener exactamente 8 digitos y 1 letra mayuscula (ej: 12345678A).\n");
        return 0;
    }
    for (int i = 0; i < 8; i++) {
        if (!isdigit((unsigned char)dni[i])) {
            printf("  [ERROR] DNI invalido: los 8 primeros caracteres deben ser digitos.\n");
            return 0;
        }
    }
    if (!isupper((unsigned char)dni[8])) {
        printf("  [ERROR] DNI invalido: el ultimo caracter debe ser una letra mayuscula.\n");
        return 0;
    }
    return 1;
}

int validar_email(const char *email) {
    if (!email) {
    	printf("  [ERROR] Email no puede estar vacio.\n");
    	return 0;
    }
    size_t len = strlen(email);
    if (len < 6 || len > 127) {
        printf("  [ERROR] Email invalido: longitud fuera de rango (6-127 caracteres).\n");
        return 0;
    }
    const char *at = strchr(email, '@');
    if (!at || at == email) {
        printf("  [ERROR] Email invalido: debe contener '@' (ej: nombre@dominio.com).\n");
        return 0;
    }
    const char *dot = strchr(at + 1, '.');
    if (!dot || *(dot + 1) == '\0') {
        printf("  [ERROR] Email invalido: dominio debe contener '.' con extension (ej: .com).\n");
        return 0;
    }
    return 1;
}

int validar_telefono(const char *telf) {
    if (!telf || strlen(telf) != 9) {
        printf("  [ERROR] Telefono invalido: debe tener exactamente 9 digitos (ej: 612345678).\n");
        return 0;
    }
    for (int i = 0; i < 9; i++) {
        if (!isdigit((unsigned char)telf[i])) {
            printf("  [ERROR] Telefono invalido: solo se admiten digitos numericos.\n");
            return 0;
        }
    }
    return 1;
}

int validar_fecha(const char *fecha) {
    if (!fecha || strlen(fecha) != 10) {
        printf("  [ERROR] Fecha invalida: formato requerido AAAA-MM-DD (ej: 2026-04-15).\n");
        return 0;
    }
    if (fecha[4] != '-' || fecha[7] != '-') {
        printf("  [ERROR] Fecha invalida: los separadores deben ser guiones (AAAA-MM-DD).\n");
        return 0;
    }
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7){
        	continue;
        }
        if (!isdigit((unsigned char)fecha[i])) {
            printf("  [ERROR] Fecha invalida: solo se admiten digitos en posiciones AAAA, MM y DD.\n");
            return 0;
        }
    }
    int anio = atoi(fecha);
    int mes  = atoi(fecha + 5);
    int dia  = atoi(fecha + 8);
    if (anio < 1900 || anio > 2100) {
        printf("  [ERROR] Fecha invalida: anio %d fuera de rango (1900-2100).\n", anio);
        return 0;
    }
    if (mes < 1 || mes > 12) {
        printf("  [ERROR] Fecha invalida: mes %d fuera de rango (01-12).\n", mes);
        return 0;
    }
    /* Dias maximos por mes (sin bissexto completo, suficiente para validacion basica) */
    int dias_mes[] = {0,31,29,31,30,31,30,31,31,30,31,30,31};
    if (dia < 1 || dia > dias_mes[mes]) {
        printf("  [ERROR] Fecha invalida: dia %d fuera de rango para el mes %02d.\n", dia, mes);
        return 0;
    }
    return 1;
}

int validar_hora(const char *hora) {
    if (!hora || (strlen(hora) != 5 && strlen(hora) != 8)) {
        printf("  [ERROR] Hora invalida: formato requerido HH:MM (ej: 08:30).\n");
        return 0;
    }
    if (hora[2] != ':') {
        printf("  [ERROR] Hora invalida: el separador debe ser ':' (HH:MM).\n");
        return 0;
    }
    if (!isdigit((unsigned char)hora[0]) || !isdigit((unsigned char)hora[1]) ||
        !isdigit((unsigned char)hora[3]) || !isdigit((unsigned char)hora[4])) {
        printf("  [ERROR] Hora invalida: solo se admiten digitos en HH y MM.\n");
        return 0;
    }
    int h = atoi(hora);
    int m = atoi(hora + 3);
    if (h < 0 || h > 23) {
        printf("  [ERROR] Hora invalida: horas %d fuera de rango (00-23).\n", h);
        return 0;
    }
    if (m < 0 || m > 59) {
        printf("  [ERROR] Hora invalida: minutos %d fuera de rango (00-59).\n", m);
        return 0;
    }
    return 1;
}

int validar_contrasenia(const char *pass) {
    if (!pass || strlen(pass) < 6) {
        printf("  [ERROR] Contraseña invalida: debe tener al menos 6 caracteres.\n");
        return 0;
    }
    return 1;
}

int validar_nombre(const char *nombre) {
    if (!nombre || strlen(nombre) < 2) {
        printf("  [ERROR] Nombre invalido: debe tener al menos 2 caracteres.\n");
        return 0;
    }
    for (size_t i = 0; i < strlen(nombre); i++) {
        unsigned char c = (unsigned char)nombre[i];
        /* Permitir letras (incluidas acentuadas UTF-8 >= 128), espacio, guion, apostrofe */
        if (!isalpha(c) && c != ' ' && c != '-' && c != '\'' && c < 128) {
            printf("  [ERROR] Nombre invalido: caracter no permitido '%c'. "
                   "Solo letras, espacios y guiones.\n", nombre[i]);
            return 0;
        }
    }
    return 1;
}

int validar_precio(double precio) {
    if (precio < 0.0) {
        printf("  [ERROR] Precio invalido: debe ser un valor positivo (%.2f introducido).\n", precio);
        return 0;
    }
    return 1;
}

int validar_duracion(int minutos) {
    if (minutos <= 0) {
        printf("  [ERROR] Duracion invalida: debe ser un numero positivo de minutos (%d introducido).\n", minutos);
        return 0;
    }
    return 1;
}
