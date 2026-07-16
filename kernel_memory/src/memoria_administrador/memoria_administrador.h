#ifndef MEMORIA_ADMINISTRADOR_H_
#define MEMORIA_ADMINISTRADOR_H_


#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <pthread.h>

typedef struct {
    int fd_socket;
    char* ip_escucha;
    char* puerto_escucha;
    uint32_t base_global;
    uint32_t tamanio;
    }t_memory_stick_info;
typedef struct {
    int id_segmento;
    uint32_t tamanio;
    int bloque_swap_inicio; // en que bloque de SWAP arranca
    int cant_bloques; // cuantos bloques ocupa
}t_segmento_swap;

extern t_dictionary* segmentos_en_swap; // PID(string) -> t_list* de t_segmento_swap
extern pthread_mutex_t m_swap;
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
t_segmento_memoria* buscar_hueco_worst_fit(uint32_t tamanio_necesario);

// Función principal para asignar espacio a un proceso
uint32_t asignar_memoria(int pid, uint32_t tamanio, int id_segmento);
void liberar_memoria(int pid, int id_segmento);
void compactar_memoria();
void leer_de_sticks(uint32_t dir_global, void* buffer, uint32_t tamanio);
void escribir_en_sticks(uint32_t dir_global, void* buffer, uint32_t tamanio);
void liberar_todos_segmentos_pid(int pid);
extern int fd_scheduler_global;
extern int fd_swap;
extern int swap_block_size;
extern int swap_file_size;
extern pthread_mutex_t m_swap_socket;
#endif


