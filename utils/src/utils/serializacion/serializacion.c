#include "utils/serializacion/serializacion.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
// -----------------------------SERIALIZACION---------------------------------//
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

void enviar_pcb(t_pcb* pcb, int socket, op_code cod_op) {
    void* buffer = malloc(sizeof(op_code) + sizeof(t_pcb));
    int desplazamiento = 0;

    memcpy(buffer + desplazamiento, &cod_op, sizeof(op_code));
    desplazamiento += sizeof(op_code);

    memcpy(buffer + desplazamiento, pcb, sizeof(t_pcb));

    send(socket, buffer, sizeof(op_code) + sizeof(t_pcb), 0);
    free(buffer);
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

t_pcb* recibir_pcb(int socket) {
    t_pcb* pcb = malloc(sizeof(t_pcb));

    if (recv(socket, pcb, sizeof(t_pcb), MSG_WAITALL) != sizeof(t_pcb)) {
        free(pcb);
        return NULL;
    }

    return pcb;
}
