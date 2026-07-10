#ifndef MAIN_H_
#define MAIN_H_

#include <utils/utils.h>
#include "config/config.h"
#include <commons/log.h>
#include <commons/string.h>
#include "ciclo_instrucciones/ciclo_instrucciones.h"


extern t_log* logger;
extern int cpu_corriendo;
extern int tam_max_segmento; // esto es para la MMU(cuando cpu lee stdin y stdout recibe direccion logica y se debe hacer una cuenta matematica para mandarsela al scheduler)
// estructura para los sticks a los que la CPU se conecta
typedef struct {
    int fd_socket;
    uint32_t base_global;
    uint32_t tamanio;
} t_stick_cpu;

extern t_list* sticks_cpu;


#endif