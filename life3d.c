#include <stdio.h>
#include <stdlib.h>
#include "./LinkedListLib/linked_list.h"
#include "./HashTableLib/hashtable.h"
#include <omp.h>
#define N_SLICES 3

void usage();
int hashfunction (struct data k);
int mindex(int i, int j);

unsigned cube_size=0;

int main(int argc, char *argv[])
{
    double end, start = omp_get_wtime();
	int i=0;
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
	int *dynamic_matrix[N_SLICES];
	for(i=0; i < N_SLICES; i++){
		dynamic_matrix[i] = (int *)calloc(cube_size * cube_size, sizeof(int));
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
	for(n=0 ; n < n_generations; n++){


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
