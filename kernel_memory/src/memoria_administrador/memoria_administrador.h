#ifndef MEMORIA_ADMINISTRADOR_H_
#define MEMORIA_ADMINISTRADOR_H_


#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>

typedef struct {
    int fd_socket;
    char* ip;
    char* puerto;
    uint32_t base_global;
    uint32_t tamanio;
    }t_memory_stick_info;
extern t_list* lista_sticks;
extern uint32_t memoria_total;
extern pthread_mutex_t m_sticks;
extern t_list* lista_cpus_conectadas; // para avisarle de sticks nuevos
extern pthread_mutex_t m_cpus;
extern t_dictionary* mapeo_archivos_procesos; // Guarda la relación PID -> Nombre del archivo .prc

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
void_leer_de_sticks(uint32_t dir_global, void* buffer, uint32_t tamanio);
void escribir_en_sticks(uint32_t dir_global, void* buffer, uint32_t tamanio);
#endif


