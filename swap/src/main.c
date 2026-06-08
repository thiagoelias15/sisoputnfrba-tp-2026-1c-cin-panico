#include "main.h"
#include "utils/instrucciones/instrucciones.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>

t_log* logger;
FILE* archivo_swap; // puntero al archivo fisico en el disco
int swap_corriendo = 1;


int main(int argc, char* argv[]) {
    
    if(argc<2) { // ./bin/swap swap.config
       printf("[ERROR] Debe utilizar el formato: ./bin/swap [Archivo Config]\n");
        return EXIT_FAILURE;
    }

    cargar_configuracion_swap(argv[1]);

    // Se inicia el Logger

    logger = log_create("swap.log","SWAP",1,LOG_LEVEL_INFO);
   
    // abrimos el archivo en modo rb+ (lectura/escritura)
    // rb+ nos permite modificar bloques sin borrar el resto del archivo
    archivo_swap = fopen(swap_config.swap_file_path, "rb+");
    
    if(archivo_swap == NULL){
        log_error(logger, "Error al abrir el archivo de Swap en: %s", swap_config.swap_file_path);
        return EXIT_FAILURE;
    }
    // servidor: Swap espera conexiones de Kernel Memory
    int fd_escucha = iniciar_servidor(swap_config.puerto_memoria);
    int socket_memoria = esperar_cliente(fd_escucha);

    send(socket_memoria, &swap_config.block_size, sizeof(int), 0);
    send(socket_memoria, &swap_config.swap_file_size, sizeof(int), 0);
    
    log_info(logger, "## Conectado a Kernel Memory");

    // Bucle infinito para antender pedido de bloques
    
    while(swap_corriendo) {

        op_code op = recibir_operacion(socket_memoria);

        if(op == -1) {
            break;
        } 
        
        int num_bloque;
        //recibimos que bloque quiere leer/escribir el Kernel Memory
        recv(socket_memoria, &num_bloque, sizeof(int), MSG_WAITALL);

        if(op == SWAP_LECTURA) {

            void* buffer = malloc(swap_config.block_size);
            // fseek: movemos el cursor del archivo a la posicion exacta del bloque
            //(Nro bloque * tamaño del bloque)
            fseek(archivo_swap, num_bloque * swap_config.block_size, SEEK_SET );
            fread(buffer, swap_config.block_size, 1, archivo_swap);
            // enviamos los bytes leidos de vuelta al Kernel Memory
            send(socket_memoria, buffer, swap_config.block_size, 0);
            log_info(logger, "## Lectura del bloque: %d", num_bloque);
            free(buffer);

        } else if(op == SWAP_ESCRITURA) {
            
            //recibimos los datos que el kernel memory quiere guardar
            void* buffer = recibir_mensaje(socket_memoria);
            //fseek: ubicamos el cursor, fwrite guarda los bytes en el archivo
            fseek(archivo_swap, num_bloque * swap_config.block_size, SEEK_SET);
            fwrite(buffer, swap_config.block_size, 1, archivo_swap); 
            //confirmamos que la escritura fue exitosa
            enviar_mensaje("OK", MENSAJE, socket_memoria);
            log_info(logger, "## Escritura del bloque: %d", num_bloque);
            free(buffer);
        }
    }

    // ------------------------------ LIMPIEZA DE LA MEMORIA ------------------------------ //
    
    fclose(archivo_swap);
    destruir_configuracion_swap();
    log_destroy(logger);
    return 0;
}

