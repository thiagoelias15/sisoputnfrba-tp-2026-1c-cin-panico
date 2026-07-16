#ifndef PCB_H
#define PCB_H

#include <stdint.h>
#include <commons/collections/list.h> 

//creamos el diccionario de estados posibles

typedef enum { 
    NEW,
    READY,
    EXEC,
    BLOCK,
    SUSP_BLOCK,
    SUSP_READY,
    EXIT
} t_estado;
typedef struct {
    int id;
    uint32_t direccion_base;
    uint32_t tamanio;
} t_segmento;

// estructura del PCB(process control block)
#pragma pack (push,1)
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
    int mem_pendiente_id;
    int mem_pendiente_tam;
} t_pcb;
#pragma pack (pop)

t_pcb* pcb_create();
#endif 
