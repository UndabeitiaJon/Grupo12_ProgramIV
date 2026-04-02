/*
 * db_manager.c
 *
 *  Created on: 30 mar 2026
 *      Author: ander.lecue
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db_manager.h"
#include "estructuras.h"

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
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, 0);
    return db;
}

/* ============================================================
 *  INIT Y SEED
 * ============================================================ */
int init_database() {
    sqlite3 *db;
    char *err = 0;

    int rc = sqlite3_open(cfg.db_path, &db);
    if (rc != SQLITE_OK) {
        printf("Error al abrir la BD: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS USUARIOS ("
        "id_u INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL, apellido TEXT NOT NULL,"
        "dni TEXT UNIQUE NOT NULL, email TEXT UNIQUE NOT NULL,"
        "telf TEXT, fecha_nac TEXT, pass_hash TEXT,"
        "rol TEXT CHECK(rol IN ('ADMIN','PASAJERO','MAQUINISTA')),"
        "activo INTEGER DEFAULT 1, fecha_registro TEXT);"

    	"CREATE TABLE IF NOT EXISTS DATOS_PASAJERO ("
    	"id_u INTEGER PRIMARY KEY,"
    	"puntos_fidelidad INTEGER DEFAULT 0,"
    	"tipo_descuento TEXT DEFAULT 'NINGUNO',"
    	"num_tarjeta_fidelizacion TEXT,"
    	"necesidad_asistencia_pmr INTEGER DEFAULT 0,"
    	"FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u));"

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

        "CREATE TABLE IF NOT EXISTS TRENES ("
        "id_t INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre_modelo TEXT NOT NULL, num_serie TEXT UNIQUE,"
        "anio_fab INTEGER, estado_mant TEXT DEFAULT 'OPERATIVO',"
        "fecha_ultima_revision TEXT);"

        "CREATE TABLE IF NOT EXISTS ESTACIONES ("
        "id_est INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nombre TEXT NOT NULL, codigo_gtfs TEXT,"
        "ciudad TEXT NOT NULL, provincia TEXT,"
        "latitud REAL, longitud REAL,"
        "num_andenes INTEGER DEFAULT 1,"
        "tiene_sala_club INTEGER DEFAULT 0);"

        "CREATE TABLE IF NOT EXISTS TRAYECTOS ("
        "id_tr INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_t INTEGER, id_est_origen INTEGER, id_est_destino INTEGER,"
        "hora_salida TEXT, hora_llegada TEXT,"
        "duracion_min INTEGER, precio_base REAL,"
        "dias_operacion TEXT DEFAULT 'LMXJVSD',"
        "estado TEXT DEFAULT 'ACTIVO',"
        "FOREIGN KEY(id_t) REFERENCES TRENES(id_t),"
        "FOREIGN KEY(id_est_origen) REFERENCES ESTACIONES(id_est),"
        "FOREIGN KEY(id_est_destino) REFERENCES ESTACIONES(id_est));"

        "CREATE TABLE IF NOT EXISTS RESERVAS ("
        "id_res INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_u INTEGER, id_tr INTEGER, fecha_viaje TEXT,"
        "clase TEXT, num_vagon INTEGER, num_asiento INTEGER,"
        "precio_base REAL, descuento_pct REAL DEFAULT 0,"
        "precio_final REAL, estado TEXT DEFAULT 'CONFIRMADA',"
        "codigo_validacion TEXT, fecha_reserva TEXT,"
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u),"
        "FOREIGN KEY(id_tr) REFERENCES TRAYECTOS(id_tr));"

        "CREATE TABLE IF NOT EXISTS EQUIPAJE ("
        "id_eq INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_res INTEGER, tipo TEXT, peso_kg REAL,"
        "exceso_kg REAL DEFAULT 0, suplemento_pago REAL DEFAULT 0,"
        "FOREIGN KEY(id_res) REFERENCES RESERVAS(id_res));"

        "CREATE TABLE IF NOT EXISTS ASIGNACION_PERSONAL ("
        "id_asig INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_u INTEGER, id_t INTEGER, id_tr INTEGER, fecha TEXT,"
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u),"
        "FOREIGN KEY(id_t) REFERENCES TRENES(id_t),"
        "FOREIGN KEY(id_tr) REFERENCES TRAYECTOS(id_tr));"

        "CREATE TABLE IF NOT EXISTS INCIDENCIAS ("
        "id_inc INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_u_reporta INTEGER, id_tr INTEGER, fecha TEXT,"
        "tipo TEXT, descripcion TEXT, prioridad TEXT DEFAULT 'MEDIA',"
        "estado TEXT DEFAULT 'ABIERTA',"
        "FOREIGN KEY(id_u_reporta) REFERENCES USUARIOS(id_u));"

        "CREATE TABLE IF NOT EXISTS LOGS ("
        "id_log INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fecha TEXT, id_u INTEGER, tipo_evento TEXT, descripcion TEXT);"

        "CREATE TABLE IF NOT EXISTS TARIFAS ("
        "id_tarifa INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_tr INTEGER UNIQUE, precio_base REAL,"
        "coef_turista REAL DEFAULT 1.0,"
        "coef_business REAL DEFAULT 1.8,"
        "suplemento_bici REAL DEFAULT 30.0,"
        "exceso_kg_precio REAL DEFAULT 12.0,"
        "FOREIGN KEY(id_tr) REFERENCES TRAYECTOS(id_tr));"

        "CREATE TABLE IF NOT EXISTS PUNTOS_FIDELIDAD ("
        "id_u INTEGER PRIMARY KEY,"
        "saldo_actual INTEGER DEFAULT 0,"
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u));"

        "CREATE TABLE IF NOT EXISTS VAGONES ("
        "id_vagon INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_tren INTEGER, numero_vagon INTEGER,"
        "clase TEXT, capacidad_total INTEGER,"
        "vagon_PMR INTEGER DEFAULT 0,"
        "FOREIGN KEY(id_tren) REFERENCES TRENES(id_t));"

        "CREATE TABLE IF NOT EXISTS PARADAS_INTERMEDIAS ("
        "id_parada INTEGER PRIMARY KEY AUTOINCREMENT,"
        "id_tr INTEGER, id_est INTEGER, orden INTEGER,"
        "hora_llegada TEXT, hora_salida TEXT,"
        "FOREIGN KEY(id_tr) REFERENCES TRAYECTOS(id_tr),"
        "FOREIGN KEY(id_est) REFERENCES ESTACIONES(id_est));"

    	"CREATE TABLE IF NOT EXISTS SERVICIOS_OPERATIVOS ("
    	"id_serv INTEGER PRIMARY KEY AUTOINCREMENT,"
    	"id_tr INTEGER, fecha TEXT,"
    	"estado_serv TEXT DEFAULT 'PROGRAMADO',"
    	"hora_inicio_real TEXT, hora_fin_real TEXT,"
    	"minutos_retraso INTEGER DEFAULT 0,"
    	"causa_retraso TEXT,"
    	"FOREIGN KEY(id_tr) REFERENCES TRAYECTOS(id_tr));"

        "CREATE TABLE IF NOT EXISTS DESCUENTOS ("
        "tipo_descuento TEXT PRIMARY KEY,"
        "porcentaje REAL);";

    rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Error creando tablas: %s\n", err);
        sqlite3_free(err);
        sqlite3_close(db);
        return 1;
    }

    printf("[OK] Base de datos conectada y tablas listas.\n");
    sqlite3_close(db);
    return 0;
}

void seed_database() {
    sqlite3 *db = abrir_bd();
    if (!db) return;

    printf("[BD] Usando base de datos: %s\n", cfg.db_path);

    /* INSERT OR IGNORE: si ya existe (por DNI/email UNIQUE) no falla */
    char *err = 0;
    const char *sql =
        "INSERT OR IGNORE INTO USUARIOS "
        "(nombre,apellido,dni,email,telf,fecha_nac,pass_hash,rol,activo,fecha_registro)"
        " VALUES ('Admin','Trenfe','00000000A','admin@trenfe.com',"
        "'600000000','1980-01-01','admin123','ADMIN',1,date('now'));"

        "INSERT OR IGNORE INTO USUARIOS "
        "(nombre,apellido,dni,email,telf,fecha_nac,pass_hash,rol,activo,fecha_registro)"
        " VALUES ('Juan','Garcia','12345678B','juan@trenfe.com',"
        "'611222333','1995-06-15','pass123','PASAJERO',1,date('now'));"

        "INSERT OR IGNORE INTO USUARIOS "
        "(nombre,apellido,dni,email,telf,fecha_nac,pass_hash,rol,activo,fecha_registro)"
        " VALUES ('Pedro','Lopez','87654321C','pedro@trenfe.com',"
        "'622333444','1985-03-20','maq123','MAQUINISTA',1,date('now'));"

    	"INSERT OR IGNORE INTO USUARIOS "
    	"(nombre,apellido,dni,email,telf,fecha_nac,pass_hash,rol,activo,fecha_registro)"
    	" VALUES ('Maria','Fernandez','11223344D','maria@trenfe.com',"
    	"'633444555','2000-09-10','pass456','PASAJERO',1,date('now'));"

    	"INSERT OR IGNORE INTO DATOS_PASAJERO "
    	"(id_u,puntos_fidelidad,tipo_descuento) "
    	"VALUES (2,150,'JOVEN');"

    	"INSERT OR IGNORE INTO DATOS_PASAJERO (id_u,puntos_fidelidad,tipo_descuento)"
    	" VALUES (4,0,'NINGUNO');"

    	"INSERT OR IGNORE INTO DATOS_EMPLEADO "
    	"(id_u,num_empleado,fecha_ingreso,rol_empleado,anios_exp,estado)"
    	" VALUES (3,'EMP-001','2015-01-01','MAQUINISTA',10,'ACTIVO');"

        "INSERT OR IGNORE INTO TRENES "
        "(nombre_modelo,num_serie,anio_fab,estado_mant,fecha_ultima_revision)"
        " VALUES ('AVE Serie 103','AVE-103-001',2006,'OPERATIVO','2025-01-15');"

        "INSERT OR IGNORE INTO TRENES "
        "(nombre_modelo,num_serie,anio_fab,estado_mant,fecha_ultima_revision)"
        " VALUES ('Alvia Serie 130','ALV-130-005',2010,'OPERATIVO','2025-03-10');"

        "INSERT OR IGNORE INTO ESTACIONES "
        "(nombre,codigo_gtfs,ciudad,provincia,latitud,longitud,num_andenes,tiene_sala_club)"
        " VALUES ('Bilbao Abando','BILBAO','Bilbao','Vizcaya',43.2630,-2.9350,8,1);"

        "INSERT OR IGNORE INTO ESTACIONES "
        "(nombre,codigo_gtfs,ciudad,provincia,latitud,longitud,num_andenes,tiene_sala_club)"
        " VALUES ('Madrid Atocha','MADRID','Madrid','Madrid',40.4063,-3.6899,20,1);"

        "INSERT OR IGNORE INTO ESTACIONES "
        "(nombre,codigo_gtfs,ciudad,provincia,latitud,longitud,num_andenes,tiene_sala_club)"
        " VALUES ('Barcelona Sants','BARCA','Barcelona','Barcelona',41.3794,2.1403,16,1);"

        "INSERT OR IGNORE INTO TRAYECTOS "
        "(id_t,id_est_origen,id_est_destino,hora_salida,hora_llegada,"
        "duracion_min,precio_base,dias_operacion,estado)"
        " VALUES (1,1,2,'08:00','12:30',270,45.50,'LMXJVSD','ACTIVO');"

        "INSERT OR IGNORE INTO TRAYECTOS "
        "(id_t,id_est_origen,id_est_destino,hora_salida,hora_llegada,"
        "duracion_min,precio_base,dias_operacion,estado)"
        " VALUES (2,2,3,'14:00','16:30',150,32.00,'LMXJVSD','ACTIVO');"

    	"INSERT OR IGNORE INTO TARIFAS (id_tr,precio_base,coef_turista,coef_business,suplemento_bici,exceso_kg_precio)"
    	" VALUES (1,45.50,1.0,1.8,30.0,12.0);"
    	"INSERT OR IGNORE INTO TARIFAS (id_tr,precio_base,coef_turista,coef_business,suplemento_bici,exceso_kg_precio)"
    	" VALUES (2,32.00,1.0,1.8,30.0,12.0);"

        "INSERT OR IGNORE INTO DESCUENTOS VALUES ('JOVEN',20.0);"
        "INSERT OR IGNORE INTO DESCUENTOS VALUES ('DORADA',40.0);"
        "INSERT OR IGNORE INTO DESCUENTOS VALUES ('NUMEROSA',20.0);"
        "INSERT OR IGNORE INTO DESCUENTOS VALUES ('ABONO',50.0);"

    	"INSERT OR IGNORE INTO ASIGNACIONES_PERSONAL (id_serv,id_u,id_t,rol_servicio)"
    	" VALUES (1,3,1,'CONDUCTOR');"

    	"INSERT OR IGNORE INTO SERVICIOS_OPERATIVOS (id_tr,fecha,estado_serv)"
        " VALUES (1,date('now'),'PROGRAMADO');";

    if (sqlite3_exec(db, sql, 0, 0, &err) != SQLITE_OK) {
        fprintf(stderr, "[SEED] Error insertando datos: %s\n", err);
        sqlite3_free(err);
    } else {
        printf("[SEED] Credenciales de acceso:\n");
        printf("       Admin:      admin@trenfe.com  / admin123\n");
        printf("       Pasajero:   juan@trenfe.com   / pass123\n");
        printf("       Maquinista: pedro@trenfe.com  / maq123\n");
        printf("       Pasajero:   maria@trenfe.com  / pass456\n");
    }

    sqlite3_close(db);
}

/* ============================================================
 *  USUARIOS
 * ============================================================ */
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
			strncpy(u.nombre,           (const char *)sqlite3_column_text(stmt, 1), sizeof(u.nombre));
			strncpy(u.apellido,         (const char *)sqlite3_column_text(stmt, 2), sizeof(u.apellido));
			strncpy(u.dni,              (const char *)sqlite3_column_text(stmt, 3), sizeof(u.dni));
			strncpy(u.email,            (const char *)sqlite3_column_text(stmt, 4), sizeof(u.email));
			strncpy(u.telf,             (const char *)sqlite3_column_text(stmt, 5), sizeof(u.telf));
			strncpy(u.fecha_nac,        (const char *)sqlite3_column_text(stmt, 6), sizeof(u.fecha_nac));
			strncpy(u.pass_hash,        (const char *)sqlite3_column_text(stmt, 7), sizeof(u.pass_hash));
		    u.rol    = (RolUsuario)      sqlite3_column_int(stmt, 8);
		    u.activo =                   sqlite3_column_int(stmt, 9);
		    strncpy(u.fecha_registro,   (const char *)sqlite3_column_text(stmt, 10), sizeof(u.fecha_registro));
		}
		sqlite3_finalize(stmt);
	}
	sqlite3_close(db);
	return u;
}


int insertar_usuario_db(Usuario u) {
    sqlite3 *db = abrir_bd();
    if (!db) return 1;

    sqlite3_stmt *stmt;
    const char *sql =
        "INSERT INTO USUARIOS (nombre,apellido,dni,email,telf,fecha_nac,"
        "pass_hash,rol,activo,fecha_registro)"
        " VALUES (?,?,?,?,?,?,?,?,?,date('now'));";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) { sqlite3_close(db); return 1; }

    sqlite3_bind_text(stmt, 1, u.nombre,           -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, u.apellido,          -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, u.dni,               -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, u.email,             -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, u.telf,              -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, u.fecha_nac,         -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, u.pass_hash,         -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, rol_a_texto(u.rol),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, 9, u.activo);

    int rc2 = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if(rc2 == SQLITE_DONE && u.rol == ROL_PASAJERO ){
    	int nuevo_id = (int) sqlite3_last_insert_rowid(db);
    	char sql2[256];
    	snprintf(sql2, sizeof(sql2),
    	"INSERT OR IGNORE INTO DATOS_PASAJERO (id_u,puntos_fidelidad,tipo_descuento)"
    	" VALUES (%d,0,'NINGUNO');", nuevo_id); // snprintf En lugar de imprimir el texto en la pantalla, lo guarda dentro de tu variable sql2
    	 sqlite3_exec(db, sql2, 0, 0, NULL);
    }


    sqlite3_close(db);

    if (rc != SQLITE_DONE) {
        printf("Error al insertar usuario: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    printf("Usuario %s %s guardado correctamente.\n", u.nombre, u.apellido);
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

bool comprobar_usuario_registrado(char *email) {
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

bool comprobar_contrasenia(char *email, char *contrasenia) {
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

/* ============================================================
 *  TRENES
 * ============================================================ */
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
