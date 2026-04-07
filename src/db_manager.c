/*
 * db_manager.c
 *

 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "db_manager.h"
#include "estructuras.h"
#include "config.h"

/* ============================================================
 *  UTILIDAD INTERNA: abrir BD
 * ============================================================ */
static sqlite3* abrir_bd() {
    sqlite3 *db;
    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) {
        fprintf(stderr, "Error al abrir BD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }
    sqlite3_busy_timeout(db, 3000);
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, 0);
    sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, 0);
    return db;
}

int init_database(void) {
    sqlite3 *db;
    char *err = NULL;

    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) {
        fprintf(stderr, "[BD] Error al abrir: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    const char *sql =
        // USUARIOS
        "CREATE TABLE IF NOT EXISTS USUARIOS ("
        "id_u INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL, apellido TEXT NOT NULL,"
        "dni TEXT UNIQUE NOT NULL, email TEXT UNIQUE NOT NULL,"
        "telf TEXT, fecha_nac TEXT, pass_hash TEXT,"
        "rol TEXT CHECK(rol IN ('ADMIN','PASAJERO','MAQUINISTA')),"
        "activo INTEGER DEFAULT 1, fecha_registro TEXT);"

        // DATOS_PASAJERO
        "CREATE TABLE IF NOT EXISTS DATOS_PASAJERO ("
        "id_u INTEGER PRIMARY KEY,"
        "puntos_fidelidad INTEGER DEFAULT 0,"
        "tipo_descuento TEXT DEFAULT 'NINGUNO',"
        "num_tarjeta_fidelizacion TEXT,"
        "necesidad_asistencia_pmr INTEGER DEFAULT 0,"
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u));"

        // DATOS_EMPLEADO
        "CREATE TABLE IF NOT EXISTS DATOS_EMPLEADO ("
        "id_u INTEGER PRIMARY KEY,"
        "num_empleado TEXT UNIQUE,"
        "num_ss TEXT,"
        "fecha_ingreso TEXT,"
        "rol_empleado TEXT,"
        "anios_exp INTEGER DEFAULT 0,"
        "telf_empresa TEXT,"
        "estado TEXT DEFAULT 'ACTIVO',"
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u));"

        // TRENES
        "CREATE TABLE IF NOT EXISTS TRENES ("
        "id_t INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre_modelo TEXT NOT NULL, num_serie TEXT UNIQUE,"
        "anio_fab INTEGER, estado_mant TEXT DEFAULT 'OPERATIVO',"
        "fecha_ultima_revision TEXT);"

        // VAGONES
        "CREATE TABLE IF NOT EXISTS VAGONES ("
        "id_vagon INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_tren INTEGER, numero_vagon INTEGER,"
        "clase TEXT, capacidad_total INTEGER DEFAULT 50,"
        "vagon_PMR INTEGER DEFAULT 0,"
        "FOREIGN KEY(id_tren) REFERENCES TRENES(id_t));"

        // ESTACIONES
        "CREATE TABLE IF NOT EXISTS ESTACIONES ("
        "id_est INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL, codigo_gtfs TEXT,"
        "ciudad TEXT NOT NULL, provincia TEXT,"
        "latitud REAL, longitud REAL,"
        "num_andenes INTEGER DEFAULT 1,"
        "tiene_sala_club INTEGER DEFAULT 0);"

        // TRAYECTOS
        "CREATE TABLE IF NOT EXISTS TRAYECTOS ("
        "id_tr INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_t INTEGER, id_est_origen INTEGER, id_est_destino INTEGER,"
        "hora_salida TEXT, hora_llegada TEXT,"
        "duracion_min INTEGER, precio_base REAL,"
        "dias_operacion TEXT DEFAULT 'LMXJVSD',"
        "estado TEXT DEFAULT 'ACTIVO',"
        "FOREIGN KEY(id_t) REFERENCES TRENES(id_t),"
        "FOREIGN KEY(id_est_origen)  REFERENCES ESTACIONES(id_est),"
        "FOREIGN KEY(id_est_destino) REFERENCES ESTACIONES(id_est));"

        // PARADAS_INTERMEDIAS
        "CREATE TABLE IF NOT EXISTS PARADAS_INTERMEDIAS ("
        "id_parada INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_tr INTEGER, id_est INTEGER, orden INTEGER,"
        "hora_llegada TEXT, hora_salida TEXT, anden INTEGER,"
        "FOREIGN KEY(id_tr)  REFERENCES TRAYECTOS(id_tr),"
        "FOREIGN KEY(id_est) REFERENCES ESTACIONES(id_est));"

        // RESERVAS
        "CREATE TABLE IF NOT EXISTS RESERVAS ("
        "id_res INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_u INTEGER, id_tr INTEGER, fecha_viaje TEXT,"
        "clase TEXT, num_vagon INTEGER, num_asiento INTEGER,"
        "precio_base REAL, descuento_pct REAL DEFAULT 0,"
        "precio_final REAL, estado TEXT DEFAULT 'CONFIRMADA',"
        "codigo_validacion TEXT, fecha_reserva TEXT,"
        "FOREIGN KEY(id_u)  REFERENCES USUARIOS(id_u),"
        "FOREIGN KEY(id_tr) REFERENCES TRAYECTOS(id_tr));"

        // EQUIPAJES
        "CREATE TABLE IF NOT EXISTS EQUIPAJES ("
        "id_eq INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_res INTEGER, tipo TEXT, peso_kg REAL DEFAULT 0,"
        "dimensiones TEXT, exceso_kg REAL DEFAULT 0,"
        "suplemento_pago REAL DEFAULT 0, facturado INTEGER DEFAULT 0,"
        "FOREIGN KEY(id_res) REFERENCES RESERVAS(id_res));"

        // SERVICIOS_OPERATIVOS
        "CREATE TABLE IF NOT EXISTS SERVICIOS_OPERATIVOS ("
        "id_serv INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_tr INTEGER, fecha TEXT,"
        "estado_serv TEXT DEFAULT 'PROGRAMADO',"
        "hora_inicio_real TEXT, hora_fin_real TEXT,"
        "minutos_retraso INTEGER DEFAULT 0,"
        "causa_retraso TEXT,"
        "FOREIGN KEY(id_tr) REFERENCES TRAYECTOS(id_tr));"

        "CREATE TABLE IF NOT EXISTS ASIGNACION_PERSONAL ("
        "id_asig INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_serv INTEGER, id_u INTEGER, id_t INTEGER,"
        "rol_servicio TEXT, observaciones TEXT,"
        "FOREIGN KEY(id_serv) REFERENCES SERVICIOS_OPERATIVOS(id_serv),"
        "FOREIGN KEY(id_u)    REFERENCES USUARIOS(id_u),"
        "FOREIGN KEY(id_t)    REFERENCES TRENES(id_t));"

        // INCIDENCIAS
        "CREATE TABLE IF NOT EXISTS INCIDENCIAS ("
        "id_inc INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_serv INTEGER, id_u_reporta INTEGER,"
        "tipo TEXT, descripcion TEXT,"
        "prioridad TEXT DEFAULT 'MEDIA',"
        "estado TEXT DEFAULT 'ABIERTA',"
        "fecha_reporte TEXT, fecha_resolucion TEXT,"
        "id_u_resuelve INTEGER,"
        "FOREIGN KEY(id_serv)       REFERENCES SERVICIOS_OPERATIVOS(id_serv),"
        "FOREIGN KEY(id_u_reporta)  REFERENCES USUARIOS(id_u),"
        "FOREIGN KEY(id_u_resuelve) REFERENCES USUARIOS(id_u));"

        // LOGS_OPERATIVOS
        "CREATE TABLE IF NOT EXISTS LOGS_OPERATIVOS ("
        "id_log INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT, id_u INTEGER,"
        "accion TEXT, entidad_afectada TEXT, id_entidad INTEGER,"
        "ip_origen TEXT DEFAULT '127.0.0.1',"
        "nivel TEXT DEFAULT 'INFO', detalle_json TEXT,"
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u));"

        // TARIFAS
        "CREATE TABLE IF NOT EXISTS TARIFAS ("
        "id_tarifa INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_tr INTEGER UNIQUE, precio_base REAL,"
        "coef_turista REAL DEFAULT 1.0,"
        "coef_business REAL DEFAULT 1.8,"
        "suplemento_bici REAL DEFAULT 30.0,"
        "exceso_kg_precio REAL DEFAULT 12.0,"
        "FOREIGN KEY(id_tr) REFERENCES TRAYECTOS(id_tr));"

        // DESCUENTOS
        "CREATE TABLE IF NOT EXISTS DESCUENTOS ("
        "tipo_descuento TEXT PRIMARY KEY, porcentaje REAL);"

        //PUNTOS_FIDELIDAD_HISTORIAL
        "CREATE TABLE IF NOT EXISTS PUNTOS_HISTORIAL ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_u INTEGER, fecha TEXT, delta INTEGER, concepto TEXT,"
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u));";

    int rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[BD] Error creando tablas: %s\n", err);
        sqlite3_free(err);
        sqlite3_close(db);
        return 1;
    }
    printf("[OK] Base de datos conectada: %s\n", cfg.db_path);
    sqlite3_close(db);
    return 0;
}

/* ============================================================
 *  SEED  (datos de prueba realistas)
 * ============================================================ */
void seed_database(void) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }

    char *err = NULL;
    const char *sql =
        // Usuarios
        "INSERT OR IGNORE INTO USUARIOS (nombre,apellido,dni,email,telf,fecha_nac,pass_hash,rol,activo,fecha_registro)"
        " VALUES ('Admin','Trenfe','00000000A','admin@trenfe.com','600000000','1980-01-01','admin123','ADMIN',1,date('now'));"
        "INSERT OR IGNORE INTO USUARIOS (nombre,apellido,dni,email,telf,fecha_nac,pass_hash,rol,activo,fecha_registro)"
        " VALUES ('Juan','Garcia','12345678B','juan@trenfe.com','611222333','1995-06-15','pass123','PASAJERO',1,date('now'));"
        "INSERT OR IGNORE INTO USUARIOS (nombre,apellido,dni,email,telf,fecha_nac,pass_hash,rol,activo,fecha_registro)"
        " VALUES ('Pedro','Lopez','87654321C','pedro@trenfe.com','622333444','1985-03-20','maq123','MAQUINISTA',1,date('now'));"
        "INSERT OR IGNORE INTO USUARIOS (nombre,apellido,dni,email,telf,fecha_nac,pass_hash,rol,activo,fecha_registro)"
        " VALUES ('Maria','Fernandez','11223344D','maria@trenfe.com','633444555','2000-09-10','pass456','PASAJERO',1,date('now'));"

        // Datos pasajeros
        "INSERT OR IGNORE INTO DATOS_PASAJERO (id_u,puntos_fidelidad,tipo_descuento) VALUES (2,150,'JOVEN');"
        "INSERT OR IGNORE INTO DATOS_PASAJERO (id_u,puntos_fidelidad,tipo_descuento) VALUES (4,0,'NINGUNO');"

        //Datos empleados
        "INSERT OR IGNORE INTO DATOS_EMPLEADO (id_u,num_empleado,fecha_ingreso,rol_empleado,anios_exp,estado)"
        " VALUES (3,'EMP-001','2015-01-01','MAQUINISTA',10,'ACTIVO');"

        // Trenes
        "INSERT OR IGNORE INTO TRENES (nombre_modelo,num_serie,anio_fab,estado_mant,fecha_ultima_revision)"
        " VALUES ('AVE Serie 103','AVE-103-001',2006,'OPERATIVO','2025-01-15');"
        "INSERT OR IGNORE INTO TRENES (nombre_modelo,num_serie,anio_fab,estado_mant,fecha_ultima_revision)"
        " VALUES ('Alvia Serie 130','ALV-130-005',2010,'OPERATIVO','2025-03-10');"
        "INSERT OR IGNORE INTO TRENES (nombre_modelo,num_serie,anio_fab,estado_mant,fecha_ultima_revision)"
        " VALUES ('Cercanias C1','CER-C1-012',2018,'OPERATIVO','2025-02-20');"

        // Vagones tren 1
        "INSERT OR IGNORE INTO VAGONES (id_tren,numero_vagon,clase,capacidad_total,vagon_PMR) VALUES (1,1,'T',50,0);"
        "INSERT OR IGNORE INTO VAGONES (id_tren,numero_vagon,clase,capacidad_total,vagon_PMR) VALUES (1,2,'T',50,1);"
        "INSERT OR IGNORE INTO VAGONES (id_tren,numero_vagon,clase,capacidad_total,vagon_PMR) VALUES (1,3,'B',30,0);"
        // Vagones tren 2
        "INSERT OR IGNORE INTO VAGONES (id_tren,numero_vagon,clase,capacidad_total,vagon_PMR) VALUES (2,1,'T',60,0);"
        "INSERT OR IGNORE INTO VAGONES (id_tren,numero_vagon,clase,capacidad_total,vagon_PMR) VALUES (2,2,'B',24,0);"

        //Estaciones
        "INSERT OR IGNORE INTO ESTACIONES (nombre,codigo_gtfs,ciudad,provincia,latitud,longitud,num_andenes,tiene_sala_club)"
        " VALUES ('Bilbao Abando','BILBAO','Bilbao','Vizcaya',43.2630,-2.9350,8,1);"
        "INSERT OR IGNORE INTO ESTACIONES (nombre,codigo_gtfs,ciudad,provincia,latitud,longitud,num_andenes,tiene_sala_club)"
        " VALUES ('Madrid Atocha','MADRID','Madrid','Madrid',40.4063,-3.6899,20,1);"
        "INSERT OR IGNORE INTO ESTACIONES (nombre,codigo_gtfs,ciudad,provincia,latitud,longitud,num_andenes,tiene_sala_club)"
        " VALUES ('Barcelona Sants','BARCA','Barcelona','Barcelona',41.3794,2.1403,16,1);"
        "INSERT OR IGNORE INTO ESTACIONES (nombre,codigo_gtfs,ciudad,provincia,latitud,longitud,num_andenes,tiene_sala_club)"
        " VALUES ('Vitoria-Gasteiz','VITORIA','Vitoria','Alava',42.8467,-2.6725,4,0);"
        "INSERT OR IGNORE INTO ESTACIONES (nombre,codigo_gtfs,ciudad,provincia,latitud,longitud,num_andenes,tiene_sala_club)"
        " VALUES ('San Sebastian Donostia','DONOSTI','San Sebastian','Guipuzcoa',43.3189,-1.9812,6,0);"

        // Trayectos
        "INSERT OR IGNORE INTO TRAYECTOS (id_t,id_est_origen,id_est_destino,hora_salida,hora_llegada,duracion_min,precio_base,dias_operacion,estado)"
        " VALUES (1,1,2,'08:00','12:30',270,45.50,'LMXJVSD','ACTIVO');"
        "INSERT OR IGNORE INTO TRAYECTOS (id_t,id_est_origen,id_est_destino,hora_salida,hora_llegada,duracion_min,precio_base,dias_operacion,estado)"
        " VALUES (2,2,3,'14:00','16:30',150,32.00,'LMXJVSD','ACTIVO');"
        "INSERT OR IGNORE INTO TRAYECTOS (id_t,id_est_origen,id_est_destino,hora_salida,hora_llegada,duracion_min,precio_base,dias_operacion,estado)"
        " VALUES (1,2,1,'17:00','21:30',270,45.50,'LMXJVSD','ACTIVO');"
        "INSERT OR IGNORE INTO TRAYECTOS (id_t,id_est_origen,id_est_destino,hora_salida,hora_llegada,duracion_min,precio_base,dias_operacion,estado)"
        " VALUES (3,4,5,'09:15','10:00',45,8.50,'LMXJVSD','ACTIVO');"

        // Tarifas
        "INSERT OR IGNORE INTO TARIFAS (id_tr,precio_base,coef_turista,coef_business,suplemento_bici,exceso_kg_precio)"
        " VALUES (1,45.50,1.0,1.8,30.0,12.0);"
        "INSERT OR IGNORE INTO TARIFAS (id_tr,precio_base,coef_turista,coef_business,suplemento_bici,exceso_kg_precio)"
        " VALUES (2,32.00,1.0,1.8,30.0,12.0);"

        // Descuentos
        "INSERT OR IGNORE INTO DESCUENTOS VALUES ('JOVEN',20.0);"
        "INSERT OR IGNORE INTO DESCUENTOS VALUES ('DORADA',40.0);"
        "INSERT OR IGNORE INTO DESCUENTOS VALUES ('NUMEROSA',20.0);"
        "INSERT OR IGNORE INTO DESCUENTOS VALUES ('ABONO',50.0);"

        // Servicio operativo de ejemplo
        "INSERT OR IGNORE INTO SERVICIOS_OPERATIVOS (id_tr,fecha,estado_serv)"
        " VALUES (1,date('now'),'PROGRAMADO');"

        // Asignación maquinista al servicio
        "INSERT OR IGNORE INTO ASIGNACION_PERSONAL (id_serv,id_u,id_t,rol_servicio)"
        " VALUES (1,3,1,'CONDUCTOR');";

    	"INSERT OR IGNORE INTO SERVICIOS_OPERATIVOS (id_tr,fecha,estado_serv)"
    	" VALUES (1,date('now'),'PROGRAMADO');";




    if (sqlite3_exec(db, sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "[SEED] Error: %s\n", err);
        sqlite3_free(err);
    } else {
        printf("[SEED] Datos de prueba cargados.\n");
        printf("       admin@trenfe.com / admin123\n");
        printf("       juan@trenfe.com  / pass123\n");
        printf("       pedro@trenfe.com / maq123\n");
    }
    sqlite3_close(db);
}

//USUARIOSS
const char* rol_a_texto(RolUsuario rol) {
    if (rol == ROL_ADMIN)    return "ADMIN";
    if (rol == ROL_EMPLEADO) return "MAQUINISTA";
    return "PASAJERO";
}

RolUsuario obtener_rol_usuario(const char *email) {
    sqlite3 *db = abrir_bd();
    if (!db) return ROL_PASAJERO;

    sqlite3_stmt *stmt;
    RolUsuario rol = ROL_PASAJERO;
    const char *sql = "SELECT rol FROM USUARIOS WHERE email = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *r = (const char*)sqlite3_column_text(stmt, 0);
            if (strcmp(r, "ADMIN")     == 0) rol = ROL_ADMIN;
            else if (strcmp(r, "MAQUINISTA") == 0) rol = ROL_EMPLEADO;
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return rol;
}

int obtener_id_usuario(const char *email){
	sqlite3 *db = abrir_bd();
	if (!db) return -1;

	sqlite3_stmt *stmt;
	int id = -1;
	const char *sql = "SELECT id_u FROM USUARIOS WHERE email=?;";

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
		sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			 id = sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}
	sqlite3_close(db);
	return id;
}


Usuario obtener_usuario (const char *email){
	Usuario u;
	sqlite3 *db = abrir_bd();
	if (!db) return u;

	sqlite3_stmt *stmt;
	const char *sql = "SELECT * FROM USUARIOS WHERE email = ?;";
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK){
		sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			u.id_u = sqlite3_column_int(stmt, 0);
			strncpy(u.nombre, (const char *)sqlite3_column_text(stmt, 1), sizeof(u.nombre));
			strncpy(u.apellido,(const char *)sqlite3_column_text(stmt, 2), sizeof(u.apellido));
			strncpy(u.dni,(const char *)sqlite3_column_text(stmt, 3), sizeof(u.dni));
			strncpy(u.email, (const char *)sqlite3_column_text(stmt, 4), sizeof(u.email));
			strncpy(u.telf,(const char *)sqlite3_column_text(stmt, 5), sizeof(u.telf));
			strncpy(u.fecha_nac, (const char *)sqlite3_column_text(stmt, 6), sizeof(u.fecha_nac));
			strncpy(u.pass_hash, (const char *)sqlite3_column_text(stmt, 7), sizeof(u.pass_hash));
		    u.rol    = (RolUsuario)sqlite3_column_int(stmt, 8);
		    u.activo =  sqlite3_column_int(stmt, 9);
		    strncpy(u.fecha_registro,(const char *)sqlite3_column_text(stmt, 10), sizeof(u.fecha_registro));
		}
		sqlite3_finalize(stmt);
	}
	sqlite3_close(db);
	return u;
}

Usuario obtener_usuario_por_id(int id_u){
	Usuario u;
	memset(&u, 0, sizeof(u)); //guarda en memoria justo lo que quiero
	u.id_u = -1;
	sqlite3 *db = abrir_bd();
	if(!db) return u;
	sqlite3_stmt *stmt;
	if (sqlite3_prepare_v2(db,
	    "SELECT id_u,nombre,apellido,dni,email,telf,fecha_nac,pass_hash,rol,activo,fecha_registro "
	    "FROM USUARIOS WHERE id_u=?;",-1, &stmt, NULL) == SQLITE_OK) {
		 sqlite3_bind_int(stmt, 1, id_u);
		 if(sqlite3_step(stmt) == SQLITE_ROW){
			 u.id_u = sqlite3_column_int(stmt, 0);
			 strncpy(u.nombre,(const char *)sqlite3_column_text(stmt,1), 63);
			 strncpy(u.apellido,(const char *)sqlite3_column_text(stmt,2), 63);
			 strncpy(u.dni, (const char *)sqlite3_column_text(stmt,3), 15);
			 strncpy(u.email, (const char *)sqlite3_column_text(stmt,4),127);
			 //Extraemos el telefono
			 if (sqlite3_column_text(stmt,5)){
				 strncpy(u.telf,  (const char *)sqlite3_column_text(stmt,5), 19);
			 }
			 //Extraemos la fecha de nacimiento
			 if (sqlite3_column_text(stmt,6)){
				 strncpy(u.fecha_nac,(const char *)sqlite3_column_text(stmt,6),10);
			 }
			 u.activo = sqlite3_column_int(stmt, 9);
			 //Extraemos la fecha de registro
			 if (sqlite3_column_text(stmt,10)){
				 strncpy(u.fecha_registro, (const char *)sqlite3_column_text(stmt,10), 19);
			 }
			 const char *r = (const char *)sqlite3_column_text(stmt,8);
			 if (!strcmp(r,"ADMIN")){
				 u.rol = ROL_ADMIN;
			 }else if (!strcmp(r,"MAQUINISTA")){
				 u.rol = ROL_EMPLEADO;
			 }else{
				 u.rol = ROL_PASAJERO;
			 }
		 }
		 sqlite3_finalize(stmt);
	}
	sqlite3_close(db);
	return u;
}

Usuario obtener_usuario_por_email(const char *email) {
    int id = obtener_id_usuario(email);
    return obtener_usuario_por_id(id);
}

int insertar_usuario_db(Usuario u) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }

    sqlite3_stmt *stmt;
    const char *sql_usuario =
        "INSERT INTO USUARIOS (nombre,apellido,dni,email,telf,fecha_nac,"
        "pass_hash,rol,activo,fecha_registro) VALUES (?,?,?,?,?,?,?,?,?,date('now'));";

    if (sqlite3_prepare_v2(db, sql_usuario, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db); return 1;
    }

    sqlite3_bind_text(stmt,1, u.nombre, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,2, u.apellido, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,3, u.dni, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,4, u.email, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,5, u.telf,-1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,6, u.fecha_nac, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,7, u.pass_hash, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,8, rol_a_texto(u.rol),-1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt,9, u.activo);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        printf("[ERR] Error al registrar usuario (DNI/email duplicado?).\n");
        sqlite3_close(db);
        return 1;
    }

    printf("[OK] Usuario %s %s registrado.\n", u.nombre, u.apellido);
    int nuevo_id = (int)sqlite3_last_insert_rowid(db);

    if (u.rol == ROL_PASAJERO) {
        const char *sql_pasajero = "INSERT OR IGNORE INTO DATOS_PASAJERO (id_u, puntos_fidelidad, tipo_descuento) VALUES (?, 0, 'NINGUNO');";
        sqlite3_prepare_v2(db,sql_pasajero, -1, &stmt, NULL);
        sqlite3_bind_int(stmt,1, nuevo_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

    } else if (u.rol == ROL_EMPLEADO) {

        srand((unsigned int)time(NULL) ^ (unsigned int)nuevo_id); //Generador de usuario aleatorio
        int num = 1000 + rand() % 9000;
        char num_emp;
        sprintf(num_emp, "EMP-%04d", num);

        const char *sql_empleado = "INSERT OR IGNORE INTO DATOS_EMPLEADO (id_u, num_empleado, fecha_ingreso, rol_empleado, estado) VALUES (?, ?, date('now'), 'MAQUINISTA', 'ACTIVO');";
        sqlite3_prepare_v2(db, sql_empleado, -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, nuevo_id);
        sqlite3_bind_text(stmt, 2, num_emp, -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        printf("[OK] Numero de empleado asignado: %s\n", num_emp);
    }

    sqlite3_close(db);
    return 0;
}
void listar_usuarios_db() {
    sqlite3 *db = abrir_bd();
    if (!db) return;

    sqlite3_stmt *stmt;
    const char *sql = "SELECT id_u, nombre, apellido, dni, email, rol, activo FROM USUARIOS ORDER BY id_u;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db); return;
    }

    printf("\n%-4s | %-15s | %-15s | %-11s | %-25s | %-10s | %s\n",
           "ID","NOMBRE","APELLIDO","DNI","EMAIL","ROL","ACTIVO");
    printf("%-4s-+-%-15s-+-%-15s-+-%-11s-+-%-25s-+-%-10s-+-%s\n",
           "----","---------------","---------------","-----------",
           "-------------------------","----------","------");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("%-4d | %-15s | %-15s | %-11s | %-25s | %-10s | %s\n",
               sqlite3_column_int (stmt, 0),
               sqlite3_column_text(stmt, 1),
               sqlite3_column_text(stmt, 2),
               sqlite3_column_text(stmt, 3),
               sqlite3_column_text(stmt, 4),
               sqlite3_column_text(stmt, 5),
               sqlite3_column_int (stmt, 6) ? "Si" : "No");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void listar_empleados_db(){
	sqlite3 *db = abrir_bd();
	if (!db) return;

	sqlite3_stmt *stmt;
	const char *sql =
			"SELECT u.id_u, u.nombre, u.apellido, u.email, de.num_empleado,"
	        "de.rol_empleado, de.anios_exp, de.estado "
	        "FROM USUARIOS u LEFT JOIN DATOS_EMPLEADO de ON u.id_u=de.id_u "
	        "WHERE u.rol='MAQUINISTA' ORDER BY u.id_u;";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	printf("\n%-4s | %-15s | %-15s | %-20s | %-10s | %-15s | %s\n",
			 "ID","NOMBRE","APELLIDO","EMAIL","NUM EMP","ROL","EST.");
	printf("%-4s-+-%-15s-+-%-15s-+-%-20s-+-%-10s-+-%-15s-+-%s\n",
	        "----","---------------","---------------","-----------",
	        "-------------------------","----------","------");
	while (sqlite3_step(stmt) == SQLITE_ROW){
		printf("%-4d | %-15s | %-15s | %-20s | %-10s | %-15s | %s\n",
		sqlite3_column_int (stmt, 0),
		(const char*) sqlite3_column_text(stmt, 1),
		(const char*) sqlite3_column_text(stmt, 2),
		(const char*) sqlite3_column_text(stmt, 3),
		sqlite3_column_text(stmt,4) ? (const char*)sqlite3_column_text(stmt,4) : "-",
		sqlite3_column_text(stmt,5) ? (const char*) sqlite3_column_text(stmt,5) : "MAQUINISTA",
		sqlite3_column_text(stmt,7) ? (const char*)sqlite3_column_text(stmt,7) : "ACTIVO");
	 }
	 sqlite3_finalize(stmt);
	 sqlite3_close(db);
}

int modificar_usuario_db(int id_u, const char *campo, const char *valor){
	sqlite3 *db = abrir_bd();
	if (!db) return 1;
	char sql[512];
	snprintf(sql, sizeof(sql),
		"UPDATE USUARIOS SET %s=? WHERE id_u=?;", campo);
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_bind_text(stmt,1,valor,-1,SQLITE_TRANSIENT);
	sqlite3_bind_int (stmt,2,id_u);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt); sqlite3_close(db);
	return (rc == SQLITE_DONE) ? 0 : 1;
}

int deshabilitar_usuario_db(int id_u){
	int rc = modificar_usuario_db(id_u, "activo", "0");
	    if (rc == 0) {

	        sqlite3 *db = abrir_bd();
	        if (db) {
	            sqlite3_stmt *stmt;
	            const char *sql = "UPDATE DATOS_EMPLEADO SET estado='INACTIVO' WHERE id_u=?;";
	            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
	                sqlite3_bind_int(stmt, 1, id_u);
	                sqlite3_step(stmt);
	                sqlite3_finalize(stmt);
	            }
	            sqlite3_close(db);
	        }
	    }
	    return rc;
}

void buscar_usuario_db(const char *dni_o_nombre){
	sqlite3 *db = abrir_bd();
	if (!db) return;

	sqlite3_stmt *stmt;
	char patron[128];

	snprintf(patron, sizeof(patron), "%%%s%%", dni_o_nombre); // al poner en sql el LIKE se usa %palabra% ya que coge todo lo que sea palabra escrito de la forma que sea, al % existir en c se pone %%
	sqlite3_prepare_v2(db,
			"SELECT id_u,nombre,apellido,dni,email,rol,activo FROM USUARIOS "
	        "WHERE dni LIKE ? OR nombre LIKE ? OR apellido LIKE ? ORDER BY id_u;",
	        -1,&stmt,NULL);
	sqlite3_bind_text(stmt,1,patron,-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,2,patron,-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,3,patron,-1,SQLITE_TRANSIENT);
	int n = 0;
	printf("\n%-4s | %-15s | %-15s | %-11s | %-25s | %-10s | %s\n",
			"ID","NOMBRE","APELLIDO","DNI","EMAIL","ROL","ACT.");
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		printf("%-4d | %-15s | %-15s | %-11s | %-25s | %-10s | %s\n",
				sqlite3_column_int(stmt,0),
				(const char*)sqlite3_column_text(stmt,1),
				(const char*)sqlite3_column_text(stmt,2),
				(const char*)sqlite3_column_text(stmt,3),
				(const char*)sqlite3_column_text(stmt,4),
				(const char*)sqlite3_column_text(stmt,5),
				sqlite3_column_int(stmt,6)?"Si":"No");
		n++;
	}
	if (!n){
		printf("[Sin resultados para '%s']\n", dni_o_nombre);
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
}



void importar_usuarios_csv(const char* ruta_csv) {
    sqlite3 *db = abrir_bd();
    if (!db) return;

    FILE *f = fopen(ruta_csv, "r");
    if (!f) {
        printf("Error: no se encontro %s\n", ruta_csv);
        sqlite3_close(db);
        return;
    }

    char linea[512];
    fgets(linea, sizeof(linea), f); /* saltar cabecera */

    int ok = 0, err = 0;
    while (fgets(linea, sizeof(linea), f)) {
        linea[strcspn(linea, "\r\n")] = 0;
        char *nombre   = strtok(linea, ";");
        char *apellido = strtok(NULL,  ";");
        char *dni      = strtok(NULL,  ";");
        char *email    = strtok(NULL,  ";");
        char *rol      = strtok(NULL,  ";");
        if (!nombre || !apellido || !dni || !email || !rol) { err++; continue; }

        char sql[512];
        snprintf(sql, sizeof(sql),
            "INSERT OR IGNORE INTO USUARIOS (nombre,apellido,dni,email,rol,fecha_registro)"
            " VALUES ('%s','%s','%s','%s','%s',date('now'));",
            nombre, apellido, dni, email, rol);
        if (sqlite3_exec(db, sql, 0, 0, 0) == SQLITE_OK) ok++;
        else err++;
    }
    fclose(f);
    sqlite3_close(db);
    printf("Importacion terminada: %d insertados, %d errores.\n", ok, err);
}

bool verificar_usuario(char *email, char *contrasenia) {
    if (comprobar_usuario_registrado(email)) {
        if (comprobar_contrasenia(email, contrasenia)) return true;
        printf("Contrasena incorrecta.\n");
    } else {
        printf("Email no registrado.\n");
    }
    return false;
}

bool comprobar_usuario_registrado(const char *email) {
    sqlite3 *db = abrir_bd();
    if (!db) return false;

    sqlite3_stmt *stmt;
    int count = 0;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM USUARIOS WHERE email=?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return count > 0;
}

bool comprobar_contrasenia(const char *email, const char *contrasenia) {
    sqlite3 *db = abrir_bd();
    if (!db) return false;

    sqlite3_stmt *stmt;
    bool ok = false;
    if (sqlite3_prepare_v2(db, "SELECT pass_hash FROM USUARIOS WHERE email=?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *hash = (const char*)sqlite3_column_text(stmt, 0);
            ok = (strcmp(hash, contrasenia) == 0);
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return ok;
}

int cambiar_contrasenia_db(const char *email, const char *nueva_pass){
	 sqlite3 *db = abrir_bd();
	 if (!db) return 1;

	 sqlite3_stmt *stmt;
	 const char *sql = "UPDATE USUARIOS SET pass_hash=? WHERE email=?;";
	 sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	 sqlite3_bind_text(stmt,1,nueva_pass,-1,SQLITE_TRANSIENT);
	 sqlite3_bind_text(stmt,2,email,-1,SQLITE_TRANSIENT);
	 int rc = sqlite3_step(stmt); //ejecuta el stmt
	 sqlite3_finalize(stmt);
	 sqlite3_close(db);
	 return (rc == SQLITE_DONE) ? 0 : 1;
}

/* ============================================================
 *  DATOS PASAJERO (puntos fidelidad, descuento)
 * ============================================================ */
int  obtener_puntos_fidelidad(int id_u){
	sqlite3 *db = abrir_bd();
	    if (!db) return -1;

	    sqlite3_stmt *stmt;
	    int puntos = -1;
	    const char *sql = "SELECT puntos_fidelidad FROM DATOS_PASAJERO WHERE id_u = ?;";

	    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
	        sqlite3_bind_int(stmt, 1, id_u);
	        if (sqlite3_step(stmt) == SQLITE_ROW) {
	            puntos = sqlite3_column_int(stmt, 0);
	        }
	        sqlite3_finalize(stmt);
	    }
	    sqlite3_close(db);
	    return puntos;

}
int  actualizar_puntos_fidelidad(int id_u, int puntos){
	sqlite3 *db = abrir_bd();
	if(!db) return -1;
	int puntos_antiguos = obtener_puntos_fidelidad(id_u);
	int puntos_actualizados = puntos_antiguos + puntos;
	sqlite3_stmt *stmt;
	const char *sql = "UPDATE DATOS_PASAJEROS SET puntos_fidelidad = ? WHERE id_u = ?";
	 if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
		        sqlite3_bind_int(stmt, 1, puntos_actualizados);
		        sqlite3_bind_int(stmt, 2, id_u);
		        sqlite3_step(stmt);
		        sqlite3_finalize(stmt);
		    }
	 sqlite3_close(db);
	 return 0;
}
void listar_historial_puntos(int id_u){

}

TipoDescuento obtener_descuento_usuario(int id_u) {
    sqlite3 *db = abrir_bd();
    if (!db) return DESCUENTO_NINGUNO;

    sqlite3_stmt *stmt;
    TipoDescuento desc = DESCUENTO_NINGUNO;
    const char *sql = "SELECT tipo_descuento FROM DATOS_PASAJERO WHERE id_u = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id_u);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *tipo = (const char*)sqlite3_column_text(stmt, 0);
            if      (strcmp(tipo, "JOVEN") == 0){
            	desc = DESCUENTO_JOVEN;
            }else if (strcmp(tipo, "DORADA") == 0){
            	desc = DESCUENTO_DORADA;
            }else if (strcmp(tipo, "NUMEROSA") == 0){
            	desc = DESCUENTO_NUMEROSA;
            }else if (strcmp(tipo, "ABONO") == 0){
            	desc = DESCUENTO_ABONO;
            }
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return desc;
}

int  actualizar_descuento_usuario(int id_u, TipoDescuento tipo){
	sqlite3 *db = abrir_bd();
		if(!db) return -1;
		char descuento = "";
		if (tipo == DESCUENTO_JOVEN){
			descuento = "JOVEN";
		}else if (tipo == DESCUENTO_DORADA){
			descuento = "DORADA";
		}else if (tipo == DESCUENTO_NUMEROSA){
			descuento = "NUMEROSA";
		}else if (tipo == DESCUENTO_ABONO){
			descuento = "ABONO";
		}
		sqlite3_stmt *stmt;
		const char *sql = "UPDATE DATOS_PASAJEROS SET tipo_descuento = ? WHERE id_u = ?";
		 if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
			        sqlite3_bind_int(stmt, 1, descuento);
			        sqlite3_bind_int(stmt, 2, id_u);
			        sqlite3_step(stmt);
			        sqlite3_finalize(stmt);
			    }
		 sqlite3_close(db);
		 return 0;
}
/* ============================================================
 *  TRENES
 * ============================================================ */

static const char *estado_tren_str(EstadoMantenimiento e) {
    const char *s[] = {"OPERATIVO","REVISION","AVERIA","RETIRADO"};
    return (e>=0&&e<=3)?s[e]:"OPERATIVO";
}


int insertar_tren_db(Tren t) {
    sqlite3 *db = abrir_bd();
    if (!db) return 1;

    sqlite3_stmt *stmt;
    const char *sql =
        "INSERT INTO TRENES (nombre_modelo,num_serie,anio_fab,estado_mant,fecha_ultima_revision)"
        " VALUES (?,?,?,?,?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db); return 1;
    }

    const char *estados[] = {"OPERATIVO","REVISION","AVERIA","RETIRADO"};
    sqlite3_bind_text(stmt, 1, t.nombre_modelo,          -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, t.num_serie,              -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 3, t.anio_fab);
    sqlite3_bind_text(stmt, 4, estados[t.estado_mant],   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, t.tiene_revision ? t.fecha_ultima_revision : NULL, -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc == SQLITE_DONE) ? 0 : 1;
}

void listar_trenes_db() {
    sqlite3 *db = abrir_bd();
    if (!db) return;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT id_t,nombre_modelo,num_serie,anio_fab,estado_mant FROM TRENES ORDER BY id_t;",
        -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db); return;
    }

    printf("\n%-4s | %-20s | %-15s | %s  | %s\n",
           "ID","MODELO","NUM SERIE","AÑO","ESTADO");
    printf("-----+----------------------+-----------------+------+-----------\n");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("%-4d | %-20s | %-15s | %-4d | %s\n",
               sqlite3_column_int (stmt,0),
               sqlite3_column_text(stmt,1),
               sqlite3_column_text(stmt,2),
               sqlite3_column_int (stmt,3),
               sqlite3_column_text(stmt,4));
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
Tren obtener_tren_por_id(int id_t) {
    Tren t; memset(&t,0,sizeof(t)); t.id_t=-1;
    sqlite3 *db = abrir_bd(); if (!db) return t;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT id_t,nombre_modelo,num_serie,anio_fab,estado_mant,fecha_ultima_revision"
        " FROM TRENES WHERE id_t=?;", -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_t);
    if (sqlite3_step(stmt)==SQLITE_ROW) {
        t.id_t = sqlite3_column_int(stmt,0);
        strncpy(t.nombre_modelo,(const char*)sqlite3_column_text(stmt,1),63);
        strncpy(t.num_serie,    (const char*)sqlite3_column_text(stmt,2),31);
        t.anio_fab = sqlite3_column_int(stmt,3);
        const char *em = (const char*)sqlite3_column_text(stmt,4);
        if (!strcmp(em,"REVISION")) t.estado_mant=TREN_REVISION;
        else if (!strcmp(em,"AVERIA"))  t.estado_mant=TREN_AVERIA;
        else if (!strcmp(em,"RETIRADO"))t.estado_mant=TREN_RETIRADO;
        else t.estado_mant=TREN_OPERATIVO;
        if (sqlite3_column_text(stmt,5)) {
            strncpy(t.fecha_ultima_revision,(const char*)sqlite3_column_text(stmt,5),10);
            t.tiene_revision=1;
        }
    }
    sqlite3_finalize(stmt); sqlite3_close(db);
    return t;
}

int modificar_estado_tren_db(int id_t, EstadoMantenimiento nuevo_estado) {
    sqlite3 *db;
    char *err_msg = 0;

    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) return 1;

    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE TRENES SET estado_mant = '%s' WHERE id_t = %d;",
		estado_tren_str(nuevo_estado), id_t);

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("Error al modificar tren: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("[OK] Estado del tren %d actualizado a %s.\n",
           id_t, estado_tren_str(nuevo_estado));
    sqlite3_close(db);
    return 0;
}

int eliminar_tren_db(int id_t) {
    sqlite3 *db;
    char *err_msg = 0;

    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) return 1;

    char sql[128];
    snprintf(sql, sizeof(sql), "DELETE FROM TRENES WHERE id_t = %d;", id_t);

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("Error al eliminar tren: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("[OK] Tren %d eliminado correctamente.\n", id_t);
    log_evento(cfg.log_path, NULL, "DELETE", "Tren eliminado");
    sqlite3_close(db);
    return 0;
}

void buscar_tren_por_modelo(const char *modelo) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) return;

    const char *sql = "SELECT id_t, modelo, num_serie, anio_fab, estado_mant FROM TRENES WHERE modelo LIKE ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        return;
    }

    char patron[68];
    snprintf(patron, sizeof(patron), "%%%s%%", modelo);
    sqlite3_bind_text(stmt, 1, patron, -1, SQLITE_TRANSIENT);

    printf("\n--- RESULTADOS BUSQUEDA: %s ---\n", modelo);
    printf("%-5s | %-20s | %-15s | %-6s | %-12s\n",
           "ID", "MODELO", "NUM SERIE", "AÑO", "ESTADO");
    printf("---------------------------------------------------------------\n");

    int encontrados = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("%-5d | %-20s | %-15s | %-6d | %-12s\n",
            sqlite3_column_int(stmt, 0),
            (const char*)sqlite3_column_text(stmt, 1),
            (const char*)sqlite3_column_text(stmt, 2),
            sqlite3_column_int(stmt, 3),
            (const char*)sqlite3_column_text(stmt, 4));
        encontrados++;
    }

    if (encontrados == 0) printf("No se encontraron trenes con ese modelo.\n");

    sqlite3_finalize(stmt);
    sqlite3_close(db);

}

int modificar_tren_db(int id_t, const char *modelo, const char *num_serie,int anio, EstadoMantenimiento estado, const char *fecha_rev){
	sqlite3 *db = abrir_bd();
	if (!db){
		return 1;
	}
	sqlite3_stmt *stmt;
	const char *sql= "UPDATE TRENES SET nombre_modelo=?,num_serie=?,anio_fab=?,estado_mant=?,fecha_ultima_revision=? WHERE id_t=?;";
	sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
	sqlite3_bind_text(stmt,1,modelo,-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,2,num_serie,-1,SQLITE_TRANSIENT);
	sqlite3_bind_int (stmt,3,anio);
	sqlite3_bind_text(stmt,4,estado_tren_str(estado),-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,5,(fecha_rev&&strlen(fecha_rev)>0)?fecha_rev:NULL,-1,SQLITE_TRANSIENT); //Si el tren no tiene fecha le asigna un NULL
	sqlite3_bind_int (stmt,6,id_t);
	 int rc = sqlite3_step(stmt);
	 sqlite3_finalize(stmt);
	 sqlite3_close(db);
	 return (rc==SQLITE_DONE)?0:1;
}

int  cambiar_estado_tren_db(int id_t, EstadoMantenimiento estado){
	sqlite3 *db = abrir_bd();
			if(!db) return -1;
			char et = "";
			if (estado == TREN_AVERIA){
				et = "AVERIA";
			}else if (estado == TREN_OPERATIVO){
				et = "OPERATIVO";
			}else if (estado == TREN_RETIRADO){
				et = "RETIRADO";
			}else if (estado == TREN_REVISION){
				et = "REVISION";
			}
			sqlite3_stmt *stmt;
			const char *sql = "UPDATE TRENES SET estado_mant = ? WHERE id_t = ?";
			 if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
				        sqlite3_bind_int(stmt, 1, et);
				        sqlite3_bind_int(stmt, 2, id_t);
				        sqlite3_step(stmt);
				        sqlite3_finalize(stmt);
				    }
			 sqlite3_close(db);
			 return 0;
}

/* ============================================================
 *  VAGONES
 * ============================================================ */

int  insertar_vagon_db(Vagon v){
	sqlite3 *db = abrir_bd();
	    if (!db) return 1;

	    sqlite3_stmt *stmt;
	    const char *sql =
	        "INSERT INTO VAGONES (id_vagon,id_tren,numero_vagon,clase,capacidad_total,vagon_PMR)"
	        " VALUES (?,?,?,?,?);";

	    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
	        sqlite3_close(db); return 1;
	    }

	    sqlite3_bind_int(stmt, 1, v.id_vagon);
	    sqlite3_bind_int(stmt, 2, v.id_tren);
	    sqlite3_bind_int (stmt, 3, v.numero_vagon);
	    sqlite3_bind_text(stmt, 4, v.clase,   -1, SQLITE_TRANSIENT);
	    sqlite3_bind_int(stmt, 5,v.capacidad_total);
	    sqlite3_bind_int(stmt, 6,v.vagon_PMR);

	    int rc = sqlite3_step(stmt);
	    sqlite3_finalize(stmt);
	    sqlite3_close(db);
	    return (rc == SQLITE_DONE) ? 0 : 1;
}

void listar_vagones_tren(int id_tren) {
    sqlite3 *db = abrir_bd(); if (!db) return;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT numero_vagon,clase,capacidad_total,vagon_PMR FROM VAGONES WHERE id_tren=? ORDER BY numero_vagon;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_tren);
    printf("\n  %-6s | %-5s | %-8s | PMR\n","VAGON","CLASE","CAPAC.");
    printf("  -------+-------+----------+----\n");
    while (sqlite3_step(stmt)==SQLITE_ROW)
        printf("  %-6d | %-5s | %-8d | %s\n",
               sqlite3_column_int(stmt,0),(const char*)sqlite3_column_text(stmt,1),
               sqlite3_column_int(stmt,2),sqlite3_column_int(stmt,3)?"Si":"No");
    sqlite3_finalize(stmt); sqlite3_close(db);
}

int contar_asientos_libres(int id_tr, const char *fecha_viaje, int num_vagon, const char *clase) {
    sqlite3 *db = abrir_bd();
    if (!db) {
        return -1;
    }

    Trayecto tr = obtener_trayecto_por_id(id_tr);
    if (tr.id_tr == -1) {
        sqlite3_close(db);
        return -1;
    }

    sqlite3_stmt *stmt;
    int capacidad = 0;
    int ocupados  = 0;

    const char *sql_cap =
        "SELECT capacidad_total FROM VAGONES WHERE id_tren = ? AND numero_vagon = ? AND clase = ?;";

    if (sqlite3_prepare_v2(db, sql_cap, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int (stmt, 1, tr.id_t);
        sqlite3_bind_int (stmt, 2, num_vagon);
        sqlite3_bind_text(stmt, 3, clase, -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            capacidad = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    if (capacidad == 0) {
        sqlite3_close(db);
        return 0;
    }

    // Asientos ocupados (reservas confirmadas o activas)
    const char *sql_ocup =
        "SELECT COUNT(*) FROM RESERVAS WHERE id_tr = ? AND fecha_viaje = ? AND num_vagon = ? AND estado NOT IN ('CANCELADA');";

    if (sqlite3_prepare_v2(db, sql_ocup, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int (stmt, 1, id_tr);
        sqlite3_bind_text(stmt, 2, fecha_viaje, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (stmt, 3, num_vagon);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            ocupados = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    return capacidad - ocupados;
}

void mostrar_mapa_asientos(int id_tr, const char *fecha_viaje, int num_vagon) {
    sqlite3 *db = abrir_bd(); if (!db) return;

    sqlite3_stmt *stmt;
    int capacidad = 0;
    sqlite3_prepare_v2(db,
        "SELECT v.capacidad_total FROM VAGONES v "
        "JOIN TRAYECTOS t ON t.id_t = v.id_tren "
        "WHERE t.id_tr=? AND v.numero_vagon=?;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_tr); sqlite3_bind_int(stmt,2,num_vagon);
    if (sqlite3_step(stmt)==SQLITE_ROW){
    	capacidad = sqlite3_column_int(stmt,0);
    }
    sqlite3_finalize(stmt);
    if (capacidad==0){
    	capacidad=50;
    }


    sqlite3_stmt *s2;
    sqlite3_prepare_v2(db,
        "SELECT num_asiento FROM RESERVAS WHERE id_tr=? AND fecha_viaje=?"
        " AND num_vagon=? AND estado IN ('CONFIRMADA','PENDIENTE');",
        -1,&s2,NULL);
    sqlite3_bind_int (s2,1,id_tr);
    sqlite3_bind_text(s2,2,fecha_viaje,-1,SQLITE_TRANSIENT);
    sqlite3_bind_int (s2,3,num_vagon);
    int ocupado[MAX_ASIENTOS+1]; memset(ocupado,0,sizeof(ocupado));
    while (sqlite3_step(s2)==SQLITE_ROW) {
        int a = sqlite3_column_int(s2,0);
        if (a>0 && a<=MAX_ASIENTOS) ocupado[a]=1;
    }
    sqlite3_finalize(s2); sqlite3_close(db);


    printf("\n  MAPA DE ASIENTOS  Vagon %d  (capacidad %d)\n", num_vagon, capacidad);
    printf("  Numero = libre  |[XX]| = ocupado\n");
    printf("  +-----+-----+-----+-----+\n");
    printf("  |  A  |  B  |  C  |  D  |\n");
    printf("  +-----+-----+-----+-----+\n");
    int filas = (capacidad+3)/4;
    for (int f=1; f<=filas; f++) {
        printf("  |");
        for (int c=0; c<4; c++) {
            int asiento = (f-1)*4 + c + 1;
            if (asiento <= capacidad) {
                if (ocupado[asiento]){
                	printf("[%2d] |", asiento);
                }
                else {
                	printf(" %2d  |", asiento);
                }
            } else printf("     |");
        }
        printf(" F%02d\n", f);
    }
    printf("  +-----+-----+-----+-----+\n");
    printf("  Introduce el numero del asiento libre (ej: 1 equivale a A-F01,etc).\n");
}

bool asiento_libre(int id_tr, const char *fecha, int vagon, int asiento) {
    sqlite3 *db = abrir_bd(); if (!db) return false;
    sqlite3_stmt *stmt; int n=0;
    sqlite3_prepare_v2(db,
        "SELECT COUNT(*) FROM RESERVAS WHERE id_tr=? AND fecha_viaje=?"
        " AND num_vagon=? AND num_asiento=? AND estado IN ('CONFIRMADA','PENDIENTE');",
        -1,&stmt,NULL);
    sqlite3_bind_int (stmt,1,id_tr);
    sqlite3_bind_text(stmt,2,fecha,-1,SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt,3,vagon);
    sqlite3_bind_int (stmt,4,asiento);
    if (sqlite3_step(stmt)==SQLITE_ROW) n=sqlite3_column_int(stmt,0);
    sqlite3_finalize(stmt); sqlite3_close(db);
    return n==0;
}


/* ============================================================
 *  ESTACIONES
 * ============================================================ */
int insertar_estacion_db(Estacion e) {
    sqlite3 *db = abrir_bd();
    if (!db) return 1;

    sqlite3_stmt *stmt;
    const char *sql =
        "INSERT INTO ESTACIONES (nombre,codigo_gtfs,ciudad,provincia,"
        "latitud,longitud,num_andenes,tiene_sala_club) VALUES (?,?,?,?,?,?,?,?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db); return 1;
    }
    sqlite3_bind_text  (stmt,1,e.nombre,      -1,SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt,2,e.codigo_gtfs, -1,SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt,3,e.ciudad,      -1,SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt,4,e.provincia,   -1,SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt,5,e.latitud);
    sqlite3_bind_double(stmt,6,e.longitud);
    sqlite3_bind_int   (stmt,7,e.num_andenes);
    sqlite3_bind_int   (stmt,8,e.tiene_sala_club);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc == SQLITE_DONE) ? 0 : 1;
}

void listar_estaciones_db() {
    sqlite3 *db = abrir_bd();
    if (!db) return;

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db,
        "SELECT id_est,nombre,ciudad,provincia,num_andenes,tiene_sala_club FROM ESTACIONES ORDER BY id_est;",
        -1,&stmt,NULL) != SQLITE_OK) {
        sqlite3_close(db); return;
    }

    printf("\n%-4s | %-25s | %-15s | %-12s | %-7s | %s\n",
           "ID","NOMBRE","CIUDAD","PROVINCIA","ANDENES","SALA CLUB");
    printf("-----+---------------------------+-----------------+--------------+---------+-----------\n");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("%-4d | %-25s | %-15s | %-12s | %-7d | %s\n",
               sqlite3_column_int (stmt,0),
               sqlite3_column_text(stmt,1),
               sqlite3_column_text(stmt,2),
               sqlite3_column_text(stmt,3),
               sqlite3_column_int (stmt,4),
               sqlite3_column_int (stmt,5) ? "Si" : "No");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
int  modificar_estacion_db(int id_est, const char *nombre, const char *ciudad, const char *provincia, int num_andenes){
	 sqlite3 *db = abrir_bd(); if (!db) return 1;
	    sqlite3_stmt *stmt;
	    sqlite3_prepare_v2(db,
	        "UPDATE ESTACIONES SET nombre=?,ciudad=?,provincia=?,num_andenes=? WHERE id_est=?;",
	        -1,&stmt,NULL);
	    sqlite3_bind_text(stmt,1,nombre,  -1,SQLITE_TRANSIENT);
	    sqlite3_bind_text(stmt,2,ciudad,  -1,SQLITE_TRANSIENT);
	    sqlite3_bind_text(stmt,3,provincia,-1,SQLITE_TRANSIENT);
	    sqlite3_bind_int (stmt,4,num_andenes);
	    sqlite3_bind_int (stmt,5,id_est);
	    int rc = sqlite3_step(stmt);
	    sqlite3_finalize(stmt); sqlite3_close(db);
	    return (rc==SQLITE_DONE)?0:1;
}

int toggle_sala_club_db(int id_est){
	sqlite3 *db = abrir_bd();
	if(!db){
		return 1;
	}
	sqlite3_stmt *stmt;
	const char *sql="UPDATE ESTACIONES SET tiene_sala_club = CASE WHEN tiene_sala_club=1 THEN 0 ELSE 1 END WHERE id_est=?;";
	sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
	sqlite3_bind_int(stmt,1,id_est);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return (rc==SQLITE_DONE)?0:1;
}


Estacion obtener_estacion_por_id(int id_est) {
	Estacion e;
	sqlite3 *db = abrir_bd();
	if (!db) {
	    return e;
	}
    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT id_est, nombre, codigo_gtfs, ciudad, provincia, "
        "latitud, longitud, num_andenes, tiene_sala_club "
        "FROM ESTACIONES WHERE id_est = ?;";
	    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id_est);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            e.id_est = sqlite3_column_int(stmt, 0);
            strncpy(e.nombre, (const char*)sqlite3_column_text(stmt, 1), sizeof(e.nombre));
            strncpy(e.codigo_gtfs, (const char*)sqlite3_column_text(stmt, 2), sizeof(e.codigo_gtfs));
            strncpy(e.ciudad, (const char*)sqlite3_column_text(stmt, 3), sizeof(e.ciudad));
            strncpy(e.provincia, (const char*)sqlite3_column_text(stmt, 4), sizeof(e.provincia));
            e.latitud = sqlite3_column_double(stmt, 5);
            e.longitud = sqlite3_column_double(stmt, 6);
            e.num_andenes = sqlite3_column_int   (stmt, 7);
            e.tiene_sala_club = sqlite3_column_int  (stmt, 8);
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    return e;
}



/* ============================================================
 *  TRAYECTOS
 * ============================================================ */
int insertar_trayecto_db(Trayecto tr) {
    sqlite3 *db = abrir_bd();
    if (!db) return 1;

    sqlite3_stmt *stmt;
    const char *sql =
        "INSERT INTO TRAYECTOS (id_t,id_est_origen,id_est_destino,"
        "hora_salida,hora_llegada,duracion_min,precio_base,dias_operacion,estado)"
        " VALUES (?,?,?,?,?,?,?,?,?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db); return 1;
    }
    sqlite3_bind_int   (stmt,1,tr.id_t);
    sqlite3_bind_int   (stmt,2,tr.id_est_origen);
    sqlite3_bind_int   (stmt,3,tr.id_est_destino);
    sqlite3_bind_text  (stmt,4,tr.hora_salida,    -1,SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt,5,tr.hora_llegada,   -1,SQLITE_TRANSIENT);
    sqlite3_bind_int   (stmt,6,tr.duracion_min);
    sqlite3_bind_double(stmt,7,tr.precio_base);
    sqlite3_bind_text  (stmt,8,tr.dias_operacion, -1,SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt,9,tr.estado == TRAYECTO_ACTIVO ? "ACTIVO":"INACTIVO",-1,SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc == SQLITE_DONE) ? 0 : 1;
}

void listar_trayectos_db() {
    sqlite3 *db = abrir_bd();
    if (!db) return;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT t.id_tr, eo.nombre, ed.nombre, t.hora_salida, t.hora_llegada,"
        " t.precio_base, t.estado"
        " FROM TRAYECTOS t"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " ORDER BY t.id_tr;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db); return;
    }

    printf("\n%-4s | %-20s | %-20s | %-5s | %-5s | %-8s | %s\n",
           "ID","ORIGEN","DESTINO","SAL","LLEGA","PRECIO","ESTADO");
    printf("-----+----------------------+----------------------+-------+-------+----------+--------\n");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("%-4d | %-20s | %-20s | %-5s | %-5s | %8.2f | %s\n",
               sqlite3_column_int (stmt,0),
               sqlite3_column_text(stmt,1),
               sqlite3_column_text(stmt,2),
               sqlite3_column_text(stmt,3),
               sqlite3_column_text(stmt,4),
               sqlite3_column_double(stmt,5),
               sqlite3_column_text(stmt,6));
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
void listar_trayectos_filtro(const char *estacion_origen, const char *estacion_destino) {
    sqlite3 *db = abrir_bd();
    if (!db) return;

    sqlite3_stmt *stmt;
    const char *sql =
        "SELECT t.id_tr, eo.nombre, ed.nombre, t.hora_salida, t.hora_llegada,"
        " t.precio_base, t.estado"
        " FROM TRAYECTOS t"
        " JOIN ESTACIONES eo ON t.id_est_origen  = eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino = ed.id_est"
        " WHERE LOWER(eo.ciudad) = LOWER(?1)"
        "   AND LOWER(ed.ciudad) = LOWER(?2)"
        " ORDER BY t.id_tr;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error al preparar consulta: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }
    printf("[DEBUG] Buscando: '%s' -> '%s'\n", estacion_origen, estacion_destino);
    sqlite3_bind_text(stmt, 1, estacion_origen,  -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, estacion_destino, -1, SQLITE_STATIC);

    printf("\n%-4s | %-20s | %-20s | %-5s | %-5s | %-8s | %s\n",
           "ID","ORIGEN","DESTINO","SAL","LLEGA","PRECIO","ESTADO");
    printf("-----+----------------------+----------------------+-------+-------+----------+--------\n");

    int encontrados = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        printf("%-4d | %-20s | %-20s | %-5s | %-5s | %8.2f | %s\n",
               sqlite3_column_int   (stmt, 0),
               sqlite3_column_text  (stmt, 1),
               sqlite3_column_text  (stmt, 2),
               sqlite3_column_text  (stmt, 3),
               sqlite3_column_text  (stmt, 4),
               sqlite3_column_double(stmt, 5),
               sqlite3_column_text  (stmt, 6));
        encontrados++;
    }

    if (encontrados == 0)
        printf("No se encontraron trayectos de '%s' a '%s'.\n",
               estacion_origen, estacion_destino);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
#define MAX_CAMPO 128
int cargar_trayectos_csv (const char *ruta_csv){
	sqlite3 *db = abrir_bd();

	FILE *f = fopen(ruta_csv, "r");
	    if (f == NULL) {
	        log_evento(cfg.log_path, NULL, "ERROR", "No se pudo abrir el CSV de trayectos");
	        return -1;
	    }

	    char linea[512];
	    int insertados = 0;
	    int es_cabecera = 1;  /* la primera linea es la cabecera, la saltamos */

	    while (fgets(linea, sizeof(linea), f) != NULL) {

	        /* Saltar la primera linea (cabecera) */
	        if (es_cabecera) {
	            es_cabecera = 0;
	            continue;
	        }

	        /* Eliminar el salto de linea al final si existe */
	        linea[strcspn(linea, "\r\n")] = '\0';

	        /* Variables para cada campo del CSV */
	        char id_tr[MAX_CAMPO];
	        char id_t[MAX_CAMPO];
	        char id_est_origen[MAX_CAMPO];
	        char id_est_destino[MAX_CAMPO];
	        char hora_salida[MAX_CAMPO];
	        char hora_llegada[MAX_CAMPO];
	        char duracion_min[MAX_CAMPO];
	        char precio_base[MAX_CAMPO];
	        char dias_operacion[MAX_CAMPO];
	        char estado[MAX_CAMPO];

	        /* Separar la linea por ';' copiando para no modificar 'linea' */
	        char copia[512];
	        strncpy(copia, linea, sizeof(copia));

	        char *token = strtok(copia, ";");
	        if (token == NULL) continue;
	        strncpy(id_tr,          token, MAX_CAMPO);
	        token = strtok(NULL, ";");
	        if (token == NULL) continue;
	        strncpy(id_t,           token, MAX_CAMPO);
	        token = strtok(NULL, ";");
	        if (token == NULL) continue;
	        strncpy(id_est_origen,  token, MAX_CAMPO);
	        token = strtok(NULL, ";");
	        if (token == NULL) continue;
	        strncpy(id_est_destino, token, MAX_CAMPO);
	        token = strtok(NULL, ";");
	        if (token == NULL) continue;
	        strncpy(hora_salida, token, MAX_CAMPO);
	        token = strtok(NULL, ";");
	        if (token == NULL) continue;
	        strncpy(hora_llegada, token, MAX_CAMPO);
	        token = strtok(NULL, ";");
	        if (token == NULL) continue;
	        strncpy(duracion_min, token, MAX_CAMPO);
	        token = strtok(NULL, ";");
	        if (token == NULL) continue;
	        strncpy(precio_base, token, MAX_CAMPO);
	        token = strtok(NULL, ";");
	        if (token == NULL) continue;
	        strncpy(dias_operacion, token, MAX_CAMPO);
	        token = strtok(NULL, ";");
	        if (token == NULL) continue;
	        strncpy(estado, token, MAX_CAMPO);

	        /* Construir la sentencia SQL */
	        char sql[512];
	        snprintf(sql, sizeof(sql),
	            "INSERT INTO TRAYECTOS "
	            "(origen_id, destino_id, hora_salida, hora_llegada, dias_operativos, estado) "
	            "VALUES (%s, %s, '%s', '%s', '%s', '%s');",
	            id_est_origen,
	            id_est_destino,
	            hora_salida,
	            hora_llegada,
	            dias_operacion,
	            estado
	        );

	        /* Ejecutar la sentencia en la BD */
	        char *error_sql = NULL;
	        int resultado = sqlite3_exec(db, sql, NULL, NULL, &error_sql);

	        if (resultado != SQLITE_OK) {
	            printf("Error al insertar trayecto (linea %d): %s\n", insertados + 2, error_sql);
	            sqlite3_free(error_sql);
	        } else {
	            insertados++;
	        }
	    }
	    char mensaje_log[128];
	    snprintf(mensaje_log, sizeof(mensaje_log),
	                "Carga CSV trayectos completada: %d trayectos insertados", insertados);
	    log_evento(cfg.log_path, NULL, "INSERT", mensaje_log);
	    fclose(f);

	return insertados;
}

int modificar_trayecto_db(int id_tr, const char *hora_sal, const char *hora_ll, double precio, const char *dias){
	sqlite3 *db = abrir_bd();
	if (!db){
		return 1;
	}
	sqlite3_stmt *stmt;
	const char *sql="UPDATE TRAYECTOS SET hora_salida=?,hora_llegada=?,precio_base=?,dias_operacion=? WHERE id_tr=?;";
	sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
	sqlite3_bind_text(stmt,1,hora_sal,-1,SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt,2,hora_ll, -1,SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt,3,precio);
	sqlite3_bind_text(stmt,4,dias,-1,SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt,5,id_tr);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return (rc==SQLITE_DONE)?0:1;
}

int cambiar_estado_trayecto_db(int id_tr, EstadoTrayecto estado){
	sqlite3 *db = abrir_bd();
	if (!db){
		return 1;
	}
	sqlite3_stmt *stmt;
	const char *sql="UPDATE TRAYECTOS SET estado=? WHERE id_tr=?;";
	sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
	sqlite3_bind_text(stmt,1,estado==TRAYECTO_ACTIVO?"ACTIVO":"INACTIVO",-1,SQLITE_TRANSIENT);
	sqlite3_bind_int (stmt,2,id_tr);
	int rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return (rc==SQLITE_DONE)?0:1;

}

Trayecto obtener_trayecto_por_id(int id_tr){
	Trayecto tr;
	memset(&tr,0,sizeof(tr));
	tr.id_tr=-1;
	sqlite3 *db = abrir_bd();
	if (!db){
		return tr;
	}
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db,
	"SELECT id_tr,id_t,id_est_origen,id_est_destino,hora_salida,hora_llegada,duracion_min,precio_base,dias_operacion,estado FROM TRAYECTOS WHERE id_tr=?;",
	-1,&stmt,NULL);
	sqlite3_bind_int(stmt,1,id_tr);
	if (sqlite3_step(stmt)==SQLITE_ROW) {
		tr.id_tr=sqlite3_column_int(stmt,0);
		tr.id_t=sqlite3_column_int(stmt,1);
		tr.id_est_origen=sqlite3_column_int(stmt,2);
		tr.id_est_destino=sqlite3_column_int(stmt,3);
		strncpy(tr.hora_salida, (const char*)sqlite3_column_text(stmt,4),5);
		strncpy(tr.hora_llegada,(const char*)sqlite3_column_text(stmt,5),5); //copia el texto que hemos encontrado y como maximo pega 5 letras
		tr.duracion_min=sqlite3_column_int(stmt,6);
		tr.precio_base=sqlite3_column_double(stmt,7);
		strncpy(tr.dias_operacion,(const char*)sqlite3_column_text(stmt,8),7);
		tr.estado=strcmp((const char*)sqlite3_column_text(stmt,9),"ACTIVO")==0?
		TRAYECTO_ACTIVO:TRAYECTO_INACTIVO;

	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return tr;
}

int buscar_trayectos_db(int id_origen, int id_destino, const char *fecha, const char *clase){
	sqlite3 *db = abrir_bd();
	if (!db){
		return 0;
	}
	sqlite3_stmt *stmt;
	/* Determina día semana de la fecha */
	sqlite3_prepare_v2(db,
	"SELECT t.id_tr, eo.nombre, ed.nombre, t.hora_salida, t.hora_llegada,"
	" t.duracion_min, t.precio_base, t.dias_operacion"
	" FROM TRAYECTOS t"
	" JOIN ESTACIONES eo ON t.id_est_origen=eo.id_est"
	" JOIN ESTACIONES ed ON t.id_est_destino=ed.id_est"
	" WHERE t.id_est_origen=? AND t.id_est_destino=? AND t.estado='ACTIVO'"
	" ORDER BY t.hora_salida;",
	-1,&stmt,NULL);
	sqlite3_bind_int(stmt,1,id_origen);
	sqlite3_bind_int(stmt,2,id_destino);
	int n=0;
	printf("\n  Trayectos disponibles de %d a %d el %s:\n",id_origen,id_destino,fecha);
	printf("\n  %-3s | %-20s | %-20s | %5s | %5s | %5s | %8s\n",
	       "ID","ORIGEN","DESTINO","SAL.","LLEGA","MIN","PRECIO");
	printf("  ----+----------------------+----------------------+-------+-------+-------+----------\n");
	while (sqlite3_step(stmt)==SQLITE_ROW) {
	   double precio = sqlite3_column_double(stmt,6);
	   if (strcmp(clase,"B")==0){
		   precio *= cfg.coef_business;

	   }
	   printf("  %-3d | %-20s | %-20s | %-5s | %-5s | %-5d | %8.2f\n",
	           sqlite3_column_int(stmt,0),
	           (const char*)sqlite3_column_text(stmt,1),
	           (const char*)sqlite3_column_text(stmt,2),
	           (const char*)sqlite3_column_text(stmt,3),
	           (const char*)sqlite3_column_text(stmt,4),
	           sqlite3_column_int(stmt,5),
			   precio);
	    n++;
	}
	if (!n){
		printf(" [No hay trayectos disponibles]\n");
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return n;
}

/* ============================================================
 *  PARADAS INTERMEDIAS
 * ============================================================ */
int insertar_parada_db(ParadaIntermedia p) {
	sqlite3 *db = abrir_bd();
	if (!db) return 1;
    sqlite3_stmt *stmt;
    const char *sql =
    		"INSERT INTO PARADAS_INTERMEDIAS (id_tr,id_est,orden,hora_llegada,hora_salida,anden) VALUES (?,?,?,?,?,?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db); return 1;
    }
    sqlite3_bind_int   (stmt,1,p.id_tr);
    sqlite3_bind_int   (stmt,2,p.id_est);
    sqlite3_bind_int   (stmt,3,p.orden);
    sqlite3_bind_text  (stmt,4,p.hora_llegada,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt,5,p.hora_salida,-1,SQLITE_TRANSIENT);
    sqlite3_bind_int   (stmt,6,p.tiene_anden?p.anden:0);
	int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc == SQLITE_DONE) ? 0 : 1;
}

void listar_paradas_trayecto(int id_tr) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT pi.id_parada, pi.orden, e.nombre, pi.hora_llegada, pi.hora_salida, pi.anden"
        " FROM PARADAS_INTERMEDIAS pi"
        " JOIN ESTACIONES e ON pi.id_est=e.id_est"
        " WHERE pi.id_tr=? ORDER BY pi.orden;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_tr);
    printf("\n  %-7s | %-5s | %-25s | %-7s | %-7s | %s\n",
           "ID_PAR","ORDEN","ESTACION","LLEGADA","SALIDA","ANDEN");
    printf("  --------+-------+---------------------------+---------+---------+------\n");
    int n=0;
    while (sqlite3_step(stmt)==SQLITE_ROW) {
        printf("  %-7d | %-5d | %-25s | %-7s | %-7s | %d\n",
               sqlite3_column_int(stmt,0),
               sqlite3_column_int(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
               (const char*)sqlite3_column_text(stmt,3),
               (const char*)sqlite3_column_text(stmt,4),
               sqlite3_column_int(stmt,5));
        n++;
    }
    if (!n) {
    	printf("  [Sin paradas intermedias]\n");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

int eliminar_parada_db(int id_parada) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,"DELETE FROM PARADAS_INTERMEDIAS WHERE id_parada=?;",-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_parada);
    int rc = sqlite3_step(stmt);
    int changes = (int)sqlite3_changes(db);
    sqlite3_finalize(stmt); sqlite3_close(db);
    return (rc==SQLITE_DONE && changes>0)?0:1;
}
//RESERVAS

void generar_codigo_validacion(char *buf, int len) {
    srand((unsigned int)time(NULL)+(unsigned int)(long)buf);
    const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int n = len-1;
    for (int i=0;i<n;i++) buf[i]=chars[rand()%36];
    buf[n]='\0';
}

double calcular_precio_final(int id_tr, const char *clase, TipoDescuento desc, double suplementos_extra) {
    sqlite3 *db = abrir_bd();
    if (!db) {
    	return 0.0;
    }
    sqlite3_stmt *stmt;
    double precio_base=0;
    double coef_business=cfg.coef_business;
    const char *sql="SELECT t.precio_base, COALESCE(ta.coef_business, ?) FROM TRAYECTOS t LEFT JOIN TARIFAS ta ON ta.id_tr=t.id_tr WHERE t.id_tr=?;";
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    sqlite3_bind_double(stmt,1,cfg.coef_business);
    sqlite3_bind_int   (stmt,2,id_tr);
    if (sqlite3_step(stmt)==SQLITE_ROW) {
        precio_base = sqlite3_column_double(stmt,0);
        coef_business = sqlite3_column_double(stmt,1);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    double precio = precio_base;
    if (strcmp(clase,"B")==0){
    	precio *= coef_business;
    }

    double pct_desc = 0.0;
    switch (desc) {
        case DESCUENTO_JOVEN:    pct_desc = 20.0; break;
        case DESCUENTO_DORADA:   pct_desc = 40.0; break;
        case DESCUENTO_NUMEROSA: pct_desc = 20.0; break;
        case DESCUENTO_ABONO:    pct_desc = 50.0; break;
        default: break;
    }
    precio *= (1.0 - pct_desc/100.0);
    precio += suplementos_extra;
    return precio;
}

int insertar_reserva_db(Reserva r) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return -1;
    }
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO RESERVAS (id_u,id_tr,fecha_viaje,clase,num_vagon,num_asiento,precio_base,descuento_pct,precio_final,estado,codigo_validacion,fecha_reserva) VALUES (?,?,?,?,?,?,?,?,?,?,?,date('now'));";
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,r.id_u);
    sqlite3_bind_int(stmt,2,r.id_tr);
    sqlite3_bind_text(stmt,3,r.fecha_viaje,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,4,r.clase,      -1,SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,5,r.num_vagon);
    sqlite3_bind_int(stmt,6,r.num_asiento);
    sqlite3_bind_double(stmt,7,r.precio_base);
    sqlite3_bind_double(stmt,8,r.descuento_pct);
    sqlite3_bind_double(stmt,9,r.precio_final);
    sqlite3_bind_text(stmt,10,"CONFIRMADA",-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,11,r.codigo_validacion,-1,SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    int new_id = (rc==SQLITE_DONE)?(int)sqlite3_last_insert_rowid(db):-1;
    sqlite3_finalize(stmt);

    /* Sumar puntos (1 punto por euro gastado) */
    if (new_id > 0) {
        int puntos = (int)r.precio_final;
        char sql2[256];
        snprintf(sql2,sizeof(sql2),
            "UPDATE DATOS_PASAJERO SET puntos_fidelidad=puntos_fidelidad+%d WHERE id_u=%d;",
            puntos, r.id_u);
        sqlite3_exec(db,sql2,0,0,NULL);
    }
    sqlite3_close(db);
    return new_id;
}

void listar_reservas_usuario(int id_u) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT r.id_res, eo.nombre, ed.nombre, r.fecha_viaje,"
        " r.clase, r.num_vagon, r.num_asiento, r.precio_final,"
        " r.estado, r.codigo_validacion"
        " FROM RESERVAS r"
        " JOIN TRAYECTOS t ON r.id_tr=t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen=eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino=ed.id_est"
        " WHERE r.id_u=? ORDER BY r.fecha_viaje DESC;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_u);
    printf("\n%-5s | %-18s | %-18s | %-10s | %5s | %-5s | %4s | %8s | %-12s | %s\n",
           "ID","ORIGEN","DESTINO","FECHA","CLASE","VAGON","ASIE","PRECIO","ESTADO","COD. VALID.");
    printf("------+--------------------+--------------------+------------+-------+-------+------+----------+--------------+----------\n");
    int n=0;
    while (sqlite3_step(stmt)==SQLITE_ROW) {
        printf("%-5d | %-18s | %-18s | %-10s | %-5s | %-5d | %4d | %8.2f | %-12s | %s\n",
               sqlite3_column_int(stmt,0),
               (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
               (const char*)sqlite3_column_text(stmt,3),
               (const char*)sqlite3_column_text(stmt,4),
               sqlite3_column_int(stmt,5),sqlite3_column_int(stmt,6),
               sqlite3_column_double(stmt,7),
               (const char*)sqlite3_column_text(stmt,8),
               (const char*)sqlite3_column_text(stmt,9));
        n++;
    }
    if (!n){
    	printf("  [Sin reservas]\n");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void listar_reservas_activas_usuario(int id_u) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT r.id_res, eo.nombre, ed.nombre, r.fecha_viaje,"
        " r.clase, r.num_vagon, r.num_asiento, r.precio_final, r.codigo_validacion"
        " FROM RESERVAS r"
        " JOIN TRAYECTOS t ON r.id_tr=t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen=eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino=ed.id_est"
        " WHERE r.id_u=? AND r.estado='CONFIRMADA' AND r.fecha_viaje >= date('now')"
        " ORDER BY r.fecha_viaje;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_u);
    printf("\n  RESERVAS ACTIVAS:\n");
    printf("  %-5s | %-18s | %-18s | %-10s | %5s | %-5s | %4s | %8s | %s\n",
           "ID","ORIGEN","DESTINO","FECHA","CLASE","VAGON","ASIE","PRECIO","COD. VALID.");
    printf("  ------+--------------------+--------------------+------------+-------+-------+------+----------+----------\n");
    int n=0;
    while (sqlite3_step(stmt)==SQLITE_ROW) {
        printf("  %-5d | %-18s | %-18s | %-10s | %-5s | %-5d | %4d | %8.2f | %s\n",
               sqlite3_column_int(stmt,0),
               (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
               (const char*)sqlite3_column_text(stmt,3),
               (const char*)sqlite3_column_text(stmt,4),
               sqlite3_column_int(stmt,5),sqlite3_column_int(stmt,6),
               sqlite3_column_double(stmt,7),
               (const char*)sqlite3_column_text(stmt,8));
        n++;
    }
    if (!n){
    	printf("  [No tienes reservas activas]\n");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void listar_historial_usuario(int id_u) {
    sqlite3 *db = abrir_bd();
    if (!db) return;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT r.id_res, eo.nombre, ed.nombre, r.fecha_viaje, r.clase, r.precio_final, r.estado"
        " FROM RESERVAS r"
        " JOIN TRAYECTOS t ON r.id_tr=t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen=eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino=ed.id_est"
        " WHERE r.id_u=? ORDER BY r.fecha_viaje DESC;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_u);
    printf("\n  HISTORIAL DE VIAJES:\n");
    printf("  %-5s | %-18s | %-18s | %-10s | %5s | %8s | %s\n",
           "ID","ORIGEN","DESTINO","FECHA","CLASE","PRECIO","ESTADO");
    printf("  ------+--------------------+--------------------+------------+-------+----------+----------\n");
    int n=0;
    while (sqlite3_step(stmt)==SQLITE_ROW) {
        printf("  %-5d | %-18s | %-18s | %-10s | %-5s | %8.2f | %s\n",
               sqlite3_column_int(stmt,0),
               (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
               (const char*)sqlite3_column_text(stmt,3),
               (const char*)sqlite3_column_text(stmt,4),
               sqlite3_column_double(stmt,5),
               (const char*)sqlite3_column_text(stmt,6));
        n++;
    }
    if (!n){
    	printf("  [Sin historial de viajes]\n");
    }
    sqlite3_finalize(stmt); sqlite3_close(db);
}

int cancelar_reserva_db(int id_res, int id_u) {
    sqlite3 *db = abrir_bd();
    if (!db) return 1;
    sqlite3_stmt *stmt;
    const char *sql="UPDATE RESERVAS SET estado='CANCELADA' WHERE id_res=? AND id_u=? AND estado='CONFIRMADA';";
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_res);
    sqlite3_bind_int(stmt,2,id_u);
    int rc = sqlite3_step(stmt);
    int changes = (int)sqlite3_changes(db);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE && changes>0)?0:1;
}

//EQUIPAJE

int insertar_equipaje_db(Equipaje eq) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "INSERT INTO EQUIPAJES (id_res,tipo,peso_kg,dimensiones,exceso_kg,suplemento_pago,facturado)"
        " VALUES (?,?,?,?,?,?,0);", -1,&stmt,NULL);
    const char *tipos[] = {"MANO","BODEGA","BICI","ESQUI"};
    sqlite3_bind_int (stmt,1,eq.id_res);
    sqlite3_bind_text (stmt,2,tipos[eq.tipo],-1,SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt,3,eq.peso_kg);
    sqlite3_bind_text(stmt,4,eq.dimensiones,-1,SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt,5,eq.exceso_kg);
    sqlite3_bind_double(stmt,6,eq.suplemento_pago);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

void listar_equipaje_reserva(int id_res) {
    sqlite3 *db = abrir_bd();
    if (!db) {
    	return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT tipo,peso_kg,exceso_kg,suplemento_pago FROM EQUIPAJES WHERE id_res=?;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_res);
    printf("  %-7s | %-7s | %-9s | %s\n","TIPO","PESO","EXCESO","SUPL.");
    while (sqlite3_step(stmt)==SQLITE_ROW)
        printf("  %-7s | %7.2f | %9.2f | %.2f EUR\n",
               (const char*)sqlite3_column_text(stmt,0),
               sqlite3_column_double(stmt,1),
			   sqlite3_column_double(stmt,2),
               sqlite3_column_double(stmt,3));
    sqlite3_finalize(stmt); sqlite3_close(db);
}

double calcular_suplemento_equipaje(TipoEquipaje tipo, double peso_kg, const char *clase) {
    double sup = 0.0;
    switch(tipo) {
        case EQUIPAJE_MANO:
            break;
        case EQUIPAJE_BODEGA: {
            double limite = (strcmp(clase,"B")==0) ? 25.0 : 15.0;
            if (peso_kg > limite) sup = (peso_kg - limite) * cfg.exceso_kg_precio;
            break;
        }
        case EQUIPAJE_BICI:
        case EQUIPAJE_ESQUI:
            sup = cfg.suplemento_bici;
            break;
    }
    return sup;
}

//SERVICIOS OPERATIVOS

int insertar_servicio_db(ServicioOperativo s) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    sqlite3_stmt *stmt;
    const char *sql="INSERT INTO SERVICIOS_OPERATIVOS (id_tr,fecha,estado_serv) VALUES (?,?,?);";
    sqlite3_prepare_v2(db,sql, -1,&stmt,NULL);
    const char *estados[]={"PROGRAMADO","EN_CURSO","FINALIZADO","CANCELADO"};
    sqlite3_bind_int (stmt,1,s.id_tr);
    sqlite3_bind_text(stmt,2,s.fecha,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,3,estados[s.estado_serv],-1,SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

void listar_servicios_db(const char *filtro_fecha, int filtro_tren) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }
    sqlite3_stmt *stmt;
    char sql[1024];
    strcpy(sql,
        "SELECT so.id_serv, so.fecha, eo.nombre, ed.nombre,"
        " so.estado_serv, so.minutos_retraso, so.hora_inicio_real, so.hora_fin_real"
        " FROM SERVICIOS_OPERATIVOS so"
        " JOIN TRAYECTOS t ON so.id_tr=t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen=eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino=ed.id_est"
        " WHERE 1=1"); //hace un sql dinamico en vez de hacerlo rigido
    if (filtro_fecha && strlen(filtro_fecha)>0)
        strcat(sql," AND so.fecha = ?");//comprueba que realmente le han pasado una fecha y lo pega al final del sql
    if (filtro_tren > 0)
        strcat(sql," AND t.id_t = ?");
    strcat(sql," ORDER BY so.fecha, so.id_serv;");
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    int idx=1;
    if (filtro_fecha && strlen(filtro_fecha)>0)
        sqlite3_bind_text(stmt,idx++,filtro_fecha,-1,SQLITE_TRANSIENT);
    if (filtro_tren>0)
        sqlite3_bind_int(stmt,idx,filtro_tren);
    printf("\n%-5s | %-10s | %-18s | %-18s | %-12s | %-5s | %-7s | %s\n",
           "ID","FECHA","ORIGEN","DESTINO","ESTADO","RETR.","INICIO","FIN");
    printf("------+------------+--------------------+--------------------+--------------+-------+---------+-------\n");
    int n=0;
    while (sqlite3_step(stmt)==SQLITE_ROW) {
        printf("%-5d | %-10s | %-18s | %-18s | %-12s | %-5d | %-7s | %s\n",
               sqlite3_column_int(stmt,0),
			   (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
			   (const char*)sqlite3_column_text(stmt,3),
               (const char*)sqlite3_column_text(stmt,4),
			   sqlite3_column_int(stmt,5),
               sqlite3_column_text(stmt,6)?(const char*)sqlite3_column_text(stmt,6):"-",
               sqlite3_column_text(stmt,7)?(const char*)sqlite3_column_text(stmt,7):"-");
        n++;
    }
    if (!n) {
    	printf("  [Sin servicios]\n");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

int cancelar_servicio_db(int id_serv) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    sqlite3_stmt *stmt;
    const char *sql ="UPDATE SERVICIOS_OPERATIVOS SET estado_serv='CANCELADO' WHERE id_serv=? AND estado_serv!='CANCELADO';";
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_serv);
    int rc = sqlite3_step(stmt);
    int changes = (int)sqlite3_changes(db);
    sqlite3_finalize(stmt); sqlite3_close(db);
    if (rc==SQLITE_DONE && changes==0){
    	printf("  [AVISO] El servicio no existe o ya estaba cancelado.\n");
    }
    return (rc==SQLITE_DONE && changes>0)?0:1;
}


int marcar_inicio_servicio(int id_serv) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    time_t ahora=time(NULL);
    struct tm *tm=localtime(&ahora);
    char hora[6];
    strftime(hora,sizeof(hora),"%H:%M",tm);
    sqlite3_stmt *stmt;
    const char *sql="UPDATE SERVICIOS_OPERATIVOS SET estado_serv='EN_CURSO',hora_inicio_real=? WHERE id_serv=?;";
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    sqlite3_bind_text(stmt,1,hora,-1,SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,2,id_serv);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

int marcar_fin_servicio(int id_serv) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    time_t ahora=time(NULL);
    struct tm *tm=localtime(&ahora);
    char hora[6];
    strftime(hora,sizeof(hora),"%H:%M",tm);
    sqlite3_stmt *stmt;
    const char *sql="UPDATE SERVICIOS_OPERATIVOS SET estado_serv='FINALIZADO',hora_fin_real=? WHERE id_serv=?;";
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    sqlite3_bind_text(stmt,1,hora,-1,SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt,2,id_serv);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

int actualizar_retraso_servicio(int id_serv, int minutos, const char *causa) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE SERVICIOS_OPERATIVOS SET minutos_retraso=?,causa_retraso=? WHERE id_serv=?;";
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,minutos);
    sqlite3_bind_text(stmt,2,causa,-1,SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,3,id_serv);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

void listar_servicios_maquinista(int id_u) {
    sqlite3 *db = abrir_bd();
    if (!db) {
    	return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT so.id_serv, so.fecha, eo.nombre, ed.nombre,"
        " t.hora_salida, t.hora_llegada, so.estado_serv, so.minutos_retraso"
        " FROM ASIGNACION_PERSONAL ap"
        " JOIN SERVICIOS_OPERATIVOS so ON ap.id_serv=so.id_serv"
        " JOIN TRAYECTOS t ON so.id_tr=t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen=eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino=ed.id_est"
        " WHERE ap.id_u=? ORDER BY so.fecha, t.hora_salida;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_u);
    printf("\n  CUADRANTE DE SERVICIOS:\n");
    printf("  %-5s | %-10s | %-18s | %-18s | %5s | %5s | %-12s | %s\n",
           "SERV","FECHA","ORIGEN","DESTINO","SAL.","LLEGA","ESTADO","RETR.");
    printf("  ------+------------+--------------------+--------------------+-------+-------+--------------+------\n");
    int n=0;
    while (sqlite3_step(stmt)==SQLITE_ROW) {
        printf("  %-5d | %-10s | %-18s | %-18s | %-5s | %-5s | %-12s | %d min\n",
               sqlite3_column_int(stmt,0),
			   (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
			   (const char*)sqlite3_column_text(stmt,3),
               (const char*)sqlite3_column_text(stmt,4),
			   (const char*)sqlite3_column_text(stmt,5),
               (const char*)sqlite3_column_text(stmt,6),
			   sqlite3_column_int(stmt,7));
        n++;
    }
    if (!n){
    	printf("  [No tienes servicios asignados]\n");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

ServicioOperativo obtener_servicio_por_id(int id_serv) {
    ServicioOperativo s;
    memset(&s,0,sizeof(s)); //pone en memoria el servicio operativo
    s.id_serv=-1;
    sqlite3 *db = abrir_bd();
    if (!db){
    	return s;
    }
    sqlite3_stmt *stmt;
    const char *sql="SELECT id_serv,id_tr,fecha,estado_serv,hora_inicio_real,hora_fin_real,minutos_retraso,causa_retraso FROM SERVICIOS_OPERATIVOS WHERE id_serv=?;";
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_serv);
    if (sqlite3_step(stmt)==SQLITE_ROW) {
        s.id_serv=sqlite3_column_int(stmt,0);
        s.id_tr=sqlite3_column_int(stmt,1);
        if (sqlite3_column_text(stmt,2)){
        	strncpy(s.fecha,(const char*)sqlite3_column_text(stmt,2),10);
        }
        const char *est=(const char*)sqlite3_column_text(stmt,3);
        if (!strcmp(est,"EN_CURSO")) {
        	s.estado_serv=SERVICIO_EN_CURSO;
        }
        else if (!strcmp(est,"FINALIZADO")){
        	s.estado_serv=SERVICIO_FINALIZADO;
        }
        else if (!strcmp(est,"CANCELADO")) {
        	s.estado_serv=SERVICIO_CANCELADO;
        }
        else {
        	s.estado_serv=SERVICIO_PROGRAMADO;
        }
        if (sqlite3_column_text(stmt,4)){
        	strncpy(s.hora_inicio_real,(const char*)sqlite3_column_text(stmt,4),5);
        	s.tiene_hora_inicio=1;
        }
        if (sqlite3_column_text(stmt,5)){
        	strncpy(s.hora_fin_real,(const char*)sqlite3_column_text(stmt,5),5);
        	s.tiene_hora_fin=1;
        }
        s.minutos_retraso=sqlite3_column_int(stmt,6);
        if (sqlite3_column_text(stmt,7)){
        	strncpy(s.causa_retraso,(const char*)sqlite3_column_text(stmt,7),255);
        	s.tiene_causa=1;
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return s;
}


//ASIGNACION PERSONAL
int insertar_asignacion_db(AsignacionPersonal a) {
    sqlite3 *db = abrir_bd();
    if (!db) {
    	return 1;
    }
    sqlite3_stmt *stmt;
    const char *roles[]={"CONDUCTOR","REVISOR","TECNICO","SERVICIO"};
    const char *sql="INSERT INTO ASIGNACION_PERSONAL (id_serv,id_u,id_t,rol_servicio,observaciones) VALUES (?,?,?,?,?);";
    sqlite3_prepare_v2(db,sql, -1,&stmt,NULL);
    sqlite3_bind_int (stmt,1,a.id_serv);
    sqlite3_bind_int (stmt,2,a.id_u);
    sqlite3_bind_int (stmt,3,a.id_t);
    sqlite3_bind_text(stmt,4,roles[a.rol_servicio],-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,5,a.tiene_observaciones?a.observaciones:NULL,-1,SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

void listar_asignaciones_servicio(int id_serv) {
    sqlite3 *db = abrir_bd();
    if (!db) {
    	return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT ap.id_asig, u.nombre, u.apellido, ap.rol_servicio, ap.observaciones"
        " FROM ASIGNACION_PERSONAL ap"
        " JOIN USUARIOS u ON ap.id_u=u.id_u"
        " WHERE ap.id_serv=?;", -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_serv);
    printf("  %-5s | %-15s | %-15s | %-12s | %s\n","ID","NOMBRE","APELLIDO","ROL","OBSERVACIONES");
    while (sqlite3_step(stmt)==SQLITE_ROW)
        printf("  %-5d | %-15s | %-15s | %-12s | %s\n",
               sqlite3_column_int(stmt,0),
			   (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
			   (const char*)sqlite3_column_text(stmt,3),
               sqlite3_column_text(stmt,4)?(const char*)sqlite3_column_text(stmt,4):"-");
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

int eliminar_asignacion_db(int id_asig) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    sqlite3_stmt *stmt;
    const char *sql="DELETE FROM ASIGNACION_PERSONAL WHERE id_asig=?;";
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_asig);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

//INCIDENCIAS
int insertar_incidencia_db(Incidencia inc) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    const char *tipos[]={"TECNICA","SEGURIDAD","CLIENTE","OPERATIVA"};
    const char *prios[]={"BAJA","MEDIA","ALTA","CRITICA"};
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "INSERT INTO INCIDENCIAS (id_serv,id_u_reporta,tipo,descripcion,"
        "prioridad,estado,fecha_reporte)"
        " VALUES (?,?,?,?,?,'ABIERTA',date('now'));", -1,&stmt,NULL);
    sqlite3_bind_int (stmt,1,inc.id_serv);
    sqlite3_bind_int (stmt,2,inc.id_u_reporta);
    sqlite3_bind_text(stmt,3,tipos[inc.tipo],-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,4,inc.descripcion,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,5,prios[inc.prioridad],-1,SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

void listar_incidencias_db(EstadoIncidencia filtro_estado, int todas) {
    sqlite3 *db = abrir_bd();
    if (!db) {
    	return;
    }
    sqlite3_stmt *stmt;
    const char *estados[]={"ABIERTA","EN_PROCESO","RESUELTA","CERRADA"};
    char sql[512];
    strcpy(sql,
        "SELECT i.id_inc, i.fecha_reporte, i.tipo, i.prioridad, i.estado,"
        " u.nombre, i.descripcion"
        " FROM INCIDENCIAS i"
        " LEFT JOIN USUARIOS u ON i.id_u_reporta=u.id_u");
    if (!todas){
    	strcat(sql," WHERE i.estado=?");//verifica si hay estado
    }
    strcat(sql," ORDER BY CASE i.prioridad WHEN 'CRITICA' THEN 0 WHEN 'ALTA' THEN 1"
               " WHEN 'MEDIA' THEN 2 ELSE 3 END, i.fecha_reporte DESC;");
    sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
    if (!todas) {
    	sqlite3_bind_text(stmt,1,estados[filtro_estado],-1,SQLITE_TRANSIENT);
    }
    printf("\n%-5s | %-10s | %-10s | %-8s | %-12s | %-15s | %s\n",
           "ID","FECHA","TIPO","PRIORID","ESTADO","REPORTA","DESCRIPCION");
    printf("------+------------+------------+----------+--------------+-----------------+---------------------\n");
    int n=0;
    while (sqlite3_step(stmt)==SQLITE_ROW) {
        const char *desc = (const char*)sqlite3_column_text(stmt,6);
        char desc_corta[40];
        strncpy(desc_corta,desc?desc:"",39);
        desc_corta[39]='\0';
        printf("%-5d | %-10s | %-10s | %-8s | %-12s | %-15s | %s\n",
               sqlite3_column_int(stmt,0),
			   (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
			   (const char*)sqlite3_column_text(stmt,3),
               (const char*)sqlite3_column_text(stmt,4),
               sqlite3_column_text(stmt,5)?(const char*)sqlite3_column_text(stmt,5):"SISTEMA",
               desc_corta);
        n++;
    }
    if (!n) {
    	printf("  [Sin incidencias]\n");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

int resolver_incidencia_db(int id_inc, int id_u_resuelve) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    sqlite3_stmt *stmt;
    const char *sql="UPDATE INCIDENCIAS SET estado='RESUELTA',id_u_resuelve=?,fecha_resolucion=date('now') WHERE id_inc=?;";
    sqlite3_prepare_v2(db,sql, -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_u_resuelve);
    sqlite3_bind_int(stmt,2,id_inc);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

void ver_detalle_incidencia(int id_inc) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT i.id_inc,i.tipo,i.prioridad,i.estado,i.descripcion,"
        "i.fecha_reporte,i.fecha_resolucion,ur.nombre,ur.apellido,"
        "ures.nombre,ures.apellido"
        " FROM INCIDENCIAS i"
        " LEFT JOIN USUARIOS ur   ON i.id_u_reporta=ur.id_u"
        " LEFT JOIN USUARIOS ures ON i.id_u_resuelve=ures.id_u"
        " WHERE i.id_inc=?;", -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_inc);
    if (sqlite3_step(stmt)==SQLITE_ROW) {
        printf("\n  =========================================\n");
        printf("  INCIDENCIA #%d\n", sqlite3_column_int(stmt,0));
        printf("  =========================================\n");
        printf("  Tipo      : %s\n",(const char*)sqlite3_column_text(stmt,1));
        printf("  Prioridad : %s\n",(const char*)sqlite3_column_text(stmt,2));
        printf("  Estado    : %s\n",(const char*)sqlite3_column_text(stmt,3));
        printf("  Descripcion:\n    %s\n",(const char*)sqlite3_column_text(stmt,4));
        printf("  Reportada : %s",(const char*)sqlite3_column_text(stmt,5));
        if (sqlite3_column_text(stmt,7)){
        	printf(" por %s %s",(const char*)sqlite3_column_text(stmt,7),(const char*)sqlite3_column_text(stmt,8));
        }
        printf("\n");
        if (sqlite3_column_text(stmt,6)){
        	printf("  Resuelta  : %s%s%s\n", (const char*)sqlite3_column_text(stmt,6), sqlite3_column_text(stmt,9)?" por ":"", sqlite3_column_text(stmt,9)?(const char*)sqlite3_column_text(stmt,9):"");
        }
        printf("  =========================================\n");
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


//TARIFAS

void listar_tarifas_db(void) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT ta.id_tr, eo.nombre, ed.nombre, ta.precio_base,"
        " ta.coef_turista, ta.coef_business, ta.suplemento_bici, ta.exceso_kg_precio"
        " FROM TARIFAS ta"
        " JOIN TRAYECTOS t ON ta.id_tr=t.id_tr"
        " JOIN ESTACIONES eo ON t.id_est_origen=eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino=ed.id_est"
        " ORDER BY ta.id_tr;", -1,&stmt,NULL);
    printf("\n%-3s | %-20s | %-20s | %8s | %6s | %8s | %5s | %s\n",
           "TR","ORIGEN","DESTINO","P.BASE","C.TUR","C.BUS","BICI","EXCS/KG");
    printf("----+----------------------+----------------------+----------+-------+---------+------+--------\n");
    while (sqlite3_step(stmt)==SQLITE_ROW)
        printf("%-3d | %-20s | %-20s | %8.2f | %6.2f | %8.2f | %5.2f | %.2f\n",
               sqlite3_column_int(stmt,0),
               (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
               sqlite3_column_double(stmt,3),
			   sqlite3_column_double(stmt,4),
               sqlite3_column_double(stmt,5),
			   sqlite3_column_double(stmt,6),
               sqlite3_column_double(stmt,7));
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

int modificar_precio_base_trayecto(int id_tr, double nuevo_precio) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    char sql[256];
    snprintf(sql,sizeof(sql),
        "UPDATE TRAYECTOS SET precio_base=%.2f WHERE id_tr=%d;"
        "UPDATE TARIFAS SET precio_base=%.2f WHERE id_tr=%d;",
        nuevo_precio,id_tr,nuevo_precio,id_tr);
    int rc = (sqlite3_exec(db,sql,0,0,NULL)==SQLITE_OK)?0:1;
    sqlite3_close(db);
    return rc;
}

int modificar_coef_business_db(int id_tr, double coef) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    sqlite3_stmt *stmt;
    const char *sql= "UPDATE TARIFAS SET coef_business=? WHERE id_tr=?;";
    sqlite3_prepare_v2(db,sql, -1,&stmt,NULL);
    sqlite3_bind_double(stmt,1,coef);
    sqlite3_bind_int(stmt,2,id_tr);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return (rc==SQLITE_DONE)?0:1;
}

int modificar_suplemento_bici_db(double precio) {
    cfg.suplemento_bici = precio;
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    char sql[128];
    snprintf(sql,sizeof(sql),"UPDATE TARIFAS SET suplemento_bici=%.2f;",precio);
    int rc = (sqlite3_exec(db,sql,0,0,NULL)==SQLITE_OK)?0:1;
    sqlite3_close(db);
    return rc;
}

int modificar_exceso_kg_db(double precio_por_kg) {
    cfg.exceso_kg_precio = precio_por_kg;
    sqlite3 *db = abrir_bd();
    if (!db){
    	return 1;
    }
    char sql[128];
    snprintf(sql,sizeof(sql),"UPDATE TARIFAS SET exceso_kg_precio=%.2f;",precio_por_kg);
    int rc = (sqlite3_exec(db,sql,0,0,NULL)==SQLITE_OK)?0:1;
    sqlite3_close(db);
    return rc;
}

//INFORMES
void informe_ocupacion_tren(int id_tren) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }
    sqlite3_stmt *stmt;
    printf("\n  ========================================\n");
    printf("  INFORME DE OCUPACION – TREN %d\n", id_tren);
    printf("  ========================================\n");
    // Por trayecto
    sqlite3_prepare_v2(db,
        "SELECT t.id_tr, eo.nombre, ed.nombre,"
        " COUNT(r.id_res) as reservas,"
        " ROUND(COUNT(r.id_res)*100.0/"
        " COALESCE((SELECT SUM(v.capacidad_total) FROM VAGONES v WHERE v.id_tren=t.id_t),100),1) as pct"
        " FROM TRAYECTOS t"
        " JOIN ESTACIONES eo ON t.id_est_origen=eo.id_est"
        " JOIN ESTACIONES ed ON t.id_est_destino=ed.id_est"
        " LEFT JOIN RESERVAS r ON r.id_tr=t.id_tr AND r.estado='CONFIRMADA'"
        " WHERE t.id_t=? GROUP BY t.id_tr ORDER BY t.id_tr;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_tren);
    printf("  %-3s | %-20s | %-20s | %8s | %s\n","TR","ORIGEN","DESTINO","RESERVAS","OCUP%");
    printf("  ----+----------------------+----------------------+----------+------\n");
    while (sqlite3_step(stmt)==SQLITE_ROW){
        printf("  %-3d | %-20s | %-20s | %8d | %.1f%%\n",
               sqlite3_column_int(stmt,0),
               (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
               sqlite3_column_int(stmt,3),sqlite3_column_double(stmt,4));
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void informe_ingresos_trayecto(int id_tr) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }
    sqlite3_stmt *stmt;
    printf("\n  ========================================\n");
    printf("  INFORME DE INGRESOS – TRAYECTO %d\n", id_tr);
    printf("  ========================================\n");
    sqlite3_prepare_v2(db,
        "SELECT r.fecha_viaje, COUNT(*) as reservas, SUM(r.precio_final) as ingresos,"
        " AVG(r.precio_final) as precio_medio"
        " FROM RESERVAS r WHERE r.id_tr=? AND r.estado='CONFIRMADA'"
        " GROUP BY r.fecha_viaje ORDER BY r.fecha_viaje DESC LIMIT 30;",
        -1,&stmt,NULL);
    sqlite3_bind_int(stmt,1,id_tr);
    printf("  %-10s | %8s | %10s | %s\n","FECHA","RESERVAS","INGRESOS","P.MEDIO");
    printf("  -----------+----------+------------+--------\n");
    double total=0;
    while (sqlite3_step(stmt)==SQLITE_ROW) {
        double ing = sqlite3_column_double(stmt,2);
        total += ing;
        printf("  %-10s | %8d | %10.2f | %.2f EUR\n",
               (const char*)sqlite3_column_text(stmt,0),
               sqlite3_column_int(stmt,1),
			   ing,sqlite3_column_double(stmt,3));
    }
    printf("  -----------+----------+------------+--------\n");
    printf("  TOTAL INGRESOS: %.2f EUR\n", total);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void informe_incidencias_periodo(const char *fecha_ini, const char *fecha_fin) {
    sqlite3 *db = abrir_bd();
    if (!db) {
    	return;
    }
    sqlite3_stmt *stmt;
    printf("\n  ========================================\n");
    printf("  INFORME INCIDENCIAS  %s - %s\n", fecha_ini, fecha_fin);
    printf("  ========================================\n");
    sqlite3_prepare_v2(db,
        "SELECT tipo, prioridad, COUNT(*) as total,"
        " SUM(CASE WHEN estado='RESUELTA' THEN 1 ELSE 0 END) as resueltas"
        " FROM INCIDENCIAS WHERE fecha_reporte BETWEEN ? AND ?"
        " GROUP BY tipo, prioridad ORDER BY tipo, prioridad;",
        -1,&stmt,NULL);
    sqlite3_bind_text(stmt,1,fecha_ini,-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,2,fecha_fin,-1,SQLITE_TRANSIENT);
    printf("  %-12s | %-8s | %5s | %s\n","TIPO","PRIORIDAD","TOTAL","RESUELTAS");
    printf("  -------------+----------+-------+---------\n");
    while (sqlite3_step(stmt)==SQLITE_ROW){
        printf("  %-12s | %-8s | %5d | %d\n",
               (const char*)sqlite3_column_text(stmt,0),
               (const char*)sqlite3_column_text(stmt,1),
               sqlite3_column_int(stmt,2),sqlite3_column_int(stmt,3));
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void informe_empleados_activos(void) {
    sqlite3 *db = abrir_bd();
    if (!db){
    	return;
    }
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT u.nombre, u.apellido, u.email,"
        " COUNT(ap.id_asig) as servicios_asignados"
        " FROM USUARIOS u"
        " LEFT JOIN ASIGNACION_PERSONAL ap ON u.id_u=ap.id_u"
        " WHERE u.rol='MAQUINISTA' AND u.activo=1"
        " GROUP BY u.id_u ORDER BY servicios_asignados DESC;",
        -1,&stmt,NULL);
    printf("\n  %-15s | %-15s | %-25s | %s\n","NOMBRE","APELLIDO","EMAIL","SERVICIOS");
    printf("  ----------------+-----------------+---------------------------+---------\n");
    while (sqlite3_step(stmt)==SQLITE_ROW){
        printf("  %-15s | %-15s | %-25s | %d\n",
               (const char*)sqlite3_column_text(stmt,0),
               (const char*)sqlite3_column_text(stmt,1),
               (const char*)sqlite3_column_text(stmt,2),
               sqlite3_column_int(stmt,3));
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}



//LOGS
void consultar_logs_db(const char *filtro_fecha, const char *filtro_usuario, const char *filtro_nivel) {
    FILE *f = fopen(cfg.log_path, "r");
    if (!f) {
    	printf("  [No se pudo abrir el fichero de logs]\n");
    	return;
    }
    char linea[512];
    int n=0;
    printf("\n  LOGS DEL SISTEMA (%s)\n", cfg.log_path);
    printf("  %-20s | %-20s | %-15s | %s\n","TIMESTAMP","USUARIO","TIPO","DESCRIPCION");
    printf("  ---------------------+----------------------+-----------------+-------------------------\n");
    while (fgets(linea,sizeof(linea),f)) {
        /* Filtra si se pide */
        if (filtro_fecha && strlen(filtro_fecha)>0 && !strstr(linea,filtro_fecha)) continue;
        if (filtro_usuario && strlen(filtro_usuario)>0 && !strstr(linea,filtro_usuario)) continue;
        if (filtro_nivel && strlen(filtro_nivel)>0 && !strstr(linea,filtro_nivel)) continue;
        linea[strcspn(linea,"\r\n")]=0;
        printf("  %s\n",linea);
        n++;
        if (n>=100) { printf("  [...truncado en 100 lineas...]\n"); break; }
    }
    if (n==0) printf("  [Sin resultados con los filtros indicados]\n");
    fclose(f);
}


//IMPLEMENTADO CON IA DEBIDO A LA COMPLEJIDAD
/* ============================================================
 *  IMPORTACION GTFS  (formato RENFE: stops.txt, routes.txt)
 * ============================================================ */


/* ============================================================
 *  IMPORTACION GTFS  –  formato propio del proyecto
 *
 *  Ficheros que lee (todos en ruta_directorio/):
 *    trenes.txt    → TRENES
 *    stops.txt     → ESTACIONES
 *    trayectos.txt → TRAYECTOS  (resuelve tipo_tren y stop_ids)
 *    paradas.txt   → PARADAS_INTERMEDIAS (usa mapa en memoria)
 *
 *  NO modifica la estructura de la BD en ningun momento.
 * ============================================================ */

/* Mapa en memoria trip_id → id_tr para vincular paradas sin
 * necesitar columnas extra en la BD                          */
typedef struct { char trip_id[64]; int id_tr; } TripEntry;

/* Normaliza un stop_id eliminando ceros a la izquierda para
 * manejar el mismatch entre stops.txt (04040) y
 * trayectos.txt (4040). Escribe en buf y devuelve buf.      */
static const char* norm_stop(const char *s, char *buf, int buflen) {
    int n = atoi(s);
    snprintf(buf, buflen, "%d", n);
    return buf;
}

/* Busca id_est en ESTACIONES por codigo_gtfs probando el valor
 * original Y el valor normalizado sin ceros iniciales.
 * Devuelve -1 si no encuentra.                              */
static int buscar_id_est(sqlite3_stmt *s_est, const char *stop_raw) {
    char nbuf[16];
    const char *stop_norm = norm_stop(stop_raw, nbuf, sizeof(nbuf));

    /* Intento 1: valor tal cual */
    sqlite3_reset(s_est);
    sqlite3_bind_text(s_est, 1, stop_raw,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s_est, 2, stop_raw,  -1, SQLITE_TRANSIENT);
    if (sqlite3_step(s_est) == SQLITE_ROW)
        return sqlite3_column_int(s_est, 0);

    /* Intento 2: sin ceros iniciales */
    sqlite3_reset(s_est);
    sqlite3_bind_text(s_est, 1, stop_norm, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(s_est, 2, stop_norm, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(s_est) == SQLITE_ROW)
        return sqlite3_column_int(s_est, 0);

    return -1;
}

static int csv_split(char *linea, char **cols, int max) {
    int n = 0;
    cols[n++] = linea;
    for (char *p = linea; *p && n < max; p++) {
        if (*p == ',') { *p = '\0'; cols[n++] = p + 1; }
    }
    return n;
}

static int csv_col(char **cab, int n_cab, const char *nombre) {
    for (int i = 0; i < n_cab; i++) {
        char *c = cab[i];
        if ((unsigned char)c[0]==0xEF &&
            (unsigned char)c[1]==0xBB &&
            (unsigned char)c[2]==0xBF) c += 3;
        while (*c == ' ') c++;
        if (strcmp(c, nombre) == 0) return i;
    }
    return -1;
}

int importar_gtfs(const char *ruta_directorio) {
    sqlite3 *db = abrir_bd();
    if (!db) return 1;

    char ruta[512];
    char raw[1024];
    char linea[1024];

    /* ══ FASE 0 – trenes.txt → TRENES ══ */
    snprintf(ruta, sizeof(ruta), "%s/trenes.txt", ruta_directorio);
    FILE *ftr = fopen(ruta, "r");
    if (!ftr) {
        printf("[GTFS] AVISO: no se encontro trenes.txt, se usara id_t=1.\n");
    } else {
        if (fgets(raw, sizeof(raw), ftr)) {
            raw[strcspn(raw, "\r\n")] = '\0';
            char *cab0[32]; int n0 = csv_split(raw, cab0, 32);
            int c_modelo = csv_col(cab0, n0, "nombre_modelo");
            int c_serie  = csv_col(cab0, n0, "num_serie");
            int c_anio   = csv_col(cab0, n0, "anio_fab");
            int c_estado = csv_col(cab0, n0, "estado_mant");
            int c_rev    = csv_col(cab0, n0, "fecha_ultima_revision");

            if (c_modelo >= 0 && c_serie >= 0) {
                sqlite3_stmt *s_tren;
                sqlite3_prepare_v2(db,
                    "INSERT OR IGNORE INTO TRENES "
                    "(nombre_modelo,num_serie,anio_fab,estado_mant,fecha_ultima_revision)"
                    " VALUES (?,?,?,?,?);", -1, &s_tren, NULL);
                int tok = 0;
                sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
                while (fgets(linea, sizeof(linea), ftr)) {
                    linea[strcspn(linea, "\r\n")] = '\0';
                    if (!linea[0]) continue;
                    char *c[32]; int nc = csv_split(linea, c, 32);
                    if (nc < 2) continue;
                    int anio = (c_anio>=0&&c_anio<nc) ? atoi(c[c_anio]) : 2000;
                    const char *est = (c_estado>=0&&c_estado<nc) ? c[c_estado] : "OPERATIVO";
                    const char *rev = (c_rev>=0&&c_rev<nc) ? c[c_rev] : NULL;
                    sqlite3_reset(s_tren);
                    sqlite3_bind_text(s_tren,1,c[c_modelo],-1,SQLITE_TRANSIENT);
                    sqlite3_bind_text(s_tren,2,c[c_serie], -1,SQLITE_TRANSIENT);
                    sqlite3_bind_int (s_tren,3,anio);
                    sqlite3_bind_text(s_tren,4,est,        -1,SQLITE_TRANSIENT);
                    if (rev) sqlite3_bind_text(s_tren,5,rev,-1,SQLITE_TRANSIENT);
                    else     sqlite3_bind_null(s_tren,5);
                    if (sqlite3_step(s_tren) == SQLITE_DONE) tok++;
                }
                sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
                sqlite3_finalize(s_tren);
                printf("[GTFS] Trenes insertados: %d\n", tok);
            }
        }
        fclose(ftr);
    }

    /* ══ FASE 1 – stops.txt → ESTACIONES ══ */
    snprintf(ruta, sizeof(ruta), "%s/stops.txt", ruta_directorio);
    FILE *f = fopen(ruta, "r");
    if (!f) {
        printf("[GTFS] ERROR: no se encontro stops.txt en '%s'\n", ruta_directorio);
        sqlite3_close(db); return 1;
    }
    if (!fgets(raw, sizeof(raw), f)) {
        printf("[GTFS] ERROR: stops.txt vacio.\n");
        fclose(f); sqlite3_close(db); return 1;
    }
    raw[strcspn(raw, "\r\n")] = '\0';
    char *cab[32]; int n_cab = csv_split(raw, cab, 32);

    int c_id     = csv_col(cab, n_cab, "stop_id");
    int c_nombre = csv_col(cab, n_cab, "nombre");
    int c_ciudad = csv_col(cab, n_cab, "ciudad");
    int c_prov   = csv_col(cab, n_cab, "provincia");
    int c_lat    = csv_col(cab, n_cab, "latitud");
    int c_lon    = csv_col(cab, n_cab, "longitud");
    int c_and    = csv_col(cab, n_cab, "num_andenes");
    int c_club   = csv_col(cab, n_cab, "tiene_sala_club");

    if (c_id<0||c_nombre<0||c_ciudad<0||c_prov<0||c_lat<0||c_lon<0) {
        printf("[GTFS] ERROR: faltan columnas obligatorias en stops.txt\n");
        fclose(f); sqlite3_close(db); return 1;
    }

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO ESTACIONES "
        "(nombre,codigo_gtfs,ciudad,provincia,latitud,longitud,num_andenes,tiene_sala_club)"
        " VALUES (?,?,?,?,?,?,?,?);", -1, &stmt, NULL);

    int ok=0, ya=0, err=0;
    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while (fgets(linea, sizeof(linea), f)) {
        linea[strcspn(linea, "\r\n")] = '\0';
        if (!linea[0]) continue;
        char *cols[32]; int n = csv_split(linea, cols, 32);
        int mx = c_id;
        if (c_nombre>mx) mx=c_nombre;
        if (c_ciudad>mx) mx=c_ciudad;
        if (c_prov>mx)   mx=c_prov;
        if (c_lat>mx)    mx=c_lat;
        if (c_lon>mx)    mx=c_lon;
        if (n <= mx) { err++; continue; }
        if (!cols[c_nombre][0]) { err++; continue; }
        int andenes = (c_and>=0&&c_and<n) ? atoi(cols[c_and])  : 1;
        int club    = (c_club>=0&&c_club<n) ? atoi(cols[c_club]) : 0;
        sqlite3_reset(stmt);
        sqlite3_bind_text  (stmt,1,cols[c_nombre],-1,SQLITE_TRANSIENT);
        sqlite3_bind_text  (stmt,2,cols[c_id],    -1,SQLITE_TRANSIENT);
        sqlite3_bind_text  (stmt,3,cols[c_ciudad],-1,SQLITE_TRANSIENT);
        sqlite3_bind_text  (stmt,4,cols[c_prov],  -1,SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt,5,atof(cols[c_lat]));
        sqlite3_bind_double(stmt,6,atof(cols[c_lon]));
        sqlite3_bind_int   (stmt,7,andenes);
        sqlite3_bind_int   (stmt,8,club);
        int rc = sqlite3_step(stmt);
        if      (rc==SQLITE_DONE)       ok++;
        else if (rc==SQLITE_CONSTRAINT) ya++;
        else                            err++;
    }
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(stmt);
    fclose(f);
    printf("[GTFS] Estaciones insertadas: %d  |  ya existian: %d  |  errores: %d\n",
           ok, ya, err);

    /* ══ FASE 2 – trayectos.txt → TRAYECTOS ══ */
    snprintf(ruta, sizeof(ruta), "%s/trayectos.txt", ruta_directorio);
    FILE *ft = fopen(ruta, "r");
    if (!ft) {
        printf("[GTFS] AVISO: no se encontro trayectos.txt.\n");
        sqlite3_close(db); return 0;
    }
    if (!fgets(raw, sizeof(raw), ft)) { fclose(ft); sqlite3_close(db); return 0; }
    raw[strcspn(raw, "\r\n")] = '\0';
    char *cab2[32]; int n_cab2 = csv_split(raw, cab2, 32);

    int c_trip  = csv_col(cab2, n_cab2, "trip_id");
    int c_tren  = csv_col(cab2, n_cab2, "tipo_tren");
    int c_orig  = csv_col(cab2, n_cab2, "origen_gtfs");
    int c_hsal  = csv_col(cab2, n_cab2, "hora_salida");
    int c_dest  = csv_col(cab2, n_cab2, "destino_gtfs");
    int c_hlle  = csv_col(cab2, n_cab2, "hora_llegada");
    int c_dur   = csv_col(cab2, n_cab2, "duracion_min");
    int c_prec  = csv_col(cab2, n_cab2, "precio_base");

    if (c_orig<0||c_hsal<0||c_dest<0||c_hlle<0) {
        printf("[GTFS] ERROR: faltan columnas en trayectos.txt\n");
        fclose(ft); sqlite3_close(db); return 1;
    }

    /* Buscar id_est probando codigo_gtfs con y sin ceros iniciales */
    sqlite3_stmt *s_est;
    sqlite3_prepare_v2(db,
        "SELECT id_est FROM ESTACIONES "
        "WHERE codigo_gtfs=? OR CAST(CAST(codigo_gtfs AS INTEGER) AS TEXT)=? LIMIT 1;",
        -1, &s_est, NULL);

    /* Buscar id_t por nombre_modelo */
    sqlite3_stmt *s_tren_id;
    sqlite3_prepare_v2(db,
        "SELECT id_t FROM TRENES WHERE nombre_modelo=? LIMIT 1;",
        -1, &s_tren_id, NULL);

    sqlite3_stmt *s_tray;
    sqlite3_prepare_v2(db,
        "INSERT INTO TRAYECTOS "
        "(id_t,id_est_origen,id_est_destino,hora_salida,hora_llegada,"
        " duracion_min,precio_base,dias_operacion,estado)"
        " VALUES (?,?,?,?,?,?,?,'LMXJVSD','ACTIVO');",
        -1, &s_tray, NULL);

    /* Mapa en memoria trip_id → id_tr para paradas */
    typedef struct { char trip_id[64]; int id_tr; } TripEntry;
    int map_cap=8192, map_n=0;
    TripEntry *trip_map = (TripEntry*)malloc(map_cap * sizeof(TripEntry));
    if (!trip_map) {
        printf("[GTFS] ERROR: sin memoria.\n");
        fclose(ft); sqlite3_close(db); return 1;
    }

    int tok=0, terr=0, tign=0;
    char linea2[1024];

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while (fgets(linea2, sizeof(linea2), ft)) {
        linea2[strcspn(linea2, "\r\n")] = '\0';
        if (!linea2[0]) continue;
        char *cols[32]; int nc = csv_split(linea2, cols, 32);
        if (nc < 4) { terr++; continue; }

        /* Resolver id_t */
        int id_t = 1;
        if (c_tren>=0&&c_tren<nc) {
            sqlite3_reset(s_tren_id);
            sqlite3_bind_text(s_tren_id,1,cols[c_tren],-1,SQLITE_TRANSIENT);
            if (sqlite3_step(s_tren_id)==SQLITE_ROW)
                id_t = sqlite3_column_int(s_tren_id,0);
        }

        /* Resolver origen */
        char nbuf[16];
        snprintf(nbuf,sizeof(nbuf),"%d",atoi(cols[c_orig]));
        sqlite3_reset(s_est);
        sqlite3_bind_text(s_est,1,cols[c_orig],-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(s_est,2,nbuf,-1,SQLITE_TRANSIENT);
        if (sqlite3_step(s_est)!=SQLITE_ROW) { tign++; continue; }
        int id_orig = sqlite3_column_int(s_est,0);

        /* Resolver destino */
        snprintf(nbuf,sizeof(nbuf),"%d",atoi(cols[c_dest]));
        sqlite3_reset(s_est);
        sqlite3_bind_text(s_est,1,cols[c_dest],-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(s_est,2,nbuf,-1,SQLITE_TRANSIENT);
        if (sqlite3_step(s_est)!=SQLITE_ROW) { tign++; continue; }
        int id_dest = sqlite3_column_int(s_est,0);

        double precio = (c_prec>=0&&c_prec<nc) ? atof(cols[c_prec]) : 0.0;
        int    dur    = (c_dur>=0&&c_dur<nc)   ? atoi(cols[c_dur])  : 0;

        sqlite3_reset(s_tray);
        sqlite3_bind_int   (s_tray,1,id_t);
        sqlite3_bind_int   (s_tray,2,id_orig);
        sqlite3_bind_int   (s_tray,3,id_dest);
        sqlite3_bind_text  (s_tray,4,cols[c_hsal],-1,SQLITE_TRANSIENT);
        sqlite3_bind_text  (s_tray,5,cols[c_hlle],-1,SQLITE_TRANSIENT);
        sqlite3_bind_int   (s_tray,6,dur);
        sqlite3_bind_double(s_tray,7,precio);

        int rc = sqlite3_step(s_tray);
        if (rc==SQLITE_DONE) {
            int id_tr = (int)sqlite3_last_insert_rowid(db);
            if (c_trip>=0&&c_trip<nc&&map_n<map_cap) {
                strncpy(trip_map[map_n].trip_id,cols[c_trip],63);
                trip_map[map_n].id_tr = id_tr;
                map_n++;
            }
            tok++;
        } else terr++;
    }
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(s_tray);
    sqlite3_finalize(s_tren_id);
    fclose(ft);
    printf("[GTFS] Trayectos insertados: %d  |  no hallados: %d  |  errores: %d\n",
           tok, tign, terr);

    /* ══ FASE 3 – paradas.txt → PARADAS_INTERMEDIAS ══ */
    snprintf(ruta, sizeof(ruta), "%s/paradas.txt", ruta_directorio);
    FILE *fp = fopen(ruta, "r");
    if (!fp) {
        printf("[GTFS] AVISO: no se encontro paradas.txt.\n");
        free(trip_map); sqlite3_finalize(s_est);
        sqlite3_close(db); return 0;
    }
    if (!fgets(raw, sizeof(raw), fp)) {
        fclose(fp); free(trip_map);
        sqlite3_finalize(s_est); sqlite3_close(db); return 0;
    }
    raw[strcspn(raw, "\r\n")] = '\0';
    char *cab3[32]; int n_cab3 = csv_split(raw, cab3, 32);

    int cp_trip  = csv_col(cab3, n_cab3, "trip_id");
    int cp_orden = csv_col(cab3, n_cab3, "orden");
    int cp_stop  = csv_col(cab3, n_cab3, "stop_id");
    int cp_hlle  = csv_col(cab3, n_cab3, "hora_llegada");
    int cp_hsal  = csv_col(cab3, n_cab3, "hora_salida");

    if (cp_trip<0||cp_orden<0||cp_stop<0) {
        printf("[GTFS] ERROR: faltan columnas en paradas.txt\n");
        fclose(fp); free(trip_map);
        sqlite3_finalize(s_est); sqlite3_close(db); return 1;
    }

    sqlite3_stmt *s_par;
    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO PARADAS_INTERMEDIAS "
        "(id_tr,id_est,hora_llegada,hora_salida,orden)"
        " VALUES (?,?,?,?,?);", -1, &s_par, NULL);

    int pok=0, perr=0, pign=0;
    char linea3[1024];

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    while (fgets(linea3, sizeof(linea3), fp)) {
        linea3[strcspn(linea3, "\r\n")] = '\0';
        if (!linea3[0]) continue;
        char *cols[32]; int nc = csv_split(linea3, cols, 32);
        if (nc < 3) { perr++; continue; }

        /* Buscar id_tr en mapa */
        int id_tray = -1;
        for (int i = 0; i < map_n; i++) {
            if (strcmp(trip_map[i].trip_id, cols[cp_trip])==0) {
                id_tray = trip_map[i].id_tr; break;
            }
        }
        if (id_tray<0) { pign++; continue; }

        /* Resolver stop → id_est */
        char nbuf2[16];
        snprintf(nbuf2,sizeof(nbuf2),"%d",atoi(cols[cp_stop]));
        sqlite3_reset(s_est);
        sqlite3_bind_text(s_est,1,cols[cp_stop],-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(s_est,2,nbuf2,-1,SQLITE_TRANSIENT);
        if (sqlite3_step(s_est)!=SQLITE_ROW) { pign++; continue; }
        int id_est_p = sqlite3_column_int(s_est,0);

        const char *hlle=(cp_hlle>=0&&cp_hlle<nc)?cols[cp_hlle]:"00:00:00";
        const char *hsal=(cp_hsal>=0&&cp_hsal<nc)?cols[cp_hsal]:"00:00:00";

        sqlite3_reset(s_par);
        sqlite3_bind_int (s_par,1,id_tray);
        sqlite3_bind_int (s_par,2,id_est_p);
        sqlite3_bind_text(s_par,3,hlle,-1,SQLITE_TRANSIENT);
        sqlite3_bind_text(s_par,4,hsal,-1,SQLITE_TRANSIENT);
        sqlite3_bind_int (s_par,5,atoi(cols[cp_orden]));

        if (sqlite3_step(s_par)==SQLITE_DONE) pok++;
        else perr++;
    }
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
    sqlite3_finalize(s_par);
    sqlite3_finalize(s_est);
    fclose(fp);
    free(trip_map);
    sqlite3_close(db);

    printf("[GTFS] Paradas intermedias: %d  |  no halladas: %d  |  errores: %d\n",
           pok, pign, perr);
    printf("[GTFS] Importacion completada.\n");
    log_evento(cfg.log_path, "SISTEMA", "GTFS", "Importacion completada");
    return 0;
}

void resumen_ultima_importacion(void) {
    sqlite3 *db = abrir_bd();
    if (!db) return;
    sqlite3_stmt *s;
    printf("\n[GTFS] ── RESUMEN BD ──────────────────\n");
    sqlite3_prepare_v2(db,"SELECT COUNT(*) FROM TRENES;",    -1,&s,NULL);
    if (sqlite3_step(s)==SQLITE_ROW) printf("[GTFS]  Trenes      : %d\n",sqlite3_column_int(s,0));
    sqlite3_finalize(s);
    sqlite3_prepare_v2(db,"SELECT COUNT(*) FROM ESTACIONES;",-1,&s,NULL);
    if (sqlite3_step(s)==SQLITE_ROW) printf("[GTFS]  Estaciones  : %d\n",sqlite3_column_int(s,0));
    sqlite3_finalize(s);
    sqlite3_prepare_v2(db,"SELECT COUNT(*) FROM TRAYECTOS;", -1,&s,NULL);
    if (sqlite3_step(s)==SQLITE_ROW) printf("[GTFS]  Trayectos   : %d\n",sqlite3_column_int(s,0));
    sqlite3_finalize(s);
    sqlite3_prepare_v2(db,"SELECT COUNT(*) FROM PARADAS_INTERMEDIAS;",-1,&s,NULL);
    if (sqlite3_step(s)==SQLITE_ROW) printf("[GTFS]  Paradas int.: %d\n",sqlite3_column_int(s,0));
    sqlite3_finalize(s);
    printf("[GTFS] ───────────────────────────────\n");
    sqlite3_close(db);
}
