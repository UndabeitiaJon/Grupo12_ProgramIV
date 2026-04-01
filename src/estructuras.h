/*
 * trenfe.h
 *
 *  Created on: 29 mar 2026
 *      Author: jon.undabeitia
 */

#ifndef SRC_ESTRUCTURAS_H_
#define SRC_ESTRUCTURAS_H_
#define MAX_PARADAS 30
#define DIAS_SEMANA 7

// ============================================================
//  ENUMS
// ============================================================

typedef enum {
    ROL_PASAJERO = 0,
    ROL_EMPLEADO = 1,
    ROL_ADMIN    = 2
} RolUsuario;

typedef enum {
    DESCUENTO_NINGUNO  = 0,
    DESCUENTO_JOVEN    = 1,
    DESCUENTO_DORADA   = 2,
    DESCUENTO_NUMEROSA = 3,
    DESCUENTO_ABONO    = 4
} TipoDescuento;

typedef enum {
    EMPLEADO_ACTIVO = 1,
    EMPLEADO_BAJA   = 0
} EstadoEmpleado;

typedef enum {
    TREN_OPERATIVO = 0,
    TREN_REVISION  = 1,
    TREN_AVERIA    = 2,
    TREN_RETIRADO  = 3
} EstadoMantenimiento;

typedef enum {
    RESERVA_PENDIENTE   = 0,
    RESERVA_CONFIRMADA  = 1,
    RESERVA_CANCELADA   = 2,
    RESERVA_COMPLETADA  = 3
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
    INCIDENCIA_ABIERTA     = 0,
    INCIDENCIA_EN_PROCESO  = 1,
    INCIDENCIA_RESUELTA    = 2,
    INCIDENCIA_CERRADA     = 3
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


// ============================================================
//  ESTRUCTURAS
// ============================================================

//Estructura para definir Usuarios
typedef struct {
    int        id_u;
    char       nombre[64];
    char       apellido[64];
    char       dni[16];
    char       email[128];
    char       telf[20];
    char       fecha_nac[11];        // "AAAA-MM-DD\0"
    char       pass_hash[256];
    RolUsuario rol;
    int        activo;               // 0=false, 1=true
    char       fecha_registro[20];   // "AAAA-MM-DD HH:MM:SS\0"
} Usuario;

//Estructura para definir Pasajeros
typedef struct {
    int           id_u;
    int           puntos_fidelidad;
    TipoDescuento tipo_descuento;
    char          num_tarjeta_fidelizacion[32];
    int           tiene_tarjeta;             // 0 si el campo es NULL en BD
    int           necesidad_asistencia_pmr;  // 0=false, 1=true
} DatosPasajero;

//Estructura para definir Empleados
typedef struct {
    int            id_u;
    char           num_empleado[32];
    char           num_ss[32];
    char           fecha_ingreso[11];    // "AAAA-MM-DD\0"
    char           rol_empleado[64];
    int            anios_exp;
    char           telf_empresa[20];
    EstadoEmpleado estado;
} DatosEmpleado;

//Estructura para definir Trenes
typedef struct {
    int                 id_t;
    char                nombre_modelo[64];
    char                num_serie[32];
    int                 anio_fab;
    EstadoMantenimiento estado_mant;
    char                fecha_ultima_revision[11];  // "AAAA-MM-DD\0"
    int                 tiene_revision;             // 0 si el campo es NULL en BD
} Tren;

//Estructura para definir Vagones
typedef struct {
    int  id_vagon;
    int  id_tren;
    int  numero_vagon;
    char clase[4];          // "B", "T", "P"
    int  capacidad_total;
    int  vagon_PMR;         // 0=false, 1=true
} Vagon;

//Estructura para definir Estacion
typedef struct {
    int    id_est;
    char   nombre[128];
    char   codigo_gtfs[16];
    char   ciudad[64];
    char   provincia[64];
    double latitud;
    double longitud;
    int    num_andenes;
    int    tiene_sala_club;  // 0=false, 1=true
} Estacion;

//Estructura para definir trayectos
typedef struct {
    int            id_tr;
    int            id_t;
    int            id_est_origen;
    int            id_est_destino;
    char           hora_salida[6];    // "HH:MM\0"
    char           hora_llegada[6];   // "HH:MM\0"
    int            duracion_min;
    double         precio_base;
    char           dias_operacion[8]; // "LMXJVSD\0"
    EstadoTrayecto estado;

} Trayecto;

//Estructura para definir Paradas intermedias
typedef struct {
    int  id_parada;
    int  id_tr;
    int  id_est;
    int  orden;
    char hora_llegada[6];   // "HH:MM\0"
    char hora_salida[6];    // "HH:MM\0"
    int  anden;
    int  tiene_anden;       // 0 si el campo es NULL en BD
} ParadaIntermedia;

//Estructura para definir Reservas
typedef struct {
    int           id_res;
    int           id_u;
    int           id_tr;
    char          fecha_viaje[11];       // "AAAA-MM-DD\0"
    char          clase[4];              // "B" o "T"
    int           num_vagon;
    int           num_asiento;
    double        precio_base;
    double        descuento_pct;
    double        precio_final;
    EstadoReserva estado;
    char          codigo_validacion[64];
    char          fecha_reserva[11];     // "AAAA-MM-DD\0"
} Reserva;

//Estructura para definir Equipaje
typedef struct {
    int           id_eq;
    int           id_res;
    TipoEquipaje  tipo;
    double        peso_kg;
    char          dimensiones[64];
    double        exceso_kg;
    double        suplemento_pago;
    int           facturado;   // 0=false, 1=true
} Equipaje;

//Estructura para definir Servicio operativo
typedef struct {
    int            id_serv;
    int            id_tr;
    char           fecha[11];           // "AAAA-MM-DD\0"
    EstadoServicio estado_serv;
    char           hora_inicio_real[6]; // "HH:MM\0"
    int            tiene_hora_inicio;   // 0 si el campo es NULL en BD
    char           hora_fin_real[6];    // "HH:MM\0"
    int            tiene_hora_fin;      // 0 si el campo es NULL en BD
    int            minutos_retraso;
    char           causa_retraso[256];
    int            tiene_causa;         // 0 si el campo es NULL en BD
} ServicioOperativo;

//Estructura para definir asignacion de personal
typedef struct {
    int         id_asig;
    int         id_serv;
    int         id_u;
    int         id_t;
    RolServicio rol_servicio;
    char        observaciones[256];
    int         tiene_observaciones;    // 0 si el campo es NULL en BD
} AsignacionPersonal;

//Estructura para definir Incidencias
typedef struct {
    int                id_inc;
    int                id_serv;
    int                id_u_reporta;
    TipoIncidencia     tipo;
    char               descripcion[512];
    PrioridadIncidencia prioridad;
    EstadoIncidencia   estado;
    char               fecha_reporte[11];     // "AAAA-MM-DD\0"
    char               fecha_resolucion[11];  // "AAAA-MM-DD\0"
    int                tiene_fecha_resolucion; // 0 si aún no resuelta
    int                id_u_resuelve;
    int                tiene_resolutor;        // 0 si aún no asignado
} Incidencia;

//Estructura para definir LOG_OPERATIVOS
typedef struct {
    int      id_log;
    char     timestamp[11];       // "AAAA-MM-DD\0"
    int      id_u;
    int      tiene_usuario;       // 0 si la acción fue del sistema (NULL en BD)
    char     accion[128];
    char     entidad_afectada[64];
    int      id_entidad;
    char     ip_origen[46];       // IPv4 o IPv6
    NivelLog nivel;
    char     detalle_json[1024];
    int      tiene_detalle;       // 0 si el campo es NULL en BD
} LogOperativo;

/*
 * ========================
 * DECLARACION DE FUNCIONES
 * ========================
 */
Usuario crearUsuario(char *nombre, char *apellido, char *dni,
                     char *email, char *telefono, char *contrasenia,
                     char *fechaNacimiento, RolUsuario rol);
#endif /* SRC_ESTRUCTURAS_H_ */
