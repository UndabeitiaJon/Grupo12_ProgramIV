/*
 * trenfe.h
 *
 *  Created on: 29 mar 2026
 *      Author: jon.undabeitia
 */

#ifndef SRC_TRENFE_H_
#define SRC_TRENFE_H_
#define MAX_PARADAS 30
#define DIAS_SEMANA 7

/*
 * ====================
 * DECLARACION DE ENUMS
 * ====================
 */
//Enum para saber el estado del tren
typedef enum {
	DEFECTUOSO,
	CORRECTO,
	BUENO,
	EXCELENTE
}EstadoTren;
//Enum para saber el tipo del tren
typedef enum{
	CERCANIAS,
	MEDIA_DISTANCIA,
	LARGA_DISTANCIA,
	AVE
}TipoTren;
//Enum que funciona como array. Se pone un 1 si ese dia el trayecto esta operativo 0 si no lo esta
typedef enum {
    LUNES = 0,
    MARTES,
    MIERCOLES,
    JUEVES,
    VIERNES,
    SABADO,
    DOMINGO
} DiaSemana;
//Declarar cuales son los usuarios del sistema
typedef enum {
    CLIENTE,
    MAQUINISTA,
    ADMINISTRADOR
} Rol;
/*
 * ==========================
 * DECLARACIÓN DE ESTRUCTURAS
 * ==========================
 */
//Estructura para definir la fecha
typedef struct {
    int dia;
    int mes;
    int anio;
} Date;
//Estructura que define la hora
typedef struct {
    int hora;
    int minutos;
} Hora;
typedef struct {
	char modelo[50];
	int capacidad;
	Date fecha_fabricacion;
	double numero_serie;
	EstadoTren estado;
	TipoTren tipo_tren;
}Tren;

typedef struct {
    char nombre[50];
    Hora hora_llegada;
    Hora hora_salida;
} Estacion;

typedef struct{
	Tren tren;
	char origen[50];
	char destino[50];

	Hora hora_llegada;
	Hora hora_salida;

	int num_paradas;
	Estacion paradas[MAX_PARADAS]; //paradas intermedias

	int diasOperativos[DIAS_SEMANA];
}Trayecto;

typedef struct {
    int id;
    char nombre[50];
    char apellido[50];
    char dni[10];
    char email[100];
    char telefono[15];
    Date fechaNacimiento;
    char contrasenia[50];
    Rol rol;
    int activo;        // 1 = activo, 0 = inactivo
    Date fechaRegistro;
} Usuario;

/*
 * ========================
 * DECLARACION DE FUNCIONES
 * ========================
 */
Usuario crearUsuario(char *nombre, char *apellido, char *dni,
                     char *email, char *telefono, char *contrasenia,
                     Date fechaNacimiento, Rol rol);
#endif /* SRC_TRENFE_H_ */
