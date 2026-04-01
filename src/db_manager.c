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

int init_database() {
	sqlite3 *db;
	char *error_mesg = 0;

	int rc = sqlite3_open(DB_PATH, &db);
	if(rc != SQLITE_OK){
		printf("Error al abrir la BD: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}

    const char *sql =
        //USUARIOS
        "CREATE TABLE IF NOT EXISTS USUARIOS ("
        "id_u INTEGER PRIMARY KEY AUTOINCREMENT, "
        "nombre TEXT NOT NULL, apellido TEXT NOT NULL, dni TEXT UNIQUE NOT NULL, "
        "email TEXT, telf TEXT, fecha_nac TEXT, pass_hash TEXT, "
        "rol TEXT CHECK(rol IN ('ADMIN', 'PASAJERO', 'MAQUINISTA')), "
        "activo INTEGER DEFAULT 1, fecha_registro TEXT);"

        //TRENES
        "CREATE TABLE IF NOT EXISTS TRENES ("
        "id_t INTEGER PRIMARY KEY AUTOINCREMENT, "
        "modelo TEXT NOT NULL, cap_turista INTEGER, cap_business INTEGER, "
        "anio_fab INTEGER, num_serie TEXT UNIQUE, estado_mant TEXT);"

        //ESTACIONES
        "CREATE TABLE IF NOT EXISTS ESTACIONES ("
        "id_est INTEGER PRIMARY KEY AUTOINCREMENT, "
        "nombre TEXT NOT NULL, ciudad TEXT NOT NULL);"

        //TRAYECTOS
        "CREATE TABLE IF NOT EXISTS TRAYECTOS ("
        "id_tray INTEGER PRIMARY KEY AUTOINCREMENT, "
        "origen_id INTEGER, destino_id INTEGER, "
        "hora_salida TEXT, hora_llegada TEXT, dias_operativos TEXT, estado TEXT, "
        "FOREIGN KEY(origen_id) REFERENCES ESTACIONES(id_est), "
        "FOREIGN KEY(destino_id) REFERENCES ESTACIONES(id_est));"

        //PARADAS_INTERMEDIAS
        "CREATE TABLE IF NOT EXISTS PARADAS_INTERMEDIAS ("
        "id_parada INTEGER PRIMARY KEY AUTOINCREMENT, "
        "id_tray INTEGER, id_est INTEGER, orden INTEGER, "
        "hora_llegada TEXT, hora_salida TEXT, "
        "FOREIGN KEY(id_tray) REFERENCES TRAYECTOS(id_tray), "
        "FOREIGN KEY(id_est) REFERENCES ESTACIONES(id_est));"

        //TARIFAS
        "CREATE TABLE IF NOT EXISTS TARIFAS ("
        "id_tarifa INTEGER PRIMARY KEY AUTOINCREMENT, "
        "id_tray INTEGER UNIQUE, precio_base REAL, "
        "coef_turista REAL DEFAULT 1.0, coef_business REAL DEFAULT 1.5, "
        "suplemento_equipaje REAL, "
        "FOREIGN KEY(id_tray) REFERENCES TRAYECTOS(id_tray));"

        //RESERVAS
        "CREATE TABLE IF NOT EXISTS RESERVAS ("
        "id_res INTEGER PRIMARY KEY AUTOINCREMENT, "
        "id_u INTEGER, id_tray INTEGER, fecha_viaje TEXT, "
        "clase TEXT, asiento TEXT, precio_final REAL, estado TEXT, "
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u), "
        "FOREIGN KEY(id_tray) REFERENCES TRAYECTOS(id_tray));"

        // EQUIPAJE
        "CREATE TABLE IF NOT EXISTS EQUIPAJE ("
        "id_eq INTEGER PRIMARY KEY AUTOINCREMENT, "
        "id_res INTEGER, tipo TEXT, peso REAL, coste_extra REAL, "
        "FOREIGN KEY(id_res) REFERENCES RESERVAS(id_res));"

        //ASIGNACION_PERSONAL
        "CREATE TABLE IF NOT EXISTS ASIGNACION_PERSONAL ("
        "id_asig INTEGER PRIMARY KEY AUTOINCREMENT, "
        "id_u_maquinista INTEGER, id_t INTEGER, id_tray INTEGER, fecha TEXT, "
        "FOREIGN KEY(id_u_maquinista) REFERENCES USUARIOS(id_u), "
        "FOREIGN KEY(id_t) REFERENCES TRENES(id_t), "
        "FOREIGN KEY(id_tray) REFERENCES TRAYECTOS(id_tray));"

        //INCIDENCIAS
        "CREATE TABLE IF NOT EXISTS INCIDENCIAS ("
        "id_inc INTEGER PRIMARY KEY AUTOINCREMENT, "
        "id_u_reporta INTEGER, id_tray INTEGER, fecha TEXT, "
        "tipo_estado TEXT, observacion TEXT, "
        "FOREIGN KEY(id_u_reporta) REFERENCES USUARIOS(id_u), "
        "FOREIGN KEY(id_tray) REFERENCES TRAYECTOS(id_tray));"

        //LOGS_OPERATIVOS
        "CREATE TABLE IF NOT EXISTS LOGS ("
        "id_log INTEGER PRIMARY KEY AUTOINCREMENT, "
        "fecha TEXT, id_u INTEGER, tipo_evento TEXT, descripcion TEXT, "
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u));"

        //PUNTOS_FIDELIDAD
        "CREATE TABLE IF NOT EXISTS PUNTOS_FIDELIDAD ("
        "id_pf INTEGER PRIMARY KEY AUTOINCREMENT, "
        "id_u INTEGER UNIQUE, saldo_actual INTEGER DEFAULT 0, "
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u));"

        //HISTORICO_PUNTOS
        "CREATE TABLE IF NOT EXISTS HISTORICO_PUNTOS ("
        "id_hp INTEGER PRIMARY KEY AUTOINCREMENT, "
        "id_u INTEGER, fecha TEXT, puntos_cambio INTEGER, motivo TEXT, "
        "FOREIGN KEY(id_u) REFERENCES USUARIOS(id_u));"

        //DESCUENTOS
        "CREATE TABLE IF NOT EXISTS DESCUENTOS ("
        "id_desc INTEGER PRIMARY KEY AUTOINCREMENT, "
        "tipo_descuento TEXT UNIQUE, porcentaje REAL);";

	rc = sqlite3_exec(db, sql, 0, 0, &error_mesg);
	    if (rc != SQLITE_OK) {
	        fprintf(stderr, "Error de SQL al crear tablas: %s\n", error_mesg);
	        sqlite3_free(error_mesg);
	        sqlite3_close(db);
	        return 1;
	    }

	    printf("[OK] Base de datos conectada y tablas listas.\n");
	    sqlite3_close(db);
	    return 0;
}

void importar_usuarios_csv(const char* ruta_csv){
	sqlite3 *db;
	char *err_msg = 0;
	if (sqlite3_open(DB_PATH, &db) != SQLITE_OK){
		return;
	}

	FILE *archivo = fopen(ruta_csv, "r");
	if (archivo == NULL){
		printf("Error: No se encontro el archivo %s\n", ruta_csv);
		sqlite3_close(db);
		return;
	}

	char linea[512];
	fgets(linea, sizeof(linea), archivo);

	printf("Importando usuarios...\n");
	while(fgets(linea, sizeof(linea), archivo)){
		linea[strcspn(linea, "\n")] = 0;
		linea[strcspn(linea, "\r")] = 0;

		char *nombre = strtok(linea, ";");
		char *apellido = strtok(NULL, ";");
		char *dni = strtok(NULL, ";");
		char *email = strtok(NULL, ";");
		char *rol = strtok(NULL, ";");

		if (nombre && apellido && dni && email && rol) {
			char sql;
		    snprintf(sql, sizeof(sql),
		    "INSERT INTO USUARIOS (nombre, apellido, dni, email, rol) "
		    "VALUES ('%s', '%s', '%s', '%s', '%s');",
		     nombre, apellido, dni, email, rol);

		     int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
		     if (rc != SQLITE_OK) {

		    	 sqlite3_free(err_msg);
		     } else {
		    	 printf(" -> Usuario %s insertado.\n", nombre);
		     }
		}

	}
	fclose(archivo);
	    sqlite3_close(db);
	    printf("Importacion terminada.\n");
}

const char* rol_a_texto(RolUsuario rol) {
    if (rol == ROL_ADMIN) return "ADMIN";
    if (rol == ROL_EMPLEADO) return "MAQUINISTA";
    return "PASAJERO";
}

int insertar_usuario_db(Usuario u){
	sqlite3 *db;
	char *err_msg = 0;
	if(sqlite3_open(DB_PATH, &db) != SQLITE_OK){
		return 1;
	}
	char sql[1024];
	// Formateamos la consulta SQL usando los datos del struct
	snprintf(sql, sizeof(sql),
	"INSERT INTO USUARIOS (nombre, apellido, dni, email, telf, fecha_nac, pass_hash, rol, activo, fecha_registro) "
	"VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s');",
	u.nombre, u.apellido, u.dni, u.email, u.telf, u.fecha_nac,
	u.pass_hash, rol_a_texto(u.rol), u.activo, u.fecha_registro);

	int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
	if(rc != SQLITE_OK){
		printf("Error no se pudo insertar usuario %s: %s\n", u.nombre, err_msg);
		sqlite3_free(err_msg);
		sqlite3_close(db);
		return 1;
	}

	printf("Ok el usuario %s %s se ha guardado correctamente en la base de datos 	\n", u.nombre, u.apellido);
	sqlite3_close(db);
	return 0;
}

void listar_usuarios_db(){
	sqlite3 *db;
	sqlite3_stmt *stmt;
	if(sqlite3_open(DB_PATH, &db) != SQLITE_OK){
		return;
	}
	const char *sql = "SELECT id_u, nombre, apellido, dni, rol FROM USUARIOS;";
	int result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (result != SQLITE_OK) {
		printf("Error al preparar SELECT: %s\n", sqlite3_errmsg(db));
	    sqlite3_close(db);
	    return;
	}

	printf("\n--- LISTA DE USUARIOS EN EL SISTEMA ---\n");
	printf("%-5s | %-15s | %-15s | %-10s | %-10s\n", "ID", "NOMBRE", "APELLIDO", "DNI", "ROL");
	printf("-----------------------------------------------------------------\n");

	// sqlite3_step lee fila por fila hasta que no queden más (SQLITE_ROW)
	while (sqlite3_step(stmt) == SQLITE_ROW) {
	    Usuario u; // Creamos un struct temporal

	    // Extraemos los datos de las columnas (0, 1, 2, 3...) y los metemos al struct
	    u.id_u = sqlite3_column_int(stmt, 0);

	    // Usamos strcpy para copiar el texto de la BD al char[] del struct
	    strcpy(u.nombre, (const char*)sqlite3_column_text(stmt, 1));
	    strcpy(u.apellido, (const char*)sqlite3_column_text(stmt, 2));
	    strcpy(u.dni, (const char*)sqlite3_column_text(stmt, 3));

	    // Conversión inversa: Texto de la BD -> Enum de C
	    const char* rol_txt = (const char*)sqlite3_column_text(stmt, 4);
	    if (strcmp(rol_txt, "ADMIN") == 0) u.rol = ROL_ADMIN;
	    else if (strcmp(rol_txt, "MAQUINISTA") == 0) u.rol = ROL_EMPLEADO;
	    else u.rol = ROL_PASAJERO;

        // Lo imprimimos (aquí podrías guardarlo en un array de structs si quisieras)
	    printf("%-5d | %-15s | %-15s | %-10s | %-10s\n",
	           u.id_u, u.nombre, u.apellido, u.dni, rol_txt);
	}

	// Limpiamos memoria y cerramos
	sqlite3_finalize(stmt);
	sqlite3_close(db);
}
bool verificar_usuario (char *email, char *contraseniaIntroducida){
	if (comprobar_usuario_registrado(email)){
		if (comprobar_contrasenia(email, contraseniaIntroducida)){
			return true;
		}else{
			printf("Contraseña incorrecta.");
		}
	}else{
		printf("Email no registrado.");
	}
	return false;
}
bool comprobar_usuario_registrado (char *email){
	sqlite3 *db;
	sqlite3_stmt *stmt;
	if(sqlite3_open(DB_PATH, &db) != SQLITE_OK){
		return false;
	}
	char sql[] = "select count(*) from USUARIOS where email = ?";

	int result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (result != SQLITE_OK){
		printf("Error preparando statement // comprobar_usuario_registrado");
		return false;
	}
	result = sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
	if (result != SQLITE_OK){
		printf("Error haciendo el bind // comprobar_usuario_registrado");
		return false;
	}
	int seleccion = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		seleccion = sqlite3_column_int(stmt, 0);
    }


	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return seleccion == 1;
}
bool comprobar_contrasenia (char *email, char *contraseniaIntroducida){
	sqlite3 *db;
	sqlite3_stmt *stmt;
	if(sqlite3_open(DB_PATH, &db) != SQLITE_OK){
		return false;
	}
	char sql[] = "select pass_hash from USUARIOS where email = ?";

	int result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	if (result != SQLITE_OK){
		printf("Error preparando statement // comprobar_usuario_registrado");
		return false;
	}
	result = sqlite3_bind_text(stmt, 1, email, -1, SQLITE_TRANSIENT);
	if (result != SQLITE_OK){
		printf("Error haciendo el bind // comprobar_usuario_registrado");
		return false;
	}
	bool coincide = false;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		//HAY QUE DECIDIR SI LA CONTRASEÑA SE GUARDA COMO UN HASH O NO EN LA BBDD PARA
		//TENER QUE CIFRAR Y DESCIFRAR O NO
	    const char *pass_hash = (const char *)sqlite3_column_text(stmt, 0);
	    if (strcmp(pass_hash, contraseniaIntroducida) == 0){

	    	coincide = true;
	    }
	}


	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return  coincide;
}
