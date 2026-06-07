#ifndef PCB_H_
#define PCB_H_

#include <stdint.h>

//creamos el diccionario de estados posibles

typedef enum { 
    NEW,
    READY,
    EXEC,
    BLOCK,
    EXIT
} t_estado;

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
} t_pcb;

#endif 
