#include "main.h"
#include <stdio.h>
#include <stdlib.h>

t_log* logger;

int main (int argc, char*argv[]) {

    t_log* logger = log_create("memoria.log","MEMORIA",1,LOG_LEVEL_INFO);

    if(argc < 2) {
        printf("[ERROR] Mal ejecutado");
        return EXIT_FAILURE;
    }
    
    // Se carga el config para extraer la información necesaria
    cargar_configuracion_memoria(argv[1]);
    logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
    log_info(logger, "Iniciando Servidor de Memoria")
    
    // Iniciar servidor
    int fd_servidor = iniciar_servidor(memoria_config.puerto);
    if(fd_servidor == -1) {
        log_error(logger, "Fallo al iniciar el servidor de Memoria.");
        return EXIT_FAILURE;
    }

    log_info(logger, "Memoria lista y escuchando clientes en el puerto %s...", memoria_config.puerto);
    }
    
   // Bucle para aceptar clientes concurrentemente
  while(1) {
        int fd_cliente = esperar_cliente(fd_servidor);
        
        if(fd_cliente != -1) {
            // Reservamos memoria dinámica para pasarle el FD del socket al hilo
            int* fd_hilo = malloc(sizeof(int));
            *fd_hilo = fd_cliente;

            // Creamos el hilo para que atienda a este módulo sin frenar a la Memoria
            pthread_t hilo_cliente;
            pthread_create(&hilo_cliente, NULL, atender_cliente, (void*)fd_hilo);
            pthread_detach(hilo_cliente); // Hilo independiente, limpia su propia basura al terminar
        }
    }

    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //

  destruir_configuracion_memoria();
    log_destroy(logger);
    return 0;








