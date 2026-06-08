#ifndef PCB_H_
#define PCB_H_

#include <stdint.h>
#include <commons/collections/list.h> 

//creamos el diccionario de estados posibles

typedef enum { 
    NEW,
    READY,
    EXEC,
    BLOCK,
    EXIT
} t_estado;
typedef struct {
    int id;
    uint32_t direccion_base;
    uint32_t tamanio;
} t_segmento;

// estructura del PCB(process control block)
typedef struct {
    uint32_t pid;      
    uint32_t pc;       
    t_estado estado;

    // Registros de 1 byte
    uint8_t ax, bx, cx, dx;

    // Registros de 4 bytes
    uint32_t eax, ebx, ecx, edx;

    // Registros de dirección lógica (4 bytes)
    uint32_t si, di;

    int prioridad;  // la prioridad actual con la que compite en READY
    int prioridad_original; //  la prioridad base para restaurarlo despues de soltar el Mutex
    t_list* tabla_segmentos;
} t_pcb;

#endif 
