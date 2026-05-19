#include "main.h"
// Definición de variables globales(en el main.h con extern solo le avisabamos al compiladro que existian pero no estaban definidas)
t_list* cola_new;
t_list* cola_ready;
t_list* cola_exec;
t_list* cola_block;
t_list* cola_exit;
pthread_mutex_t m_ready, m_new, m_block, m_exit;
sem_t sem_procesos_ready;
t_dictionary* dic_mutex;
t_dictionary* dic_interfaces;
int fd_cpu = -1;
int fd_memoria = -1;
t_log* logger;
int scheduler_corriendo = 1;

int main(int argc,char* argv[]) {

    // Se verifica la información en el config

    if(argc<2) {  

        printf("[ERROR] Mal ejecutado");
        return EXIT_FAILURE; 
    }

    // Se carga el config, extrayendo la información necesaria para la conexión de módulos.

    cargar_configuracion_kernel(argv[1]);
    logger = log_create("Scheduler.log", "SCHEDULER", 1, LOG_LEVEL_INFO);
    log_info(logger, "Iniciando Scheduler con algoritmo: %s", kernel_config.algoritmo_planificacion);

    // 2. INICIALIZAMOS ESTRUCTURAS

    cola_new = list_create();
    cola_ready = list_create();
    cola_exec = list_create();
    cola_block = list_create();
    cola_exit = list_create();
    dic_mutex = dictionary_create();
    dic_interfaces = dictionary_create();

    pthread_mutex_init(&m_ready, NULL);
    pthread_mutex_init(&m_new, NULL);
    pthread_mutex_init(&m_block, NULL);
    pthread_mutex_init(&m_exit, NULL);
    sem_init(&sem_procesos_ready, 0, 0);

    // ------------------------------ SCHEDULER COMO CLIENTE ------------------------------ //

    // 3. CONEXIÓN A MEMORIA (Usando nuestra esctuctura config)el . indica que ingresamos o tomamos dicho campo de un determinado struct
    fd_memoria = crear_conexion(kernel_config.ip_memoria, kernel_config.puerto_memoria);
    
    if(fd_memoria != -1) {
        enviar_mensaje(kernel_config.id_modulo, MENSAJE, fd_memoria);
        log_info(logger, "Scheduler conectado a la memoria con ID: %s", kernel_config.id_modulo);
    } else {
        log_error(logger, "Fallo en la conexión a la memoria");
        return EXIT_FAILURE; 
    }

    //------------------------ Planificacion a largo plazo(proceso 0 o inical) ---------------------------------------

    //el proceso 0 o inicial seria PID 0
    t_pcb* pcb_inicial = malloc(sizeof(t_pcb));
    pcb_inicial -> pid = 0;
    pcb_inicial -> pc = 0;

    //inicializamos los registros en 0
    pcb_inicial->ax = 0; pcb_inicial->bx = 0; pcb_inicial->cx = 0; pcb_inicial->dx = 0;
    pcb_inicial->eax = 0; pcb_inicial->ebx = 0; pcb_inicial->ecx = 0; pcb_inicial->edx = 0;
    pcb_inicial->si = 0; pcb_inicial->di = 0;

    log_info(logger, "## (%d) Se crea el proceso - Estado: NEW",pcb_inicial->pid);
    //entrada NEW
    pthread_mutex_lock(&m_new);
    list_add(cola_new, pcb_inicial);
    pthread_mutex_unlock(&m_new);

    //pasaje de NEW a READY
    pthread_mutex_lock(&m_new);
    t_pcb* pcb_a_ready = list_remove(cola_new, 0);
    pthread_mutex_unlock(&m_new);

    log_info(logger, "## (%d) Pasa del estado NEW a READY", pcb_a_ready->pid); //%d imprime numeros enteros, en este caso va a buscar el el valor numerico que este guardado en pcb_a_ready->pid

    pthread_mutex_lock(&m_ready);
    list_add(cola_ready, pcb_a_ready);
    pthread_mutex_unlock(&m_ready);
    //activamos el sem para que el planificado de corto plazo lo mande  a la cpu
    sem_post(&sem_procesos_ready);

    // --------------------------Planificador a corto plazo-------------------------------------//

    pthread_t hilo_planificador; //declaramos variable de tipo pthread_t llamada hilo_planificador
    pthread_create(&hilo_planificador, NULL, planificador_corto_plazo, NULL); /*creamos un hilo nuevo con estos 4 parametros
    &hilo_planificador es para guardar el ID del hilo creado,NULL es para que no haga nada especial solo configuracion estandar,planificador_corto_plazo es la funcion que va a ejecutar el hilo */
    pthread_detach(hilo_planificador); //al crear un hilo linux deja vinculados el hilo nuevo al hilo main con detach desvinculamos los hilos para que el nuevo trabaje de forma independiente

    // ------------------------------ SCHEDULER COMO SERVIDOR ------------------------------ //

    int fd_escucha = iniciar_servidor(kernel_config.puerto_escucha);
    log_info(logger, "Servidor del scheduler encendido. Escuchando CPUs e IOs");

    while(scheduler_corriendo) { //al poner while(scheduler_corrieendo) es un TRUE osea siempre verdadero lo que permite que el while sea infinito
        
        int* socket_cliente = malloc(sizeof(int));
        *socket_cliente = esperar_cliente(fd_escucha);
        pthread_t hilo_cliente;
        pthread_create(&hilo_cliente, NULL, atender_cliente, socket_cliente);
        pthread_detach(hilo_cliente);
    }

    destruir_configuracion_kernel();
    return 0;
} 

    //----------------------------------Planificacion de corto plazo--------------------------/

void* planificador_corto_plazo(void* arg) {

    while(scheduler_corriendo) {

        sem_wait(&sem_procesos_ready);
        pthread_mutex_lock(&m_ready);
        t_pcb* pcb_a_ejecutar = list_remove(cola_ready, 0);
        pthread_mutex_unlock(&m_ready);
        
        log_info(logger, "## (%d) Pasa del estado READY al estado EXEC",pcb_a_ejecutar->pid);

        //manda el pcb al cpu a ejecutar
        enviar_pcb(pcb_a_ejecutar, fd_cpu, CONTEXTO_PCB);
        
        if(strcmp(kernel_config.algoritmo_planificacion, "RR")== 0) {
            
            pthread_t hilo_quantum;
            pthread_create(&hilo_quantum,NULL,temporizador_quantum,pcb_a_ejecutar);
            pthread_detach(hilo_quantum);
        }
    }
    return NULL;
}

void* temporizador_quantum(void* arg) {
    
    t_pcb* pcb = (t_pcb*)arg; //aca le decimos al compilador que trate a ese arg como un puntero a un pcb para leer el PID
    usleep(kernel_config.quantum_rr * 1000); //la funcion usleep espera una x cantidad de microsegundos y por mil para pasar esos microsegundos a milisegundos
    log_info(logger,"## (%d) Desalojo de quantum",pcb->pid);
    enviar_mensaje("INTERRUPCION_RR",INTERRUPCION,fd_cpu); //aca el scheduler le pide a la cpu que frene la ejecucion del procesos y se lo devuelva
    return NULL;
}


void* atender_cliente(void* arg) {
    
    int socket_cliente = *(int*)arg; // arg es un puntero que apunta a la direccion de memoria donde el main guardo el numero de socket y (int*)*arg le dice al compilador que trate a ese puntero como un entero
    free(arg);

    recibir_operacion(socket_cliente);
    char* id_recibida = recibir_mensaje(socket_cliente);

    if(strcmp(id_recibida, "CPU")== 0) {
        
        fd_cpu = socket_cliente;
        log_info(logger,"## CPU <ID CPU> conectada");
        
        while (scheduler_corriendo) {
            
            op_code cod_op = recibir_operacion(socket_cliente);
            if(cod_op == -1)break;
            t_pcb* pcb_upd = recibir_pcb(fd_cpu);

            switch (cod_op) {

                case SYSCALL_EXIT: {

                    log_info(logger, "## (%d) Solicito syscall: EXIT",pcb_upd->pid);
                    log_info(logger, "## (%d) Pasa del estado EXEC al estado EXIT", pcb_upd->pid);
                    log_info(logger, "## (%d) finalizó su ejecución", pcb_upd->pid);
                    list_add(cola_exit, pcb_upd);
                    sem_post(&sem_procesos_ready); // La CPU queda libre
                    break;
                }

                case SYSCALL_SLEEP: {

                    int tiempo_ms;
                    recv(fd_cpu,&tiempo_ms,sizeof(int),MSG_WAITALL);
                    log_info(logger, "## (%d) Solicito syscall: SLEEP", pcb_upd->pid);
                    log_info(logger, "## (%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid);
                    pthread_mutex_lock(&m_block);
                    list_add(cola_block, pcb_upd);
                    pthread_mutex_unlock(&m_block);

                    if(dictionary_has_key(dic_interfaces, "SLEEP")) {
                        
                        // Buscamos el enchufe específico del SLEEP
                        int socket_sleep = (int)(intptr_t)dictionary_get(dic_interfaces, "SLEEP");
                        enviar_mensaje("SLEEP", SYSCALL_SLEEP, socket_sleep);
                        send(socket_sleep, &tiempo_ms, sizeof(int), 0);
                        send(socket_sleep, &(pcb_upd->pid), sizeof(int), 0);
                    } else {
                        log_error(logger, "La interfaz SLEEP no está conectada.");
                    }
                    sem_post(&sem_procesos_ready);
                    break;
                }

                case SYSCALL_MUTEX_CREATE: { //mutex create crea una cola con un nombre determinado y guarda en dictionary todas esas colas que va creando para no repetir
                
                    char* m_name = recibir_mensaje(fd_cpu); //recibe el nombre que mande la cpu
                    if(!dictionary_has_key(dic_mutex, m_name)) { //se fija si ya existe ese nombre
                        dictionary_put(dic_mutex, m_name, queue_create()); //si es nuevo lo guarda y crea una cola de espera vacia
                    }
                    enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB); 
                    free(m_name);
                    break;
                }

                case SYSCALL_MUTEX_LOCK: { 

                    char* m_name = recibir_mensaje(fd_cpu);
                    t_queue* q = dictionary_get(dic_mutex, m_name);
                
                    if(queue_is_empty(q)) { //si la cola esta vacia el proceso se mete primero y se lo devuelve a la cpu para que siga corriendo, sino esta vacio el prcoceso pierde su turno actual de cpu
                    
                        log_info(logger, "## (%d) Toma el mutex %s",pcb_upd->pid, m_name);
                        queue_push(q, pcb_upd);
                        enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB); 
                    } else {
                    
                        log_info(logger, "## (%d) Pasa del estado de EXEC al estado BLOCK", pcb_upd->pid);
                        pthread_mutex_lock(&m_block);
                        list_add(cola_block,pcb_upd); //movemos el proceso a la cola block porque tiene que esperar
                        pthread_mutex_unlock(&m_block);
                        queue_push(q, pcb_upd); //pone el proceso en el final de la cola del mutex 
                        sem_post(&sem_procesos_ready); //como la CPU ahora esta libre le avisa al scheduler que mande otro proceso a ejecutar
                    }

                    free(m_name);
                    break;
                }

                case SYSCALL_MUTEX_UNLOCK: {
                    
                    char* m_name = recibir_mensaje(fd_cpu);
                    log_info(logger,"## (%d) Libera el mutex %s",pcb_upd->pid,m_name);
                    t_queue* q= dictionary_get(dic_mutex,m_name);
                    queue_pop(q); // Saca al proceso actual de la cabeza de la cola del mutex

                    if(!queue_is_empty(q)) { //se fija si hay otros procesos detras, si hay mira quien esta con queue_peek
                        
                        t_pcb* proximo = queue_peek(q); // busca o se fija el  proceso siguiente en la fila 
                        mover_a_ready(proximo->pid); //busca ese siguiente proceso en la colaa de BLOCK y lo pasa a READY
                    }
                    
                    enviar_pcb(pcb_upd,fd_cpu, CONTEXTO_PCB); //el proceso que solto el mutex vuelve a CPU para ejectuar la instruccion que sigue
                    free(m_name);
                    break;
                }

                case SYSCALL_STDIN: {

                    int tam, dir; // Recibimos de la CPU los parámetros necesarios: tamaño y dirección física
                    // Recibimos de forma bloqueante el tamaño del buffer que STDIN debe leer
                    // MSG_WAITALL asegura que no continúe hasta recibir los 4 bytes del int
                    recv(fd_cpu, &tam, sizeof(int), MSG_WAITALL);
                    // Recibimos la dirección física de memoria donde se debe escribir lo ingresado
                    // Esta información la envía la CPU tras traducir la dirección lógica
                    recv(fd_cpu, &dir, sizeof(int), MSG_WAITALL);
                    log_info(logger, "##(%d) Solicitó syscall: STDIN", pcb_upd->pid);
                    log_info(logger, "##(%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid);
                    pthread_mutex_lock(&m_block); // Bloquea acceso a cola de bloqueados
                    list_add(cola_block, pcb_upd); // Agrega el proceso a la cola BLOCK
                    pthread_mutex_unlock(&m_block); // Libera el mutex de la cola
                
                    if(dictionary_has_key(dic_interfaces, "STDIN")) {

                        int socket_stdin = (int)(intptr_t)dictionary_get(dic_interfaces, "STDIN");
                        enviar_mensaje("STDIN", SYSCALL_STDIN, socket_stdin);
                        send(socket_stdin, &tam, sizeof(int), 0);
                        send(socket_stdin, &dir, sizeof(int), 0);
                        send(socket_stdin, &(pcb_upd->pid), sizeof(int), 0);
                    } else {
                        log_error(logger, "La interfaz STDIN no está conectada.");
                    }

                    sem_post(&sem_procesos_ready);
                    break;
                }

                case SYSCALL_STDOUT: {
                    
                    int tam, dir;
                    recv(fd_cpu, &tam, sizeof(int), MSG_WAITALL); // Recibe tamaño a mostrar desde CPU
                    recv(fd_cpu, &dir, sizeof(int), MSG_WAITALL); //Recibe dirección física de orígen
                    log_info(logger, "##(%d) Solicitó syscall: STDOUT", pcb_upd->pid);
                    log_info (logger,"##(%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid); //Como toda operación de I/O es lenta, el proceso no puede seguir en la CPU. Se lo mueve de EXEC a BLOCK.
                    pthread_mutex_lock(&m_block); // Protege la cola de bloqueados
                    list_add(cola_block, pcb_upd); // Mueve el PCB a estado bloqueado
                    pthread_mutex_unlock(&m_block); // Libera la protección de la cola
                    
                    /*Si hay una interfaz conectada , el Kernel le envía un mensaje avisando que hay una tarea de STDOUT. 
                    Le pasa el tamaño, la dirección y el PID del proceso para que la interfaz sepa a quién pertenece la operación*/
                    
                    if(dictionary_has_key(dic_interfaces, "STDOUT")) {

                        int socket_stdout = (int)(intptr_t)dictionary_get(dic_interfaces, "STDOUT"); // intptr_t es de una biblioteca stdin.h y lo que hace es una variable que se asegura que el dato tenga el mismo tamaño en bytes que el puntero para que no tire error
                        enviar_mensaje("STDOUT", SYSCALL_STDOUT, socket_stdout);
                        send(socket_stdout, &tam, sizeof(int), 0);
                        send(socket_stdout, &dir, sizeof(int), 0);
                        send(socket_stdout, &(pcb_upd->pid), sizeof(int), 0);
                    } else {
                        log_error(logger, "La interfaz STDOUT no está conectada.");
                    }
                    
                    sem_post(&sem_procesos_ready);
                    break;
                }
                
                default: break; // si no es ninguno de los anteriores casos sale del switch y sigue con el codigo que esta abajo

            }
        }

    } else {

        //si no es la CPU es una de las IOs
        log_info(logger,"## Interfaz de IO %s conectada", id_recibida); 
        
        //la anotamos en el diccionario usando su nombre como Clave y su socket como valor
        dictionary_put(dic_interfaces, id_recibida, (void*)(intptr_t)socket_cliente); 

        while(scheduler_corriendo) {

            //Usamos socket_cliente y no fd_io como antes 
            op_code cod_op = recibir_operacion(socket_cliente);
            
            if(cod_op == -1) {
                
                log_warning(logger, "Se desconecto la IO %s", id_recibida);
                dictionary_remove(dic_interfaces, id_recibida); //como hubo error borramos esa IO del dicccionario
                break;
            }

            if(cod_op == MENSAJE) {
                
                char* resp = recibir_mensaje(socket_cliente);
                
                if(strcmp(resp, "FIN_IO")== 0) {
                    
                    int pid_fin;
                    recv(socket_cliente, &pid_fin, sizeof(int), MSG_WAITALL);
                    log_info(logger, "## (%d) finalizo IO y paso a READY",pid_fin);
                    mover_a_ready(pid_fin);
                }

                free(resp);  
            }
        }
    }
    
    free(id_recibida);
    return NULL;
}

//--------------------------------- Auxiliares de planificacion---------------------------/

void mover_a_ready(int pid_buscado) { //La función entra a la "sala de espera" de procesos bloqueados (cola_block)
    t_pcb* pcb_a_mover = NULL;

    //bloqueamos la cola BLOCK para evitar "choques" o que entren nuevos hilos a la cola
    pthread_mutex_lock(&m_block);

    for(int i = 0; i< list_size(cola_block);i++) { // va a recorrer la lista buscando el PID que necesitamos
        
        t_pcb* p= list_get(cola_block,i); //obtenemos el PCB en la posicion i
        if(p->pid == pid_buscado) {

            //si coincide el PID, lo extraemos de la lista de bloqueados
            pcb_a_mover = list_remove(cola_block,i);
            break;
        }
    }

    pthread_mutex_unlock(&m_block);

    if(pcb_a_mover != NULL) { //si encontro el proceso lo mete en la cola de listos
        
        pcb_a_mover->estado = READY; //actualizamods el estado interno del PCB
        //metemos a la cola de READY
        pthread_mutex_lock(&m_ready);
        list_add(cola_ready, pcb_a_mover);
        pthread_mutex_unlock(&m_ready);
        sem_post(&sem_procesos_ready); //avisamos que hay un proceso nuevo para mandar al cpu
    }
}
