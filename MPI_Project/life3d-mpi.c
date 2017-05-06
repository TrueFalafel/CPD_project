#include <stdio.h>
#include <stdlib.h>
#include "./LinkedListLib/linked_list.h"
#include "./HashTableLib/hashtable.h"
#include <string.h>
#define N_SLICES 3
#define MIDDLE_SLICE 1
#define TAG 1
#define MINDEX(i, j) (i + j*cube_size)
#define SLICE_CLEAN(slice) (memset(slice, 0, cube_size*cube_size))
#define CHUNK_SIZE(id, p) (id + 1 == p ? cube_size - chunk_index[id] :
                                chunk_index[id + 1] - chunk_index[id])

void usage();
int hashfunction (struct data k);
int insert_in_slice(signed char * slice, hashtable_s *hashtable, int entry);
void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count);
void check_entry(signed char *entry, item **dead_to_live, data K, int *count);
void compute_generations(hashtable_s *hashtable);
void chunks_indexes(int *, int);
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

    hashtable_s *hashtable;
    data k;
    hashtable = hash_create(cube_size, &hashfunction);
	/*GET ALL LIVE CELLS IN THE BEGINNING**************************************/
	while(fscanf(pf, "%d %d %d", &k.x, &k.y, &k.z) != EOF)
    	hash_insert(hashtable, k);
	fclose(pf);

	/*INITIATE PROCESSES*******************************************************/
    MPI_Status status;
    int id, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    int my_index, my_size; // PROCESS FIRST INDEX ITERATION AND NUMBER OF ITERATIONS

    if(!id){
        int old_p = p;
        if(cube_size/N_SLICES < p) // CHECK IF N_SLICES * p > cube_size
            p = cube_size/N_SLICES;
        int *ranks = malloc(sizeof(int) * old_p == p ? 1 : old_p - p); // 1...N-1 PROCCESSES TO EXCLUDE

        for(int i = old_p, j = 0; i != p; i--, j++)
            ranks[j] = i - 1; // TAG RANKS TO NOT TO APPEAR IN NEW GROUP

        MPI_Group new_world; // CREATE NEW GROUP EXCLUDING PROCESSES IF NECESSARY
        MPI_Group_excl(MPI_COMM_WORLD, old_p - p, ranks, &new_world);
        free(ranks);

        //FIRST PROCESS HOLDS THE CHUNKS INDEXES TO ALL OTHERS
        int chunk_index[p]; chunks_indexes(chunk_index, p);
        my_index = chunk_index[id]; my_size = CHUNK_SIZE(id, p);
        if(p != 1) // IF THERE IS MORE THAN 1 PROCESS SEND CHUNKS INFO TO OTHER PROCESSES
            for(int i = 1; i != p; i++)
                if(i != p - 1)
                    MPI_Send(chunk_index + i, 2, MPI_INT, i, TAG, new_world);
                else{
                    int aux[2] = {chunk_index[i], chunk_index[i] + CHUNK_SIZE(i, p)};
                    MPI_Send(aux, 2, MPI_INT, i, TAG, new_world);
                }
    }
    else{
        int aux[2];
        MPI_Recv(aux, 2, MPI_INT, 0, TAG, new_world, &status);
        my_index = aux[1];
        my_size = aux[2] - aux[1];
    }
    // AFTER THIS ALL PROCESSES HAVE THEIR HASH SPACE

	/****Cycle for generations*************************************************/
	int i;
	signed char *matrix_tmp = NULL;
	// To save the first and second slices
	signed char *first_slice, *second_slice;
	// Matrix that runs through the cube
	signed char *dynamic_matrix[N_SLICES];
	item *first_list, *second_list;

	for(i=0; i < N_SLICES; i++)
		dynamic_matrix[i] = (signed char *)calloc(cube_size * cube_size, sizeof(char));

    while(1){
        if(!id) //Only master thread decreases the generation counter
            n_generations--;

        MPI_Barrier(new_world);
        if(n_generations == -1)
            break;

		int empty_slices = 0;
		//first 3 slice insertions
		for(i = 0; i < N_SLICES; i++){
			int inserted = insert_in_slice(dynamic_matrix[i], hashtable, my_index + i);
			inserted ? empty_slices = 0 : empty_slices++;
		}

		/*lists that takes all the possible dead candidates to become live
		one for each slice*/
		item *dead_to_live[N_SLICES];
		for(i = 0; i < N_SLICES; i++)
			dead_to_live[i] = list_init();

		int middle = my_index + 1; //keeps track of the hashlist correspondig to the middle slice
		for(i = 0; i < my_size; i++){
			item *list_aux = list_init(), *aux = NULL;

			int count;
			//in the last iteration we should examine the first hash_list
			if(middle == cube_size)
				middle = 0;

			//If there are 3 consecutive empty slices there is no need to clean the middle one
			int safe_slice = 0;
			if(empty_slices > 2)
				safe_slice = 1;
			/*first 3 slices are already filled, and when it wraps around
			slices 1 and 2 are already filled too*/
			if(i!=0 && i!=my_size-2 && i!=my_size-1){
				int inserted = insert_in_slice(dynamic_matrix[2], hashtable, middle+1);
				inserted ? empty_slices = 0 : empty_slices++;
			}
			// Loop that sees what cells stay alive, and increments the dead cells
			while(hashtable->table[middle] != NULL){
				count = 0;
				aux = hash_first(hashtable, middle);
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
			//insert dead cells that become live in hashtable (only if not in the first or second iteration)
			if(i!=0 && i!=1 && i!=my_size-1){
				hashtable->table[middle-1] = lists_concatenate(hashtable->table[middle-1], dead_to_live[0]);
				dead_to_live[0] = NULL;
			}
			if(i == my_size-1){
				hashtable->table[my_size-1] = lists_concatenate(hashtable->table[my_size-1], dead_to_live[0]);
				hashtable->table[0] = lists_concatenate(hashtable->table[0], dead_to_live[1]);
				hashtable->table[1] = lists_concatenate(hashtable->table[1], dead_to_live[2]);
				dead_to_live[0] = NULL;
				dead_to_live[1] = NULL;
				dead_to_live[2] = NULL;
			}

			//Save the first and the second slice and dead_to_live lists
            // Pass the information to the next process
            if(i==0){
				first_slice = dynamic_matrix[0];
				dynamic_matrix[0] = (signed char *)malloc(cube_size * cube_size * sizeof(char));
				first_list = dead_to_live[0];
                dead_to_live[0] = NULL;
			}
			if(i==1){
				second_slice = dynamic_matrix[0];
				dynamic_matrix[0] = (signed char *)malloc(cube_size * cube_size * sizeof(char));
				second_list = dead_to_live[0];
				dead_to_live[0] = NULL;
			}

			//dead_to_live lists shift
			dead_to_live[0] = dead_to_live[1];
			dead_to_live[1] = dead_to_live[2];
			if(i == my_size - 3){
				dead_to_live[2] = first_slice;
			}else if(i == my_size - 2){
				dead_to_live[2] = second_slice;
			}else{
				dead_to_live[2] = list_init();
			}
			//matrix shift
			matrix_tmp = dynamic_matrix[0];
			dynamic_matrix[0] = dynamic_matrix[1];
			dynamic_matrix[1] = dynamic_matrix[2];
			if(i == my_size - 3){
				//grabs first slice
				dynamic_matrix[2] = first_slice;
				free(matrix_tmp);
			}else if(i == my_size - 2){
				//grabs second slice
				dynamic_matrix[2] = second_slice;
				free(matrix_tmp);
			}else{
				dynamic_matrix[2] = matrix_tmp;
				if(!safe_slice)//if the leftmost slice is safe(empty), it doesn't need to be cleaned
					SLICE_CLEAN(dynamic_matrix[2]);
			}
			//goes on in the hashtable
			middle++;

		}

		for(i = 0; i < N_SLICES; i++)
			SLICE_CLEAN(dynamic_matrix[i]);
	}
    /*END PROCESSES AND PRINT HASH TABLE***************************************/
    for(i = 0; i < N_SLICES; i++)
        free(dynamic_matrix[i]);
    MPI_Finalize();
	hash_sort(hashtable);
    hash_print(hashtable);
    hash_free(hashtable);
    exit(0);


}
/****************************END of MAIN****************************************/

/***********************************************************************
*Receives and changes array w, so that it has the first iteration for
*each thread
***********************************************************************/
void chunks_indexes(int *w, int n_chunks){
    int aux = n_chunks, size = cube_size, sum = 0;
    for(int i = 0; i != n_chunks; i++){
        w[i] = sum;
        sum += (int)ceil(((double)size/aux--));
        size -= sum;
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
			perror("Entry > 6\n");
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
