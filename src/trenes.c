/*
 * trenes.c
 *
 *  Created on: 3 abr 2026
 *      Author: e.aranoa
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trenes.h"
#include "config.h"
#include "logs.h"
#include "sqlite3.h"

// ============================================================
//  HELPERS
// ============================================================

const char* estado_tren_a_texto(EstadoMantenimiento estado) {
    switch(estado) {
        case TREN_OPERATIVO: return "OPERATIVO";
        case TREN_REVISION:  return "REVISION";
        case TREN_AVERIA:    return "AVERIA";
        case TREN_RETIRADO:  return "RETIRADO";
        default:             return "DESCONOCIDO";
    }
}

EstadoMantenimiento texto_a_estado_tren(const char *texto) {
    if (strcmp(texto, "OPERATIVO") == 0) return TREN_OPERATIVO;
    if (strcmp(texto, "REVISION")  == 0) return TREN_REVISION;
    if (strcmp(texto, "AVERIA")    == 0) return TREN_AVERIA;
    if (strcmp(texto, "RETIRADO")  == 0) return TREN_RETIRADO;
    return TREN_OPERATIVO;
}

// ============================================================
//  CRUD
// ============================================================

int insertar_tren_db(Tren t) {
    sqlite3 *db;
    char *err_msg = 0;

    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) return 1;

    char sql[512];
    snprintf(sql, sizeof(sql),
        "INSERT INTO TRENES (modelo, cap_turista, cap_business, anio_fab, num_serie, estado_mant) "
        "VALUES ('%s', %d, %d, %d, '%s', '%s');",
        t.nombre_modelo, 0, 0, t.anio_fab,
        t.num_serie, estado_tren_a_texto(t.estado_mant));

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("Error al insertar tren: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("[OK] Tren %s insertado correctamente.\n", t.nombre_modelo);
    log_evento(cfg.log_path, NULL, "INSERT", t.nombre_modelo);
    sqlite3_close(db);
    return 0;
}

void listar_trenes_db() {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) return;

    const char *sql = "SELECT id_t, modelo, num_serie, anio_fab, estado_mant FROM TRENES;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        return;
    }

    printf("\n%-5s | %-20s | %-15s | %-6s | %-12s\n",
           "ID", "MODELO", "NUM SERIE", "AÑO", "ESTADO");
    printf("---------------------------------------------------------------\n");

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int    id     = sqlite3_column_int(stmt, 0);
        const char *modelo  = (const char*)sqlite3_column_text(stmt, 1);
        const char *serie   = (const char*)sqlite3_column_text(stmt, 2);
        int    anio   = sqlite3_column_int(stmt, 3);
        const char *estado  = (const char*)sqlite3_column_text(stmt, 4);

        printf("%-5d | %-20s | %-15s | %-6d | %-12s\n",
               id, modelo, serie, anio, estado);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

Tren obtener_tren_por_id(int id_t) {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    Tren t;
    memset(&t, 0, sizeof(t));

    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) return t;

    char sql[256];
    snprintf(sql, sizeof(sql),
        "SELECT id_t, modelo, num_serie, anio_fab, estado_mant FROM TRENES WHERE id_t = %d;", id_t);

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        return t;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        t.id_t = sqlite3_column_int(stmt, 0);
        strncpy(t.nombre_modelo, (const char*)sqlite3_column_text(stmt, 1), sizeof(t.nombre_modelo)-1);
        strncpy(t.num_serie,     (const char*)sqlite3_column_text(stmt, 2), sizeof(t.num_serie)-1);
        t.anio_fab    = sqlite3_column_int(stmt, 3);
        t.estado_mant = texto_a_estado_tren((const char*)sqlite3_column_text(stmt, 4));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return t;
}

int modificar_estado_tren_db(int id_t, EstadoMantenimiento nuevo_estado) {
    sqlite3 *db;
    char *err_msg = 0;

    if (sqlite3_open(cfg.db_path, &db) != SQLITE_OK) return 1;

    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE TRENES SET estado_mant = '%s' WHERE id_t = %d;",
        estado_tren_a_texto(nuevo_estado), id_t);

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("Error al modificar tren: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }

    printf("[OK] Estado del tren %d actualizado a %s.\n",
           id_t, estado_tren_a_texto(nuevo_estado));
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

// ============================================================
//  MENUS
// ============================================================

void menu_añadir_tren() {
    Tren t;
    memset(&t, 0, sizeof(t));
    int estado_opcion;

    printf("\n=== AÑADIR NUEVO TREN ===\n");
    printf("Modelo: ");
    scanf("%63s", t.nombre_modelo);
    printf("Numero de serie: ");
    scanf("%31s", t.num_serie);
    printf("Año de fabricacion: ");
    scanf("%d", &t.anio_fab);
    printf("Estado (0:Operativo, 1:Revision, 2:Averia, 3:Retirado): ");
    scanf("%d", &estado_opcion);
    t.estado_mant = (EstadoMantenimiento)estado_opcion;

    insertar_tren_db(t);
}

void menu_modificar_estado_tren() {
    int id_t, estado_opcion;

    listar_trenes_db();
    printf("\nID del tren a modificar: ");
    scanf("%d", &id_t);
    printf("Nuevo estado (0:Operativo, 1:Revision, 2:Averia, 3:Retirado): ");
    scanf("%d", &estado_opcion);

    modificar_estado_tren_db(id_t, (EstadoMantenimiento)estado_opcion);
    log_evento(cfg.log_path, NULL, "UPDATE", "Estado de tren modificado");
}

void menu_eliminar_tren() {
    int id_t;
    char confirm;

    listar_trenes_db();
    printf("\nID del tren a eliminar: ");
    scanf("%d", &id_t);

    Tren t = obtener_tren_por_id(id_t);
    if (t.id_t == 0) {
        printf("Tren no encontrado.\n");
        return;
    }

    printf("¿Seguro que quieres eliminar el tren %s? (s/n): ", t.nombre_modelo);
    scanf(" %c", &confirm);
    if (confirm == 's' || confirm == 'S') {
        eliminar_tren_db(id_t);
    } else {
        printf("Operacion cancelada.\n");
    }
}

void menu_buscar_tren() {
    char modelo[64];
    printf("Introduce modelo a buscar: ");
    scanf("%63s", modelo);
    buscar_tren_por_modelo(modelo);
}

void menu_gestion_trenes() {
    int opcion;
    do {
        printf("\n=== GESTION DE TRENES ===\n");
        printf("1. Listar trenes\n");
        printf("2. Añadir tren\n");
        printf("3. Modificar estado de tren\n");
        printf("4. Eliminar tren\n");
        printf("5. Buscar tren por modelo\n");
        printf("0. Volver\n");
        printf("Opcion: ");
        scanf("%d", &opcion);

        switch(opcion) {
            case 1: listar_trenes_db();          break;
            case 2: menu_añadir_tren();           break;
            case 3: menu_modificar_estado_tren(); break;
            case 4: menu_eliminar_tren();         break;
            case 5: menu_buscar_tren();           break;
            case 0: break;
            default: printf("Opcion no valida.\n");
        }
    } while(opcion != 0);
}

