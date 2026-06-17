#ifndef CONEXIONES_MEMORIA_H_
#define CONEXIONES_MEMORIA_H_

#include <utils/utils.h>
#include <commons/log.h>
#include <pthread.h>
#include "../core/memoria_core.h"
#include "../../../utils/src/utils/serializacion/serializacion.h"

extern t_log* logger;

// Función que va a ejecutar cada hilo para atender a un cliente específico
void* atender_cliente(void* arg);
void atender_mem_alloc(int fd_cliente);
void atender_mem_free(int fd_cliente);
#endif