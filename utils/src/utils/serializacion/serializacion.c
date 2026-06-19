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
    int cantidad_segmentos = list_size(pcb->tabla_segmentos);
    int tamaño_segmentos = cantidad_segmentos * sizeof(t_segmento);
    
    // Calculamos tamaño fijo de los campos (pid + pc + estado + registros)
    int tamaño_fijo = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(t_estado) + 
                      (sizeof(uint8_t) * 4) + (sizeof(uint32_t) * 6) + 
                      sizeof(int) + sizeof(int);
    
    int tamaño_total = sizeof(op_code) + tamaño_fijo + sizeof(int) + tamaño_segmentos;
    
    void* buffer = malloc(tamaño_total);
    int desp = 0;

    memcpy(buffer + desp, &cod_op, sizeof(op_code)); desp += sizeof(op_code);
    
    // Serialización manual campo por campo (evita punteros basura)
    memcpy(buffer + desp, &pcb->pid, sizeof(uint32_t)); desp += sizeof(uint32_t);
    memcpy(buffer + desp, &pcb->pc, sizeof(uint32_t)); desp += sizeof(uint32_t);
    memcpy(buffer + desp, &pcb->estado, sizeof(t_estado)); desp += sizeof(t_estado);
    memcpy(buffer + desp, &pcb->ax, sizeof(uint8_t)); desp += sizeof(uint8_t);
    memcpy(buffer + desp, &pcb->bx, sizeof(uint8_t)); desp += sizeof(uint8_t);
    memcpy(buffer + desp, &pcb->cx, sizeof(uint8_t)); desp += sizeof(uint8_t);
    memcpy(buffer + desp, &pcb->dx, sizeof(uint8_t)); desp += sizeof(uint8_t);
    memcpy(buffer + desp, &pcb->eax, sizeof(uint32_t)); desp += sizeof(uint32_t);
    memcpy(buffer + desp, &pcb->ebx, sizeof(uint32_t)); desp += sizeof(uint32_t);
    memcpy(buffer + desp, &pcb->ecx, sizeof(uint32_t)); desp += sizeof(uint32_t);
    memcpy(buffer + desp, &pcb->edx, sizeof(uint32_t)); desp += sizeof(uint32_t);
    memcpy(buffer + desp, &pcb->si, sizeof(uint32_t)); desp += sizeof(uint32_t);
    memcpy(buffer + desp, &pcb->di, sizeof(uint32_t)); desp += sizeof(uint32_t);
    memcpy(buffer + desp, &pcb->prioridad, sizeof(int)); desp += sizeof(int);
    memcpy(buffer + desp, &pcb->prioridad_original, sizeof(int)); desp += sizeof(int);

    memcpy(buffer + desp, &cantidad_segmentos, sizeof(int)); desp += sizeof(int);
    for (int i = 0; i < cantidad_segmentos; i++) {
        t_segmento* seg = list_get(pcb->tabla_segmentos, i);
        memcpy(buffer + desp, seg, sizeof(t_segmento));
        desp += sizeof(t_segmento);
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

    // Recibimos campo por campo en el mismo orden que enviamos
    recv(socket, &pcb->pid, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &pcb->pc, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &pcb->estado, sizeof(t_estado), MSG_WAITALL);
    recv(socket, &pcb->ax, sizeof(uint8_t), MSG_WAITALL);
    recv(socket, &pcb->bx, sizeof(uint8_t), MSG_WAITALL);
    recv(socket, &pcb->cx, sizeof(uint8_t), MSG_WAITALL);
    recv(socket, &pcb->dx, sizeof(uint8_t), MSG_WAITALL);
    recv(socket, &pcb->eax, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &pcb->ebx, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &pcb->ecx, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &pcb->edx, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &pcb->si, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &pcb->di, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, &pcb->prioridad, sizeof(int), MSG_WAITALL);
    recv(socket, &pcb->prioridad_original, sizeof(int), MSG_WAITALL);

    pcb->tabla_segmentos = list_create();
    int cantidad_segmentos;
    recv(socket, &cantidad_segmentos, sizeof(int), MSG_WAITALL);

    for (int i = 0; i < cantidad_segmentos; i++) {
        t_segmento* seg = malloc(sizeof(t_segmento));
        recv(socket, seg, sizeof(t_segmento), MSG_WAITALL);
        list_add(pcb->tabla_segmentos, seg);
    }

    return pcb;
}