#include "memoria_core.h"

// Devolver lista de instrucciones
void atender_fetch_cpu(int fd_cpu) {
    int pid;
    uint32_t pc;
    
    recv(fd_cpu, &pid, sizeof(int), MSG_WAITALL);
    recv(fd_cpu, &pc, sizeof(uint32_t), MSG_WAITALL);

    log_info(logger, "Pedido de FETCH - PID: %d - PC: %d", pid, pc);
    usleep(memoria_config.instruction_delay * 1000); // Retardo 

    // Mock: Lista de instrucciones de prueba
    char* script_prueba[] = {
        "SET AX 10",
        "SET BX 5",
        "SUM AX BX",
        "SLEEP 2",
        "EXIT"
    };

    char* instruccion;
    // Si el PC está dentro de nuestra lista, le damos esa instrucción. Si se pasa, tiramos EXIT.
    if(pc < 5) {
        instruccion = script_prueba[pc];
    } else {
        instruccion = "EXIT";
    }
    
    enviar_mensaje(instruccion, FETCH_INSTRUCCION, fd_cpu);
}

// Devolver un valor fijo de espacio libre
void atender_consulta_espacio(int fd_kernel) {
    log_info(logger, "El Kernel consulto el espacio libre.");
    
    // Mock: Siempre decimos que tenemos 4096 bytes libres (o el valor que prefieras)
    int espacio_libre_falso = 4096; 
    send(fd_kernel, &espacio_libre_falso, sizeof(int), 0);
}

// Contestar OK a Lecturas (Sin implementarlas)
void atender_lectura_memoria(int fd_modulo) {
    log_info(logger, "Peticion de LECTURA recibida (Mock).");
    usleep(memoria_config.instruction_delay * 1000); // Retardo
    
    // Mock: Supongamos que nos piden leer, devolvemos siempre un número fijo, ej: 0.
    int valor_leido_falso = 0;
    send(fd_modulo, &valor_leido_falso, sizeof(int), 0);
}

// Contestar OK a Escrituras (Sin implementarlas)
void atender_escritura_memoria(int fd_modulo) {
    log_info(logger, "Peticion de ESCRITURA recibida (Mock).");
    usleep(memoria_config.instruction_delay * 1000); // Retardo
    
    // Mock: Devolvemos un código "OK" ( 1 = Exito, 0 = Error)
    int confirmacion_ok = 1;
    send(fd_modulo, &confirmacion_ok, sizeof(int), 0);
}