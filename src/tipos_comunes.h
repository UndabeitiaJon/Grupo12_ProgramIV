/*
 * tipos_comunes.h
 *
 *  Created on: 1 may 2026
 *      Author: ander.lecue
 */


#ifndef TIPOS_COMUNES_H_
#define TIPOS_COMUNES_H_

#define MAX_PARADAS  30
#define DIAS_SEMANA   7
#define MAX_VAGONES  20
#define MAX_ASIENTOS 100

typedef enum { ROL_PASAJERO = 0, ROL_EMPLEADO = 1, ROL_ADMIN = 2 } RolUsuario;

typedef enum {
    DESCUENTO_NINGUNO  = 0,
    DESCUENTO_JOVEN    = 1,   /* <26 años: -20% */
    DESCUENTO_DORADA   = 2,   /* >60 años: -40% */
    DESCUENTO_NUMEROSA = 3,   /* familia numerosa: -20% */
    DESCUENTO_ABONO    = 4    /* abono cercanías: -50% */
} TipoDescuento;

typedef enum { EMPLEADO_ACTIVO = 1, EMPLEADO_BAJA = 0 } EstadoEmpleado;

typedef enum {
    TREN_OPERATIVO = 0,
    TREN_REVISION  = 1,
    TREN_AVERIA    = 2,
    TREN_RETIRADO  = 3
} EstadoMantenimiento;

typedef enum {
    RESERVA_PENDIENTE  = 0,
    RESERVA_CONFIRMADA = 1,
    RESERVA_CANCELADA  = 2,
    RESERVA_COMPLETADA = 3
} EstadoReserva;

typedef enum {
    EQUIPAJE_MANO   = 0,
    EQUIPAJE_BODEGA = 1,
    EQUIPAJE_BICI   = 2,
    EQUIPAJE_ESQUI  = 3
} TipoEquipaje;

typedef enum {
    SERVICIO_PROGRAMADO = 0,
    SERVICIO_EN_CURSO   = 1,
    SERVICIO_FINALIZADO = 2,
    SERVICIO_CANCELADO  = 3
} EstadoServicio;

typedef enum {
    ROL_SERV_CONDUCTOR = 0,
    ROL_SERV_REVISOR   = 1,
    ROL_SERV_TECNICO   = 2,
    ROL_SERV_SERVICIO  = 3
} RolServicio;

typedef enum {
    INCIDENCIA_TECNICA   = 0,
    INCIDENCIA_SEGURIDAD = 1,
    INCIDENCIA_CLIENTE   = 2,
    INCIDENCIA_OPERATIVA = 3
} TipoIncidencia;

typedef enum {
    PRIORIDAD_BAJA    = 0,
    PRIORIDAD_MEDIA   = 1,
    PRIORIDAD_ALTA    = 2,
    PRIORIDAD_CRITICA = 3
} PrioridadIncidencia;

typedef enum {
    INCIDENCIA_ABIERTA    = 0,
    INCIDENCIA_EN_PROCESO = 1,
    INCIDENCIA_RESUELTA   = 2,
    INCIDENCIA_CERRADA    = 3
} EstadoIncidencia;

typedef enum {
    LOG_DEBUG    = 0,
    LOG_INFO     = 1,
    LOG_WARNING  = 2,
    LOG_ERROR    = 3,
    LOG_CRITICAL = 4
} NivelLog;

typedef enum {
    TRAYECTO_INACTIVO = 0,
    TRAYECTO_ACTIVO   = 1
} EstadoTrayecto;


#endif /* SRC_TIPOS_COMUNES_H_ */
