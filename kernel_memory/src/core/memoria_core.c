#include "memoria_core.h"
#include "../memoria_administrador/memoria_administrador.h"
#include <stdio.h>

//funcion que lee una linea especifica de un archivo
char* leer_instruccion_de_archivo(int pid, uint32_t pc) {
    char path_completo[256];
    sprintf(path_completo, "%s/MEMORIA_PRE_%d.prc", memoria_config.scripts_basepath, pid);

    FILE* archivo = fopen(path_completo, "r");
    if(archivo == NULL) {
        return "EXIT"; // si no existe el script cortamos todo
    }
    char* linea = malloc(128);
    int linea_actual = 0;

    //buscamos la linea correspondiente al PC
    while(fgets(linea, 128, archivo) != NULL){
        if(linea_actual == pc){
            //quitamos el salto de linea al final por las dudas
            linea[strcspn(linea, "\n")] = 0;
            fclose(archivo);
            return linea;
        }
        loena_actual++;
    }
    fclose(archivo);
    free(linea);
    return "EXIT" // si el pc es mayor a la cantidad de lineas, salimos
}
// Devolver lista de instrucciones
void atender_fetch_cpu(int fd_cpu) {
    int pid;
    uint32_t pc;
    
    recv(fd_cpu, &pid, sizeof(int), MSG_WAITALL);
    recv(fd_cpu, &pc, sizeof(uint32_t), MSG_WAITALL);

    log_info(logger, "Pedido de FETCH - PID: %d - PC: %d", pid, pc);
    usleep(memoria_config.instruction_delay * 1000); // Retardo 

    char* instruccion = leer_instruccion_de_archivo(pid, pc);
    enviar_mensaje(instruccion, FETCH_INSTRUCCION, fd_cpu);
    // si no fue el valor por defecto "EXIT", liberamos memoria de la linea leida
    if(strcmp(instruccion, "EXIT") != 0) free(instruccion);
}

// consulta el espacio real gestionado por memoria.administrador
void atender_consulta_espacio (int fd_kernel) {
    log_info(logger, "El Kernel consulto el espacio libre.");
    int espacio_libre_total = 0;
    pthread_mutex_lock(&m_memoria);
    for (int i = 0; i < list_size(tabla_segmentos_global); i++) {
        t_segmento_memoria* seg = list_get(tabla_segmentos_global, i);
        if (seg->ocupado == 0) {
            espacio_libre_total += seg->tamanio;
        }
    }
    pthread_mutex_unlock(&m_memoria);

    send(fd_kernel, &espacio_libre_total, sizeof(int), 0);
}

// lectura de memoria
void atender_lectura_memoria(int fd_modulo) {
    uint32_t dir_fisica;
    uint32_t tamanio;
 
    recv(fd_modulo, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
    recv(fd_modulo, &tamanio, sizeof(uint32_t), MSG_WAITALLA);
    usleep(memoria_config.instruction_delay *  1000);
    void* buffer = malloc(tamanio);
    pthread_mutex_lock(&m_memoria);
    memcpy(buffer, espacio_memoria_real + dir_fisica, tamanio);
    pthread_mutex_unlock(&m_memoria);
    send(fd_modulo, buffer, tamanio, 0 );
    log_info(logger,"## Lectura - Dir. Fisica: %d - Tamaño: %d", dir_fisica, tamanio);
}
    
    // Escritura de memoria REAL
    void atender_escritura_memoria(int fd_modulo) {
    uint32_t dir_fisica;
    uint32_t tamanio;
    
    recv(fd_modulo, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
    recv(fd_modulo, &tamanio, sizeof(uint32_t), MSG_WAITALL);
    void* datos = malloc(tamanio);
    recv(fd_modulo, datos, tamanio, MSG_WAITALL);

    usleep(memoria_config.instruction_delay * 1000);
    
    pthread_mutex_lock(&m_memoria);
    memcpy(espacio_memoria_real + dir_fisica, datos, tamanio);
    pthread_mutex_unlock(&m_memoria);

    log_info(logger, "## Escritura - Dir. Física: %d - Tamaño: %d", dir_fisica, tamanio);
    
    int confirmacion_ok = 1;
    send(fd_modulo, &confirmacion_ok, sizeof(int), 0);
    free(datos);
}
