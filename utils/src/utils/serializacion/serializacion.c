#include "serializacion.h"
#include <stdlib.h>
#include <string.h>

void enviar_mensaje(char* mensaje, op_code codigo_operacion, int socket_cliente) {
    int tamaño_mensaje = strlen(mensaje) + 1;
    int cod_op = codigo_operacion; 
    int tamaño_total = sizeof(int) * 2 + tamaño_mensaje;
    void* buffer = malloc(tamaño_total);
    int desplazamiento = 0;

    memcpy(buffer + desplazamiento, &cod_op, sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(buffer + desplazamiento, &tamaño_mensaje, sizeof(int));
    desplazamiento += sizeof(int);

    memcpy(buffer + desplazamiento, mensaje, tamaño_mensaje);

    send(socket_cliente, buffer, tamaño_total, 0);
    free(buffer);
}

/* convierte una estructura PCB (con lista de segmentos)en una secuencia de bytes
1. empaqueta los datos fijos del PCB,2.Escribe el tamaño de la lista de segmentos,
3.recorre la lista y serializa cada segmento uno por uno. Esto permite enviar estructuras 
dinamicas por un socket*/

void enviar_pcb(t_pcb* pcb, int socket, op_code cod_op) {
    // 1. Calculamos cuánto espacio necesitamos: 
    // Opcode + PCB fijo + tamaño de la lista + (cantidad de segmentos * tamaño de cada segmento)
    int cantidad_segmentos = list_size(pcb->tabla_segmentos);
    int tamaño_lista = cantidad_segmentos * sizeof(t_segmento);
    int tamaño_total = sizeof(op_code) + sizeof(t_pcb) + sizeof(int) + tamaño_lista;
    
    void* buffer = malloc(tamaño_total);
    int desplazamiento = 0;

    // 2. Copiamos el Opcode
    memcpy(buffer + desplazamiento, &cod_op, sizeof(op_code));
    desplazamiento += sizeof(op_code);

    // 3. Copiamos el PCB (la lista de segmentos acá es solo un puntero, se copia como dirección, hay que ignorarlo en el receptor)
    memcpy(buffer + desplazamiento, pcb, sizeof(t_pcb));
    desplazamiento += sizeof(t_pcb);

    // 4. Copiamos la cantidad de segmentos y el contenido de la lista
    memcpy(buffer + desplazamiento, &cantidad_segmentos, sizeof(int));
    desplazamiento += sizeof(int);

    for (int i = 0; i < cantidad_segmentos; i++) {
        t_segmento* seg = list_get(pcb->tabla_segmentos, i);
        memcpy(buffer + desplazamiento, seg, sizeof(t_segmento));
        desplazamiento += sizeof(t_segmento);
    }

    send(socket, buffer, tamaño_total, 0);
    free(buffer);
}

void enviar_operacion(op_code operacion, int socket_cliente) {
    send(socket_cliente, &operacion, sizeof(op_code),0);
}

int recibir_operacion(int socket_cliente) {
    int cod_op;
    if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0) {
        return cod_op;
    } else {
        close(socket_cliente);
        return -1;
    }
}

char* recibir_mensaje(int socket_cliente) {
    int tamaño_mensaje;

    if (recv(socket_cliente, &tamaño_mensaje, sizeof(int), MSG_WAITALL) != sizeof(int)) {
        return NULL;
    }

    char* buffer = malloc(tamaño_mensaje);

    if (recv(socket_cliente, buffer, tamaño_mensaje, MSG_WAITALL) != tamaño_mensaje) {
        free(buffer);
        return NULL;
    }

    return buffer;
}
/*Deserializacion de PCB, reconstruye un PCB a partir de una secuencia de bytes recibida.
  1. Recibe la parte estatica, 2. crea una nueva lista vacia,3. usa el tamaño recibido para iterar y recrear cada objeto segmento*/
t_pcb* recibir_pcb(int socket) {
    t_pcb* pcb = malloc(sizeof(t_pcb));

    // 1. Recibimos la parte fija del PCB
    if (recv(socket, pcb, sizeof(t_pcb), MSG_WAITALL) != sizeof(t_pcb)) {
        free(pcb);
        return NULL;
    }

    // 2. Inicializamos la lista de segmentos antes de cargarla, al recibir por red el puntero a la lista no vale nada, tenemos que crear una nueva.
    pcb->tabla_segmentos = list_create();

    // 3. Recibimos la cantidad de segmentos
    int cantidad_segmentos;
    recv(socket, &cantidad_segmentos, sizeof(int), MSG_WAITALL);

    // 4. Recibimos cada segmento y lo agregamos a la lista
    for (int i = 0; i < cantidad_segmentos; i++) {
        t_segmento* seg = malloc(sizeof(t_segmento));
        recv(socket, seg, sizeof(t_segmento), MSG_WAITALL);
        list_add(pcb->tabla_segmentos, seg);
    }

    return pcb;
}