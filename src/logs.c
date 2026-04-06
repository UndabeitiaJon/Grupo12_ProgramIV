/*
 * logs.c
 *
 *  Created on: 1 abr 2026

 */
#include <stdio.h>
#include <time.h>
#include "logs.h"

void log_evento(const char *log_path, const char *email_usuario,
                const char *tipo, const char *descripcion) {
    FILE *f = fopen(log_path, "a");
    if (!f) return;

    time_t ahora = time(NULL);
    struct tm *t = localtime(&ahora);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    fprintf(f, "[%s] [%s] [%s] %s\n",
            timestamp,
            email_usuario ? email_usuario : "SISTEMA",
            tipo,
            descripcion);

    fclose(f);
}

void exportar_logs(const char *log_path, const char *destino){
	FILE *fsrc = fopen(log_path, "r");
	if (!fsrc) {
		printf("No se pudo abrir el log origen: %s\n", log_path);
		return;
	}
	FILE *fdest = fopen(destino, "w");
	if (!fdest) {
		 printf("No se pudo crear el destino: %s\n", destino);
		 fclose(fsrc); return;
	}
	char buf[1024];
	int lineas = 0;
	while (fgets(buf, sizeof(buf), fsrc)) {
	    fputs(buf, fdest);
	    lineas++;
	}
	fclose(fsrc);
	fclose(fdest);
	printf("[LOG] %d lineas exportadas a %s\n", lineas, destino);
}

