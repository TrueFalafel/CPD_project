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
#define MAX_SIZE 5000000
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
#define DEBUG_dtl(i) printf("ltd = %p dtl = %p\n", l_t_d[i], d_t_l[i])
#define DEBUG_id printf("proc id = %d\n", id)

void usage();
int hashfunction (struct data k);
int insert_in_slice(signed char * slice, hashtable_s *hashtable, int entry);
void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count, int my_size);
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

    /*INITIATE PROCESSES*******************************************************/
    MPI_Status status;
    /*CREATE DATA STRUCT*******************************************************/
    MPI_Datatype MPI_DATA;
    MPI_Datatype type[3] = {MPI_INT, MPI_INT, MPI_INT};
    int blocklen[3] = {1, 1, 1};
    MPI_Aint disp[3];
    // Get the offsets from the given field to the start of the struct
    disp[0] = offsetof(data, x);
    disp[1] = offsetof(data, y);
    disp[2] = offsetof(data, z);

    MPI_Type_create_struct(3, blocklen, disp, type, &MPI_DATA); // Create struct datatype that contains X Y and Z indexes
    MPI_Type_commit(&MPI_DATA); // Commit new datatype

    hashtable_s *hashtable;
    hashtable = hash_create(my_size+2, &hashfunction);
	/*GET ALL LIVE CELLS IN THE BEGINNING**************************************/
	//Check number of lines ia a file = second integer in file's name
    int dummy = 0, lines = 0;
    char mult;
    sscanf(argv[1], "%*[^0123456789]%d%*[^0123456789]%d%c", &dummy, &lines, &mult);
    //cell multiplier
    if(mult == 'k' || mult == 'K')
        lines = lines * 1000;
    if(mult == 'm' || mult == 'M')
        lines = lines * 1000000;
    if(mult == 'g' || mult == 'G')
        lines = lines * 1000000000;

    //Reduce buffer_size to prevent memory overflowing
    int buffer_size = lines;
    if(lines > MAX_SIZE)
        buffer_size = ceil(lines/2);
    if(lines > 2*MAX_SIZE)
        buffer_size = ceil(lines/4);

    int stored = 0;
    int sent = 0;
    data *buffer = malloc(buffer_size * sizeof(data));
    data k;
    while(1){
        if(id == 0){
            while(fscanf(pf, "%d %d %d", &k.x, &k.y, &k.z) != EOF){//read file
                buffer[stored++] = k;
                if(stored == buffer_size){
                    break;
                }
            }
            for(int i = stored; i < buffer_size; i++)
                buffer[i] = set_data(-1, -1, -1);
        }
        //Broadcast of the buffer
        MPI_Bcast(buffer, buffer_size, MPI_DATA, 0, new_world);

        //Store info in hashtable
        data final = {-1,-1,-1}; //check end
        int idx;
        for(idx = 0; idx < buffer_size && !equal_data(buffer[idx], final); idx++){
            if(p > 1){ //for more then one machine
                if(hashtable->hash_function(buffer[idx]) >= my_index-1 && hashtable->hash_function(buffer[idx]) < (my_index+my_size+1)){
                    buffer[idx] = my_hash_index(buffer[idx], my_index, my_size);
                    hash_insert(hashtable, buffer[idx]);
                }
                else if(id == p-1 && hashtable->hash_function(buffer[idx]) == 0){//last proc needs slice 0
                    buffer[idx] = my_hash_index(buffer[idx], my_index, my_size);
                    hash_insert(hashtable, buffer[idx]);
                }
                else if(id == 0 && hashtable->hash_function(buffer[idx]) == cube_size-1){//first proc needs slice cube_size-1
                    buffer[idx] = my_hash_index(buffer[idx], my_index, my_size);
                    hash_insert(hashtable, buffer[idx]);
                }
            }else{ //for one machine
                if(hashtable->hash_function(buffer[idx]) >= my_index-1 && hashtable->hash_function(buffer[idx]) < (my_index+my_size+1)){
                    buffer[idx] = my_hash_index(buffer[idx], my_index, my_size);
                    hash_insert(hashtable, buffer[idx]);
                }
                //buffer[idx].x already comes transformed by my_hash_index.
                //data with x = 0, has x = my_size + 1
                //data with x = my_size-1, has x = 0
                if(hashtable->hash_function(buffer[idx]) == 0){ //put x=my_size-1 in slice my_size
                    buffer[idx].x = my_size;
                    hash_insert(hashtable, buffer[idx]);
                }else if(hashtable->hash_function(buffer[idx]) == my_size+1){ //put x = 0 in slice 1
                    buffer[idx].x = 1;
                    hash_insert(hashtable, buffer[idx]);
                }
            }
        }

        if(sent*buffer_size + idx == lines){
            break;
        }
        sent++;
        stored = 0;
    }
    free(buffer);
	fclose(pf);
	/****Cycle for generations*************************************************/
	int i;
	signed char *matrix_tmp = NULL;

	// Matrix that runs through the cube
	signed char *dynamic_matrix[N_SLICES];
	//item *first_list, *second_list;

	for(i = 0; i < N_SLICES; i++)
		dynamic_matrix[i] = calloc(cube_size * cube_size, sizeof(char));

    //first and last dead to live lists
    item *d_t_l[2];
    //first and last live to dead lists
    item *l_t_d[2];

    while(n_generations--){
		for(int i=0; i < 2; i++){
			l_t_d[i] = list_init();
			d_t_l[i] = list_init();
		}
        MPI_Barrier(new_world); //REQUIRED!!
		int empty_slices = 0;
		//first 3 slice insertions
		for(i = 1; i < N_SLICES; i++)
			insert_in_slice(dynamic_matrix[i], hashtable, i-1) ? empty_slices = 0 : empty_slices++;

		/*lists that takes all the possible dead candidates to become live
		one for each slice*/
		item *dead_to_live[N_SLICES];
		for(i = 0; i < N_SLICES; i++)
			dead_to_live[i] = list_init();

        int middle;

		for(i = 0; i < my_size + 2; i++){
			item *list_aux = list_init(), *aux = NULL;
			int count;

			middle = i;
			//If there are 3 consecutive empty slices there is no need to clean the middle one
			int safe_slice = 0;
			if(empty_slices > 2)
				safe_slice = 1;

            //In the last slice there's no need to fill the right slice
			if(i > 0 && i < my_size+1){
                insert_in_slice(dynamic_matrix[2], hashtable, (middle+1)) ? empty_slices = 0 : empty_slices++;
            }
            // Loop that sees what cells stay alive, and increments the dead cells
			while(hashtable->table[middle] != NULL){
				count = 0;
				aux = hash_first(hashtable, middle);

				//checks neighbors of the live cell in aux
				check_neighbors(dynamic_matrix, dead_to_live, aux, &count, my_size);

                //In the limit slices it doesn't check for the live ones
				if(middle == 0 || middle == my_size + 1){
					list_aux = list_push(list_aux, aux);
				//if cell stays alive goes to the temporary list
				}else if(count >= 2 && count <= 4){
					list_aux = list_push(list_aux, aux);
				}else{ //else it dies, so doesn't stay in the hash table
                    //in the first and last slices, the live_to_dead should be kept
                    if(middle == 1){
                        l_t_d[0] = list_push(l_t_d[0], aux);
                    }else if(middle == my_size){
                        l_t_d[1] = list_push(l_t_d[1], aux);
                    }else
                        free(aux);
                }
			}//CHECK GOOD 1
			//inserts just the live cells that stayed alive
			hashtable->table[middle] = list_aux;
			list_aux = NULL;

			//When first slice's or last slice's dead_to_live is complete
			//It is transfered to d_t_l be sent
            if(middle == 2)
                d_t_l[0] = dead_to_live[0];
            else if(middle == my_size+1)
                d_t_l[1] = dead_to_live[0];

            //insert dead cells that become live in hashtable
            if(i > 1){
	           hashtable->table[middle - 1] = lists_concatenate(hashtable->table[middle - 1], dead_to_live[0]);
               dead_to_live[0] = list_init();
        	}

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
		}//end for hashlist

        if(p > 1){
            int n_coms = 2; //number of comunications
			//Destinies: previous, next processors
            int dest[2] = {PREV_P(id), NEXT_P(id)};
			//Sources: next, previous processors
            int source[2] = {NEXT_P(id), PREV_P(id)};

            for(int v = 0; v != n_coms; v++){
                int incoming_lsize[2]={0,0};
                int list_size[2];

                list_size[0] = list_count_el(l_t_d[v]);
                list_size[1] = list_count_el(d_t_l[v]);
                int total_send_size = list_size[0] + list_size[1];

                // SEND SIZE OF LISTS
                MPI_Sendrecv(list_size, 2, MPI_INT, dest[v], TAG + v,
                        incoming_lsize, 2, MPI_INT, source[v], TAG + v,
                        new_world, &status);

                // CONVERT LIST TO VECTOR
                data *dsend = NULL;//Vector to receive list
                if(total_send_size){
                    dsend = malloc(total_send_size * sizeof(data));

					//Vectorizes the list
                    item* list = lists_concatenate(l_t_d[v], d_t_l[v]);
					item* aux = list;
                    int k = 0;
                    while(aux != NULL){
                        dsend[k++] = aux->K;
                        //Separate l_t_d from d_t_l so l_t_d can be freed later
						if(k == list_size[0]){
							item* splitter = aux;
							aux = aux->next;
							splitter->next = NULL;
						}else
							aux = aux->next;
                    }
                }

                // ALLOC MEMORY TO RECEIVE THE INCOMING LIST IN VECTOR FORM
                data *drecv = NULL;
                int total_recv_size = incoming_lsize[0] + incoming_lsize[1];
                if(total_recv_size)
                    drecv = malloc(total_recv_size * sizeof(data));

				//Decides whether it just sends, just receives or sends and receives
                if(!total_send_size && !total_recv_size)
                    ; // SKIP
                else if(total_send_size && !total_recv_size)
                    MPI_Send(dsend, total_send_size, MPI_DATA, dest[v], TAG + v + 2, new_world); // SEND
                else if(!total_send_size && total_recv_size){
                    MPI_Recv(drecv, total_recv_size, MPI_DATA, source[v], TAG + v + 2, new_world, &status); // RECEIVE

                    for(int k = 0; k < incoming_lsize[0]; k++){ //Remove l_t_d from hashtable
                        //transform indexes to the receiver thread hashtable
                        if(drecv[k].x == 1)
							drecv[k].x = my_size+1;
						else
							drecv[k].x = 0;
                        hash_remove(hashtable, drecv[k]);
                    }

                    for(int k = incoming_lsize[0]; k < total_recv_size; k++){//Insert d_t_l in hashtable
                        //transform indexes to the receiver thread hashtable
                        if(drecv[k].x == 1)
							drecv[k].x = my_size+1;
						else
							drecv[k].x = 0;
                        hash_insert(hashtable, drecv[k]);
                    }
                }
                else{
                    MPI_Sendrecv(dsend, total_send_size, MPI_DATA, dest[v], TAG + v + 2,
                                drecv, total_recv_size, MPI_DATA, source[v], TAG + v + 2,
                                new_world, &status);

                    for(int k = 0; k < incoming_lsize[0]; k++){ //Remove l_t_d from hashtable
                        //transform indexes to the receiver thread hashtable
                        if(drecv[k].x == 1)
							drecv[k].x = my_size+1;
						else
                            drecv[k].x = 0;
                        hash_remove(hashtable, drecv[k]);
                    }

                    for(int k = incoming_lsize[0]; k < total_recv_size; k++){//Insert d_t_l in hashtable
                        //transform indexes to the receiver thread hashtable
                        if(drecv[k].x == 1)
							drecv[k].x = my_size+1;
						else
							drecv[k].x = 0;
                        hash_insert(hashtable, drecv[k]);
                    }
                }
                if(dsend != NULL){
                    /*Free live to dead cells. Do not free dead to live cells
                    because they are appended in the hashtable*/
					if(l_t_d[v] != NULL)
						list_free(l_t_d[v]);
                    free(dsend);
				}
                if(drecv != NULL){
                    free(drecv);
				}
            }
        }else{ //for just one thread, copy the boundary slices
            list_free(hashtable->table[my_size+1]);
            list_free(hashtable->table[0]);
            hashtable->table[my_size+1] = list_copy_and_change(hashtable->table[1], my_size);
            hashtable->table[0] = list_copy_and_change(hashtable->table[my_size], my_size);
        }
		/******INFO SENT********************************/
		for(i = 0; i < N_SLICES; i++)
			SLICE_CLEAN(dynamic_matrix[i]);
		/*********END GENERATION**********************/
	}
    MPI_Barrier(new_world);
    /*END PROCESSES AND PRINT HASH TABLE***************************************/
    for(i = 0; i < N_SLICES; i++)
        free(dynamic_matrix[i]);
    // All processes sort their own chunk
    hash_sort_chunk(hashtable, 1, my_size);
    hash_revert(hashtable, my_index);

    /*To print the final output, all machines (in order) have to send
    * their hashtable to the "master" macine. The "master" have to print their
    * hashtables as it receives them*/
    if(id == 0){
        hash_print_chunk(hashtable, 1, my_size);
        hash_free(hashtable);
        int tsize;
        data *hrecv = NULL;
        for(int j = 1; j < p; j++){
            MPI_Recv(&tsize, 1, MPI_INT, j, TAG + j, new_world, &status);
            if(tsize){
                hrecv = malloc(tsize * sizeof(data));
                MPI_Recv(hrecv, tsize, MPI_DATA, j, TAG + j + 1, new_world, &status);
                for(int n = 0; n < tsize; n++)
                    print_data(hrecv[n]);
            }
            free(hrecv);
        }
        fflush(stdout);
    }else{
        int hsize = 0;
        data *hsend = NULL;
        for(int j = 1; j < my_size+1; j++){
            hsize += list_count_el(hashtable->table[j]); //Count all the live cells in the hashtable
        }
        hsend = malloc(hsize * sizeof(data));
        for(int j = 1, l = 0; j < my_size+1; j++){
            while(hashtable->table[j] != NULL){
                hsend[l++] = hash_first(hashtable, j)->K;
            }
        }
        hash_free(hashtable);
        MPI_Send(&hsize, 1, MPI_INT, 0, TAG + id, new_world);
        if(hsize)
            MPI_Send(hsend, hsize, MPI_DATA, 0, TAG + id + 1, new_world);
        free(hsend);
    }
    MPI_Finalize();
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

    if(K.x == WRAP(my_index-1))//last slice of prev proc
        Y.x = 0;
    else if(K.x == (my_index + my_size)%cube_size) //first slice of next proc
        Y.x = my_size + 1;
    else
        Y.x = K.x - my_index + 1;
    return Y;
}

/***********************************************************************
*Transforms data back to the global indexes
***********************************************************************/
data hash_index_revert(data K, int my_index){
    data Y = K;
    Y.x = WRAP((Y.x + my_index-1)%cube_size);

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
void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count, int my_size){
	int x = node->K.x;
	int y = node->K.y;
	int z = node->K.z;
	data K = {.x = x, .y = y, .z = z};

    if(x >= 1){
        K.x = x - 1;
        check_entry(&matrix[0][MINDEX(y,z)], &dead_to_live[0], K, count);
    }
    if(x <= my_size){
        K.x = x + 1;
        check_entry(&matrix[2][MINDEX(y,z)], &dead_to_live[2], K, count);
    }
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

/***********************************************************************
* Sets the coordinates of a struct to the input coordinates
***********************************************************************/
data set_data(int x, int y, int z){
	data K;
	K.x = x; K.y = y; K.z = z;
	return K;
}

/***********************************************************************
* Reverts the coordinates of each cell in an hashtable to the global
* coordinates
***********************************************************************/
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

/***********************************************************************
* Reverts the coordinates of each cell in a list to the global coodinates
***********************************************************************/
void list_revert(item *list, int my_index){
    item *aux = list;
    while(aux != NULL){
        aux->K = hash_index_revert(aux->K, my_index);
        aux = aux->next;
    }
}
