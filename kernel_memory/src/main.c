#include "main.h"
#include "memoria_administrador.h"
#include "conexiones/conexiones.h" 

t_log* logger;

int main(int argc, char* argv[]) { 
    
    // Verificación de argumentos
    if(argc < 2) {
        printf("[ERROR] Uso: ./bin/kernel_memory [config_path]\n");
        return EXIT_FAILURE;
    }
    
    // Inicialización: Cargamos configuración y creamos el logger 
    cargar_configuracion_memoria(argv[1]);
    logger = log_create("memoria.log", "MEMORIA", 1, LOG_LEVEL_INFO);
    
    log_info(logger, "Iniciando Kernel Memory...");

    // Inicializamos nuestra administración de memoria (la lista de segmentos)
    inicializar_memoria();
    
    // Iniciar servidor
    int fd_servidor = iniciar_servidor(memoria_config.puerto);
    if(fd_servidor == -1) {
        log_error(logger, "Fallo al iniciar el servidor.");
        return EXIT_FAILURE;
    }

    log_info(logger, "Memoria escuchando en puerto %s", memoria_config.puerto);
    
    // Bucle principal para atender clientes concurrentemente
    while(1) {
        int fd_cliente = esperar_cliente(fd_servidor);
        
        if(fd_cliente != -1) {
            int* fd_hilo = malloc(sizeof(int));
            *fd_hilo = fd_cliente;

            pthread_t hilo_cliente;
            
            pthread_create(&hilo_cliente, NULL, atender_cliente, (void*)fd_hilo);
            pthread_detach(hilo_cliente); 
        }
    }

    // Limpieza final 
    destruir_configuracion_memoria();
    log_destroy(logger);
    
    return 0; 
}