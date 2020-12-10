/**
 * @defgroup   SAXPY saxpy
 *
 * @brief      This file implements an iterative saxpy operation
 * 
 * @param[in] <-p> {vector size} 
 * @param[in] <-s> {seed}
 * @param[in] <-n> {number of threads to create} 
 * @param[in] <-i> {maximum itertions} 
 *
 * @author     Danny Munera
 * 			   Kevin Restrepo Garcia & Carlos Andres Gomez
 * @date       2020
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h> 

//Firma del método que modela la instrucción Saxpy
void *compute(void *);

//Semaforo para proteger la región crítica 
sem_t mutex;

//Definición de variables globales
double *X, *Y, *Y_avgs;
double a;

//Estructura para pasar los parametros de cada hilo
typedef struct _param{
	int init;
	int end;
	int it;
	int p;
}param_t;

int main(int argc, char* argv[]){
	// Variables to obtain command line parameters
	unsigned int seed = 1;
  	int p = 10000000;
  	int n_threads = 2;
  	int max_iters = 1;
	//
	int i,it;
	// Variables to get execution time
	struct timeval t_start, t_end;
	double exec_time;

	// Getting input values
	int opt;
	while((opt = getopt(argc, argv, ":p:s:n:i:")) != -1){  
		switch(opt){  
			case 'p':  
			printf("vector size: %s\n", optarg);
			p = strtol(optarg, NULL, 10);
			assert(p > 0 && p <= 2147483647);
			break;  
			case 's':  
			printf("seed: %s\n", optarg);
			seed = strtol(optarg, NULL, 10);
			break;
			case 'n':  
			printf("threads number: %s\n", optarg);
			n_threads = strtol(optarg, NULL, 10);
			break;  
			case 'i':  
			printf("max. iterations: %s\n", optarg);
			max_iters = strtol(optarg, NULL, 10);
			break;  
			case ':':  
			printf("option -%c needs a value\n", optopt);  
			break;  
			case '?':  
			fprintf(stderr, "Usage: %s [-p <vector size>] [-s <seed>] [-n <threads number>]\n", argv[0]);
			exit(EXIT_FAILURE);
		}  
	}  
	srand(seed);

	printf("p = %d, seed = %d, n_threads = %d, max_iters = %d\n", \
	 p, seed, n_threads, max_iters);	

	// initializing data
	X = (double*) malloc(sizeof(double) * p);
	Y = (double*) malloc(sizeof(double) * p);
	Y_avgs = (double*) malloc(sizeof(double) * max_iters);

	for(i = 0; i < p; i++){
		X[i] = (double)rand() / RAND_MAX;
		Y[i] = (double)rand() / RAND_MAX;
	}
	for(i = 0; i < max_iters; i++){
		Y_avgs[i] = 0.0;
	}
	a = (double)rand() / RAND_MAX;

#ifdef DEBUG
	printf("vector X= [ ");
	for(i = 0; i < p-1; i++){
		printf("%f, ",X[i]);
	}
	printf("%f ]\n",X[p-1]);

	printf("vector Y= [ ");
	for(i = 0; i < p-1; i++){
		printf("%f, ", Y[i]);
	}
	printf("%f ]\n", Y[p-1]);

	printf("a= %f \n", a);	
#endif

	/*
	 *	Function to parallelize 
	 */
	gettimeofday(&t_start, NULL);

	//Calculo de porciones asignadas a cada hilo (section per thread-> spt)
	int spt = p/n_threads;
	//Inicialización del semaforo, al ser de tipo binario se inicializa en 1
	sem_init(&mutex, 0, 1);
	//Vector para almacenar n hilos
	pthread_t threads[n_threads];
	//Vector para almacenar los argumentos a pasar a n hilos
	param_t params[n_threads];

	//SAXPY iterative SAXPY mfunction
	for(it = 0; it < max_iters; it++){
		//Ciclo para la creación de los n hilos
		for (i = 0; i < n_threads; i++){
			params[i].init = i*spt;
			if((i+1) != n_threads){
				params[i].end = (i+1)*spt;
			}else{
				params[i].end = p+1;
			}
			params[i].it = it;
			params[i].p = p;
			pthread_create(&threads[i],NULL,compute, &params[i]);
		}
	       	for (i = 0; i < n_threads; i++){
			pthread_join(threads[i], NULL);
		}
	}
	gettimeofday(&t_end, NULL);

#ifdef DEBUG
	printf("RES: final vector Y= [ ");
	for(i = 0; i < p-1; i++){
		printf("%f, ", Y[i]);
	}
	printf("%f ]\n", Y[p-1]);
#endif
	
	// Computing execution time
	exec_time = (t_end.tv_sec - t_start.tv_sec) * 1000.0;  // sec to ms
	exec_time += (t_end.tv_usec - t_start.tv_usec) / 1000.0; // us to ms
	printf("Execution time: %f ms \n", exec_time);
	printf("Last 3 values of Y: %f, %f, %f \n", Y[p-3], Y[p-2], Y[p-1]);
	printf("Last 3 values of Y_avgs: %f, %f, %f \n", Y_avgs[max_iters-3], Y_avgs[max_iters-2], Y_avgs[max_iters-1]);
	return 0;
}

//Función que es ejecutada por los hilos
void* compute(void *args){
	 param_t *param = (param_t *)args;
	 int i = param -> init;
	 int end = param -> end;
	 int it = param -> it;
	 int p = param -> p;
	 //Variable local para optimizar el calculo del promedio
	 double acum = 0.0;

	 while(i<end){
		Y[i] = Y[i] + a * X[i];
		acum += Y[i];
		i++;
	}
	 sem_wait(&mutex);
	 //Sección crítica protegida por el semaforo
	 Y_avgs[it] += acum / p;
	 sem_post(&mutex);
	 pthread_exit(NULL);
}