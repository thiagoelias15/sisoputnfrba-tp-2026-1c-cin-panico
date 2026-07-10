#include "main.h"
#include "conexiones/conexiones.h" // esto para atender a la cpu

void* espacio_memoria = NULL;
t_log* logger;
int tamanio_memoria;
int ms_corriendo = 1;

int main(int argc, char* argv[]) {

   if(argc < 3) { // (El enunciado ahora pide Config + Tamaño)
        printf("[ERROR] Uso: ./bin/memory_stick [Archivo Config] [Tamaño]\n");
        return EXIT_FAILURE;
    }
    
    //leemos el tamaño pasado por parametro en la terminal
    tamanio_memoria = atoi(argv[2]);
    
    // Se inicializan las herramientas de las commons y se extraen los parámetros correspondientes del config
    cargar_configuracion_ms(argv[1]);
    
    // Se inicia el Logger
    logger = log_create("ms.log","MEMORY_STICK",1,LOG_LEVEL_INFO);
    log_info(logger, "Iniciando modulo Memory Stick");

    //reservar espacio contiguo en memoria
    espacio_memoria = calloc(1, tamanio_memoria); // calloc es una funcion hermana de malloc que reserva memoria y la limpia poniendola todo en ceros al mismo tiempo
    if(espacio_memoria == NULL){
        log_error(logger, "No hay memoria suficiente en el sistema para iniciar la Stick");
        destruir_configuracion_ms();
        log_destroy(logger);
        return EXIT_FAILURE;
    }
    // ------------------------------ CONEXIONES ------------------------------ //

    int fd_memoria = crear_conexion(ms_config.ip_memoria, ms_config.puerto_memoria);
    
    if(fd_memoria != -1) {
        enviar_mensaje(ms_config.id_modulo, MENSAJE, fd_memoria);
        send(fd_memoria, &tamanio_memoria, sizeof(int), 0);
        
        int len_ip = strlen(ms_config.ip_escucha) + 1;
        send(fd_memoria, &len_ip, sizeof(int), 0);
        send(fd_memoria, ms_config.ip_escucha, len_ip, 0);
        //le manda el puerto escucha a KM para que se la pase a CPU
        int len_puerto = strlen(ms_config.puerto_escucha) + 1;
        send(fd_memoria, &len_puerto, sizeof(int), 0);
        send(fd_memoria, ms_config.puerto_escucha, len_puerto, 0);
        log_info(logger, "Memory Stick conectado a Kernel Memory");
    } else {
        log_error(logger, "No se pudo conectar a memoria");
        free(espacio_memoria);
        destruir_configuracion_ms();
        log_destroy(logger);
        return EXIT_FAILURE;
    }
        // Servidor para las CPUs
        int fd_escucha = iniciar_servidor(ms_config.puerto_escucha);
        log_info(logger, "Memory Stick listo. Escuchando en puerto %s", ms_config.puerto_escucha);
        
           while(ms_corriendo) {
        int* socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(fd_escucha);
    
        if(*socket_cliente != -1) {
          pthread_t hilo;
          pthread_create(&hilo, NULL, atender_cpu, socket_cliente);
          pthread_detach(hilo);
        } else {
            free(socket_cliente);
        }
    }
    
    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //
    free(espacio_memoria);
    close(fd_memoria);
    destruir_configuracion_ms();
    log_destroy(logger);

    return 0;
}
