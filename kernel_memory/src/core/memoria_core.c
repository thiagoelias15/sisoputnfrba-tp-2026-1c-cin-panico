#include "memoria_core.h"
#include "memoria_administrador/memoria_administrador.h"
#include <stdio.h>
#include <commons/collections/dictionary.h>
#include "../main.h"


// El Kernel avisa que se crea un proceso y nos pasa su archivo real
void atender_creacion_proceso(int fd_kernel) {
    int pid;
    uint32_t tam_nombre;
    
    recv(fd_kernel, &pid, sizeof(int), MSG_WAITALL);
    recv(fd_kernel, &tam_nombre, sizeof(uint32_t), MSG_WAITALL);
    
    char* nombre_archivo = malloc(tam_nombre);
    recv(fd_kernel, nombre_archivo, tam_nombre, MSG_WAITALL);

    // Convertimos el PID a string para usarlo como clave en el diccionario de las commons
    char clave_pid[10];
    sprintf(clave_pid, "%d", pid);
    
    // Guardamos el nombre real del archivo asociado a este PID
    dictionary_put(mapeo_archivos_procesos, clave_pid, nombre_archivo);
    
    log_info(logger, "## PID: %d - Proceso creado con archivo: %s", pid, nombre_archivo);
    
    int ok = 1;
    send(fd_kernel, &ok, sizeof(int), 0);
}

// Función interna: lee una línea específica usando el nombre dinámico guardado
char* leer_instruccion_de_archivo(int pid, uint32_t pc) {
    char clave_pid[10];
    sprintf(clave_pid, "%d", pid);
    
    // Buscamos el nombre real del archivo que nos mandó el Kernel para este PID
    char* nombre_archivo = dictionary_get(mapeo_archivos_procesos, clave_pid);
    if (nombre_archivo == NULL) {
        log_error(logger, "No se encontró un archivo registrado para el PID: %d", pid);
        return strdup("EXIT");
    }

    char path_completo[256];
    sprintf(path_completo, "%s/%s", memoria_config.scripts_basepath, nombre_archivo);
    
    FILE* archivo = fopen(path_completo, "r");
    if (archivo == NULL) {
        log_error(logger, "Error al abrir el archivo en la ruta: %s", path_completo);
        return strdup("EXIT");
    }

    char* linea = malloc(128);
    int linea_actual = 0;

    while (fgets(linea, 128, archivo) != NULL) {
        if (linea_actual == pc) {
            linea[strcspn(linea, "\n")] = 0; // Quitamos el salto de línea
            fclose(archivo);
            return linea;
        }
        linea_actual++;
    }

    fclose(archivo);
    free(linea);
    return strdup("EXIT"); 
}

// Devolver lista de instrucciones (FETCH)
void atender_fetch_cpu(int fd_cpu) {
    int pid;
    uint32_t pc;
    
    recv(fd_cpu, &pid, sizeof(int), MSG_WAITALL);
    recv(fd_cpu, &pc, sizeof(uint32_t), MSG_WAITALL);

    log_info(logger, "## PID: %d - Obtener instrucción: %d", pid, pc);
    usleep(memoria_config.instruction_delay * 1000); 

    char* instruccion = leer_instruccion_de_archivo(pid, pc);
    int tamaño_instruccion = strlen(instruccion) + 1;

    // Solo enviamos lo que la CPU espera recibir: tamaño + mensaje
    send(fd_cpu, &tamaño_instruccion, sizeof(int), 0);
    send(fd_cpu, instruccion, tamaño_instruccion, 0);
    
   free(instruccion);
}

// Consulta el espacio real gestionado por memoria_administrador
void atender_consulta_espacio(int fd_kernel) {
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

// Lectura de memoria REAL
void atender_lectura_memoria(int fd_modulo) {
    uint32_t dir_fisica;
    uint32_t tamanio;
    
    recv(fd_modulo, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
    recv(fd_modulo, &tamanio, sizeof(uint32_t), MSG_WAITALL);
    
    usleep(memoria_config.instruction_delay * 1000);
    
    void* buffer = malloc(tamanio);
    leer_de_sticks(dir_fisica, buffer, tamanio);
    
    send(fd_modulo, buffer, tamanio, 0);
    log_info(logger, "## Lectura - Dir. Física: %d - Tamaño: %d", dir_fisica, tamanio);
    free(buffer);
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
    
    escribir_en_sticks(dir_fisica, datos, tamanio);

    log_info(logger, "## Escritura - Dir. Física: %d - Tamaño: %d", dir_fisica, tamanio);
    
    int confirmacion_ok = 1;
    send(fd_modulo, &confirmacion_ok, sizeof(int), 0);
    free(datos);
}