#include <stdio.h>
#include <stdlib.h>
#include <math.h>

unsigned cube_size = 20;

void chunks_indexes(int *w, int n_chunks){
    int aux = n_chunks, sum = 0;
    for(int i = 0; i != n_chunks; i++){
        w[i] = sum;
        sum += (int)ceil(((double)(cube_size - sum)/aux--));
    }
}

int main(int argc, char *argv[])
{
    int p = 4;
    int chunk_index[p]; chunks_indexes(chunk_index, p);
    for(int i = 0; i < p; i++)
        printf("%d ", chunk_index[i]);
    putchar('\n');
    return 0;
}
