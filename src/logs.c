/*
 * logs.c
 *
 *  Created on: 1 abr 2026
 *      Author: e.aranoa
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

