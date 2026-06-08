#include "memoria_administrador.h"
#include "../main.h"

t_list* tabla_segmentos_global;
pthread_mutex_t m_memoria;
void* espacio_memoria_real;

void inicializar_memoria() {
    tabla_segmentos_global = list_create();
    pthread_mutex_init(&m_memoria, NULL);
    // reservamos el bloque de memoria
    espacio_memoria_real = malloc(memoria_config.memoria_operando);

    //creamos el segmento inicial(toda la memoria libre)
    t_segmento_memoria* segmento_inicial = malloc(sizeof(t_segmento_memoria));
    segmento_inicial -> id = 0;
    segmento_inicial -> pid = -1; //-1 es libre
    segmento_inicial -> base = 0;
    segmento_inicial -> tamanio = memoria_config.memoria_operando;
    segmento_inicial -> ocupado = 0;

    list_add(tabla_segmentos_global, segmento_inicial);
    log_info(logger, "## Memoria inicializada. Segmento inicial creado (Base: 0, Tamaño: %d)", 
             memoria_config.memoria_operando);
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
        resto-> tamanio = hueco-> tamanio - tamnio;
        resto-> ocupado = 0; // Libre

        list_add(tabla_segmentos_global, resto);
    }
    // actualizamos el hueco original para que sea el segmento del proceso
    hueco-> ocupado = 1;
    hueco_pid = pid;
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
    pthread_mutex_lock(&m_memoria);

    uint32_t direccion_actual = 0;
//recorremos la tabla buscando los ocupados para moverlos

    for(int i = 0; i < list_size(tabla_segmentos_global); i++){
        t_segmento_memoria* seg = list_get(tabla_segmentos_global,i);

        if(seg -> ocupado == 1){
            // si la base es distinta a la actual, hay que mover los datos
            if(seg -> base != direccion_actual){
            //movemos los datos en el espacio real de memoria
                memcpy(espacio_memoria_real + direccion_actual, espacio_memoria_real + seg -> base, seg -> tamanio);

        // actualizamos la direccion base en la estructura
                seg -> base =  direccion_actual;
            }
            direccion_actual += seg -> tamanio;
        }
    }
// limpiamos la tabla: eliminamos todos los huecos libres y creamos uno solo gigante al final
// primero, eliminamos los segmentos libres actuales de la lista
    for(int i = list_size(tabla_segmentos_global)-1; i>=0; i--){
        t_segmento_memori* seg = list_get(tabla_segmentos_global,i);
        if(seg-> ocupado == 0){
        list_remove_and_destroy_element(tabla_segmentos_global, i, free);
        }
    }

// creamos el nuevo gran hueco libre al final
    t_segmento_memoria* gran_hueco = malloc(sizeof(t_segmento_memoria));
    gran_hueco -> id = list_size(tabla_segmentos_global);
    gran_hueco -> pid = -1;
    gran_hueco -> base = direccion_actual;
    gran_hueco -> tamanio = memoria_config.memoria_operando - direccion_actual;
    gran_hueco -> ocupado = 0;

    list_add(tabla_segmentos_global, gran_hueco);
    log_info(logger,"## Compactacion finalizada. Memoria unifica al final");
    pthread_mutex_unlock(&m_memoria);

}

void liberar_todos_segmentos_pid(int pid){
    pthread_mutex_lock(&m_memoria);


    for(int i = 0; i < list_size(tabla_segmentos_global), i++){
       t_segmento_memoria* seg = list_get(tabla_segmentos_global, i);
        if (seg -> pid == pid) {
            seg -> ocupado = 0;
            seg -> pid = -1;
            log_info(logger, "## PID: %d - Segmento %d liberado por finalizacion", pid, seg->id);
        }
    }

    pthread_mutex_unlock(&m_memoria);
}