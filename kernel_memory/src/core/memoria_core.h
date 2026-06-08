#ifndef MEMORIA_CORE_H_
#define MEMORIA_CORE_H_



#include <stdint.h>
#include <commons/log.h>
#include <commons/config.h>
#include <stdlib.h>      // Para malloc y free
#include <string.h>      // Para strcmp, memcpy y strcspn
#include <unistd.h>      // Para usleep
#include <sys/socket.h>  // Para recv, send y MSG_WAITALL
#include <pthread.h>     // Para los mutex
#include "utils/serializacion/serializacion.h" 
#include <commons/collections/list.h>

extern t_log* logger;
extern pthread_mutex_t m_memoria;
extern t_list* tabla_segmentos_global;
extern void* espacio_memoria_real;

void atender_fetch_cpu(int fd_cpu);
void atender_consulta_espacio(int fd_kernel);
void atender_lectura_memoria(int fd_modulo);
void atender_escritura_memoria(int fd_modulo);
void atender_creacion_proceso(int fd_kernel);
#endif