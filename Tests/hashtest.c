#include <stdio.h>
#include <stdlib.h>
#include "../LinkedListLib/linked_list.h"
#include "../HashTableLib/hashtable.h"
#define MAX 256

int hashfunction (struct data k){
    return k.x + k.y + k.z;
}

int main(){
    hashtable_s *hashtable;
    int i, size = 20;
    char line[256];

    hashtable = hash_create(size, hashfunction);

    data k;

    printf("Introduza célula\n");
	fgets(line, MAX, stdin);
	sscanf(line,"%d %d %d", &k.x, &k.y, &k.z);
	while(k.x != -1){
		hash_insert(hashtable, k);
		printf("Introduza célula\n");
		fgets(line, MAX, stdin);
		sscanf(line,"%d %d %d", &k.x, &k.y, &k.z);
	}

    hash_print(hashtable);
    list_sort(&hashtable->table[3]);
    hash_print(hashtable);

    printf("Agora remova\n");
	fgets(line, MAX, stdin);
	sscanf(line,"%d %d %d", &k.x, &k.y, &k.z);
	while(k.x != -1){
		hash_remove(hashtable, k);
		printf("outra:\n");
		fgets(line, MAX, stdin);
		sscanf(line,"%d %d %d", &k.x, &k.y, &k.z);
	}

    hash_print(hashtable);
    hash_free(hashtable);

    return 0;
}
