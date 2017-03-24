#include <stdio.h>
#include <stdlib.h>
#include "./LinkedListLib/linked_list.h"
#include "./HashTableLib/hashtable.h"
#include <omp.h>
#define N_SLICES 3
#define MIDDLE_SLICE 1

void usage();
int hashfunction (struct data k);
int mindex(int i, int j);
void insert_in_slice(signed char * slice, hashtable_s *hashtable, int entry);
void slice_clean(signed char * slice);
void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count);
void check_entry(signed char *entry, item **dead_to_live, item *node, int *count);

unsigned cube_size=0;

int main(int argc, char *argv[])
{
    double end, start = omp_get_wtime();
	int i=0, j=0;
    /*GET INPUT TEXT FILE, CHECK FOR ERRORS**************************************/
    if(argc != 3)
    {
        printf("Incorrect number of arguments\n");
        usage();
        exit(1);
    }

    FILE *pf;
    if(!(pf = fopen(argv[1], "r")))
    {
        printf("File couldn't be found\n");
        usage();
        exit(2);
    }
    /**************************************************************************/
    /*GET CUBE SIZE -> FIRST LINE OF INPUT TEXT FILE & NUMBER OF GENERATIONS***/
    unsigned n_generations = atoi(argv[2]);
    fscanf(pf, "%d", &cube_size);
	/* Matrix that runs through the cube*/
	signed char *dynamic_matrix[N_SLICES];
	for(i=0; i < N_SLICES; i++){
		dynamic_matrix[i] = (signed char *)calloc(cube_size * cube_size, sizeof(char));
	}

	dynamic_matrix[0][mindex(1,1)] = 1;
	printf("Teste: %d %d\n", dynamic_matrix[0][mindex(1,1)], dynamic_matrix[0][mindex(0,0)]);
    /**************************************************************************/

    /*GET ALL LIVE CELLS IN THE BEGINNING**************************************/
    hashtable_s *hashtable;
    data k;
    hashtable = hash_create( cube_size, &hashfunction);
    while(fscanf(pf, "%d %d %d", &k.x, &k.y, &k.z) != EOF){
    	hash_insert( hashtable, k);
	}
    /****************************************************************************/

	/****Cycle for generations****************************************************/
	int n=0;
	signed char * matrix_tmp=NULL;
	for(n=0 ; n < n_generations; n++){
		//first 3 slice insertions
		for(i=0; i < N_SLICES; i++){
			insert_in_slice(dynamic_matrix[i], hashtable, i);
		}

		/*lists that takes all the possible dead candidates to becaome live
		one for each slice*/
		item *dead_to_live[N_SLICES];
		for(j=0; j<N_SLICES; j++)
			dead_to_live[j] = list_init();

		for( i=0; i < cube_size; i++){
			int middle = 2; //keeps track of the hashlist correspondig to the middle slice
			item *list_aux = list_init(), *aux=NULL;

			int count;
			while(hashtable->table[middle] != NULL){
				count = 0;
				aux = hash_first(hashtable,middle);
				check_neighbors(dynamic_matrix, dead_to_live, aux, &count);
				//if cell stays alive goes to the temporary list
				if(count >= 2 && count <= 4)
					list_aux = list_push(list_aux, aux);
				//else it dies, so doesn't stay in the hash table
			}
			//inserts just the live cells that stayed alive
			hashtable->table[middle] = list_aux;
			//insert dead cells that become live in hashtable
			hashtable->table[middle-1] = lists_concatenate(hashtable->table[middle-1], dead_to_live[0]);

			/****************************
			ATENÇÃO à PRIMEIRA SLICE
			**********************/
			//dead_to_live lists shift
			dead_to_live[0] = dead_to_live[1];
			dead_to_live[1] = dead_to_live[2];
			dead_to_live[2] = list_init();
			//matrix shift
			matrix_tmp = dynamic_matrix[0];
			dynamic_matrix[0] = dynamic_matrix[1];
			dynamic_matrix[1] = dynamic_matrix[2];
			dynamic_matrix[2] = matrix_tmp;
			slice_clean(dynamic_matrix[2]);
		}

	}

    hash_print(hashtable);
    hash_free(hashtable);
    end = omp_get_wtime();
    fclose(pf);
    printf("Execution time: %e s\n", end - start); // PRINT IN SCIENTIFIC NOTATION
    exit(0);
}
/****************************END of MAIN******************************************/



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
	if(K1.x == K2.x && K1.y == K2.y && K1.z == K2.z)
		return 1;
	else
		return 0;
}

void print_data(data K){
	printf("%d %d %d\n", K.x, K.y, K.z);
}

int mindex(int i, int j){
	return i*cube_size + j;
}

void insert_in_slice(signed char * slice, hashtable_s *hashtable, int entry){
	item *aux;
	item *list_aux=NULL;

	while(hashtable->table[entry]!=NULL){
		aux = hash_first(hashtable, entry);
		slice[mindex(aux->K.y, aux->K.z)] = -1;
		list_aux = list_push(list_aux, aux);
	}
	hashtable->table[entry] = list_aux;
}

void slice_clean(signed char * slice){
	int i,j;

	for(i=0; i< cube_size; i++)
		for(j=0; j< cube_size; j++)
			slice[mindex(i,j)] = 0;
}

void check_neighbors(signed char **matrix, item **dead_to_live, item *node, int *count){
	int y = node->K.y;
	int z = node->K.z;

	check_entry(&matrix[0][mindex(y,z)], &dead_to_live[0], node, count);
	check_entry(&matrix[2][mindex(y,z)], &dead_to_live[2], node, count);
	check_entry(&matrix[1][mindex(y+1,z)], &dead_to_live[1], node, count);
	check_entry(&matrix[1][mindex(y-1,z)], &dead_to_live[1], node, count);
	check_entry(&matrix[1][mindex(y,z+1)], &dead_to_live[1], node, count);
	check_entry(&matrix[1][mindex(y,z-1)], &dead_to_live[1], node, count);
}

void check_entry(signed char *entry, item **dead_to_live, item *node, int *count){

	if(*entry==0){
			(*entry)++;
		if((*entry) == 2)
			(*dead_to_live) = list_push((*dead_to_live), node);
		else if((*entry) == 4)
			(*dead_to_live) = list_remove((*dead_to_live), node->K);
	}else{
		(*count)++;
	}
}
