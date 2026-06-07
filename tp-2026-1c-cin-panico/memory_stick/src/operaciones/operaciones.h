#ifndef MS_OPERACIONES_H_
#define MS_OPERACIONES_H_

#include "../main.h"

void escribir_en_memoria(int dir_fisica, void* contenido, int tamanio);
void* leer_de_memoria(int dir_fisica, int tamanio);

#endif