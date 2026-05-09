/*
 * hash.h
 *
 * Utilidad SHA-256 para TRENFE.
 * Uso:
 *   char hash[65];
 *   sha256_hex("mi_contrasena", hash);
 *   // hash contiene algo como "a3f1c9..."
 */

#ifndef HASH_H_
#define HASH_H_

#include <stdint.h>   /* uint8_t, uint32_t, uint64_t */
#include <stddef.h>   /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Calcula el SHA-256 de 'input' y escribe el resultado
 * como cadena hexadecimal en 'output'.
 * 'output' debe tener al menos 65 bytes (64 hex + '\0').
 */
void sha256_hex(const char *input, char output[65]);

#ifdef __cplusplus
}
#endif

#endif /* HASH_H_ */
