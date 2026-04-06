/*
 * menus.c  –  Sistema TRENFE  –  Fase 1
 *
 * Menús completos para 3 roles:
 *   ADMIN      → 13 secciones (Trenes, Trayectos, Estaciones, Personal,
 *                               Pasajeros, Servicios, Tarifas, GTFS,
 *                               Incidencias, Informes, Logs, Config)
 *   PASAJERO   → Buscar/Reservar, Mis Reservas, Puntos, Mis Datos
 *   MAQUINISTA → Cuadrante, Reportar estado, Mis Datos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "menus.h"
#include "db_manager.h"
#include "config.h"
#include "logs.h"
#include "estructuras.h"

/* ============================================================
 *  UTILIDADES DE ENTRADA / PANTALLA
 * ============================================================ */
void limpiar_pantalla(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pausar(void) {
    printf("\n  [Pulse ENTER para continuar...]");
    int c; while ((c = getchar()) != '\n' && c != EOF);
    getchar();
}

/* Pausa sin vaciar búfer (ya fue vaciado por leer_entero / leer_cadena) */
static void pausar_s(void) {
    printf("\n  [Pulse ENTER para continuar...]");
    getchar();
}

int leer_entero(const char *prompt) {
    int v = 0;
    printf("%s", prompt);
    fflush(stdout);
    if (scanf("%d", &v) != 1) v = 0;
    int c; while ((c = getchar()) != '\n' && c != EOF);
    return v;
}

double leer_double(const char *prompt) {
    double v = 0.0;
    printf("%s", prompt);
    fflush(stdout);
    if (scanf("%lf", &v) != 1) v = 0.0;
    int c; while ((c = getchar()) != '\n' && c != EOF);
    return v;
}

void leer_cadena(const char *prompt, char *buf, int max) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, max, stdin))
        buf[strcspn(buf, "\r\n")] = 0;
}

static void sep(void) {
    printf("-----------------------------------------\n");
}

static void titulo(const char *t) {
    printf("\n=========================================\n");
    printf("  %s\n", t);
    printf("=========================================\n");
}

/* ============================================================
 *  MENÚ INICIAL  (Login / Registro / Salir)
 * ============================================================ */
void menu_inicial(void) {
    int op;
    do {
        titulo("SISTEMA TRENFE  –  Administrador Local");
        printf("  1. Iniciar sesion\n");
        printf("  2. Registrarse como nuevo pasajero\n");
        printf("  0. Salir\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
            case 1: menu_login();             break;
            case 2: menu_registro_pasajero(); break;
            case 0: printf("\n  Hasta luego.\n\n"); break;
            default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ============================================================
 *  LOGIN
 * ============================================================ */
void menu_login(void) {
    char email[128], pass[256];
    int intentos = 3;

    titulo("INICIO DE SESION");
    while (intentos-- > 0) {
        leer_cadena("  Email     : ", email, sizeof(email));
        leer_cadena("  Contrasena: ", pass,  sizeof(pass));

        if (verificar_usuario(email, pass)) {
            RolUsuario rol = obtener_rol_usuario(email);
            int id_u       = obtener_id_usuario(email);
            log_evento(cfg.log_path, email, "LOGIN", "Sesion iniciada correctamente");
            printf("\n  Bienvenido/a!\n");
            switch (rol) {
                case ROL_ADMIN:    menu_principal_admin    (id_u, email); break;
                case ROL_EMPLEADO: menu_principal_maquinista(id_u, email); break;
                default:           menu_principal_pasajero (id_u, email); break;
            }
            return;
        }
        log_evento(cfg.log_path, email, "LOGIN_FAIL", "Credenciales incorrectas");
        if (intentos > 0)
            printf("  Credenciales incorrectas. Intentos restantes: %d\n", intentos);
    }
    printf("  Demasiados intentos fallidos.\n");
    log_evento(cfg.log_path, email, "BLOQUEO", "Demasiados intentos de login");
}

/* ============================================================
 *  REGISTRO NUEVO PASAJERO
 * ============================================================ */
void menu_registro_pasajero(void) {
    titulo("REGISTRO DE NUEVO PASAJERO");
    char nombre[64], apellido[64], dni[16], email[128];
    char telf[20], fecha_nac[11], pass[256], pass2[256];

    leer_cadena("  Nombre            : ", nombre,   sizeof(nombre));
    leer_cadena("  Apellido          : ", apellido, sizeof(apellido));
    leer_cadena("  DNI               : ", dni,      sizeof(dni));
    leer_cadena("  Email             : ", email,    sizeof(email));
    leer_cadena("  Telefono          : ", telf,     sizeof(telf));
    leer_cadena("  F. Nac(AAAA-MM-DD): ", fecha_nac,sizeof(fecha_nac));
    leer_cadena("  Contrasena        : ", pass,     sizeof(pass));
    leer_cadena("  Repite contrasena : ", pass2,    sizeof(pass2));

    if (strcmp(pass, pass2) != 0) {
        printf("  ERROR: Las contrasenas no coinciden.\n");
        pausar_s(); return;
    }

    Usuario u = crearUsuario(nombre, apellido, dni, email,
                              telf, pass, fecha_nac, ROL_PASAJERO);
    if (insertar_usuario_db(u) == 0) {
        log_evento(cfg.log_path, email, "REGISTRO", "Nuevo pasajero registrado");
        printf("\n  Registro completado. Ya puede iniciar sesion.\n");

        /* Detectar descuento por edad automáticamente */
        int anio_nac = 0;
        if (strlen(fecha_nac) >= 4)
            anio_nac = atoi(fecha_nac);
        int anio_actual = 2026; /* Año académico del proyecto */
        int edad = anio_actual - anio_nac;
        int id_nuevo = obtener_id_usuario(email);
        if (id_nuevo > 0) {
            if (edad < 26) {
                actualizar_descuento_usuario(id_nuevo, DESCUENTO_JOVEN);
                printf("  Descuento JOVEN (<26 anios) aplicado automaticamente: -20%%\n");
            } else if (edad > 60) {
                actualizar_descuento_usuario(id_nuevo, DESCUENTO_DORADA);
                printf("  Descuento DORADA (>60 anios) aplicado automaticamente: -40%%\n");
            }
        }
    } else {
        printf("  Error al registrar (DNI/email ya existente).\n");
    }
    pausar_s();
}

/* ============================================================
 *  ╔═══════════════════════════════════╗
 *  ║   MENÚS ADMINISTRADOR (13 secs)   ║
 *  ╚═══════════════════════════════════╝
 * ============================================================ */

void menu_principal_admin(int id_admin, const char *email) {
    int op;
    do {
        titulo("MENU PRINCIPAL  –  ADMINISTRADOR");
        printf("   1. Gestion de Trenes\n");
        printf("   2. Gestion de Trayectos\n");
        printf("   3. Gestion de Estaciones\n");
        printf("   4. Gestion de Personal\n");
        printf("   5. Gestion de Pasajeros\n");
        printf("   6. Gestion de Servicios Operativos\n");
        printf("   7. Gestion de Tarifas\n");
        printf("   8. Importar Datos GTFS\n");
        printf("   9. Incidencias\n");
        printf("  10. Informes\n");
        printf("  11. Logs del Sistema\n");
        printf("  12. Configuracion\n");
        printf("   0. Cerrar sesion\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
            case 1:  menu_gestion_trenes    (id_admin, email); break;
            case 2:  menu_gestion_trayectos (id_admin, email); break;
            case 3:  menu_gestion_estaciones(id_admin, email); break;
            case 4:  menu_gestion_personal  (id_admin, email); break;
            case 5:  menu_gestion_pasajeros (id_admin, email); break;
            case 6:  menu_gestion_servicios (id_admin, email); break;
            case 7:  menu_gestion_tarifas   (id_admin, email); break;
            case 8:  menu_importar_gtfs     (id_admin, email); break;
            case 9:  menu_incidencias       (id_admin, email); break;
            case 10: menu_informes          (id_admin, email); break;
            case 11: menu_logs             (id_admin, email);  break;
            case 12: menu_configuracion    (id_admin, email);  break;
            case 0:
                log_evento(cfg.log_path, email, "LOGOUT", "Sesion admin cerrada");
                printf("\n  Sesion cerrada.\n");
                break;
            default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 1. GESTIÓN DE TRENES ─── */
void menu_gestion_trenes(int id_admin, const char *email) {
    int op;
    do {
        titulo("GESTION DE TRENES");
        printf("  1. Listar trenes\n");
        printf("  2. Anyadir tren\n");
        printf("  3. Modificar tren\n");
        printf("  4. Cambiar estado de mantenimiento\n");
        printf("  5. Ver vagones de un tren\n");
        printf("  6. Anyadir vagon\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");

        switch (op) {
        case 1:
            listar_trenes_db();
            pausar_s();
            break;

        case 2: {
            titulo("ANYADIR TREN");
            Tren t; memset(&t, 0, sizeof(t));
            leer_cadena("  Modelo           : ", t.nombre_modelo, sizeof(t.nombre_modelo));
            leer_cadena("  Num. serie        : ", t.num_serie,    sizeof(t.num_serie));
            t.anio_fab = leer_entero("  Anyo fabricacion  : ");
            printf("  Estado (0=Operativo 1=Revision 2=Averia 3=Retirado): ");
            int e = leer_entero("");
            t.estado_mant = (EstadoMantenimiento)(e >= 0 && e <= 3 ? e : 0);
            leer_cadena("  Fecha ultima rev. (AAAA-MM-DD, ENTER=ninguna): ",
                        t.fecha_ultima_revision, sizeof(t.fecha_ultima_revision));
            t.tiene_revision = (strlen(t.fecha_ultima_revision) > 0);
            if (insertar_tren_db(t) == 0) {
                log_evento(cfg.log_path, email, "INSERT_TREN", t.nombre_modelo);
                printf("  Tren anyadido correctamente.\n");
            } else printf("  Error al insertar tren (num_serie duplicado?).\n");
            pausar_s();
            break;
        }

        case 3: {
            listar_trenes_db();
            int id_t = leer_entero("\n  ID del tren a modificar: ");
            Tren t = obtener_tren_por_id(id_t);
            if (t.id_t < 0) { printf("  Tren no encontrado.\n"); pausar_s(); break; }
            titulo("MODIFICAR TREN  (ENTER = mantener valor actual)");
            char buf[64];
            leer_cadena("  Nuevo modelo     : ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(t.nombre_modelo, buf, 63);
            leer_cadena("  Nuevo num. serie : ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(t.num_serie, buf, 31);
            int anio = leer_entero("  Nuevo anyo (0=mantener): ");
            if (anio > 0) t.anio_fab = anio;
            leer_cadena("  Nueva fecha rev. (ENTER=mantener): ", buf, sizeof(buf));
            if (strlen(buf) > 0) { strncpy(t.fecha_ultima_revision, buf, 10); t.tiene_revision = 1; }
            if (modificar_tren_db(id_t, t.nombre_modelo, t.num_serie, t.anio_fab,
                                   t.estado_mant,
                                   t.tiene_revision ? t.fecha_ultima_revision : "") == 0) {
                log_evento(cfg.log_path, email, "UPDATE_TREN", "Tren modificado");
                printf("  Tren actualizado.\n");
            } else printf("  Error al modificar.\n");
            pausar_s();
            break;
        }

        case 4: {
            listar_trenes_db();
            int id_t = leer_entero("\n  ID tren: ");
            printf("  Nuevo estado (0=Operativo 1=Revision 2=Averia 3=Retirado): ");
            int e = leer_entero("");
            if (cambiar_estado_tren_db(id_t, (EstadoMantenimiento)(e >= 0 && e <= 3 ? e : 0)) == 0) {
                log_evento(cfg.log_path, email, "ESTADO_TREN", "Estado tren cambiado");
                printf("  Estado actualizado.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }

        case 5: {
            int id_t = leer_entero("  ID tren: ");
            listar_vagones_tren(id_t);
            pausar_s();
            break;
        }

        case 6: {
            Vagon v; memset(&v, 0, sizeof(v));
            listar_trenes_db();
            v.id_tren      = leer_entero("\n  ID tren    : ");
            v.numero_vagon = leer_entero("  Num. vagon  : ");
            char cl[4]; leer_cadena("  Clase (T/B) : ", cl, sizeof(cl));
            cl[0] = (cl[0] == 'b' ? 'B' : (cl[0] == 't' ? 'T' : cl[0]));
            strncpy(v.clase, cl, 3);
            v.capacidad_total = leer_entero("  Capacidad   : ");
            v.vagon_PMR       = leer_entero("  PMR (0/1)   : ");
            if (insertar_vagon_db(v) == 0) {
                log_evento(cfg.log_path, email, "INSERT_VAGON", "Vagon anyadido");
                printf("  Vagon anyadido.\n");
            } else printf("  Error al insertar vagon.\n");
            pausar_s();
            break;
        }

        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 2. GESTIÓN DE TRAYECTOS ─── */
void menu_gestion_trayectos(int id_admin, const char *email) {
    int op;
    do {
        titulo("GESTION DE TRAYECTOS");
        printf("  1. Listar trayectos\n");
        printf("  2. Anyadir trayecto\n");
        printf("  3. Modificar trayecto\n");
        printf("  4. Gestionar paradas intermedias\n");
        printf("  5. Cambiar estado trayecto\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");

        switch (op) {
        case 1:
            listar_trayectos_db();
            pausar_s();
            break;

        case 2: {
            titulo("ANYADIR TRAYECTO");
            listar_trenes_db();
            Trayecto tr; memset(&tr, 0, sizeof(tr));
            tr.id_t = leer_entero("\n  ID tren              : ");
            listar_estaciones_db();
            tr.id_est_origen  = leer_entero("\n  ID estacion origen   : ");
            tr.id_est_destino = leer_entero("  ID estacion destino  : ");
            leer_cadena("  Hora salida (HH:MM)  : ", tr.hora_salida,  sizeof(tr.hora_salida));
            leer_cadena("  Hora llegada (HH:MM) : ", tr.hora_llegada, sizeof(tr.hora_llegada));
            tr.duracion_min = leer_entero("  Duracion (minutos)   : ");
            tr.precio_base  = leer_double("  Precio base Turista  : ");
            leer_cadena("  Dias operacion (ej LMXJVSD): ", tr.dias_operacion, sizeof(tr.dias_operacion));
            if (strlen(tr.dias_operacion) == 0) strcpy(tr.dias_operacion, "LMXJVSD");
            tr.estado = TRAYECTO_ACTIVO;
            if (insertar_trayecto_db(tr) == 0) {
                log_evento(cfg.log_path, email, "INSERT_TRAYECTO", "Trayecto creado");
                printf("  Trayecto creado (+ tarifa base generada automaticamente).\n");
            } else printf("  Error al crear trayecto.\n");
            pausar_s();
            break;
        }

        case 3: {
            listar_trayectos_db();
            int id_tr = leer_entero("\n  ID trayecto a modificar: ");
            Trayecto tr = obtener_trayecto_por_id(id_tr);
            if (tr.id_tr < 0) { printf("  No encontrado.\n"); pausar_s(); break; }
            titulo("MODIFICAR TRAYECTO  (ENTER = mantener)");
            char buf[16];
            leer_cadena("  Nueva hora salida  : ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(tr.hora_salida, buf, 5);
            leer_cadena("  Nueva hora llegada : ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(tr.hora_llegada, buf, 5);
            double p = leer_double("  Nuevo precio base (0=mantener): ");
            if (p > 0.0) tr.precio_base = p;
            leer_cadena("  Nuevos dias (ENTER=mantener): ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(tr.dias_operacion, buf, 7);
            if (modificar_trayecto_db(id_tr, tr.hora_salida, tr.hora_llegada,
                                       tr.precio_base, tr.dias_operacion) == 0) {
                log_evento(cfg.log_path, email, "UPDATE_TRAYECTO", "Trayecto modificado");
                printf("  Trayecto actualizado.\n");
            } else printf("  Error al modificar.\n");
            pausar_s();
            break;
        }

        case 4: {
            listar_trayectos_db();
            int id_tr = leer_entero("\n  ID trayecto: ");
            int sub;
            do {
                titulo("PARADAS INTERMEDIAS");
                listar_paradas_trayecto(id_tr);
                printf("\n  1. Anyadir parada  2. Eliminar parada  0. Volver\n");
                sub = leer_entero("  Opcion: ");
                if (sub == 1) {
                    ParadaIntermedia p; memset(&p, 0, sizeof(p));
                    p.id_tr = id_tr;
                    listar_estaciones_db();
                    p.id_est = leer_entero("\n  ID estacion  : ");
                    p.orden  = leer_entero("  Orden (1,2..): ");
                    leer_cadena("  Hora llegada : ", p.hora_llegada, sizeof(p.hora_llegada));
                    leer_cadena("  Hora salida  : ", p.hora_salida,  sizeof(p.hora_salida));
                    p.anden = leer_entero("  Anden (0=N/D): ");
                    p.tiene_anden = (p.anden > 0);
                    if (insertar_parada_db(p) == 0) {
                        log_evento(cfg.log_path, email, "INSERT_PARADA", "Parada intermedia anyadida");
                        printf("  Parada anyadida.\n");
                    } else printf("  Error.\n");
                } else if (sub == 2) {
                    int id_p = leer_entero("  ID parada a eliminar: ");
                    if (eliminar_parada_db(id_p) == 0) {
                        log_evento(cfg.log_path, email, "DEL_PARADA", "Parada eliminada");
                        printf("  Parada eliminada.\n");
                    } else printf("  Error.\n");
                }
                if (sub != 0) pausar_s();
            } while (sub != 0);
            break;
        }

        case 5: {
            listar_trayectos_db();
            int id_tr = leer_entero("\n  ID trayecto: ");
            printf("  Nuevo estado (0=Inactivo 1=Activo): ");
            int est = leer_entero("");
            if (cambiar_estado_trayecto_db(id_tr, (EstadoTrayecto)est) == 0) {
                log_evento(cfg.log_path, email, "ESTADO_TRAYECTO", "Estado cambiado");
                printf("  Estado actualizado.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }

        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 3. GESTIÓN DE ESTACIONES ─── */
void menu_gestion_estaciones(int id_admin, const char *email) {
    int op;
    do {
        titulo("GESTION DE ESTACIONES");
        printf("  1. Listar estaciones\n");
        printf("  2. Anyadir estacion\n");
        printf("  3. Modificar estacion\n");
        printf("  4. Activar/Desactivar Sala Club\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");

        switch (op) {
        case 1:
            listar_estaciones_db();
            pausar_s();
            break;

        case 2: {
            titulo("ANYADIR ESTACION");
            Estacion e; memset(&e, 0, sizeof(e));
            leer_cadena("  Nombre        : ", e.nombre,     sizeof(e.nombre));
            leer_cadena("  Codigo GTFS   : ", e.codigo_gtfs,sizeof(e.codigo_gtfs));
            leer_cadena("  Ciudad        : ", e.ciudad,     sizeof(e.ciudad));
            leer_cadena("  Provincia     : ", e.provincia,  sizeof(e.provincia));
            e.latitud       = leer_double("  Latitud       : ");
            e.longitud      = leer_double("  Longitud      : ");
            e.num_andenes   = leer_entero("  Num. andenes  : ");
            e.tiene_sala_club = leer_entero("  Sala Club (0/1): ");
            if (insertar_estacion_db(e) == 0) {
                log_evento(cfg.log_path, email, "INSERT_ESTACION", e.nombre);
                printf("  Estacion creada.\n");
            } else printf("  Error (nombre duplicado?).\n");
            pausar_s();
            break;
        }

        case 3: {
            listar_estaciones_db();
            int id_e = leer_entero("\n  ID estacion a modificar: ");
            Estacion e = obtener_estacion_por_id(id_e);
            if (e.id_est < 0) { printf("  No encontrada.\n"); pausar_s(); break; }
            titulo("MODIFICAR ESTACION  (ENTER = mantener)");
            char buf[128];
            leer_cadena("  Nuevo nombre    : ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(e.nombre, buf, 127);
            leer_cadena("  Nueva ciudad    : ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(e.ciudad, buf, 63);
            leer_cadena("  Nueva provincia : ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(e.provincia, buf, 63);
            int na = leer_entero("  Nuevos andenes (0=mantener): ");
            if (na > 0) e.num_andenes = na;
            if (modificar_estacion_db(id_e, e.nombre, e.ciudad, e.provincia, e.num_andenes) == 0) {
                log_evento(cfg.log_path, email, "UPDATE_ESTACION", e.nombre);
                printf("  Estacion actualizada.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }

        case 4: {
            listar_estaciones_db();
            int id_e = leer_entero("\n  ID estacion: ");
            if (toggle_sala_club_db(id_e) == 0) {
                log_evento(cfg.log_path, email, "SALA_CLUB", "Estado sala club toggled");
                printf("  Estado Sala Club cambiado.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }

        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 4. GESTIÓN DE PERSONAL ─── */
void menu_gestion_personal(int id_admin, const char *email) {
    int op;
    do {
        titulo("GESTION DE PERSONAL");
        printf("  1. Listar personal (maquinistas)\n");
        printf("  2. Anyadir empleado / maquinista\n");
        printf("  3. Modificar datos de empleado\n");
        printf("  4. Dar de baja empleado\n");
        printf("  5. Asignar empleado a servicio operativo\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");

        switch (op) {
        case 1:
            listar_empleados_db();
            pausar_s();
            break;

        case 2: {
            titulo("ANYADIR EMPLEADO");
            char nombre[64], apellido[64], dni[16], em[128];
            char telf[20], fn[11], pass[256];
            leer_cadena("  Nombre         : ", nombre,   sizeof(nombre));
            leer_cadena("  Apellido       : ", apellido, sizeof(apellido));
            leer_cadena("  DNI            : ", dni,      sizeof(dni));
            leer_cadena("  Email          : ", em,       sizeof(em));
            leer_cadena("  Telefono       : ", telf,     sizeof(telf));
            leer_cadena("  F.Nac(AAAA-MM-DD): ", fn,    sizeof(fn));
            leer_cadena("  Contrasena     : ", pass,     sizeof(pass));
            Usuario u = crearUsuario(nombre, apellido, dni, em,
                                      telf, pass, fn, ROL_EMPLEADO);
            if (insertar_usuario_db(u) == 0) {
                log_evento(cfg.log_path, email, "INSERT_EMPLEADO", em);
                printf("  Empleado creado correctamente.\n");
            } else printf("  Error (DNI/email duplicado?).\n");
            pausar_s();
            break;
        }

        case 3: {
            listar_empleados_db();
            int id_u = leer_entero("\n  ID empleado a modificar: ");
            char campo[32], valor[128];
            printf("  Campos disponibles: nombre, apellido, email, telf\n");
            leer_cadena("  Campo   : ", campo, sizeof(campo));
            leer_cadena("  Nuevo valor: ", valor, sizeof(valor));
            if (modificar_usuario_db(id_u, campo, valor) == 0) {
                log_evento(cfg.log_path, email, "UPDATE_EMPLEADO", "Datos empleado modificados");
                printf("  Actualizado.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }

        case 4: {
            listar_empleados_db();
            int id_u = leer_entero("\n  ID empleado a dar de baja: ");
            printf("  Confirmar baja (1=Si 0=No): ");
            if (leer_entero("") == 1 && deshabilitar_usuario_db(id_u) == 0) {
                log_evento(cfg.log_path, email, "BAJA_EMPLEADO", "Baja registrada");
                printf("  Baja registrada.\n");
            } else printf("  Operacion cancelada.\n");
            pausar_s();
            break;
        }

        case 5: {
            titulo("ASIGNAR PERSONAL A SERVICIO");
            listar_servicios_db("", 0);
            int id_serv = leer_entero("\n  ID servicio  : ");
            listar_empleados_db();
            int id_u2   = leer_entero("\n  ID empleado  : ");
            listar_trenes_db();
            int id_t    = leer_entero("\n  ID tren      : ");
            printf("  Rol (0=Conductor 1=Revisor 2=Tecnico 3=Servicio): ");
            int rol = leer_entero("");
            AsignacionPersonal a; memset(&a, 0, sizeof(a));
            a.id_serv = id_serv; a.id_u = id_u2; a.id_t = id_t;
            a.rol_servicio = (RolServicio)(rol >= 0 && rol <= 3 ? rol : 0);
            leer_cadena("  Observaciones (ENTER=ninguna): ", a.observaciones, sizeof(a.observaciones));
            a.tiene_observaciones = (strlen(a.observaciones) > 0);
            if (insertar_asignacion_db(a) == 0) {
                log_evento(cfg.log_path, email, "ASIGNACION", "Personal asignado a servicio");
                printf("  Personal asignado correctamente.\n");
            } else printf("  Error en la asignacion.\n");
            pausar_s();
            break;
        }

        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 5. GESTIÓN DE PASAJEROS ─── */
void menu_gestion_pasajeros(int id_admin, const char *email) {
    int op;
    do {
        titulo("GESTION DE PASAJEROS");
        printf("  1. Buscar pasajero (DNI/nombre)\n");
        printf("  2. Ver reservas de un pasajero\n");
        printf("  3. Modificar datos de pasajero\n");
        printf("  4. Deshabilitar cuenta de pasajero\n");
        printf("  5. Ver/cambiar descuento de un pasajero\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");

        switch (op) {
        case 1: {
            char busq[64];
            leer_cadena("  DNI o nombre a buscar: ", busq, sizeof(busq));
            buscar_usuario_db(busq);
            pausar_s();
            break;
        }
        case 2: {
            int id_u = leer_entero("  ID pasajero: ");
            listar_reservas_usuario(id_u);
            pausar_s();
            break;
        }
        case 3: {
            int id_u = leer_entero("  ID pasajero: ");
            char campo[32], valor[128];
            printf("  Campos: nombre, apellido, email, telf\n");
            leer_cadena("  Campo     : ", campo, sizeof(campo));
            leer_cadena("  Nuevo valor: ", valor, sizeof(valor));
            if (modificar_usuario_db(id_u, campo, valor) == 0) {
                log_evento(cfg.log_path, email, "UPDATE_PASAJERO", "Datos pasajero modificados");
                printf("  Actualizado.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }
        case 4: {
            int id_u = leer_entero("  ID pasajero a deshabilitar: ");
            printf("  Confirmar (1=Si): ");
            if (leer_entero("") == 1 && deshabilitar_usuario_db(id_u) == 0) {
                log_evento(cfg.log_path, email, "DESHABILITAR", "Cuenta pasajero deshabilitada");
                printf("  Cuenta deshabilitada.\n");
            } else printf("  Cancelado.\n");
            pausar_s();
            break;
        }
        case 5: {
            int id_u = leer_entero("  ID pasajero: ");
            TipoDescuento actual = obtener_descuento_usuario(id_u);
            const char *dn[] = {"NINGUNO","JOVEN -20%","DORADA -40%","NUMEROSA -20%","ABONO -50%"};
            printf("  Descuento actual: %s\n", dn[actual]);
            printf("  Nuevo descuento (0=Ninguno 1=Joven 2=Dorada 3=Numerosa 4=Abono): ");
            int nd = leer_entero("");
            if (nd >= 0 && nd <= 4 && actualizar_descuento_usuario(id_u, (TipoDescuento)nd) == 0) {
                log_evento(cfg.log_path, email, "UPDATE_DESCUENTO", dn[nd]);
                printf("  Descuento actualizado a: %s\n", dn[nd]);
            } else printf("  Error o valor no valido.\n");
            pausar_s();
            break;
        }
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 6. GESTIÓN DE SERVICIOS OPERATIVOS ─── */
void menu_gestion_servicios(int id_admin, const char *email) {
    int op;
    do {
        titulo("GESTION DE SERVICIOS OPERATIVOS");
        printf("  1. Listar servicios (filtro por fecha)\n");
        printf("  2. Crear servicio operativo\n");
        printf("  3. Ver detalle de servicio\n");
        printf("  4. Cancelar servicio\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");

        switch (op) {
        case 1: {
            char fd[11] = "";
            leer_cadena("  Filtrar por fecha (AAAA-MM-DD, ENTER=todas): ", fd, sizeof(fd));
            listar_servicios_db(fd, 0);
            pausar_s();
            break;
        }
        case 2: {
            titulo("CREAR SERVICIO OPERATIVO");
            listar_trayectos_db();
            ServicioOperativo s; memset(&s, 0, sizeof(s));
            s.id_tr = leer_entero("\n  ID trayecto  : ");
            leer_cadena("  Fecha (AAAA-MM-DD): ", s.fecha, sizeof(s.fecha));
            s.estado_serv = SERVICIO_PROGRAMADO;
            if (insertar_servicio_db(s) == 0) {
                log_evento(cfg.log_path, email, "INSERT_SERVICIO", s.fecha);
                printf("  Servicio operativo creado.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }
        case 3: {
            listar_servicios_db("", 0);
            int id_s = leer_entero("\n  ID servicio: ");
            titulo("DETALLE DE SERVICIO");
            ServicioOperativo s = obtener_servicio_por_id(id_s);
            if (s.id_serv < 0) { printf("  No encontrado.\n"); pausar_s(); break; }
            printf("  ID Servicio: %d\n", s.id_serv);
            printf("  Fecha      : %s\n", s.fecha);
            const char *ests[] = {"PROGRAMADO","EN_CURSO","FINALIZADO","CANCELADO"};
            printf("  Estado     : %s\n", ests[s.estado_serv]);
            if (s.tiene_hora_inicio) printf("  Inicio real: %s\n", s.hora_inicio_real);
            if (s.tiene_hora_fin)    printf("  Fin real   : %s\n", s.hora_fin_real);
            if (s.minutos_retraso > 0)
                printf("  Retraso    : %d min — %s\n",
                       s.minutos_retraso,
                       s.tiene_causa ? s.causa_retraso : "sin causa registrada");
            printf("\n  — Personal asignado —\n");
            listar_asignaciones_servicio(id_s);
            pausar_s();
            break;
        }
        case 4: {
            listar_servicios_db("", 0);
            int id_s = leer_entero("\n  ID servicio a cancelar: ");
            printf("  Confirmar (1=Si): ");
            if (leer_entero("") == 1 && cancelar_servicio_db(id_s) == 0) {
                log_evento(cfg.log_path, email, "CANCELAR_SERV", "Servicio cancelado");
                printf("  Servicio cancelado.\n");
            } else printf("  Cancelado.\n");
            pausar_s();
            break;
        }
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 7. GESTIÓN DE TARIFAS ─── */
void menu_gestion_tarifas(int id_admin, const char *email) {
    int op;
    do {
        titulo("GESTION DE TARIFAS");
        printf("  1. Ver tarifas actuales\n");
        printf("  2. Modificar precio base de trayecto\n");
        printf("  3. Modificar coeficiente Business de trayecto\n");
        printf("  4. Modificar suplemento bici/esqui (global)\n");
        printf("  5. Modificar precio exceso equipaje kg (global)\n");
        printf("  6. Ver descuentos configurados\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");

        switch (op) {
        case 1:
            listar_tarifas_db();
            pausar_s();
            break;
        case 2: {
            listar_trayectos_db();
            int id_tr = leer_entero("\n  ID trayecto: ");
            double p = leer_double("  Nuevo precio base Turista (EUR): ");
            if (modificar_precio_base_trayecto(id_tr, p) == 0) {
                log_evento(cfg.log_path, email, "UPD_TARIFA_PRECIO", "Precio base modificado");
                printf("  Precio actualizado.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }
        case 3: {
            listar_trayectos_db();
            int id_tr = leer_entero("\n  ID trayecto: ");
            double c = leer_double("  Nuevo coef. Business (ej: 1.80): ");
            if (modificar_coef_business_db(id_tr, c) == 0) {
                log_evento(cfg.log_path, email, "UPD_TARIFA_BUS", "Coef Business modificado");
                printf("  Coeficiente Business actualizado.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }
        case 4: {
            printf("  Suplemento actual: %.2f EUR\n", cfg.suplemento_bici);
            double p = leer_double("  Nuevo suplemento bici/esqui (EUR): ");
            if (modificar_suplemento_bici_db(p) == 0) {
                guardar_config("./data/config.cfg", &cfg);
                log_evento(cfg.log_path, email, "UPD_SUPL_BICI", "Suplemento bici modificado");
                printf("  Suplemento actualizado y guardado en config.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }
        case 5: {
            printf("  Precio exceso actual: %.2f EUR/kg\n", cfg.exceso_kg_precio);
            double p = leer_double("  Nuevo precio exceso kg (EUR/kg): ");
            if (modificar_exceso_kg_db(p) == 0) {
                guardar_config("./data/config.cfg", &cfg);
                log_evento(cfg.log_path, email, "UPD_EXCESO_KG", "Precio exceso kg modificado");
                printf("  Precio exceso actualizado y guardado en config.\n");
            } else printf("  Error.\n");
            pausar_s();
            break;
        }
        case 6:
            printf("\n  DESCUENTOS DEL SISTEMA:\n");
            printf("  %-15s | %-8s | %s\n", "TIPO","PCT","CRITERIO");
            printf("  ----------------+----------+------------------------------\n");
            printf("  %-15s | %7.1f%% | Menores de 26 anios (automatico)\n", "JOVEN",    20.0);
            printf("  %-15s | %7.1f%% | Mayores de 60 anios (automatico)\n", "DORADA",   40.0);
            printf("  %-15s | %7.1f%% | Familia numerosa (manual)\n",        "NUMEROSA", 20.0);
            printf("  %-15s | %7.1f%% | Cercanias/media distancia\n",        "ABONO",    50.0);
            printf("\n  Coef. Business actual : x%.2f\n", cfg.coef_business);
            printf("  Exceso kg actual      : %.2f EUR/kg\n", cfg.exceso_kg_precio);
            printf("  Suplemento bici actual: %.2f EUR\n", cfg.suplemento_bici);
            printf("  Menu turista actual   : %.2f EUR\n", cfg.menu_turista);
            pausar_s();
            break;
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 8. IMPORTAR GTFS ─── */
void menu_importar_gtfs(int id_admin, const char *email) {
    int op;
    do {
        titulo("IMPORTAR DATOS GTFS  (formato RENFE)");
        printf("  1. Importar desde directorio local (stops.txt)\n");
        printf("  2. Ver resumen de la ultima importacion\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
        case 1: {
            char ruta[256];
            leer_cadena("  Ruta del directorio GTFS: ", ruta, sizeof(ruta));
            printf("  Importando datos RENFE...\n");
            if (importar_gtfs(ruta) == 0) {
                log_evento(cfg.log_path, email, "IMPORT_GTFS", ruta);
                printf("  Importacion completada correctamente.\n");
            } else {
                printf("  ERROR. Verifique que la ruta contiene stops.txt en formato GTFS.\n");
                printf("  Descargue los datos de: https://www.renfe.com/content/dam/renfe/es/General/ficheros/opendata/google_transit.zip\n");
            }
            pausar_s();
            break;
        }
        case 2:
            titulo("RESUMEN ULTIMA IMPORTACION");
            resumen_ultima_importacion();
            pausar_s();
            break;
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 9. INCIDENCIAS ─── */
void menu_incidencias(int id_admin, const char *email) {
    int op;
    do {
        titulo("GESTION DE INCIDENCIAS");
        printf("  1. Listar TODAS las incidencias\n");
        printf("  2. Listar incidencias ABIERTAS\n");
        printf("  3. Ver detalle de incidencia\n");
        printf("  4. Resolver incidencia\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
        case 1:
            listar_incidencias_db(INCIDENCIA_ABIERTA, 1);
            pausar_s();
            break;
        case 2:
            listar_incidencias_db(INCIDENCIA_ABIERTA, 0);
            pausar_s();
            break;
        case 3: {
            int id_i = leer_entero("  ID incidencia: ");
            ver_detalle_incidencia(id_i);
            pausar_s();
            break;
        }
        case 4: {
            listar_incidencias_db(INCIDENCIA_ABIERTA, 0);
            int id_i = leer_entero("\n  ID incidencia a resolver: ");
            if (resolver_incidencia_db(id_i, id_admin) == 0) {
                log_evento(cfg.log_path, email, "RESOLVER_INC", "Incidencia resuelta");
                printf("  Incidencia marcada como RESUELTA.\n");
            } else printf("  Error o incidencia no encontrada.\n");
            pausar_s();
            break;
        }
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 10. INFORMES ─── */
void menu_informes(int id_admin, const char *email) {
    int op;
    do {
        titulo("INFORMES");
        printf("  1. Ocupacion por tren\n");
        printf("  2. Ingresos por trayecto\n");
        printf("  3. Incidencias por periodo\n");
        printf("  4. Empleados mas activos\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
        case 1: {
            listar_trenes_db();
            int id_t = leer_entero("\n  ID tren: ");
            informe_ocupacion_tren(id_t);
            pausar_s();
            break;
        }
        case 2: {
            listar_trayectos_db();
            int id_tr = leer_entero("\n  ID trayecto: ");
            informe_ingresos_trayecto(id_tr);
            pausar_s();
            break;
        }
        case 3: {
            char fi[11], ff[11];
            leer_cadena("  Fecha inicio (AAAA-MM-DD): ", fi, sizeof(fi));
            leer_cadena("  Fecha fin    (AAAA-MM-DD): ", ff, sizeof(ff));
            informe_incidencias_periodo(fi, ff);
            pausar_s();
            break;
        }
        case 4:
            informe_empleados_activos();
            pausar_s();
            break;
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 11. LOGS ─── */
void menu_logs(int id_admin, const char *email) {
    int op;
    do {
        titulo("LOGS DEL SISTEMA");
        printf("  1. Ver logs (con filtros)\n");
        printf("  2. Exportar logs a fichero\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
        case 1: {
            char f_fecha[32] = "", f_user[64] = "", f_niv[16] = "";
            leer_cadena("  Filtro fecha   (ENTER=todos): ", f_fecha, sizeof(f_fecha));
            leer_cadena("  Filtro usuario (ENTER=todos): ", f_user,  sizeof(f_user));
            leer_cadena("  Filtro nivel   (ENTER=todos): ", f_niv,   sizeof(f_niv));
            consultar_logs_db(
                strlen(f_fecha) > 0 ? f_fecha : NULL,
                strlen(f_user)  > 0 ? f_user  : NULL,
                strlen(f_niv)   > 0 ? f_niv   : NULL);
            pausar_s();
            break;
        }
        case 2: {
            char destino[256];
            leer_cadena("  Ruta destino (ej: ./logs/export.log): ", destino, sizeof(destino));
            exportar_logs(cfg.log_path, destino);
            pausar_s();
            break;
        }
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── 12. CONFIGURACIÓN ─── */
void menu_configuracion(int id_admin, const char *email) {
    int op;
    do {
        titulo("CONFIGURACION DEL SISTEMA");
        printf("  1. Ver configuracion actual\n");
        printf("  2. Modificar parametros de sistema\n");
        printf("  3. Modificar tarifas globales\n");
        printf("  4. Cambiar contrasena del admin\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
        case 1:
            mostrar_config(&cfg);
            pausar_s();
            break;
        case 2: {
            titulo("MODIFICAR PARAMETROS  (ENTER = mantener)");
            char buf[256];
            leer_cadena("  Nueva ruta BD   : ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(cfg.db_path, buf, 255);
            leer_cadena("  Nueva ruta log  : ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(cfg.log_path, buf, 255);
            int p = leer_entero("  Nuevo puerto servidor (0=mantener): ");
            if (p > 0) cfg.puerto_servidor = p;
            leer_cadena("  Nuevo host servidor (ENTER=mantener): ", buf, sizeof(buf));
            if (strlen(buf) > 0) strncpy(cfg.host_servidor, buf, 63);
            guardar_config("./data/config.cfg", &cfg);
            log_evento(cfg.log_path, email, "CONFIG", "Parametros sistema modificados");
            printf("  Configuracion guardada.\n");
            pausar_s();
            break;
        }
        case 3: {
            titulo("MODIFICAR TARIFAS GLOBALES");
            printf("  Valores actuales:\n");
            printf("    Coef. Business  : %.2f\n", cfg.coef_business);
            printf("    Exceso kg       : %.2f EUR/kg\n", cfg.exceso_kg_precio);
            printf("    Supl. bici/esqui: %.2f EUR\n", cfg.suplemento_bici);
            printf("    Menu turista    : %.2f EUR\n", cfg.menu_turista);
            printf("\n  Nuevos valores (0 = mantener):\n");
            double v;
            v = leer_double("  Coef. Business  : ");
            if (v > 0.0) cfg.coef_business = v;
            v = leer_double("  Exceso kg       : ");
            if (v > 0.0) cfg.exceso_kg_precio = v;
            v = leer_double("  Supl. bici/esqui: ");
            if (v > 0.0) cfg.suplemento_bici = v;
            v = leer_double("  Menu turista     : ");
            if (v > 0.0) cfg.menu_turista = v;
            guardar_config("./data/config.cfg", &cfg);
            log_evento(cfg.log_path, email, "CONFIG_TARIFAS", "Tarifas globales modificadas");
            printf("  Tarifas globales actualizadas y guardadas.\n");
            pausar_s();
            break;
        }
        case 4: {
            char actual[256], nueva[256], nueva2[256];
            leer_cadena("  Contrasena actual    : ", actual, sizeof(actual));
            if (!comprobar_contrasenia(email, actual)) {
                printf("  Contrasena actual incorrecta.\n");
            } else {
                leer_cadena("  Nueva contrasena     : ", nueva,  sizeof(nueva));
                leer_cadena("  Repite nueva contras.: ", nueva2, sizeof(nueva2));
                if (strcmp(nueva, nueva2) != 0) {
                    printf("  Las contrasenas no coinciden.\n");
                } else if (cambiar_contrasenia_db(email, nueva) == 0) {
                    log_evento(cfg.log_path, email, "CAMBIO_PASS", "Contrasena admin cambiada");
                    printf("  Contrasena cambiada correctamente.\n");
                } else printf("  Error al cambiar contrasena.\n");
            }
            pausar_s();
            break;
        }
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ============================================================
 *  ╔══════════════════════════════════╗
 *  ║    MENÚS PASAJERO  (5 secs)      ║
 *  ╚══════════════════════════════════╝
 * ============================================================ */

void menu_principal_pasajero(int id_u, const char *email) {
    int op;
    do {
        titulo("MENU PRINCIPAL  –  PASAJERO");
        int pts = obtener_puntos_fidelidad(id_u);
        TipoDescuento desc = obtener_descuento_usuario(id_u);
        const char *dn[] = {"Ninguno","Joven -20%","Dorada -40%","Numerosa -20%","Abono -50%"};
        printf("  Puntos: %d pts  |  Descuento: %s\n\n", pts, dn[desc]);
        printf("  1. Buscar trayecto y reservar billete\n");
        printf("  2. Mis reservas\n");
        printf("  3. Mis puntos de fidelizacion\n");
        printf("  4. Mis datos personales\n");
        printf("  0. Cerrar sesion\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
            case 1: menu_buscar_trayecto(id_u);          break;
            case 2: menu_mis_reservas(id_u);             break;
            case 3: menu_puntos_fidelizacion(id_u);      break;
            case 4: menu_mis_datos_pasajero(id_u, email);break;
            case 0:
                log_evento(cfg.log_path, email, "LOGOUT", "Sesion pasajero cerrada");
                printf("\n  Hasta luego.\n");
                break;
            default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── BUSCAR TRAYECTO Y RESERVAR ─── */
void menu_buscar_trayecto(int id_u) {
    titulo("BUSCAR TRAYECTO");
    listar_estaciones_db();

    int id_orig = leer_entero("\n  ID estacion origen : ");
    int id_dest = leer_entero("  ID estacion destino: ");
    char fecha[11];
    leer_cadena("  Fecha viaje (AAAA-MM-DD): ", fecha, sizeof(fecha));
    char clase[4];
    leer_cadena("  Clase (T=Turista B=Business): ", clase, sizeof(clase));
    if (clase[0] == 'b' || clase[0] == 'B') strcpy(clase, "B");
    else strcpy(clase, "T");

    int n = buscar_trayectos_db(id_orig, id_dest, fecha, clase);
    if (n == 0) { pausar_s(); return; }

    int id_tr = leer_entero("\n  Selecciona ID trayecto (0=cancelar): ");
    if (id_tr == 0) return;

    Trayecto tr = obtener_trayecto_por_id(id_tr);
    if (tr.id_tr < 0) { printf("  Trayecto no valido.\n"); pausar_s(); return; }

    /* Paradas intermedias */
    printf("\n  Paradas intermedias del trayecto:\n");
    listar_paradas_trayecto(id_tr);

    /* Selección de vagón y asiento */
    printf("\n  VAGONES DISPONIBLES DEL TREN:\n");
    listar_vagones_tren(tr.id_t);
    int num_vagon = leer_entero("\n  Numero de vagon (segun lista): ");
    mostrar_mapa_asientos(id_tr, fecha, num_vagon);
    printf("  Introduce el NUMERO que aparece en la celda libre del mapa.\n");
    printf("  (Columna A=1a col, B=2a col, C=3a col, D=4a col; fila F01=primera fila, etc.)\n");
    int num_asiento = leer_entero("  Numero de asiento: ");

    if (!asiento_libre(id_tr, fecha, num_vagon, num_asiento)) {
        printf("\n  *** Ese asiento esta OCUPADO. Elige otro. ***\n");
        pausar_s(); return;
    }

    /* Descuento del pasajero */
    TipoDescuento desc = obtener_descuento_usuario(id_u);
    const char *dn[] = {"Ninguno","Joven -20%","Dorada -40%","Numerosa -20%","Abono -50%"};
    printf("\n  Descuento aplicado: %s\n", dn[desc]);

    /* Equipaje */
    double supl_eq = 0.0;
    Equipaje eq; memset(&eq, 0, sizeof(eq));
    int tiene_eq = 0;
    printf("\n  EQUIPAJE EXTRA:\n");
    printf("  0. Sin equipaje extra (maleta de mano incluida)\n");
    printf("  1. Maleta bodega (limite: %s kg, exceso: %.2f EUR/kg)\n",
           strcmp(clase,"B")==0 ? "25" : "15", cfg.exceso_kg_precio);
    printf("  2. Bicicleta (%.2f EUR)\n", cfg.suplemento_bici);
    printf("  3. Esqui/snowboard (%.2f EUR)\n", cfg.suplemento_bici);
    int eq_op = leer_entero("  Opcion equipaje: ");

    if (eq_op == 1) {
        double peso = leer_double("  Peso de la maleta (kg): ");
        double limite = strcmp(clase,"B")==0 ? 25.0 : 15.0;
        supl_eq = peso > limite ? (peso - limite) * cfg.exceso_kg_precio : 0.0;
        eq.tipo = EQUIPAJE_BODEGA; eq.peso_kg = peso;
        eq.exceso_kg = (supl_eq > 0.0) ? (peso - limite) : 0.0;
        eq.suplemento_pago = supl_eq;
        tiene_eq = 1;
        if (supl_eq > 0.0)
            printf("  Suplemento maleta: +%.2f EUR (exceso %.1f kg)\n",
                   supl_eq, eq.exceso_kg);
        else
            printf("  Maleta dentro del limite. Sin suplemento.\n");
    } else if (eq_op == 2) {
        supl_eq = cfg.suplemento_bici;
        eq.tipo = EQUIPAJE_BICI; eq.suplemento_pago = supl_eq;
        tiene_eq = 1;
        printf("  Suplemento bicicleta: +%.2f EUR\n", supl_eq);
    } else if (eq_op == 3) {
        supl_eq = cfg.suplemento_bici;
        eq.tipo = EQUIPAJE_ESQUI; eq.suplemento_pago = supl_eq;
        tiene_eq = 1;
        printf("  Suplemento esqui: +%.2f EUR\n", supl_eq);
    }

    /* Servicio a bordo (solo Turista paga menú) */
    double supl_menu = 0.0;
    if (strcmp(clase, "B") == 0) {
        printf("\n  [Business] Menu a bordo incluido.\n");
    } else {
        printf("\n  MENU A BORDO (+%.2f EUR):\n", cfg.menu_turista);
        printf("  0. No\n  1. Si — Estandar\n  2. Si — Vegano\n");
        printf("  3. Si — Gluten-Free\n  4. Si — Halal\n");
        int mo = leer_entero("  Opcion menu: ");
        if (mo > 0) {
            supl_menu = cfg.menu_turista;
            printf("  Menu seleccionado: +%.2f EUR\n", supl_menu);
        }
    }

    /* Precio sin puntos */
    double precio_final = calcular_precio_final(id_tr, clase, desc,
                                                 supl_eq + supl_menu);
    double pct_d[] = {0, 20, 40, 20, 50};

    /* ── Canjear puntos de fidelizacion antes de pagar ── */
    int pts_disponibles = obtener_puntos_fidelidad(id_u);
    double descuento_puntos = 0.0;
    int puntos_canjeados = 0;
    if (pts_disponibles >= 10) {
        printf("\n  =========================================\n");
        printf("  PUNTOS DE FIDELIZACION\n");
        printf("  =========================================\n");
        printf("  Tienes %d puntos = %.2f EUR de descuento disponibles.\n",
               pts_disponibles, pts_disponibles / 10.0);
        printf("  Precio actual del billete: %.2f EUR\n", precio_final);
        int max_canjeable = (int)(precio_final * 10.0); /* no gastar mas que el precio */
        if (max_canjeable > pts_disponibles) max_canjeable = pts_disponibles;
        /* redondear a multiplo de 10 hacia abajo */
        max_canjeable = (max_canjeable / 10) * 10;
        printf("  Maximo canjeable ahora: %d puntos (= %.2f EUR)\n",
               max_canjeable, max_canjeable / 10.0);
        int canjear = leer_entero("  Puntos a canjear (0=ninguno, multiplos de 10): ");
        if (canjear > 0 && canjear <= pts_disponibles &&
            canjear % 10 == 0 && canjear <= max_canjeable) {
            descuento_puntos = canjear / 10.0;
            puntos_canjeados = canjear;
            precio_final -= descuento_puntos;
            if (precio_final < 0.0) precio_final = 0.0;
            printf("  Descuento por puntos aplicado: -%.2f EUR\n", descuento_puntos);
        } else if (canjear != 0) {
            printf("  Cantidad no valida. No se canjearan puntos.\n");
        }
    }

    printf("\n  =========================================\n");
    printf("  RESUMEN DE LA RESERVA\n");
    printf("  =========================================\n");
    printf("  Trayecto   : %d  |  Fecha: %s\n", id_tr, fecha);
    printf("  Clase      : %s\n", strcmp(clase,"B")==0?"Business":"Turista");
    printf("  Vagon %d  |  Asiento %d\n", num_vagon, num_asiento);
    printf("  Descuento  : %s (%.0f%%)\n", dn[desc], pct_d[desc]);
    if (supl_eq > 0.0)   printf("  Supl. equip: +%.2f EUR\n", supl_eq);
    if (supl_menu > 0.0) printf("  Supl. menu : +%.2f EUR\n", supl_menu);
    if (puntos_canjeados > 0)
        printf("  Dto. puntos: -%d pts = -%.2f EUR\n", puntos_canjeados, descuento_puntos);
    printf("  PRECIO TOTAL: %.2f EUR\n", precio_final);
    printf("  =========================================\n");

    int conf = leer_entero("  Confirmar reserva (1=Si 0=No): ");
    if (conf != 1) { printf("  Reserva cancelada.\n"); pausar_s(); return; }

    /* Insertar reserva */
    Reserva r; memset(&r, 0, sizeof(r));
    r.id_u         = id_u;
    r.id_tr        = id_tr;
    strncpy(r.fecha_viaje, fecha, 10);
    strncpy(r.clase,       clase,  3);
    r.num_vagon    = num_vagon;
    r.num_asiento  = num_asiento;
    r.precio_base  = tr.precio_base;
    r.descuento_pct= pct_d[desc];
    r.precio_final = precio_final;
    r.estado       = RESERVA_CONFIRMADA;
    generar_codigo_validacion(r.codigo_validacion, 12);

    int id_res = insertar_reserva_db(r);
    if (id_res > 0) {
        /* Descontar puntos canjeados */
        if (puntos_canjeados > 0)
            actualizar_puntos_fidelidad(id_u, -puntos_canjeados);
        /* Guardar equipaje si lo hay */
        if (tiene_eq) { eq.id_res = id_res; insertar_equipaje_db(eq); }
        log_evento(cfg.log_path, NULL, "RESERVA", r.codigo_validacion);
        printf("\n  *** RESERVA CONFIRMADA ***\n");
        printf("  Codigo de validacion: %s\n", r.codigo_validacion);
        printf("  ID Reserva          : %d\n", id_res);
        printf("  Puntos acumulados   : +%d pts\n", (int)precio_final);
    } else {
        printf("  Error al crear la reserva.\n");
    }
    pausar_s();
}

/* ─── MIS RESERVAS ─── */
void menu_mis_reservas(int id_u) {
    int op;
    do {
        titulo("MIS RESERVAS");
        printf("  1. Ver reservas activas\n");
        printf("  2. Historial completo de viajes\n");
        printf("  3. Cancelar una reserva activa\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
        case 1:
            listar_reservas_activas_usuario(id_u);
            pausar_s();
            break;
        case 2:
            listar_historial_usuario(id_u);
            pausar_s();
            break;
        case 3: {
            listar_reservas_activas_usuario(id_u);
            int id_res = leer_entero("\n  ID reserva a cancelar (0=no): ");
            if (id_res > 0) {
                if (cancelar_reserva_db(id_res, id_u) == 0) {
                    log_evento(cfg.log_path, NULL, "CANCELAR_RES", "Reserva cancelada por pasajero");
                    printf("  Reserva cancelada correctamente.\n");
                } else printf("  No se pudo cancelar (ya cancelada o no te pertenece).\n");
            }
            pausar_s();
            break;
        }
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── PUNTOS DE FIDELIZACIÓN ─── */
void menu_puntos_fidelizacion(int id_u) {
    int op;
    do {
        titulo("MIS PUNTOS DE FIDELIZACION");
        int saldo = obtener_puntos_fidelidad(id_u);
        printf("  Saldo actual: %d puntos\n", saldo);
        printf("  (10 puntos = 1 EUR de descuento al canjear)\n");
        printf("  Los puntos se canjean automaticamente al hacer una nueva reserva.\n\n");
        printf("  1. Ver historico de puntos\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
        case 1:
            listar_historial_puntos(id_u);
            pausar_s();
            break;
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── MIS DATOS PASAJERO ─── */
void menu_mis_datos_pasajero(int id_u, const char *email) {
    int op;
    do {
        titulo("MIS DATOS PERSONALES");
        printf("  1. Ver perfil\n");
        printf("  2. Modificar datos\n");
        printf("  3. Cambiar contrasena\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
        case 1: {
            Usuario u = obtener_usuario_por_id(id_u);
            const char *dn[] = {"Ninguno","Joven -20%","Dorada -40%","Numerosa -20%","Abono -50%"};
            printf("\n  =========================================\n");
            printf("  PERFIL DE USUARIO\n");
            printf("  =========================================\n");
            printf("  Nombre    : %s %s\n", u.nombre, u.apellido);
            printf("  DNI       : %s\n",    u.dni);
            printf("  Email     : %s\n",    u.email);
            printf("  Telefono  : %s\n",    u.telf);
            printf("  F. Nac.   : %s\n",    u.fecha_nac);
            printf("  Puntos    : %d pts\n", obtener_puntos_fidelidad(id_u));
            printf("  Descuento : %s\n",    dn[obtener_descuento_usuario(id_u)]);
            printf("  =========================================\n");
            pausar_s();
            break;
        }
        case 2: {
            char campo[32], valor[128];
            printf("  Campos modificables: nombre, apellido, telf\n");
            leer_cadena("  Campo     : ", campo, sizeof(campo));
            leer_cadena("  Nuevo valor: ", valor, sizeof(valor));
            if (modificar_usuario_db(id_u, campo, valor) == 0) {
                log_evento(cfg.log_path, email, "UPDATE_PERFIL", "Perfil pasajero actualizado");
                printf("  Datos actualizados.\n");
            } else printf("  Error al actualizar.\n");
            pausar_s();
            break;
        }
        case 3: {
            char actual[256], nueva[256], nueva2[256];
            leer_cadena("  Contrasena actual    : ", actual, sizeof(actual));
            if (!comprobar_contrasenia(email, actual)) {
                printf("  Contrasena actual incorrecta.\n");
            } else {
                leer_cadena("  Nueva contrasena     : ", nueva,  sizeof(nueva));
                leer_cadena("  Repite nueva contras.: ", nueva2, sizeof(nueva2));
                if (strcmp(nueva, nueva2) != 0) {
                    printf("  Las contrasenas no coinciden.\n");
                } else if (cambiar_contrasenia_db(email, nueva) == 0) {
                    log_evento(cfg.log_path, email, "CAMBIO_PASS", "Contrasena pasajero cambiada");
                    printf("  Contrasena cambiada correctamente.\n");
                } else printf("  Error al cambiar contrasena.\n");
            }
            pausar_s();
            break;
        }
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ============================================================
 *  ╔══════════════════════════════════╗
 *  ║   MENÚS MAQUINISTA  (3 secs)     ║
 *  ╚══════════════════════════════════╝
 * ============================================================ */

void menu_principal_maquinista(int id_u, const char *email) {
    int op;
    do {
        titulo("MENU PRINCIPAL  –  MAQUINISTA");
        printf("  1. Mi cuadrante de servicios\n");
        printf("  2. Mis datos\n");
        printf("  0. Cerrar sesion\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
            case 1: menu_cuadrante_servicios(id_u);          break;
            case 2: menu_mis_datos_maquinista(id_u, email);  break;
            case 0:
                log_evento(cfg.log_path, email, "LOGOUT", "Sesion maquinista cerrada");
                printf("\n  Hasta luego.\n");
                break;
            default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}

/* ─── CUADRANTE DE SERVICIOS ─── */
void menu_cuadrante_servicios(int id_u) {
    int op;
    do {
        titulo("MI CUADRANTE DE SERVICIOS");
        listar_servicios_maquinista(id_u);
        printf("\n  1. Seleccionar servicio y operar\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");

        if (op == 1) {
            int id_serv = leer_entero("  ID servicio: ");
            ServicioOperativo s = obtener_servicio_por_id(id_serv);
            if (s.id_serv < 0) { printf("  Servicio no encontrado.\n"); pausar_s(); continue; }

            /* Datos técnicos del tren */
            Trayecto tr = obtener_trayecto_por_id(s.id_tr);
            if (tr.id_tr >= 0) {
                Tren t = obtener_tren_por_id(tr.id_t);
                printf("\n  — DATOS TECNICOS DEL TREN —\n");
                printf("  Modelo     : %s\n", t.nombre_modelo);
                printf("  Num. serie : %s\n", t.num_serie);
                printf("  Anyo fab.  : %d\n", t.anio_fab);
                const char *es[] = {"OPERATIVO","REVISION","AVERIA","RETIRADO"};
                printf("  Estado     : %s\n", es[t.estado_mant]);
                printf("\n  — VAGONES —\n");
                listar_vagones_tren(tr.id_t);
                printf("\n  — PARADAS INTERMEDIAS —\n");
                listar_paradas_trayecto(s.id_tr);
            }

            int sub;
            do {
                printf("\n  OPCIONES DEL SERVICIO #%d:\n", id_serv);
                printf("  1. Marcar INICIO de trayecto\n");
                printf("  2. Marcar FIN   de trayecto\n");
                printf("  3. Reportar RETRASO\n");
                printf("  4. Reportar AVERIA\n");
                printf("  5. Anyadir observacion\n");
                printf("  0. Volver\n");
                sep();
                sub = leer_entero("  Opcion: ");

                switch (sub) {
                case 1:
                    if (marcar_inicio_servicio(id_serv) == 0) {
                        log_evento(cfg.log_path, NULL, "INICIO_SERV", "Trayecto iniciado");
                        printf("  Inicio de trayecto registrado.\n");
                    } else printf("  Error.\n");
                    pausar_s();
                    break;
                case 2:
                    if (marcar_fin_servicio(id_serv) == 0) {
                        log_evento(cfg.log_path, NULL, "FIN_SERV", "Trayecto finalizado");
                        printf("  Fin de trayecto registrado.\n");
                    } else printf("  Error.\n");
                    pausar_s();
                    break;
                case 3: {
                    int minutos = leer_entero("  Minutos de retraso: ");
                    char causa[256];
                    leer_cadena("  Causa del retraso  : ", causa, sizeof(causa));
                    if (actualizar_retraso_servicio(id_serv, minutos, causa) == 0) {
                        log_evento(cfg.log_path, NULL, "RETRASO", causa);
                        printf("  Retraso registrado: %d min — %s\n", minutos, causa);
                    } else printf("  Error.\n");
                    pausar_s();
                    break;
                }
                case 4: {
                    /* Avería: crea incidencia + actualiza estado tren */
                    Trayecto tr2 = obtener_trayecto_por_id(s.id_tr);
                    char desc_av[512];
                    leer_cadena("  Descripcion de la averia: ", desc_av, sizeof(desc_av));
                    Incidencia inc; memset(&inc, 0, sizeof(inc));
                    inc.id_serv      = id_serv;
                    inc.id_u_reporta = id_u;
                    inc.tipo         = INCIDENCIA_TECNICA;
                    inc.prioridad    = PRIORIDAD_ALTA;
                    strncpy(inc.descripcion, desc_av, 511);
                    if (insertar_incidencia_db(inc) == 0) {
                        /* Cambiar estado tren a AVERÍA */
                        if (tr2.id_tr >= 0)
                            cambiar_estado_tren_db(tr2.id_t, TREN_AVERIA);
                        /* Registrar retraso */
                        int ret = leer_entero("  Minutos de retraso estimado: ");
                        actualizar_retraso_servicio(id_serv, ret, desc_av);
                        log_evento(cfg.log_path, NULL, "AVERIA", desc_av);
                        printf("  Averia reportada. Administrador notificado.\n");
                    } else printf("  Error al reportar.\n");
                    pausar_s();
                    break;
                }
                case 5: {
                    char obs[256];
                    leer_cadena("  Observacion: ", obs, sizeof(obs));
                    /* La observación se guarda actualizando la asignación */
                    log_evento(cfg.log_path, NULL, "OBSERVACION", obs);
                    printf("  Observacion registrada en logs.\n");
                    pausar_s();
                    break;
                }
                case 0: break;
                default: printf("  Opcion no valida.\n");
                }
            } while (sub != 0);
        }
    } while (op != 0);
}

/* ─── MIS DATOS MAQUINISTA ─── */
void menu_mis_datos_maquinista(int id_u, const char *email) {
    int op;
    do {
        titulo("MIS DATOS");
        printf("  1. Ver perfil\n");
        printf("  2. Cambiar contrasena\n");
        printf("  0. Volver\n");
        sep();
        op = leer_entero("  Opcion: ");
        switch (op) {
        case 1: {
            Usuario u = obtener_usuario_por_id(id_u);
            printf("\n  =========================================\n");
            printf("  PERFIL MAQUINISTA\n");
            printf("  =========================================\n");
            printf("  Nombre    : %s %s\n", u.nombre, u.apellido);
            printf("  DNI       : %s\n",    u.dni);
            printf("  Email     : %s\n",    u.email);
            printf("  Telefono  : %s\n",    u.telf);
            printf("  F. Nac.   : %s\n",    u.fecha_nac);
            printf("  Rol       : MAQUINISTA\n");
            printf("  =========================================\n");
            pausar_s();
            break;
        }
        case 2: {
            char actual[256], nueva[256], nueva2[256];
            leer_cadena("  Contrasena actual    : ", actual, sizeof(actual));
            if (!comprobar_contrasenia(email, actual)) {
                printf("  Contrasena actual incorrecta.\n");
            } else {
                leer_cadena("  Nueva contrasena     : ", nueva,  sizeof(nueva));
                leer_cadena("  Repite nueva contras.: ", nueva2, sizeof(nueva2));
                if (strcmp(nueva, nueva2) != 0) {
                    printf("  Las contrasenas no coinciden.\n");
                } else if (cambiar_contrasenia_db(email, nueva) == 0) {
                    log_evento(cfg.log_path, email, "CAMBIO_PASS", "Contrasena maquinista cambiada");
                    printf("  Contrasena cambiada correctamente.\n");
                } else printf("  Error al cambiar contrasena.\n");
            }
            pausar_s();
            break;
        }
        case 0: break;
        default: printf("  Opcion no valida.\n");
        }
    } while (op != 0);
}
