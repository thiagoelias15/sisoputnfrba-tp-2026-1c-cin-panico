#ifndef MEMORIA_ADMINISTRADOR_H_
#define MEMORIA_ADMINISTRADOR_H_


#include <stdint.h>
#include <commons/collections/list.h>

typedef struct {
    int id;
    int pid;
    uint32_t base;
    uint32_t tamanio;
    int ocupado; // 1 = ocupado, 0 = libre
} t_segmento_memoria;

// Funciones principales de gestión
void inicializar_memoria();
// Algoritmo de búsqueda interna (Best Fit)
t_segmento_memoria* buscar_hueco_best_fit(uint32_t tamanio_necesario);

// Función principal para asignar espacio a un proceso
int asignar_memoria(int pid, uint32_t tamanio);
void liberar_memoria(int pid, int id_segmento);
void compactar_memoria();
#endif


