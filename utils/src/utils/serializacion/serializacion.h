#ifndef UTILS_SERIALIZACION_H_
#define UTILS_SERIALIZACION_H_

#include "utils/pcb/pcb.h"// Para usar t_pcb
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

// Códigos para avisarle al que recibe que tipo de paquete estamos mandando

/*
Creamos una variable typedef(op_code) de tipo enum, la cual asigna números enteros a palabras al compilar.

MENSAJE = 0
PAQUETE = 1
PETICION_LECTURA = 2
DATO_MEMORIA = 3
...
*/

typedef enum {
    MENSAJE = 0,
    PAQUETE = 1,
    HANDSHAKE_CPU_A_MS = 2,
    SOLICITUD_INSTRUCCION = 3,
    CONTEXTO_PCB = 4,
    INTERRUPCION = 5,

    //Syscalls(peticiones de CPU a scheduler)
    SYSCALL_SLEEP = 6,
    SYSCALL_STDIN = 7,
    SYSCALL_STDOUT = 8,
    SYSCALL_MUTEX_CREATE = 9,
    SYSCALL_MUTEX_LOCK  = 10,
    SYSCALL_MUTEX_UNLOCK = 11,
    SYSCALL_EXIT = 12,
    SEG_FAULT =13,

    //Syscalls(cpu)
    FETCH_INSTRUCCION = 14,
    SYSCALL_MEM_ALLOC = 15,
    SYSCALL_MEM_FREE = 16,
    SYSCALL_INIT_PROC = 17,
    //Memoria
    CONSULTAR_ESPACIO = 18,
    LEER_MEMORIA = 19,
    ESCRIBIR_MEMORIA = 20,
    SWAP_LECTURA = 21,
    SWAP_ESCRITURA = 22,
    NUEVO_STICK=23,
    MEMORIA_CORRUPTA=24,
    PEDIDO_COMPACTACION = 25,
    COMPACTACION_OK= 26
} op_code;

// Funciones para enviar y recibir mensajes y PCBs
void enviar_mensaje(char* mensaje, op_code codigo_operacion, int socket_cliente);
char* recibir_mensaje(int socket_cliente);

//funciones para el intercambio de contextos
void enviar_pcb(t_pcb* pcb, int socket, op_code cod_op);
t_pcb* recibir_pcb(int socket);

void enviar_operacion(op_code operacion, int socket_cliente);

int recibir_operacion(int socket_cliente);

#endif 
