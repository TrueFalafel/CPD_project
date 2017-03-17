#include <stdio.h>
#include <stdlib.h>
#include "./LinkedListLib/linked_list.h"
#include "./HashTableLib/hashtable.h"
#include <omp.h>

void usage();
int hashfunction (struct data k);
data set_data(int x, int y, int z);
int equal_data(data K1, data K2);
void print_data(data K);

int main(int argc, char *argv[])
{
    double end, start = omp_get_wtime();
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
    unsigned cube_size, n_generations = atoi(argv[2]);
    fscanf(pf, "%d", &cube_size);
    /**************************************************************************/

    /*GET ALL LIVE CELLS IN THE BEGINNING**************************************/
    hashtable_s *hashtable;
    data k;
    hashtable = hash_create( cube_size, &hashfunction);
    while(fscanf(pf, "%d %d %d", &k.x, &k.y, &k.z) != EOF){
    	hash_insert( hashtable, k);
	}
    /****************************************************************************/

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
