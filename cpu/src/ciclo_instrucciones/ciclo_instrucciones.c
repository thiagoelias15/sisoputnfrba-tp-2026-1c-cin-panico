#include "ciclo_instrucciones.h"

char* realizar_fetch(t_pcb* pcb, int fd_memoria, t_log* logger) {
    // Le mandamos a Memoria el codigo de operacion, el PID y el PC actual
    op_code op_fetch = FETCH_INSTRUCCION;
    send(fd_memoria, &op_fetch, sizeof(op_code), 0);
    send(fd_memoria, &(pcb->pid), sizeof(int), 0);
    send(fd_memoria, &(pcb->pc), sizeof(uint32_t), 0);

    // Esperamos la respuesta (el texto de la instruccion)
    char* instruccion = recibir_mensaje(fd_memoria);
    log_info(logger, "## PID: %d - FETCH - Program Counter: %d", pcb->pid, pcb->pc);
    return instruccion;
}

int hay_interrupcion(int fd_scheduler, t_log* logger) {
    int interrupcion = 0;
    // MSG_DONTWAIT permite ver el socket sin quedarse bloqueado esperando. 
    // Si no hay nada, la CPU sigue a la siguiente instruccion.
    // Revisa el socket del Scheduler sin bloquearse (MSG_DONTWAIT)
    int status = recv(fd_scheduler, &interrupcion, sizeof(int), MSG_DONTWAIT);
    if (status > 0) {
        log_info(logger, "## Interrupcion recibida");
        return 1; // Ej: puede pasar por fin de quantum
    }
    return 0;
}

void gestionar_desalojo(t_pcb* pcb, op_code motivo, char** tokens, int fd_scheduler) {
    // Si se rompio el ciclo se le devuelve todo al scheduler
    enviar_operacion(motivo, fd_scheduler);

    // le devolvemos el PCB actualizado (el "contexto")
    enviar_pcb(pcb, fd_scheduler, motivo);
    
    // Mandamos los datos extra dependiendo de que Syscall provoco el desalojo
    if (motivo == SYSCALL_SLEEP) {
        int tiempo = atoi(tokens[1]);
        send(fd_scheduler, &tiempo, sizeof(int), 0);
    } 
    else if (motivo == SYSCALL_STDOUT || motivo == SYSCALL_STDIN) {
        int dir_logica = obtener_valor_registro(pcb, tokens[1]); 
        int tam = obtener_valor_registro(pcb, tokens[2]); 
        //aca ya sabemos que el proceso para la barrera de la MMU
        //volvemos a traducir de forma segura para darsela al scheduler
        int dir_fisica = traducir_direccion_mmu(dir_logica, tam, pcb);
        send(fd_scheduler, &tam, sizeof(int), 0);
        send(fd_scheduler, &dir_fisica, sizeof(int), 0);
        send(fd_scheduler, &(pcb->pid), sizeof(int), 0);
    }
    else if (motivo == SYSCALL_MUTEX_CREATE || motivo == SYSCALL_MUTEX_LOCK || motivo == SYSCALL_MUTEX_UNLOCK) {
        // Mandamos un char* (el nombre del recurso) al Scheduler
        int len_string = strlen(tokens[1]) + 1;
        send(fd_scheduler, &len_string, sizeof(int), 0);
        send(fd_scheduler, tokens[1], len_string, 0);
    }

    
}