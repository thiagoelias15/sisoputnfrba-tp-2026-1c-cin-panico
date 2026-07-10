#include "memoria_administrador.h"
#include "../main.h"

t_list* tabla_segmentos_global;
pthread_mutex_t m_memoria;
void* espacio_memoria_real;
t_dictionary* mapeo_archivos_procesos;
t_list* lista_sticks;
uint32_t memoria_total = 0;
pthread_mutex_t m_sticks;
t_list* lista_cpus_conectadas;
pthread_mutex_t m_cpus;

void inicializar_memoria() {
    tabla_segmentos_global = list_create();
    pthread_mutex_init(&m_memoria, NULL);
    mapeo_archivos_procesos = dictionary_create();
    lista_sticks = list_create();
    pthread_mutex_init(&m_sticks,NULL);
    lista_cpus_conectadas = list_create();
    pthread_mutex_init(&m_cpus,NULL);
    log_info(logger, "## Memoria inicializada. Esperando Memory Sticks");
}



    // funcion best fit: recorre la tabla global y elige el segmento libre mas pequeño que entra el nuevo proceso

t_segmento_memoria* buscar_hueco_best_fit(uint32_t tamanio_necesario){
    t_segmento_memoria* mejor_hueco = NULL;

    for(int i = 0; i < list_size(tabla_segmentos_global); i++) {
        t_segmento_memoria* seg = list_get(tabla_segmentos_global, i);
        if(seg -> ocupado == 0 && seg-> tamanio >= tamanio_necesario) {
            if(mejor_hueco == NULL || seg-> tamanio < mejor_hueco -> tamanio) {
                mejor_hueco = seg;
            }
        }
    }
    return mejor_hueco;
}

// asignar memoria: llama a best fit y "parte" el hueco encontrado, 1. encuenta el hueco,2.si es mas grande que el pedido, crea un nuevo segmento de "resto"
// 3. actualiza los punteros y marcas de ocupado

int asignar_memoria(int pid, uint32_t tamanio) {
    pthread_mutex_lock(&m_memoria); // bloqueamos el acceso para que nadie mas la toque la tabla
    t_segmento_memoria* hueco = buscar_hueco_best_fit(tamanio);

    if(hueco == NULL) {
        pthread_mutex_unlock(&m_memoria);
        return -1; // no hay espacio
    }

    // si sobra espacio, creamos un nuevo segmento con el "resto"
    if(hueco -> tamanio > tamanio) {
        t_segmento_memoria* resto = malloc(sizeof(t_segmento_memoria));
        resto -> id = list_size(tabla_segmentos_global); // ID nuevo
        resto -> pid = -1;
        resto -> base = hueco -> base + tamanio;
        resto-> tamanio = hueco-> tamanio - tamanio;
        resto-> ocupado = 0; // Libre

        list_add(tabla_segmentos_global, resto);
    }
    // actualizamos el hueco original para que sea el segmento del proceso
    hueco-> ocupado = 1;
    hueco -> pid = pid;
    hueco-> tamanio = tamanio;

    pthread_mutex_unlock(&m_memoria);
    return hueco -> id; // retornamos el ID del segmento asignado
}

// funcion liberar memoria, busca el segmento por PID y ID, lo marca como libre y une huecos adyacentes

void liberar_memoria(int pid, int id_segmento){
    
    pthread_mutex_lock(&m_memoria);

    for(int i = 0; i < list_size(tabla_segmentos_global); i++){
        t_segmento_memoria* seg = list_get(tabla_segmentos_global, i);
        if(seg -> pid == pid && seg -> id == id_segmento){
            seg -> ocupado = 0;
            seg -> pid =-1;
            log_info(logger, "## Segmento %d liberado (PID: %d)", id_segmento, pid);
            break;
        }
    }
    pthread_mutex_unlock(&m_memoria);
}

void compactar_memoria() {
    log_info(logger, "## Iniciando proceso de compactacion");
    usleep(memoria_config.compaction_delay * 1000); 
    pthread_mutex_lock(&m_memoria);

    uint32_t direccion_actual = 0;
//recorremos la tabla buscando los ocupados para moverlos

    for(int i = 0; i < list_size(tabla_segmentos_global); i++){
        t_segmento_memoria* seg = list_get(tabla_segmentos_global,i);

        if(seg -> ocupado == 1){
            // si la base es distinta a la actual, hay que mover los datos
            if(seg -> base != direccion_actual){
          //leer datos del stick viejo y escribirlos en la posicion nueva
          void* buffer = malloc(seg->tamanio);
          leer_de_sticks(seg->base, buffer, seg->tamanio);
          escribir_en_sticks(direccion_actual, buffer, seg->tamanio);
                free(buffer);
                log_info(logger, "## Segmento %d movido de %d a %d", seg->id, seg->base, direccion_actual);
                seg -> base = direccion_actual;
            }
            direccion_actual += seg -> tamanio;
        }
    }
//eliminar huecos libers
    for(int i = list_size(tabla_segmentos_global) - 1; i >= 0; i--){
        t_segmento_memoria* seg = list_get(tabla_segmentos_global, i);
        if(seg -> ocupado == 0){
            list_remove_and_destroy_element(tabla_segmentos_global, i, free);
        }
    }
// creamos el nuevo hueco libre
    t_segmento_memoria* gran_hueco = malloc(sizeof(t_segmento_memoria));
    gran_hueco -> id = list_size(tabla_segmentos_global);
    gran_hueco -> pid = -1;
    gran_hueco -> base = direccion_actual;
    gran_hueco -> tamanio = memoria_total - direccion_actual;
    gran_hueco -> ocupado = 0;
    list_add(tabla_segmentos_global, gran_hueco);
    log_info(logger, "## Compactacion finalizada."):
    pthread_mutex_unlock(&m_memoria);
}

//funcion que busca a que stick pertenece una direccion global y le pidea que lea
    void leer_de_sticks(uint32_t dir_global, void* buffer, uint32_t tamanio){
        pthread_mutex_lock(&m_sticks);

        uint32_t bytes_leidos = 0;
        while(bytes_leidos < tamanio){
            //buscar que stick tiene la direccion actual
            uint32_t dir_actual = dir_global + bytes_leidos;
            t_memory_stick_info* stick = NULL;
            for(int i = 0; i < list_size(lista_sticks); i++){
                t_memory_stick_info* s = list_get(lista_sticks, i);
                if(dir_actual >= s->base_global && dir_actual < s->base_global + s->tamanio){
                    stick = s;
                    break;
                }
            }
        uint32_t dir_local = dir_actual - stick->base_global;
        uint32_t espacio_en_stick = stick->tamanio - dir_local;
        uint32_t bytes_a_leer = tamanio - bytes_leidos;
        if(bytes_a_leer > espacio_en_stick) bytes_a_leer = espacio_en_stick;
            op_code op = LEER_MEMORIA;
            send(stick->fd_socket,&op, sizeof(op_code),0);
            int dir_l = (int)dir_local;
            int tam_l = (int)bytes_a_leer;
            send(stick->fd_socket, &dir_l, sizeof(int), 0);
            send(stick->fd_socket, &tam_l, sizeof(int), 0);
            recv(stick->fd_socket, buffer + bytes_leidos, bytes_a_leer, MSG_WAITALL);
            bytes_leidos += bytes_a_leer;
    }
    pthread_mutex_unlock(&m_sticks);    
}

// busca a que stick pertenece una direccion global y le pide que excriba
    void escribir_en_sticks(uint32_t dir_global, void* buffer, uint32_t tamanio){
        pthread_mutex_lock(&m_sticks);
        uint32_t bytes_escritos = 0;
    while(bytes_escritos < tamanio) {
        uint32_t dir_actual = dir_global + bytes_escritos;
        t_memory_stick_info* stick = NULL;
        for(int i = 0; i < list_size(lista_sticks); i++) {
            t_memory_stick_info* s = list_get(lista_sticks, i);
            if(dir_actual >= s->base_global && dir_actual < s->base_global + s->tamanio) {
                stick = s;
                break;
            }
        }

        uint32_t dir_local = dir_actual - stick->base_global;
        uint32_t espacio_en_stick = stick->tamanio - dir_local;
        uint32_t bytes_a_escribir = tamanio - bytes_escritos;
        if(bytes_a_escribir > espacio_en_stick) bytes_a_escribir = espacio_en_stick;

        op_code op = ESCRIBIR_MEMORIA;
        send(stick->fd_socket, &op, sizeof(op_code), 0);
        int dir_l = (int)dir_local;
        int tam_l = (int)bytes_a_escribir;
        send(stick->fd_socket, &dir_l, sizeof(int), 0);
        send(stick->fd_socket, &tam_l, sizeof(int), 0);
        send(stick->fd_socket, datos + bytes_escritos, bytes_a_escribir, 0);

        // Esperar confirmación (el stick manda enviar_mensaje "OK")
        recibir_operacion(stick->fd_socket);  // op_code del MENSAJE
        char* ok = recibir_mensaje(stick->fd_socket);
        free(ok);

        bytes_escritos += bytes_a_escribir;
    }

    pthread_mutex_unlock(&m_sticks);
}
    void liberar_todos_segmentos_pid(int pid){
    pthread_mutex_lock(&m_memoria);


    for(int i = 0; i < list_size(tabla_segmentos_global); i++){
       t_segmento_memoria* seg = list_get(tabla_segmentos_global, i);
        if (seg -> pid == pid) {
            seg -> ocupado = 0;
            seg -> pid = -1;
            log_info(logger, "## PID: %d - Segmento %d liberado por finalizacion", pid, seg->id);
        }
    }

    pthread_mutex_unlock(&m_memoria);
}