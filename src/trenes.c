/*
 * trenes.c

 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trenes.h"
#include "config.h"
#include "logs.h"
#include "sqlite3.h"
#include "db_manager.h"

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
//  MENUS
// ============================================================

void menu_anadir_tren() {
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
            case 2: menu_anadir_tren();           break;
            case 3: menu_modificar_estado_tren(); break;
            case 4: menu_eliminar_tren();         break;
            case 5: menu_buscar_tren();           break;
            case 0: break;
            default: printf("Opcion no valida.\n");
        }
    } while(opcion != 0);
}

