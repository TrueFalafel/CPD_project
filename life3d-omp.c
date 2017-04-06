#include <stdio.h>
#include <stdlib.h>
#include "./LinkedListLib/linked_list.h"
#include "./HashTableLib/hashtable.h"
#include <string.h>
#include <omp.h>
#include <math.h>

#define N_SLICES 3
#define MIDDLE_SLICE 1
#define CLEAN_SIZE_LIMIT 300

#define MINDEX(i, j) (i + j*cube_size)
#define SLICE_CLEAN(slice) (memset(slice, 0, cube_size*cube_size))

//Multi-thread macros
#define THREAD_ID (omp_get_thread_num())
#define NEXT_THREAD (THREAD_ID == omp_get_num_threads()-1 ? 0 : THREAD_ID+1)
#define THREAD_SPACE(j) ((N_SLICES) * THREAD_ID + (j)) // Domain in dynamic_matrix
#define RIGHT_LIM (THREAD_ID < omp_get_num_threads() - 1 ? first_iter[THREAD_ID + 1] : cube_size)
#define RIGHT_WRAP (RIGHT_LIM == cube_size ? 0 : RIGHT_LIM)

void usage();
int hashfunction (struct data k);
void insert_in_slice(signed char * slice, hashtable_s *hashtable, int entry);
void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count);
void check_entry(signed char *entry, item **dead_to_live, data K, int *count);
void check_neighbors_shared(signed char **matrix, item **dead_to_live, item *node, int *count);
void check_entry_shared(signed char *entry, item **dead_to_live, data K, int *count);
void matrix_print(signed char ** matrix);
void compute_generations(hashtable_s *hashtable);

//Multi-thread functions
void threads_1st_iter(int *, int);

//Global variables
unsigned cube_size, n_generations;

int main(int argc, char *argv[]){
	//double end, start = omp_get_wtime();

	/*GET INPUT TEXT FILE, CHECK FOR ERRORS************************************/
	if(argc != 3){
		printf("Incorrect number of arguments\n");
		usage();
		exit(1);
  	}

  	FILE *pf;
  	if(!(pf = fopen(argv[1], "r"))){
  		printf("File couldn't be found\n");
    	usage();
    	exit(2);
  	}
	/**************************************************************************/
	/*GET CUBE SIZE -> FIRST LINE OF INPUT TEXT FILE & NUMBER OF GENERATIONS***/
	n_generations = atoi(argv[2]);
	fscanf(pf, "%d", &cube_size);
	/*GET ALL LIVE CELLS IN THE BEGINNING**************************************/
	hashtable_s *hashtable;
	data k;
	hashtable = hash_create( cube_size, &hashfunction);
	while(fscanf(pf, "%d %d %d", &k.x, &k.y, &k.z) != EOF)
		hash_insert( hashtable, k);
	fclose(pf);
	/****************************************************************************/
	compute_generations(hashtable);
	hash_sort(hashtable);
	hash_print(hashtable);
	hash_free(hashtable);
	//end = omp_get_wtime();
	//printf("Execution time: %e s\n", end - start); // PRINT IN SCIENTIFIC NOTATION
	exit(0);
}
/****************************END of MAIN******************************************/

void compute_generations(hashtable_s *hashtable){

	/*SHARED VARIABLES***********************************************************/
  	int *first_iter;
	int launched_threads = cube_size/N_SLICES;
	if(launched_threads >= omp_get_max_threads())
		launched_threads = omp_get_max_threads();
	else
		omp_set_num_threads(launched_threads);
	first_iter = malloc(sizeof(int)*launched_threads);
	threads_1st_iter(first_iter, launched_threads);
	int i;
	int threads_finished = 0, slices_cleaned = 0, thread_help = 0;
	item *first_list[launched_threads], *second_list[launched_threads];
	item *dead_to_live[N_SLICES*launched_threads];
	signed char *dynamic_matrix[N_SLICES*launched_threads], *cleaned_slice[launched_threads];
	signed char *first_slice[launched_threads], *second_slice[launched_threads];
	int first_slice_finished[launched_threads], second_slice_finished[launched_threads];

	for(i = 0; i < N_SLICES*launched_threads; i++)
		dynamic_matrix[i] = calloc(cube_size * cube_size, sizeof(char));

	/**************************************************************************/

	#pragma omp parallel
	{	//printf("NUM THREADS: %d\n",omp_get_num_threads() );
		while(1){ // INSERTING WHILE INSIDE LOOP
			if(!omp_get_thread_num())
				n_generations--;

			/*to keep other threads from reaching this thread first and second slice
			before it has finished working with them*/
			first_slice_finished[THREAD_ID] = 0;
			second_slice_finished[THREAD_ID] = 0;
			#pragma omp barrier
			if(n_generations == -1)
				break;


			int n;

		  	for(n = 0; n < N_SLICES; n++){ //first 3 slice insertions
		  		insert_in_slice(dynamic_matrix[THREAD_SPACE(n)], hashtable, first_iter[THREAD_ID] + n);
			}

			/*lists that takes all the possible dead candidates to become live
		  	one for each slice*/
		  	for(n = 0; n < N_SLICES; n++)   // INITIALIZE TO NULL
		  		dead_to_live[THREAD_SPACE(n)] = list_init();

			signed char *matrix_tmp = NULL;
		  	int middle = first_iter[THREAD_ID] + 1; //keeps track of the hashlist correspondig to the middle slice
		    /*******************************************************************/


		    /*BEGIN PARALLEL FOR***********************************************/


		    #pragma omp for
		        for(i = 0; i < cube_size; i++){
		    		if(middle == cube_size) // IF LAST ITERATION SET MIDDLE = 0 (WRAP of x)
		    			middle = 0;

		          	item *list_aux = list_init(), *aux = NULL;
		          	int count;
		            /*first 3 slices are already filled, and when it wraps around
		    		slices 1 and 2 are already filled too*/
					if(i != first_iter[THREAD_ID] && i != RIGHT_LIM - 2 && i != RIGHT_LIM - 1)
		    			insert_in_slice(dynamic_matrix[THREAD_SPACE(2)], hashtable, middle + 1);

					int checks_shared=0;
					if(threads_finished > 0 && thread_help == 0){
						#pragma omp critical (threads_finished)
						{
							checks_shared = (thread_help == 0);
							thread_help++;
						}
					}
					while(hashtable->table[middle] != NULL){
		            	count = 0;
		    			aux = hash_first(hashtable, middle);
						if(checks_shared){
							#pragma omp task shared(dead_to_live, dynamic_matrix)
							{
								check_neighbors_shared(dynamic_matrix + THREAD_SPACE(0),
									dead_to_live + THREAD_SPACE(0), aux, &count);

								//if cell stays alive goes to the temporary list
					    		if(count >= 2 && count <= 4)
									#pragma omp critical (list_aux)
					    			list_aux = list_push(list_aux, aux);
					    		else //else it dies, so doesn't stay in the hash table
									#pragma omp critical (list_aux)
					    			free(aux);
							}
						}else{
		    			check_neighbors(dynamic_matrix + THREAD_SPACE(0),
							dead_to_live + THREAD_SPACE(0), aux, &count);

						//if cell stays alive goes to the temporary list
			    		if(count >= 2 && count <= 4)
							#pragma omp critical (list_aux)
			    			list_aux = list_push(list_aux, aux);
			    		else //else it dies, so doesn't stay in the hash table
							#pragma omp critical (list_aux)
			    			free(aux);
						}

		    		}
					//PODE ESTAR À ESPERA DE TASKS DE OUTROS EM VAO!
					#pragma omp taskwait
		    		//inserts just the live cells that stayed alive
		    		hashtable->table[middle] = list_aux;
					list_aux = NULL;

					//insert dead cells that become live in hashtable (only if not in the first or second iteration)
		    		if(i != first_iter[THREAD_ID] && i != first_iter[THREAD_ID]+1 && i != RIGHT_LIM - 1){
		    			hashtable->table[middle - 1] = lists_concatenate(hashtable->table[middle - 1], dead_to_live[THREAD_SPACE(0)]);
		    			dead_to_live[THREAD_SPACE(0)] = NULL;
		    		}
		    		else if(i == RIGHT_LIM - 1){
		    			hashtable->table[RIGHT_LIM - 1] = lists_concatenate(hashtable->table[RIGHT_LIM - 1], dead_to_live[THREAD_SPACE(0)]);
		    			hashtable->table[RIGHT_WRAP] = lists_concatenate(hashtable->table[RIGHT_WRAP], dead_to_live[THREAD_SPACE(1)]);
		    			hashtable->table[RIGHT_WRAP + 1] = lists_concatenate(hashtable->table[RIGHT_WRAP + 1], dead_to_live[THREAD_SPACE(2)]);
		    			dead_to_live[THREAD_SPACE(0)] = NULL;
		    			dead_to_live[THREAD_SPACE(1)] = NULL;
		    			dead_to_live[THREAD_SPACE(2)] = NULL;
		    		}/*Thread i+1 has to be here before thread i reaches Thread i+1 first and second slices*/

		    		else if(i == first_iter[THREAD_ID]){
		    			first_slice[THREAD_ID] = dynamic_matrix[THREAD_SPACE(0)];
		    			dynamic_matrix[THREAD_SPACE(0)] = malloc(cube_size * cube_size * sizeof(char));
		    			first_list[THREAD_ID] = dead_to_live[THREAD_SPACE(0)];
		    			dead_to_live[THREAD_SPACE(0)] = NULL;
						first_slice_finished[THREAD_ID] = 1;
		    		}
		    		else if(i == first_iter[THREAD_ID]+1){
		    			second_slice[THREAD_ID] = dynamic_matrix[THREAD_SPACE(0)];
		    			dynamic_matrix[THREAD_SPACE(0)] = malloc(cube_size * cube_size * sizeof(char));
		    			second_list[THREAD_ID] = dead_to_live[THREAD_SPACE(0)];
		    			dead_to_live[THREAD_SPACE(0)] = NULL;
						second_slice_finished[THREAD_ID] = 1;
		    		}

		    		//dead_to_live lists shift
		    		dead_to_live[THREAD_SPACE(0)] = dead_to_live[THREAD_SPACE(1)];
		    		dead_to_live[THREAD_SPACE(1)] = dead_to_live[THREAD_SPACE(2)];
		    		if(i == RIGHT_LIM - 3){
						//waits for next thread to finish danger zone
						while(!first_slice_finished[NEXT_THREAD]);
		    			dead_to_live[THREAD_SPACE(2)] = first_list[NEXT_THREAD];
		    		}else if(i == RIGHT_LIM - 2){
						while(!second_slice_finished[NEXT_THREAD]);
		    			dead_to_live[THREAD_SPACE(2)] = second_list[NEXT_THREAD];
		    		}else{
		    			dead_to_live[THREAD_SPACE(2)] = list_init();
		    		}
		    		//matrix shift
		    		matrix_tmp = dynamic_matrix[THREAD_SPACE(0)];
		    		dynamic_matrix[THREAD_SPACE(0)] = dynamic_matrix[THREAD_SPACE(1)];
		    		dynamic_matrix[THREAD_SPACE(1)] = dynamic_matrix[THREAD_SPACE(2)];
		    		if(i == RIGHT_LIM - 3){
		    			dynamic_matrix[THREAD_SPACE(2)] = first_slice[NEXT_THREAD];
		    			free(matrix_tmp);
		    		}else if(i == RIGHT_LIM - 2){
		    			dynamic_matrix[THREAD_SPACE(2)] = second_slice[NEXT_THREAD];
		    			free(matrix_tmp);
		    		}else{
						if(threads_finished > 0 && slices_cleaned > 0){
							dynamic_matrix[THREAD_SPACE(2)] = cleaned_slice[slices_cleaned];
							#pragma omp atomic
							slices_cleaned --;
							free(matrix_tmp);
						}else{
							dynamic_matrix[THREAD_SPACE(2)] = matrix_tmp;
		    				SLICE_CLEAN(dynamic_matrix[THREAD_SPACE(2)]);
						}
		    		}
		    		middle++; //goes on in the hashtable

					if(i == RIGHT_LIM-1){
						int condition;
						#pragma omp critical (finished)
						{
							threads_finished++; //a thread finished!
							condition = (threads_finished == 1);
						}
						if(condition && cube_size > CLEAN_SIZE_LIMIT){
							while(1){
								if(slices_cleaned < launched_threads){
									cleaned_slice[slices_cleaned] = calloc(cube_size * cube_size, sizeof(char));
									#pragma omp atomic
									slices_cleaned ++;
								}
								if(threads_finished == launched_threads)
									break;
							}
						}
					}
		    	}/*PARALLEL FOR LOOP FINISH*****************************************/
			for(n = 0; n < N_SLICES; n++)
				SLICE_CLEAN(dynamic_matrix[THREAD_SPACE(n)]);
	    }

	}/*END OF PARALLEL SECTION*******************************************/
	free(first_iter);
	for(i = 0; i < N_SLICES*launched_threads; i++){
		free(dynamic_matrix[i]);
	}
}

void threads_1st_iter(int *w, int parts){
	int n = cube_size, size = 0, k;
	do{
		k = (int)ceil((double)n/parts);
	    w[size++] = k;
	    n -= k;
	}while(--parts && n);
	int aux, accum = 0;
	for(n = 0; n != size; n++){
	   	aux = w[n];
	   	w[n] = accum;
	   	accum += aux;
	}
}

void usage(){
    printf("Usage: ./life3d <input file> <number of generations>\n");
}

int hashfunction (struct data k){
    return k.x;
}

data set_data(int x, int y, int z){
	data k;

	k.x = x;
	k.y = y;
	k.z = z;

	return k;
}

int equal_data(data K1, data K2){
	return K1.x == K2.x && K1.y == K2.y && K1.z == K2.z;
}

void print_data(data K){
	printf("%d %d %d\n", K.x, K.y, K.z);
}

void insert_in_slice(signed char * slice, hashtable_s *hashtable, int entry){
	item *aux;
	item *list_aux = NULL;

	while(hashtable->table[entry]!=NULL){
		aux = hash_first(hashtable, entry);
		slice[MINDEX(aux->K.y, aux->K.z)] = -1;
		list_aux = list_push(list_aux, aux);
	}
	hashtable->table[entry] = list_aux;
}

void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count){
	int x = node->K.x;
	int y = node->K.y;
	int z = node->K.z;
	data K = {.x = x, .y = y, .z = z};

	// x coordinate neighbor search
	K.x = (x != 0) ? x - 1 : cube_size - 1;
	check_entry(&matrix[0][MINDEX(y,z)], &dead_to_live[0], K, count);
	K.x = (x != cube_size - 1) ? x + 1 : 0;
	check_entry(&matrix[2][MINDEX(y,z)], &dead_to_live[2], K, count);
	K.x = x;

	// y coordinate neighbor search
	K.y = (y != 0) ? y - 1 : cube_size - 1;
	check_entry(&matrix[1][MINDEX(K.y,z)], &dead_to_live[1], K, count);
	K.y = (y != cube_size-1) ? y + 1 : 0;
	check_entry(&matrix[1][MINDEX(K.y,z)], &dead_to_live[1], K, count);
	K.y = y;

	// z coordinate neighbor search
	K.z = (z != 0) ? z - 1 : cube_size - 1;
	check_entry(&matrix[1][MINDEX(y,K.z)], &dead_to_live[1], K, count);

	K.z = (z != cube_size-1) ? z + 1 : 0;
	check_entry(&matrix[1][MINDEX(y,K.z)], &dead_to_live[1], K, count);
}

void check_entry(signed char *entry, item **dead_to_live, data K, int *count){

	if(*entry != -1){
		(*entry)++;
		if(*entry == 2)
			*dead_to_live = list_append(*dead_to_live, K);
		else if(*entry == 4)
			*dead_to_live = list_remove(*dead_to_live, K);
		else if(*entry > 6){
			perror("Entry > 6\n");
			exit(-1);
		}
	}else
		(*count)++;
}

void check_neighbors_shared(signed char **matrix, item **dead_to_live, item *node, int *count){
	int x = node->K.x;
	int y = node->K.y;
	int z = node->K.z;
	data K = {.x = x, .y = y, .z = z};

	// x coordinate neighbor search
	K.x = (x != 0) ? x - 1 : cube_size - 1;
	check_entry_shared(&matrix[0][MINDEX(y,z)], &dead_to_live[0], K, count);
	K.x = (x != cube_size - 1) ? x + 1 : 0;
	check_entry_shared(&matrix[2][MINDEX(y,z)], &dead_to_live[2], K, count);
	K.x = x;

	// y coordinate neighbor search
	K.y = (y != 0) ? y - 1 : cube_size - 1;
	check_entry_shared(&matrix[1][MINDEX(K.y,z)], &dead_to_live[1], K, count);
	K.y = (y != cube_size-1) ? y + 1 : 0;
	check_entry_shared(&matrix[1][MINDEX(K.y,z)], &dead_to_live[1], K, count);
	K.y = y;

	// z coordinate neighbor search
	K.z = (z != 0) ? z - 1 : cube_size - 1;
	check_entry_shared(&matrix[1][MINDEX(y,K.z)], &dead_to_live[1], K, count);

	K.z = (z != cube_size-1) ? z + 1 : 0;
	check_entry_shared(&matrix[1][MINDEX(y,K.z)], &dead_to_live[1], K, count);
}

void check_entry_shared(signed char *entry, item **dead_to_live, data K, int *count){
	int check;

	if(*entry != -1){
		#pragma omp atomic
		(*entry)++;
		if(*entry == 2){
			#pragma omp critical (dead_to_live)
			*dead_to_live = list_append(*dead_to_live, K);
		}else if(*entry == 4){
			#pragma omp critical (dead_to_live)
			*dead_to_live = list_remove(*dead_to_live, K);
		}else if(*entry > 6){
			perror("Entry > 6\n");
			exit(-1);
		}
	}else
		(*count)++;
}

item* sort(item* list1, item* list2){
    item* result = NULL;

    if(list1 == NULL){
        return list2;
    }else if (list2 == NULL){
        return list1;
    }

    if(list1->K.x < list2->K.x){
        result = list1;
        result->next = sort(list1->next, list2);
    }else if((list1->K.x > list2->K.x)){
        result = list2;
        result->next = sort(list1, list2->next);
    }else{
        if(list1->K.y < list2->K.y){
            result = list1;
            result->next = sort(list1->next, list2);
        }else if((list1->K.y > list2->K.y)){
            result = list2;
            result->next = sort(list1, list2->next);
        }else{
            if(list1->K.z < list2->K.z){
                result = list1;
                result->next = sort(list1->next, list2);
            }else if((list1->K.z > list2->K.z)){
                result = list2;
                result->next = sort(list1, list2->next);
            }
        }
    }
    return result;
}

void matrix_print(signed char **matrix){
	int i, j;

	for(i = 0; i < N_SLICES; i++){
		printf("%d slice:", i);
		for(j = 0; j < cube_size*cube_size; j++){
			if(j % cube_size == 0)
				printf("\n");
			printf("%d ", matrix[i][j]);
		}
		printf("\n\n");
	}
}
