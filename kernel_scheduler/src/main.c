#include <utils/utils.h>


    //------------------------------ Variables globales(estados y sincronizacion)--------------------//
   t_list* cola_new;
   t_list* cola_ready;
   t_list* cola_exec;
   t_list* colar_block;
   t_list* cola_exit;
   
   //Mutex p
   pthread_mutex_t m_ready, m_new, m_block, m_exit; //evita que 2 hilos intenten intenter entrar a la cola ready al mismo tiempo, al entrar 1 tira pthread_mutex_t para bloquear la cola y hasta que no sale no se abre de nuevo la cola para otro hilo
   sem_t sem_procesos_ready; //avisa al scheduler que hay un proceso en la cola ready
   t_dictionary* dic_mutex //guarda la info de los sem que se crean al hacer MUTEX_LOCK
   
   
   //Socket globales
   int fd_cpu = -1;
   int fd_memoria = -1;
   int fd_io = -1;
   
   //leer configuracion cargada de que metodo utilizar(fifo,rr,etc)
   char* algoritmo_planificacion;
   int quantum_rr;
   t_log* logger;
   

   //firmas de funciones
   void* atender_cliente(void* arg);
   void* planificador_corto_plazo(void* arg);
   void* temporizador_quantum(void* arg);
   void mover_a_ready(int pid_buscado);

   int main(int argc,char* argv[]) {
  
    // Se verifica la información en el config
   if(argc<3) { //<3 porque ahora tenemos que ponerle el path del proceso principal 

        printf("[ERROR] Mal ejecutado");
        return EXIT_FAILURE; 
    }

    // Se carga el config, extrayendo la información necesaria para la conexión de módulos.
    t_config* config = iniciar_config(argv[1]);

    char* ip_mem = config_get_string_value(config, "IP_MEMORIA");
    char* port_mem = config_get_string_value(config, "PUERTO_MEMORIA");
    char* puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    char* mi_id = config_get_string_value(config, "ID_MODULO");
    algoritmo_planificacion = config_get_string_value(config, "PLANIFICATION_ALGORITHM");
    quantum_rr = config_get_string_value(config, "RR_QUANTUM");
    
    // Se inicia el Logger
    t_log* logger = log_create("Scheduler.log","SCHEDULER",1,LOG_LEVEL_INFO);
    log_info(logger,"Iniciando Scheduler");

    //------------------------------- Inicializacion de estructuras--------------------------
    cola_new = list_create();
    cola_ready = list_create();
    cola_exec = list_create();
    cola_block = list_create();
    cola_exti = list_create();

    dic_mutex = dictionary_create();

    pthread_mutex_init(&m_ready, NULL);
    pthread_mutex_init(&m_new, NULL);
    pthread_mutex_init(&m_block, NULL);
    pthread_mutex_init(&m_exit, NULL);
    sem_init(&sem_procesos_ready, 0, 0);

    // ------------------------------ SCHEDULER COMO CLIENTE ------------------------------ //

    int fd_memoria = crear_conexion(ip_mem,port_mem);

    if(fd_memoria !=-1) {

        enviar_mensaje(mi_id, MENSAJE, fd_memoria);
        log_info(logger, "Scheduler conectado a la memoria con ID: %s",mi_id);

    }
    else {

        log_error(logger,"Fallo en la conexión a la memoria: Kernel Memory no se encuentra activo");
        return EXIT_FAILURE; // Retorna error si la memoria no esta prendida.
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
    
    log_info(logger, "## (%d) Se crea el proceso - Estado: NEW",pcb-inicial->pid);
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

    pthread hilo_planificador; //declaramos variable de tipo pthread llamada hilo_planificador
    pthread_create(&hilo_planificador, NULL, planificador_corto_plazo, NULL); /*creamos un hilo nuevo con estos 4 parametros
    &hilo_planificador es para guardar el ID del hilo creado,NULL es para que no haga nada especial solo configuracion estandar,planificador_corto_plazo es la funcion que va a ejecutar el hilo */
    pthread_detach(hilo_planificaodor); //al crear un hilo linux deja vinculados el hilo nuevo al hilo main con detach desvinculamos los hilos para que el nuevo trabaje de forma independiente

    // ------------------------------ SCHEDULER COMO SERVIDOR ------------------------------ //

    int fd_escucha = iniciar_servidor(puerto_escucha);
    log_info(logger, "Servidor del scheduler encendido. Escuchando CPUs e IOs");

   while(1){ //al poner while(1) ese uno es un TRUE osea siempre verdadero lo que permite que el while sea infinito
    int* socket_cliente = malloc(sizeof(int));
    *socket_cliente = esperar_cliente(fd_escucha);
    pthread hilo_cliente;
    pthread_create(&hilo_cliente, NULL, atender_cliente, socket_cliente);
    pthread_detach(hilo_cliente);
   }
   return 0;
}
   
//----------------------------------Planificacion de corto plazo--------------------------/

void* planificador_corto_plazo(void* arg){
           while(1){
            sem_wait(&sem_procesos_ready);
            pthread_mutex_lock(&m_ready);
            t_pcb* pcb_a_ejecutar = list_remove(cola_ready, 0);
            pthread_mutex_unlock(&m_ready);
            log_info(logger, "## (%d) Pasa del estado READY al estado EXEC",pcb_a_ejecutar->pid);
            //manda el pcb al cpu a ejecutar
            enviar_pcb(pcb_a_ejecutar, fd_cpu, CONTEXTO_PCB);
            if(strcmp(algoritmo_planificacion, "RR")== 0){
                pthread_t hilo_quantum;
                pthread_create(&hilo_quantum,NULL,temporizador_quantum,pcb_a_ejecutar);
                pthread_detach(hilo_quantum);
            }
            return NULL;
           }

}
void* temporizador_quantum(void* arg){
    t_pcb* pcb = (t_pcb*)arg; //aca le decimos al compilador que trate a ese arg como un puntero a un pcb para leer el PID
    usleep(quantum_rr * 1000); //la funcion usleep espera una x cantidad de microsegundos y por mil para pasar esos microsegundos a milisegundos
    log_info(logger,"## (%d) Desalojo de quantum",pcb->pid);
    enviar_mensaje("INTERRUPCION_RR",INTERRUPCION,fd_cpu); //aca el scheduler le pide a la cpu que frene la ejecucion del procesos y se lo devuelva
    return NULL;
}


void* atender_cliente(void* arg){
    int socket_cliente = *(int*)arg; // arg es un puntero que apunta a la direccion de memoria donde el main guardo el numero de socket y (int*)*arg le dice al compilador que trate a ese puntero como un entero
    free(arg);

    op_code cod_op = recibir_operacion(socket_cliente);
    char* id_recibida = recibir_mensaje(socket_cliente);
    
    if(strcmp(id_recibida, "CPU")== 0){
        fd_cpu = socket_cliente;
        log_info(logger,"## CPU <ID CPU> conectada");
        while (1) {
         op_code cod_op = recibir_operacion(socket_cliente);
        if(cod_op == -1)break;
       
        t_pcb* pcb_upd = recibir_pcb(fd_cpu);
            switch (cod_op) {
            case SYSCALL_EXIT:
            log_info(logger, "## (%d) Solicito syscall: EXIT",pcb_upd->pid);
            log_info(logger, "## (%d) Pasa del estado EXEC al estado EXIT", pcb_upd->pid);
            log_info(logger, "## (%d) finalizó su ejecución", pcb_upd->pid);
            list_add(cola_exit, pcb_upd);
            sem_post(&sem_procesos_ready); // La CPU queda libre
            break;    
          
            case SYSCALL_SLEEP: { 
            int tiempo_ms;
            recv(fd_cpu,&tiempo_ms,sizeof(int),MSG_WAITALL);
            log_info(logger, "## (%d) Solicito syscall: SLEEP", pcb_upd->pid);
            log_info(logger, "## (%d) Pasa del estado EXEC al estado BLOCK", pcb_upd->pid);
            pthread_mutex_lock(&m_block);
            list_add(cola_block, pcb_upd);
            pthread_mutex_unlock(&m_block);
            
            if(fd_io != -1){
                enviar_mensaje("SLEEP", SYSCALL_SLEEP,fd_io);
                send(fd_io,&tiempo_ms,sizeof(int),0);
                send(fd_io,&(pcb_upd-<pid),sizeof(int),0);
              }
              sem_post(&sem_procesos_ready);
              break;
            }

            case SYSCALL_MUTEX_CREATE: { //mutex create crea una cola con un nombre determinado y guarda en dictionary todas esas colas que va creando para no repetir
                char* m_name = recibir_mensaje(fd_cpu);
                if(!dictionary_has_key(dic_mutex, m_name)){
                    dictionary_put(dic_mutex, m_name, queue_create());
                }
            enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB);
            free(m_name);
            break;
            }
        
            case SYSCALL_MUTEX_LOCK: {
                 char* m_name = recibir_mensaje(fd_cpu);
                 t_queue* q = dictionary_get(dic_mutex, m_name);
                 if(queue_is_empty(q)){
                    log_info(logger, "## (%d) Toma el mutex %s",pcb_upd->pid);
                    queue_push(q, pcb_upd);
                    enviar_pcb(pcb_upd, fd_cpu, CONTEXTO_PCB); 
                }else{
                    log_info(logger, "## (%d) Psa del estado de EXEC al estado BLOCK"pcb_upd->pid);
                    pthre_mutex_lock(&m_block);
                    list_add(cola_block,pcb_upd);
                    pthread_mutex_unlock(&m_block);
                    queue_push(q, pcb_upd)
                    sem__post(&sem_procesos_ready);
                }
            free(m_name);
            break;
            }
            

            
            
            

            
            
            
            
            case SYSCALL_STDIN:{
                int tam, dir;
                recv(fd_cpu, &tam, sizeof(int), MSG_WAITALL);
                recv(fd_cpu, &dir, siz)of(int), MSG_WAITALL

l                ;
            }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        }
        
        }
    }      
}

void mover_a_ready(int pid_buscado){
    
}

















