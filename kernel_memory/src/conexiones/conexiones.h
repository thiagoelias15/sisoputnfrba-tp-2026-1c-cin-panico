#ifndef CONEXIONES_MEMORIA_H_
#define CONEXIONES_MEMORIA_H_

#include <utils/utils.h>
#include <commons/log.h>
#include <pthread.h>
#include "../core/memoria_core.h"

extern t_log* logger;

// Función que va a ejecutar cada hilo para atender a un cliente específico
void* atender_cliente(void* arg);

#endif