/*
 * logs.h
 *

 */

#ifndef LOGS_H
#define LOGS_H

void log_evento(const char *log_path, const char *email_usuario,
                const char *tipo, const char *descripcion);

void exportar_logs(const char *log_path, const char *destino); // Sirve para sacar la informacion del log
// a otro sitio ya que cada dia se va llenando para exportarlo a otro archivo


#endif
