#include <stdio.h>
#include <stdlib.h>
#include "./LinkedListLib/linked_list.h"
#include "./HashTableLib/hashtable.h"
#include <string.h>
#include <omp.h>
#include <math.h>

#define N_SLICES 3
#define MIDDLE_SLICE 1
/********************************************************
*------------------MACROS-------------------------------
*MINDEX - vectorizes a matrix
*SLICE_CLEAN - cleans a slice
*THREAD_ID - gets thread ID
*NEXT_THREAD - gets next thread ID
*THREAD SPACE - gets j-th slice of the calling thread (j= 0,1,2)
*RIGTH_LIM - gets the first slice of the next thread
*RIGHT_WRAP - like RIGHT_LIM, but takes wrap into account
*********************************************************/
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
int insert_in_slice(signed char * slice, hashtable_s *hashtable, int entry);
void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count,void (*check)(signed char*, item **, data, int*));
void check_entry(signed char *entry, item **dead_to_live, data K, int *count);
void check_entry_shared(signed char *entry, item **dead_to_live, data K, int *count);
void matrix_print(signed char ** matrix);
void compute_generations(hashtable_s *hashtable);

//Multi-thread functions
void threads_1st_iter(int *, int);

//Global variables
unsigned cube_size, n_generations;

int main(int argc, char *argv[]){
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

	hashtable_s *hashtable;
	data k;
	hashtable = hash_create( cube_size, &hashfunction);
	/*GET ALL LIVE CELLS IN THE BEGINNING**************************************/
	while(fscanf(pf, "%d %d %d", &k.x, &k.y, &k.z) != EOF)
		hash_insert( hashtable, k);
	fclose(pf);
	/****************************************************************************/
	compute_generations(hashtable);
	hash_sort(hashtable);
	hash_print(hashtable);
	hash_free(hashtable);
	exit(0);
}
/****************************END of MAIN******************************************/




void compute_generations(hashtable_s *hashtable){

	/*SHARED VARIABLES***********************************************************/
  	int *first_iter;

  	//Launched threads = number of threads that will have slices assigned to them
	int launched_threads = cube_size/N_SLICES;
	if(launched_threads >= omp_get_max_threads())
		launched_threads = omp_get_max_threads();
	else
		omp_set_num_threads(launched_threads);

  	//Array with the first iteration for each thread
	first_iter = malloc(sizeof(int)*launched_threads);
	threads_1st_iter(first_iter, launched_threads);
	int i;
	int threads_finished = 0; //keeps track of how many threads finished their work
  	int thread_help = 0; //boolean that says if a thread is being helped with its tasks
	/*lists that takes all the possible dead candidates to become live one for each slice*/
	item *dead_to_live[N_SLICES*launched_threads];
  	// saves dead_to_live lists of first and second slices, respectively
 	item *first_list[launched_threads], *second_list[launched_threads];
	signed char *dynamic_matrix[N_SLICES*launched_threads];//set of slices
	signed char *first_slice[launched_threads], *second_slice[launched_threads];
  	// booleans to say if a thread finished processing its first/second slice
	int first_slice_finished[launched_threads], second_slice_finished[launched_threads];

  	// Initialize slices
	for(i = 0; i < N_SLICES*launched_threads; i++)
		dynamic_matrix[i] = calloc(cube_size * cube_size, sizeof(char));

	/**************************************************************************/

	#pragma omp parallel
	{
		while(1){
			if(!omp_get_thread_num()) //Only master thread decreases the generation counter
				n_generations--;

			int empty_slices=0; //Keeps track of how many consecutive slices have no live cells

			/*To keep other threads from reaching this thread first and second slices
			before it has finished working with them*/
			first_slice_finished[THREAD_ID] = 0;
			second_slice_finished[THREAD_ID] = 0;

          	#pragma omp single
			threads_finished = 0;

        	#pragma omp barrier

			if(n_generations == -1)
				break;

			int n;

		  	for(n = 0; n < N_SLICES; n++){ //first 3 slice insertions
		  		int inserted = insert_in_slice(dynamic_matrix[THREAD_SPACE(n)], hashtable, first_iter[THREAD_ID] + n);
				inserted ? empty_slices=0 : empty_slices++;
			}

			/*lists that takes all the possible dead candidates to become live
		  	one for each slice*/
		  	for(n = 0; n < N_SLICES; n++)   // INITIALIZE TO NULL
		  		dead_to_live[THREAD_SPACE(n)] = list_init();

			signed char *matrix_tmp = NULL; //temporary pointer to indexed matrix for pointer change
		  	int middle = first_iter[THREAD_ID] + 1; //keeps track of the hashlist correspondig to the middle slice
		    /*******************************************************************/


		    /*BEGIN PARALLEL FOR***********************************************/
				#pragma omp for
		        for(i = 0; i < cube_size; i++){
		    		if(middle == cube_size) // IF LAST ITERATION SET MIDDLE = 0 (WRAP of x)
		    			middle = 0;

                  	//List_aux keeps track of the live cells of a slice
		          	item *list_aux = list_init(), *aux = NULL;

					//If there are 3 consecutive empty slices there is no need to clean the middle one
					int safe_slice = 0;
					if(empty_slices > 2)
						safe_slice = 1;

		            /*first 3 slices are already filled, and when it wraps around
		    		slices 1 and 2 are already filled too*/
					if(i != first_iter[THREAD_ID] && i != RIGHT_LIM - 2 && i != RIGHT_LIM - 1){
		    			int inserted = insert_in_slice(dynamic_matrix[THREAD_SPACE(2)], hashtable, middle + 1);
						inserted ? empty_slices=0 : empty_slices++;
					}

					int checks_shared = 0; //boolean that sees if a thread is being helped
					if(threads_finished > 0 && thread_help == 0){
						#pragma omp critical (threads_finished)
						{
							checks_shared = (thread_help == 0);
							thread_help++; /*only one thread can be helped at a time
                            , but by various threads*/
						}
					}

                  	// Loop that sees what cells stay alive, and increments the dead cells
					while(hashtable->table[middle] != NULL){
		            	int count = 0; //counter to see what happens to a live cell
		    			aux = hash_first(hashtable, middle);
						if(checks_shared){ //if there are threads available to help this one, it enters here
							int this_thread = THREAD_ID;
							#pragma omp task shared(dead_to_live, dynamic_matrix, list_aux) firstprivate(count, this_thread)
							{	//checks neighbors of the live cell in aux
								check_neighbors(dynamic_matrix + N_SLICES*this_thread,
									dead_to_live + N_SLICES*this_thread, aux, &count, &check_entry_shared);

								//if cell stays alive goes to the temporary list
					    		if(count >= 2 && count <= 4)
									#pragma omp critical (list_aux)
					    			list_aux = list_push(list_aux, aux);
					    		else //else it dies, so doesn't stay in the hash table
					    			free(aux);
							}
						}else{
							//checks neighbors of the live cell in aux
			    			check_neighbors(dynamic_matrix + THREAD_SPACE(0),
								dead_to_live + THREAD_SPACE(0), aux, &count, &check_entry);

							//if cell stays alive goes to the temporary list
				    		if(count >= 2 && count <= 4)
				    			list_aux = list_push(list_aux, aux);
				    		else //else it dies, so doesn't stay in the hash table
				    			free(aux);
						}

		    		}
					//if being helped, waits for all its live cells to be processed
					#pragma omp taskwait

					#pragma omp critical (threads_finished)
                  	thread_help = 0;
		    		//inserts just the live cells that stayed alive
		    		hashtable->table[middle] = list_aux;
					list_aux = NULL;

					//insert dead cells that became live in hashtable (only if not in the first, second or last iteration)
		    		if(i != first_iter[THREAD_ID] && i != first_iter[THREAD_ID]+1 && i != RIGHT_LIM - 1){
		    			hashtable->table[middle - 1] = lists_concatenate(hashtable->table[middle - 1], dead_to_live[THREAD_SPACE(0)]);
		    			dead_to_live[THREAD_SPACE(0)] = NULL;
		    		} // in the last iteration inserts the remaining dead_to_live lists in the hashtable
		    		else if(i == RIGHT_LIM - 1){
		    			hashtable->table[RIGHT_LIM - 1] = lists_concatenate(hashtable->table[RIGHT_LIM - 1], dead_to_live[THREAD_SPACE(0)]);
		    			hashtable->table[RIGHT_WRAP] = lists_concatenate(hashtable->table[RIGHT_WRAP], dead_to_live[THREAD_SPACE(1)]);
		    			hashtable->table[RIGHT_WRAP + 1] = lists_concatenate(hashtable->table[RIGHT_WRAP + 1], dead_to_live[THREAD_SPACE(2)]);
		    			dead_to_live[THREAD_SPACE(0)] = NULL;
		    			dead_to_live[THREAD_SPACE(1)] = NULL;
		    			dead_to_live[THREAD_SPACE(2)] = NULL;
		    		}
					/*Saves the first and second slice of each thread, so another one can use them later*/
		    		else if(i == first_iter[THREAD_ID]){
		    			first_slice[THREAD_ID] = dynamic_matrix[THREAD_SPACE(0)];
		    			dynamic_matrix[THREAD_SPACE(0)] = malloc(cube_size * cube_size * sizeof(char));
		    			first_list[THREAD_ID] = dead_to_live[THREAD_SPACE(0)];
		    			dead_to_live[THREAD_SPACE(0)] = NULL;
						first_slice_finished[THREAD_ID] = 1; //thread says that it finished working with its first slice
		    		}
		    		else if(i == first_iter[THREAD_ID]+1){
		    			second_slice[THREAD_ID] = dynamic_matrix[THREAD_SPACE(0)];
		    			dynamic_matrix[THREAD_SPACE(0)] = malloc(cube_size * cube_size * sizeof(char));
		    			second_list[THREAD_ID] = dead_to_live[THREAD_SPACE(0)];
		    			dead_to_live[THREAD_SPACE(0)] = NULL;
						second_slice_finished[THREAD_ID] = 1; //thread says that it finished working with its second slice
		    		}

		    		//dead_to_live lists shift
		    		dead_to_live[THREAD_SPACE(0)] = dead_to_live[THREAD_SPACE(1)];
		    		dead_to_live[THREAD_SPACE(1)] = dead_to_live[THREAD_SPACE(2)];
		    		if(i == RIGHT_LIM - 3){
						//waits for next thread to finish its first slice processing
						while(!first_slice_finished[NEXT_THREAD]);
		    			dead_to_live[THREAD_SPACE(2)] = first_list[NEXT_THREAD];
		    		}else if(i == RIGHT_LIM - 2){
                      	//waits for next thread to finish its second slice processing
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
                      	//grabs next thread's first slice
		    			dynamic_matrix[THREAD_SPACE(2)] = first_slice[NEXT_THREAD];
		    			free(matrix_tmp);
		    		}else if(i == RIGHT_LIM - 2){
                      	//grabs next thread's second slice
		    			dynamic_matrix[THREAD_SPACE(2)] = second_slice[NEXT_THREAD];
		    			free(matrix_tmp);
		    		}else{
                        dynamic_matrix[THREAD_SPACE(2)] = matrix_tmp;
                        if(!safe_slice)//if the leftmost slice is safe(empty), it doesn't need to be cleaned
                        	SLICE_CLEAN(dynamic_matrix[THREAD_SPACE(2)]);
		    		}
		    		middle++; //goes on in the hashtable


					if(i == RIGHT_LIM-1){
						#pragma omp atomic
						threads_finished++; //thread anounces it has finished
					}
		    	}/*PARALLEL FOR LOOP FINISH*****************************************/
			for(n = 0; n < N_SLICES; n++)//clean of all slices
				SLICE_CLEAN(dynamic_matrix[THREAD_SPACE(n)]);
	    }

	}/*END OF PARALLEL SECTION*******************************************/
	free(first_iter);
	for(i = 0; i < N_SLICES*launched_threads; i++){
		free(dynamic_matrix[i]);
	}
}

/***********************************************************************
*Receives and changes array w, so that it has the first iteration for
*each thread
***********************************************************************/
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

/***********************************************************************
* usage function
***********************************************************************/
void usage(){
    printf("Usage: ./life3d <input file> <number of generations>\n");
}

/***********************************************************************
* Hashtable indexing function: h(x,y,z) = x
***********************************************************************/
int hashfunction (struct data k){
    return k.x;
}

/***********************************************************************
* Equal function for data K
***********************************************************************/
int equal_data(data K1, data K2){
	return K1.x == K2.x && K1.y == K2.y && K1.z == K2.z;
}

/***********************************************************************
* Print function for data K
***********************************************************************/
void print_data(data K){
	printf("%d %d %d\n", K.x, K.y, K.z);
}

/***********************************************************************
*This function inserts all live cells from a given X index of the hashtable
*to a slice.
*Return 1 if the entry of the hashtable had live cells.
*Return 0 if no insertion is made.
***********************************************************************/
int insert_in_slice(signed char * slice, hashtable_s *hashtable, int entry){
	item *aux;
	item *list_aux = NULL;
	int bool = 0;

	while(hashtable->table[entry]!=NULL){
		bool = 1;
		aux = hash_first(hashtable, entry);
		slice[MINDEX(aux->K.y, aux->K.z)] = -1;
		list_aux = list_push(list_aux, aux);
	}
	hashtable->table[entry] = list_aux;

	return bool;
}

/***********************************************************************
* Checks the 6 neighbors of one live cell and calls check_entry for each
*of them
***********************************************************************/
void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count, void (*check)(signed char*, item **, data, int*)){
	int x = node->K.x;
	int y = node->K.y;
	int z = node->K.z;
	data K = {.x = x, .y = y, .z = z};

	// x coordinate neighbor search
	K.x = (x != 0) ? x - 1 : cube_size - 1;
	check(&matrix[0][MINDEX(y,z)], &dead_to_live[0], K, count);
	K.x = (x != cube_size - 1) ? x + 1 : 0;
	check(&matrix[2][MINDEX(y,z)], &dead_to_live[2], K, count);
	K.x = x;

	// y coordinate neighbor search
	K.y = (y != 0) ? y - 1 : cube_size - 1;
	check(&matrix[1][MINDEX(K.y,z)], &dead_to_live[1], K, count);
	K.y = (y != cube_size-1) ? y + 1 : 0;
	check(&matrix[1][MINDEX(K.y,z)], &dead_to_live[1], K, count);
	K.y = y;

	// z coordinate neighbor search
	K.z = (z != 0) ? z - 1 : cube_size - 1;
	check(&matrix[1][MINDEX(y,K.z)], &dead_to_live[1], K, count);

	K.z = (z != cube_size-1) ? z + 1 : 0;
	check(&matrix[1][MINDEX(y,K.z)], &dead_to_live[1], K, count);
}

/***********************************************************************
* Receives one cell
* If it is death increases its entry, that keeps track of how many live
*cells are neighbors of the received death cell.
* If it is alive increases the count variable that keeps track of how
*many live cells are neighbors of that live cell
***********************************************************************/
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


/***********************************************************************
* Same as check_entry but is called when an available thread is
*helping another one
***********************************************************************/
void check_entry_shared(signed char *entry, item **dead_to_live, data K, int *count){

	if(*entry != -1){
      	//Only one thread must increase the entry at a time and change the dead_to_live list
		#pragma omp critical (check_entry)
		{
		(*entry)++;
			if(*entry == 2){
				*dead_to_live = list_append(*dead_to_live, K);
			}else if(*entry == 4){
				*dead_to_live = list_remove(*dead_to_live, K);
			}else if(*entry > 6){
				perror("Entry > 6\n");
				exit(-1);
			}
		}
	}else
		(*count)++;
}

/***********************************************************************
* Receives 2 elements of a list and sorts them in ascending order from x to z
***********************************************************************/
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
