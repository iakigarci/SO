#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>   
#include <time.h>    
#include <stdbool.h>
#include <limits.h> 

#define MAXTHREAD       8
#define DELAY_TIMER     5
#define BUFFER_MAX      5
#define WAITING_TO_EXIT 30
#define NUM_CPU 		1
#define NUM_CORE		2

typedef struct { 
  pthread_t tid; 
} identif_t; 

struct parametros {
	int tid;
	int frec;
};

enum scheduler_m { timer, event } scheduler_flag;

/** ESTRUCTURAS CPU **/
typedef struct PCB {
	int id;
	int quantum;
	int prioridad; // 0..3
}PCB; 

typedef struct Queue { 
	int front, rear, size; 
	struct PCB arr_pcb[BUFFER_MAX]; 
}Queue;  

typedef struct core_thread 
{
	bool is_process;
	struct PCB t_pcb;
} c_thread;

typedef struct cpu_core
{
   c_thread arr_th[MAXTHREAD];
} core;

typedef struct cpu
{
   core arr_core[NUM_CORE];
} cpu_t;

/** ---------------- **/
struct cpu arr_cpu[NUM_CPU];
struct Queue *queue1,*queue2,*queue3;
struct Queue *queue0_ptr, queue0;
pthread_mutex_t mutex, mutexC;
sem_t sem_cola;
int clockTime, priorityTime;

void *kernelClock(void *arg); 
void *timerScheduler(void *arg); 
void *processGenerator(void *arg); 
void *schedulerTiempo(void *arg); 
void *schedulerEvento(void *c_thread);
// void *schedulerEvento(struct core_thread c_thread);
struct Queue* createQueue() ;
int isFull(struct Queue* queue);
int isEmpty(struct Queue* queue) ;
struct PCB front(struct Queue* queue) ;
struct PCB rear(struct Queue* queue) ;
void enqueue(struct Queue* queue, struct PCB pcb) ;
struct PCB dequeue(struct Queue* queue);
void inicializar();
int mensaje_error(char *s);
void asignarPCB(struct PCB pcb);
void decrementarQ_PCB(struct core_thread c_thread);
void aumentarPrioridad();
void subirPrioridadColas(struct Queue* pQueue1, struct Queue* pQueue2);
int todosHilosOcupados();


/*----------------------------------------------------------------- 
 *   main
 *----------------------------------------------------------------*/

/*
	1	clockFrec
	2	timerFrec
	3	processFrec
*/
int main(int argc, char *argv[]) {
	
	//	Inicialización de estructuras	
	identif_t idtimer, idclock, idprocessgenerator;
	
	//	Control de parametros de entrada
	if (argc != 4)
	{
		mensaje_error("Los parametros sos: ./scritp frecuenciaClock frecuenciaTimer frecuenciaProcessGen");
	}
	struct parametros p1; //	clock
	struct parametros p2; //	timer
	struct parametros p3; //	processGenerator
	struct parametros p4; //	scheduler

	inicializar();

	//	Inicializacion de los parámetros que van a tener los hilos
	p1.tid=idclock.tid;
	p1.frec=atoi(argv[1]);
	p2.tid=idtimer.tid;
	p2.frec=atoi(argv[2]);
	p3.tid=idprocessgenerator.tid;
	p3.frec=atoi(argv[3]);

	printf("Comienza el programa V.2\n");
	inicializar();
	//	Se crean los hilos necesarios para todo el sistema
	// pthread_create(&(idHilo),atributosHilo,funcion, parametroVoid)
	pthread_create(&(idclock.tid),NULL,kernelClock,(void *)&p1);
	pthread_create(&(idtimer.tid),NULL,timerScheduler,(void *)&p2);
	pthread_create(&(idprocessgenerator.tid),NULL,processGenerator,(void *)&p3);

	sleep(WAITING_TO_EXIT);
	printf("Fin\n");
	return(0);
}

/*----------------------------------------------------------------- 
 *   inicializar
 *----------------------------------------------------------------*/

void inicializar() {
   pthread_mutex_init(&mutex, NULL);
   pthread_mutex_init(&mutexC, NULL);
   //sem_init(&sem_cola, 0, 0);

   clockTime=0;
   queue0_ptr = createQueue();
   struct Queue* queue1 = createQueue();
   struct Queue* queue2 = createQueue();
   struct Queue* queue3 = createQueue();
} // inicializar

/*----------------------------------------------------------------- 
 *   clock
 *----------------------------------------------------------------*/

void *kernelClock(void *arg) {
	struct parametros *ptr, p;
	ptr = (struct parametros *)arg;
	int id = p.tid;
	int frec = p.frec;
	int count;
	while(1) {
		count++;
		// Aproximado: 1s
		if (count == 100000000)
		{
			count=0;
			pthread_mutex_lock(&mutexC);
			clockTime++;
			printf("  CLOCK[%d] \n", clockTime);
			pthread_mutex_unlock(&mutexC);	
		}
	}
}

/*----------------------------------------------------------------- 
 *   timer
 *----------------------------------------------------------------*/

void *timerScheduler(void *arg) {
	struct parametros *ptr, p;
	ptr = (struct parametros *)arg;
	int id = p.tid;
	int frec = p.frec;
	while(1) {
		sleep(1000);
		pthread_mutex_lock(&mutexC);
		if (clockTime>=frec)
		{
			identif_t idscheduler;
			clockTime=0;
			priorityTime++;
			printf("  TIMER[%d] avisa\n", id);
			pthread_create(&(idscheduler.tid),NULL,schedulerTiempo,NULL);
			if (priorityTime>=10)
			{
				priorityTime=0;
				aumentarPrioridad();
				if(todosHilosOcupados()) {
					scheduler_flag = event;
				}
			}
			
		}
		pthread_mutex_unlock(&mutexC);	
	}
} 
/*----------------------------------------------------------------- 
 *   process generator
 *----------------------------------------------------------------*/

void *processGenerator(void *arg) {
	struct parametros *ptr, p;
	ptr = (struct parametros *)arg;
	int id = p.tid;
	int frec = p.frec;
	int i=0;
	struct PCB pcb;
	while(1) {
		sleep(frec);
		pcb.id=i;
		pcb.prioridad = rand() % (3+1-0) + 0;
		pcb.quantum = rand() % (1000+1-1) + 1;
		//sem_wait(&sem_cola);
		pthread_mutex_lock(&mutex);
		switch (pcb.prioridad)
		{
		case 0:
			enqueue(queue0_ptr,pcb);
			break;
		case 1:
			enqueue(queue1,pcb);
			break;
		case 2:
			enqueue(queue2,pcb);
			break;
		case 3:
			enqueue(queue3,pcb);
			break;
		default:
			break;
		} 
		printf("  Process Generator[%d] produce %02d\n", id, pcb.id);
		pthread_mutex_unlock(&mutex);
		//sem_post(&sem_cola);
		i++;	
	}
}

/*----------------------------------------------------------------- 
 *   scheduler
 *----------------------------------------------------------------*/
void *schedulerTiempo(void *arg) {
	struct parametros *ptr, p;
	ptr = (struct parametros *)arg;
	int id = p.tid;
	int frec = p.frec;
	printf("Soy un Scheduler por tiempo con número [%d] \n", id);
	pthread_mutex_lock(&mutex);
	bool seguir;
	while (seguir)
	{
		if (isEmpty(queue0_ptr))
		{
			if (isEmpty(queue1))
			{
				if (isEmpty(queue2))
				{
					if (isEmpty(queue3))
					{
						seguir=false;
					}else{asignarPCB(dequeue(queue3));}
				}else{asignarPCB(dequeue(queue2));}
			}else{asignarPCB(dequeue(queue1));}
		}else{asignarPCB(dequeue(queue0_ptr));}
	}
	pthread_mutex_unlock(&mutex);
}

void *schedulerEvento(void *c_ptr) {
	bool seguir;
	struct PCB pcb; 
	pthread_mutex_lock(&mutex);
	while (seguir)
	{
		if (isEmpty(queue0_ptr))
		{
			if (isEmpty(queue1))
			{
				if (isEmpty(queue2))
				{
					if (isEmpty(queue3))
					{
						seguir=false;
					}else{pcb = dequeue(queue3);}
				}else{pcb = dequeue(queue2);}
			}else{pcb = dequeue(queue1);}
		}else{pcb = dequeue(queue0_ptr);}
	}
	pthread_mutex_unlock(&mutex);
	if (!seguir)
	{
		struct core_thread *c_ptrAux = (struct core_thread*)c_ptr;
		c_ptrAux->t_pcb=pcb;
		c_ptrAux->is_process=true;
	}
}

void asignarPCB(struct PCB pcb) {
	int i,j,k;
	bool salir;
	while (i < NUM_CPU)
	{
		while (j < NUM_CORE)
		{
			while (k < MAXTHREAD)
			{
				if(arr_cpu[i].arr_core[j].arr_th[k].is_process) {
					k++;
				}else{
					arr_cpu[i].arr_core[j].arr_th[k].t_pcb = pcb;
					decrementarQ_PCB(arr_cpu[i].arr_core[j].arr_th[k]);
					salir = true;
				}
			}j++;
		}i++;
	}
	if (!salir){
		asignarPCB(pcb);
	}
}

void decrementarQ_PCB(struct core_thread c_thread) {
	int i;
	struct PCB pcb = c_thread.t_pcb;
	while (i<pcb.quantum) ;
	{
		pcb.quantum--;
		i++;
	}
	pcb.quantum=0;
	c_thread.is_process=false;
	if (scheduler_flag == event)
	{
		identif_t idscheduler;
		struct core_thread *c_ptr, c_thread;
		pthread_create(&(idscheduler.tid),NULL,schedulerEvento,(void*) c_ptr);
	}
}

void aumentarPrioridad() { 
	subirPrioridadColas(queue1,queue0_ptr);
	subirPrioridadColas(queue2,queue1);
	subirPrioridadColas(queue3,queue2);
}

/**
 * Sube cola 1 a cola 2
**/
void subirPrioridadColas(struct Queue* pQueue1, struct Queue* pQueue2) {
	bool seguir;
	while (seguir)
	{
		if (!isEmpty(pQueue1))
		{
			enqueue(pQueue2,dequeue(pQueue1));
		}else{ seguir=false; }
	}
}
/**
 * 0	False
 * 1	True
**/
int todosHilosOcupados() {
	int i,j,k;
	while (i < NUM_CPU)
	{
		while (j < NUM_CORE)
		{
			while (k < MAXTHREAD)
			{
				if(arr_cpu[i].arr_core[j].arr_th[k].is_process) {
					k++;
				}else{
					return 0;
				}
			}j++;
		}i++;
	}
	return 1;
}
/*----------------------------------------------------------------- 
 *   Queue
 *----------------------------------------------------------------*/
struct Queue* createQueue() 
{ 
	struct Queue* pQueue = (struct Queue*)malloc(sizeof(struct Queue)); 
	pQueue->front = pQueue->size = 0; 
  
	pQueue->rear = BUFFER_MAX - 1; 
	return pQueue; 
} 

// Queue is full when size becomes 
// equal to the capacity 
int isFull(struct Queue* pQueue) 
{ 
	return (pQueue->size == BUFFER_MAX); 
} 
  
// Queue is empty when size is 0 
int isEmpty(struct Queue* pQueue) 
{ 
	return (pQueue->size == 0); 
} 
  
// Function to add an item to the queue. 
// It changes rear and size 
void enqueue(struct Queue* pQueue, struct PCB pcb) 
{ 
	if (isFull(pQueue)) 
		return; 
	pQueue->rear = (pQueue->rear + 1) 
				  % BUFFER_MAX; 
	pQueue->arr_pcb[pQueue->rear] = pcb; 
	pQueue->size = pQueue->size + 1; 
	//printf("%d enqueued to queue\n", pcb.id); 
} 
  
// Function to remove an item from queue. 
// It changes front and size 
struct PCB dequeue(struct Queue* pQueue) 
{ 
	if (isEmpty(pQueue)) {
		struct PCB pcbNulo; 
		pcbNulo.quantum=0;
		return pcbNulo;
	} 
	struct PCB pcb = pQueue->arr_pcb[pQueue->front]; 
	pQueue->front = (pQueue->front + 1) 
				   % BUFFER_MAX; 
	pQueue->size = pQueue->size - 1; 
	return pcb; 
} 
  
// Function to get front of queue 
struct PCB front(struct Queue* pQueue) 
{ 
	if (isEmpty(pQueue)) { 	
		struct PCB pcbNulo;
		pcbNulo.quantum=0;
		return pcbNulo; 
	}
	return pQueue->arr_pcb[pQueue->front]; 
} 
  
// Function to get rear of queue 
struct PCB rear(struct Queue* pQueue) 
{ 
	if (isEmpty(pQueue)) { 	
		struct PCB pcbNulo;
		pcbNulo.quantum=0;
		return pcbNulo; 
	}
	return pQueue->arr_pcb[pQueue->rear]; 
} 

/*----------------------------------------------------------------- 
 *   mensajes
 *----------------------------------------------------------------*/

int mensaje_error(char *s) {
	printf("***error*** %s\n",s);
	exit(-1);
}