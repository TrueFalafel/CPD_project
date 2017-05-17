#include <stdio.h>
#include <stdlib.h>
#include "./LinkedListLib/linked_list.h"
#include "./HashTableLib/hashtable.h"
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <unistd.h>

#define N_SLICES 3
#define MIDDLE_SLICE 1
#define TAG 1
#define MINDEX(i, j) (i + j*cube_size)
#define SLICE_CLEAN(slice) (memset(slice, 0, cube_size*cube_size))
#define WRAP(i) (i != -1 ? i : cube_size - 1)
#define NEXT_P(id) ((id+1)%p)
#define PREV_P(id) (id - 1 != -1 ? id - 1 : p - 1)
// SIMPLE DEBUGS
#define DEBUG do{printf("aqui?\n"); sleep(3); fflush(stdout);}while(0)
#define DEBUG_middle printf("middle = %d\n", middle)
#define DEBUG_cell(cell) (printf("cell = [%d, %d, %d]\n", cell.x, cell.y, cell.z))
#define DEBUG_gen printf("generation = %d\n", n_generations)
#define DEBUG_sizes printf("%d: incoming_lsize = %d , list_size = %d\n", id, incoming_lsize, list_size);

void usage();
int hashfunction (struct data k);
int insert_in_slice(signed char * slice, hashtable_s *hashtable, int entry);
void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count);
void check_entry(signed char *entry, item **dead_to_live, data K, int *count);
void compute_generations(hashtable_s *hashtable);
void chunks_indexes(int *, int);
data my_hash_index(data K, int my_index, int my_size);
data hash_index_revert(data K, int my_index);
void hash_revert(hashtable_s* hashtable, int my_index);

//Global variables
unsigned cube_size, n_generations;

int main(int argc, char *argv[]){
    /*GET INPUT TEXT FILE, CHECK FOR ERRORS**************************************/
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

    /**************************************************************************/
    MPI_Init(&argc, &argv);
    int id, p;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    // Obtain the group of processes in the world communicator
    MPI_Group world_group;
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    // Remove all unnecessary ranks
    int old_p = p;
    if(cube_size/N_SLICES < p) // CHECK IF N_SLICES * p > cube_size
        p = cube_size/N_SLICES;
    MPI_Group new_group;
    int *ranks = malloc(sizeof(int) * old_p == p ? 1 : old_p - p);
    for(int i = old_p, j = 0; i != p; i--, j++)
        ranks[j] = i - 1; // TAG RANKS TO NOT TO APPEAR IN NEW GROUP
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_excl(world_group, old_p - p, ranks, &new_group);
    // Create a new communicator
    MPI_Comm new_world;
    MPI_Comm_create(MPI_COMM_WORLD, new_group, &new_world);

    int my_index, my_size; // PROCESS FIRST INDEX ITERATION AND NUMBER OF ITERATIONS
    if(id >= p){
        free(ranks);
        MPI_Finalize();
        exit(0);
    }
    int chunk_index[p]; chunks_indexes(chunk_index, p);
    my_index = chunk_index[id];
    my_size = id + 1 != p ? chunk_index[id + 1] - chunk_index[id] : cube_size - chunk_index[id];
    // AFTER THIS ALL PROCESSES HAVE THEIR HASH SPACE

    hashtable_s *hashtable;
    data k;
    hashtable = hash_create(my_size+3, &hashfunction);
	/*GET ALL LIVE CELLS IN THE BEGINNING**************************************/
	// TODO todos lerem o ficheiro, cada um guardar so o que deve
	while(fscanf(pf, "%d %d %d", &k.x, &k.y, &k.z) != EOF){
        if(hashtable->hash_function(k) >= my_index && hashtable->hash_function(k) < (my_index+my_size+3)){
            k = my_hash_index(k, my_index, my_size);
            hash_insert(hashtable, k);
        }
        else if(id == p-1 && hashtable->hash_function(k) >= 0 && hashtable->hash_function(k) <= 2){
            k = my_hash_index(k, my_index, my_size);
            hash_insert(hashtable, k);
        }
    }
    /*if(id==3){
        hash_print(hashtable);
    }
    for(int middle =0; middle < my_size+3; middle++){
        while(hashtable->table[middle] != NULL){
            item* aux = hash_first(hashtable, middle);
            if(list_search(hashtable->table[middle],aux->K)){
                printf("REPEATED LIVE CELL!\n" );
                printf("ID: %d\n", id);
                DEBUG_cell(aux->K);
                DEBUG_middle;
            }

        }
    }
    MPI_Barrier(new_world);
    exit(1);*/
	fclose(pf);

	/*INITIATE PROCESSES*******************************************************/
    MPI_Status status;
    /*CREATE DATA STRUCT*******************************************************/
    MPI_Datatype MPI_DATA;
    MPI_Datatype type[3] = {MPI_INT, MPI_INT, MPI_INT};
    int blocklen[3] = {1, 1, 1};
    MPI_Aint disp[3];
    disp[0] = offsetof(data, x);
    disp[1] = offsetof(data, y);
    disp[2] = offsetof(data, z);

    MPI_Type_create_struct(3, blocklen, disp, type, &MPI_DATA);
    MPI_Type_commit(&MPI_DATA);

	/****Cycle for generations*************************************************/
	int i;
	signed char *matrix_tmp = NULL;
	// To save the first and second slices
	//signed char *first_slice, *second_slice;
	// Matrix that runs through the cube
	signed char *dynamic_matrix[N_SLICES];
	//item *first_list, *second_list;

	for(i = 0; i < N_SLICES; i++)
		dynamic_matrix[i] = calloc(cube_size * cube_size, sizeof(char));

    while(n_generations--){
		//DEBUG_gen;
        MPI_Barrier(new_world); //REQUIRED!!
		int empty_slices = 0;
		//first 3 slice insertions
		for(i = 0; i < N_SLICES; i++)
			insert_in_slice(dynamic_matrix[i], hashtable, i) ? empty_slices = 0 : empty_slices++;

		/*lists that takes all the possible dead candidates to become live
		one for each slice*/
		item *dead_to_live[N_SLICES];
		for(i = 0; i < N_SLICES; i++)
			dead_to_live[i] = list_init();

        int middle;
        //if(id == 3)
        //    printf("middle = %d my_size = %d\n", middle, my_size);
		for(i = 0; i < my_size + 2; i++){
			item *list_aux = list_init(), *aux = NULL;
			int count;
			//in the last iteration we should examine the first hash_list
			middle = i+1;
			//If there are 3 consecutive empty slices there is no need to clean the middle one
			int safe_slice = 0;
			if(empty_slices > 2)
				safe_slice = 1;
			/*first 3 slices are already filled, and when it wraps around
			slices 1 and 2 are already filled too*/
			if(i > 0 && i < my_size+1){
                insert_in_slice(dynamic_matrix[2], hashtable, (middle+1)) ? empty_slices = 0 : empty_slices++;
            }
            // Loop that sees what cells stay alive, and increments the dead cells
			while(hashtable->table[middle] != NULL){
				count = 0;
				aux = hash_first(hashtable, middle);
				if(list_search(hashtable->table[middle],aux->K)){
					printf("REPEATED LIVE CELL!\n" );
					DEBUG_cell(aux->K);
					DEBUG_middle;
				}
				//checks neighbors of the live cell in aux
				check_neighbors(dynamic_matrix, dead_to_live, aux, &count);
				//if cell stays alive goes to the temporary list
				if(count >= 2 && count <= 4)
					list_aux = list_push(list_aux, aux);
				else //else it dies, so doesn't stay in the hash table
					free(aux);
			}
			//inserts just the live cells that stayed alive
			hashtable->table[middle] = list_aux;
			list_aux = NULL;

            //insert dead cells that become live in hashtable
            if(i>1){
	           hashtable->table[WRAP(middle - 1)] = lists_concatenate(hashtable->table[WRAP(middle - 1)], dead_to_live[0]);
               dead_to_live[0] = list_init();
        	}
          // if(id == 3)
            //    list_print(hashtable->table[WRAP(middle - 1)]);

            //dead_to_live lists shift
            dead_to_live[0] = dead_to_live[1];
			dead_to_live[1] = dead_to_live[2];
			dead_to_live[2] = NULL;

			//matrix shift
			matrix_tmp = dynamic_matrix[0];
			dynamic_matrix[0] = dynamic_matrix[1];
			dynamic_matrix[1] = dynamic_matrix[2];
			dynamic_matrix[2] = matrix_tmp;
			if(!safe_slice) //if the leftmost slice is safe(empty), it doesn't need to be cleaned
		          SLICE_CLEAN(dynamic_matrix[2]);
			//goes on in the hashtable
            //middle++;
            //if(id == 3)
        //        printf("middle = %d\n", middle);
		}
        /*SEND HASH_LIST[middle - 1] to (id + 1)%p*****************************/
        /*RECEIVE HASH_LIST[middle] from (id + 1)%p or dead_to_live ? *********/
        //if(id == 3)
        //    printf("middle = %d\n", middle);
    //    exit(0);
        if(p > 1){
            int send_idx[3] = {my_size, my_size + 1, 2};
            int recv_idx[3] = {0, 1, my_size+2};
			//Destinies: next, next, previous processors
            int dest[3] = {NEXT_P(id), NEXT_P(id), PREV_P(id)};
			//Sources: previous, previous, next processors
            int source[3] = {PREV_P(id), PREV_P(id), NEXT_P(id)};
            for(int v = 0; v != sizeof send_idx/sizeof *send_idx; v++){
                int incoming_lsize=0, list_size = list_count_el(hashtable->table[send_idx[v]]);
                // SEND SIZE OF HASHLIST AND GET SIZE OF THE INCOMING HASHLIST
                MPI_Sendrecv(&list_size, 1, MPI_INT, dest[v], TAG + v,
                        &incoming_lsize, 1, MPI_INT, source[v], TAG + v,
                        new_world, &status);
                // CONVERT LIST TO VECTOR
                data *dsend = NULL;
                if(list_size){
                    dsend = malloc(list_size * sizeof(data));
                    int k = 0;
					//Vectorizes the hashlist
                    data K;
                    while(hashtable->table[send_idx[v]] != NULL){
                        K = hash_first(hashtable, send_idx[v])->K;
                        K = hash_index_revert(K, my_index);
                        dsend[k++] = K;
                    }
					//Refills the hashlist
					while(k--){
                        K = my_hash_index(dsend[k], my_index, my_size);
                        hash_insert(hashtable, K);
                    }
                }
                // ALLOC MEMORY TO RECEIVE THE INCOMING LIST IN VECTOR FORM
                data *drecv = NULL;
                if(incoming_lsize)
                    drecv = malloc(incoming_lsize * sizeof(data));

				//Decides whether it just sends, just receives or sends and receives
                if(!list_size && !incoming_lsize)
                    ; // SKIP
                else if(list_size && !incoming_lsize)
                    MPI_Send(dsend, list_size, MPI_DATA, dest[v], TAG + v + 1, new_world); // SEND
                else if(!list_size && incoming_lsize){
                    MPI_Recv(drecv, incoming_lsize, MPI_DATA, source[v], TAG + v + 1, new_world, &status); // RECEIVE
                    // CONVERT VECTOR TO HASHLIST
                    hashtable->table[recv_idx[v]] = NULL; //TODO free da lista em vez de meter a NULL
                    for(int k = 0; k != incoming_lsize; k++){
                        drecv[k] = my_hash_index(drecv[k], my_index, my_size);
                        hash_insert(hashtable, drecv[k]);
                    }
                }
                else{
                    MPI_Sendrecv(dsend, list_size, MPI_DATA, dest[v], TAG + v +1,
                                drecv, incoming_lsize, MPI_DATA, source[v], TAG + v+ 1,
                                new_world, &status);
                    // CONVERT VECTOR TO HASHLIST
                    hashtable->table[recv_idx[v]] = NULL; //TODO free da lista em vez de meter a NULL
                    for(int k = 0; k != incoming_lsize; k++){
                        drecv[k] = my_hash_index(drecv[k], my_index, my_size);
                        hash_insert(hashtable, drecv[k]);
                    }
                }
				/*if(id==0){
					printf("--%d--\n",id );
					list_sort(&hashtable->table[0]);
					list_print(hashtable->table[0]);
					printf("*******************\n" );
					fflush(stdout);
					//sleep(5);
				}
				MPI_Barrier(new_world);
				if(id==p-1){
					printf("--%d--\n",id );
					list_sort(&hashtable->table[0]);
					list_print(hashtable->table[0]);
					printf("*******************\n" );
					fflush(stdout);
					//sleep(5);
				}*/
                if(dsend != NULL)
                    free(dsend);
                if(drecv != NULL)
                    free(drecv);
            }
        }
		/******INFO SENT********************************/
		for(i = 0; i < N_SLICES; i++)
			SLICE_CLEAN(dynamic_matrix[i]);
		/*********END GENERATION**********************/
	}
    /*END PROCESSES AND PRINT HASH TABLE***************************************/
    for(i = 0; i < N_SLICES; i++)
        free(dynamic_matrix[i]);
    // All processes sort their own chunk
    hash_sort_chunk(hashtable, 0, my_size);
    // All processes print their own chunk iteratively
    for(i = 0; i < p; i++){
        if(id == i){
            hash_revert(hashtable, my_index);
            hash_print_chunk(hashtable, 0, my_size);
			fflush(stdout);
		}
        MPI_Barrier(new_world);
    }
    MPI_Finalize();
    hash_free(hashtable);
    exit(0);
}
/****************************END of MAIN****************************************/

/***********************************************************************
*Receives and changes array w, so that it has the first iteration for
*each thread
***********************************************************************/
void chunks_indexes(int *w, int n_chunks){
    int aux = n_chunks, sum = 0;
    for(int i = 0; i != n_chunks; i++){
        w[i] = sum;
        sum += (int)ceil(((double)(cube_size - sum)/aux--));
    }
}

/***********************************************************************
*Changes data so that it fits in each thread hashtable
***********************************************************************/
data my_hash_index(data K, int my_index, int my_size){
    data Y = K;
    if(K.x < my_index){
        Y.x = my_size + K.x;
    }else{
        Y.x -= my_index;
    }
    return Y;
}

/***********************************************************************
*Transforms data back to the global indexes
***********************************************************************/
data hash_index_revert(data K, int my_index){
    data Y = K;
    Y.x = (Y.x + my_index)%cube_size;

    return Y;
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
	if(K1.x == K2.x && K1.y == K2.y && K1.z == K2.z)
		return 1;
	else
		return 0;
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
	item *list_aux=NULL;
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
void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count){
	int x = node->K.x;
	int y = node->K.y;
	int z = node->K.z;
	data K = {.x = x, .y = y, .z = z};

	// x coordinate neighbor search
	if(x != 0)
		K.x = x - 1;
	else
		K.x = cube_size - 1;
	check_entry(&matrix[0][MINDEX(y,z)], &dead_to_live[0], K, count);
	if(x != cube_size-1)
		K.x = x + 1;
	else
		K.x = 0;
	check_entry(&matrix[2][MINDEX(y,z)], &dead_to_live[2], K, count);
	K.x = x;

	// y coordinate neighbor search
	if(y != 0)
		K.y = y - 1;
	else
		K.y = cube_size-1;
	check_entry(&matrix[1][MINDEX(K.y,z)], &dead_to_live[1], K, count);

	if(y != cube_size-1)
		K.y = y + 1;
	else
		K.y = 0;
	check_entry(&matrix[1][MINDEX(K.y,z)], &dead_to_live[1], K, count);

	K.y = y;

	// z coordinate neighbor search
	if(z != 0)
		K.z = z - 1;
	else
		K.z = cube_size-1;
	check_entry(&matrix[1][MINDEX(y,K.z)], &dead_to_live[1], K, count);

	if(z != cube_size-1)
		K.z = z + 1;
	else
		K.z = 0;
	check_entry(&matrix[1][MINDEX(y,K.z)], &dead_to_live[1], K, count);

}
/***********************************************************************
* Receives one cell
* If it is death increases its entry, that keeps track of how many live
*cells are neighbors of the received death cell.
* If it is alive increases the count variable that keeps track of how
*many live cells are neighbors of that live cell
***********************************************************************/
void check_entry(signed char *entry, item **dead_to_live, data K, int *count){

	if((*entry)!=-1){
		(*entry)++;
		if((*entry) == 2)
			(*dead_to_live) = list_append((*dead_to_live), K);
		if((*entry) == 4)
			(*dead_to_live) = list_remove((*dead_to_live), K);
		if((*entry) > 6){
			printf("Entry > 6\n");
			DEBUG_cell(K);
			exit(-1);
		}
	}else{
		(*count)++;
	}
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
            if(list1->K.z <= list2->K.z){
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

data set_data(int x, int y, int z){
	data K;
	K.x = x; K.y = y; K.z = z;
	return K;
}

void hash_revert(hashtable_s* hashtable, int my_index){
    item *aux;
    for(int i = 0; i < hashtable->size; i++){
        aux = hashtable->table[i];
        while(aux != NULL){
            aux->K = hash_index_revert(aux->K, my_index);
            aux = aux->next;
        }
    }
}
