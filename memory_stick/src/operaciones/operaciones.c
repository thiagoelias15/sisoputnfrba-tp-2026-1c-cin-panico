#include "operaciones.h"

void escribir_en_memoria(int dir_fisica, void* contenido, int tamanio) {
    // simulamos el tiempo fisico del hardware
    usleep(ms.config.memory_delay * 1000);
    // escribir los datos a partir de la direccion fisica que agarramos antes
    memcpy((char*)espacio_memoria + dir_fisica, contenido, tamanio);
    log_info(logger, "## Escritura de %d bytes", tamanio);
}

void* leer_de_memoria(int dir_fisica, int tamanio){
    usleep(ms_config.memory-delay * 1000);
    //preparamos un espacio en memoria para meter los datos
    void* buffer_lectura = malloc(tamanio);
    memcpy(buffer_lectura, (char*)espacio_memoria + dir_fisica, tamanio);
    log_info(logger, "## Lectura de %d bytes", tamanio);
    return buffer_lectura; //Devolvemos los bytes leidos
}

