/*
 * protocolo.h
 *
 *  Created on: 7 may 2026
 *      Author: e.aranoa
 */





#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

/* ══════════════════════════════════════════════
   SEPARADOR DE CAMPOS
   ══════════════════════════════════════════════ */
#define SEP             "|"
#define SEP_CHAR        '|'

/* ══════════════════════════════════════════════
   TAMAÑO MAXIMO DE MENSAJE
   ══════════════════════════════════════════════ */
#define PROTO_BUF_MAX   8192

/* ══════════════════════════════════════════════
   COMANDOS  (cliente -> servidor)
   ══════════════════════════════════════════════ */

/* -- Sesion -- */
#define CMD_LOGIN   "LOGIN"           /* LOGIN|email|pass_hash        */
#define CMD_LOGOUT "LOGOUT"          /* LOGOUT                       */
#define CMD_REGISTRO  "REGISTRO"    /* REGISTRO|nombre|apellido|dni|email|telf|fecha_nac|pass_hash */

/* -- Trayectos -- */
#define CMD_LISTAR_TRAYECTOS  "LISTAR_TRAYECTOS"   /* LISTAR_TRAYECTOS              */
#define CMD_BUSCAR_TRAYECTO  "BUSCAR_TRAYECTO"    /* BUSCAR_TRAYECTO|id_orig|id_dest|fecha|clase */
#define CMD_DETALLE_TRAYECTO "DETALLE_TRAYECTO"   /* DETALLE_TRAYECTO|id_tr        */

/* -- Estaciones -- */
#define CMD_LISTAR_ESTACIONES "LISTAR_ESTACIONES"  /* LISTAR_ESTACIONES             */
#define CMD_LISTAR_CIUDADES "LISTAR_CIUDADES"    /* LISTAR_CIUDADES               */

/* -- Vagones y asientos -- */
#define CMD_LISTAR_VAGONES "LISTAR_VAGONES"  /* LISTAR_VAGONES|id_tr|fecha|clase  */
#define CMD_MAPA_VAGON     "MAPA_VAGON"      /* MAPA_VAGON|id_tr|fecha|num_vagon  */

/* -- Reservas -- */
#define CMD_HACER_RESERVA   "HACER_RESERVA"      /* HACER_RESERVA|id_u|id_tr|fecha|clase|vagon|asiento|tipo_eq|peso_eq */
#define CMD_CANCELAR_RESERVA "CANCELAR_RESERVA"   /* CANCELAR_RESERVA|id_res|id_u  */
#define CMD_MIS_RESERVAS "MIS_RESERVAS"       /* MIS_RESERVAS|id_u             */
#define CMD_HISTORIAL  "HISTORIAL"          /* HISTORIAL|id_u                */
#define CMD_DETALLE_RESERVA "DETALLE_RESERVA"    /* DETALLE_RESERVA|id_res        */

/* -- Puntos fidelidad -- */
#define CMD_MIS_PUNTOS "MIS_PUNTOS"         /* MIS_PUNTOS|id_u               */
#define CMD_CANJEAR_PUNTOS "CANJEAR_PUNTOS"     /* CANJEAR_PUNTOS|id_u|cantidad  */

/* -- Datos personales -- */
#define CMD_MIS_DATOS "MIS_DATOS"          /* MIS_DATOS|id_u                */
#define CMD_CAMBIAR_PASS "CAMBIAR_PASS"       /* CAMBIAR_PASS|email|nueva_hash */

/* -- Maquinista -- */
#define CMD_CUADRANTE "CUADRANTE"          /* CUADRANTE|id_u                */
#define CMD_MARCAR_INICIO "MARCAR_INICIO"      /* MARCAR_INICIO|id_serv         */
#define CMD_MARCAR_FIN "MARCAR_FIN"         /* MARCAR_FIN|id_serv            */
#define CMD_REPORTAR_RETRASO "REPORTAR_RETRASO"   /* REPORTAR_RETRASO|id_serv|min|causa */

/* -- Admin: trenes -- */
#define CMD_LISTAR_TRENES "LISTAR_TRENES"      /* LISTAR_TRENES                 */
#define CMD_INSERTAR_TREN "INSERTAR_TREN"      /* INSERTAR_TREN|modelo|serie|anio|estado|fecha_rev */
#define CMD_MODIFICAR_TREN "MODIFICAR_TREN"     /* MODIFICAR_TREN|id_t|modelo|serie|anio|estado|fecha_rev */
#define CMD_ELIMINAR_TREN "ELIMINAR_TREN"      /* ELIMINAR_TREN|id_t            */

/* -- Admin: estaciones -- */
#define CMD_INSERTAR_ESTACION   "INSERTAR_ESTACION"  /* INSERTAR_ESTACION|nombre|ciudad|provincia|andenes */
#define CMD_MODIFICAR_ESTACION  "MODIFICAR_ESTACION" /* MODIFICAR_ESTACION|id_est|nombre|ciudad|provincia|andenes */

/* -- Admin: trayectos -- */
#define CMD_INSERTAR_TRAYECTO   "INSERTAR_TRAYECTO"  /* INSERTAR_TRAYECTO|id_t|id_orig|id_dest|h_sal|h_ll|dur|precio|dias */
#define CMD_MODIFICAR_TRAYECTO  "MODIFICAR_TRAYECTO" /* MODIFICAR_TRAYECTO|id_tr|h_sal|h_ll|precio|dias */
#define CMD_ESTADO_TRAYECTO "ESTADO_TRAYECTO"    /* ESTADO_TRAYECTO|id_tr|estado  */

/* -- Admin: personal -- */
#define CMD_LISTAR_EMPLEADOS "LISTAR_EMPLEADOS"   /* LISTAR_EMPLEADOS              */
#define CMD_LISTAR_USUARIOS "LISTAR_USUARIOS"    /* LISTAR_USUARIOS               */
#define CMD_DESHABILITAR_USER "DESHABILITAR_USER"  /* DESHABILITAR_USER|id_u        */

/* -- Admin: servicios -- */
#define CMD_LISTAR_SERVICIOS    "LISTAR_SERVICIOS"   /* LISTAR_SERVICIOS|fecha|id_t   */
#define CMD_INSERTAR_SERVICIO   "INSERTAR_SERVICIO"  /* INSERTAR_SERVICIO|id_t|fecha  */
#define CMD_CANCELAR_SERVICIO   "CANCELAR_SERVICIO"  /* CANCELAR_SERVICIO|id_serv     */

/* -- Admin: incidencias -- */
#define CMD_LISTAR_INCIDENCIAS  "LISTAR_INCIDENCIAS" /* LISTAR_INCIDENCIAS|estado     */
#define CMD_RESOLVER_INCIDENCIA "RESOLVER_INCIDENCIA"/* RESOLVER_INCIDENCIA|id_inc|id_u */
#define CMD_INSERTAR_INCIDENCIA "INSERTAR_INCIDENCIA"/* INSERTAR_INCIDENCIA|id_serv|tipo|desc|prioridad */

/* -- Admin: informes -- */
#define CMD_INFORME_OCUPACION "INFORME_OCUPACION"  /* INFORME_OCUPACION|id_t        */
#define CMD_INFORME_INGRESOS "INFORME_INGRESOS"   /* INFORME_INGRESOS|id_tr        */
#define CMD_INFORME_INCIDENCIAS "INFORME_INCIDENCIAS"/* INFORME_INCIDENCIAS|f_ini|f_fin */

/* -- Admin: tarifas -- */
#define CMD_LISTAR_TARIFAS "LISTAR_TARIFAS"     /* LISTAR_TARIFAS                */
#define CMD_MOD_PRECIO_BASE "MOD_PRECIO_BASE"    /* MOD_PRECIO_BASE|id_tr|precio  */
#define CMD_MOD_COEF_BUSINESS "MOD_COEF_BUSINESS"  /* MOD_COEF_BUSINESS|coef        */
#define CMD_MOD_EXCESO_KG  "MOD_EXCESO_KG"      /* MOD_EXCESO_KG|precio          */
#define CMD_MOD_SUPL_BICI "MOD_SUPL_BICI"      /* MOD_SUPL_BICI|precio          */

/* -- Admin: logs -- */
#define CMD_VER_LOGS "VER_LOGS"           /* VER_LOGS|fecha|usuario|nivel  */

/* ══════════════════════════════════════════════
   RESPUESTAS  (servidor -> cliente)
   ══════════════════════════════════════════════ */

#define RESP_OK  "OK"          /* OK  o  OK|dato1|dato2|...          */
#define RESP_ERROR  "ERROR"       /* ERROR|codigo|descripcion           */
#define RESP_AUTH_OK  "AUTH_OK"     /* AUTH_OK|id_u|rol|nombre            */
#define RESP_AUTH_FAIL "AUTH_FAIL"   /* AUTH_FAIL|motivo                   */
#define RESP_FIN_LISTA  "FIN_LISTA"   /* FIN_LISTA  (cierra una lista)      */

/* Prefijos de filas en listas (servidor -> cliente) */
#define ROW_TRAYECTO "TRAYECTO"    /* TRAYECTO|id|id_t|orig|dest|h_sal|h_ll|dur|precio|dias|estado */
#define ROW_ESTACION  "ESTACION"    /* ESTACION|id|nombre|ciudad|provincia|andenes|sala_club */
#define ROW_RESERVA "RESERVA"     /* RESERVA|id|id_tr|fecha|clase|vagon|asiento|precio|estado|cod */
#define ROW_TREN  "TREN"        /* TREN|id|modelo|serie|anio|estado|fecha_rev */
#define ROW_EMPLEADO "EMPLEADO"    /* EMPLEADO|id|nombre|apellido|email|rol_emp|estado */
#define ROW_USUARIO "USUARIO"     /* USUARIO|id|nombre|apellido|email|rol|activo */
#define ROW_SERVICIO "SERVICIO"    /* SERVICIO|id|id_t|fecha|estado|retraso */
#define ROW_INCIDENCIA "INCIDENCIA"  /* INCIDENCIA|id|tipo|prioridad|estado|desc */
#define ROW_LOG "LOG"         /* LOG|timestamp|usuario|accion|nivel */
#define ROW_PUNTOS  "PUNTOS"      /* PUNTOS|id_u|puntos|descuento       */
#define ROW_VAGON   "VAGON"       /* VAGON|num_vagon|capacidad|libres   */
#define ROW_MAPA_INFO "MAPA_INFO"  /* MAPA_INFO|num_vagon|capacidad      */
#define ROW_ASIENTO "ASIENTO"     /* ASIENTO|num_asiento|ocupado(0/1)   */

/* ══════════════════════════════════════════════
   CODIGOS DE ERROR
   ══════════════════════════════════════════════ */
#define ERR_NO_AUTH "401"   /* No autenticado                          */
#define ERR_FORBIDDEN "403"   /* Sin permisos para esta operacion        */
#define ERR_NOT_FOUND "404"   /* Recurso no encontrado                   */
#define ERR_CONFLICT "409"   /* Conflicto (asiento ocupado, etc.)       */
#define ERR_BAD_PARAMS "400"   /* Parametros incorrectos o incompletos    */
#define ERR_SERVER "500"   /* Error interno del servidor              */

/* ══════════════════════════════════════════════
   ROLES  (usados en AUTH_OK)
   ══════════════════════════════════════════════ */
#define ROL_STR_PASAJERO "PASAJERO"
#define ROL_STR_EMPLEADO "EMPLEADO"
#define ROL_STR_ADMIN "ADMIN"

#endif /* PROTOCOLO_H_ */
