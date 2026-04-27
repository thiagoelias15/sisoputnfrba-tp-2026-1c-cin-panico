#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/log.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/config.h> // Libreria para leer archivos .config
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/collections/dictionary.h>

// Códigos para avisarle al que recibe que tipo de paquete estamos mandando

/*
    Creamos una variable typedef(op_code) de tipo enum, la cual asigna números enteros a palabras al compilar.

    MENSAJE = 0
    PAQUETE = 1
    PETICION_LECTURA = 2
    DATO_MEMORIA = 3
*/

typedef enum { 
    MENSAJE,
    PAQUETE,
    HANDSHAKE_CPU_A_MS,
   //operaciones del checkpoint 2
    SOLICITUD_INSTRUCCION,
    CONTEXTO_PCB,
    INTERRUPCION,
    //Syscalls(peticiones de CPU a scheduler)
    SYSCALL_SLEEP,
    SYSCALL_STDIN,
    SYSCALL_STDOUT,
    SYSCALL_MUTEX_CREATE,
    SYSCALL_MUTEX_LOCK,
    SYSCALL_MUTEX_UNLOCK,
    SYSCALL_EXIT
} op_code;

typedef enum { //creamos el diccionario de estados posibles 
    NEW,
    READY,
    EXEC,
    BLOCK,
    EXIT
}t_estado; 



// estructura del PCB(process control block)
typedef struct{
uint32_t pid;      
    uint32_t pc;       
    
    t_estado estado;
    // Registros de 1 byte
    uint8_t ax, bx, cx, dx;

    // Registros de 4 bytes
    uint32_t eax, ebx, ecx, edx;

    // Registros de dirección lógica (4 bytes)
    uint32_t si, di;
} t_pcb;

// Se definen las funciones a utilizar en los módulos

int iniciar_servidor(char* puerto);
int esperar_cliente(int socket_servido);
int crear_conexion(char* ip,char* puerto);
t_config* iniciar_config(char*path_config);

void enviar_mensaje(char* mensaje, op_code codigo_operacion, int socket_cliente);
int recibir_operacion(int socket_cliente);
char* recibir_mensaje(int socket_cliente);

//funciones para el intercambio de contextos
void enviar_pcb(t_pcb* pcb,int socket,op_code cod_op);
t_pcb* recibir_pcb(int socket);

#endif