#include <stdio.h>
#include <stdlib.h>
#include "linked_list.h"
#include <omp.h>

void usage(){
    printf("Usage: ./life3d <input file> <number of generations>\n");
}

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
    item *root;
    data k;
    root = list_init();
    while(fscanf(pf, "%d %d %d", &k.x, &k.y, &k.z) != EOF)
    root = list_append(root, k);
    /****************************************************************************/

    list_print(root);
    list_free(root);
    end = omp_get_wtime();
    fclose(pf);
    printf("Execution time: %e s\n", end - start); // PRINT IN SCIENTIFIC NOTATION
    exit(0);
}
