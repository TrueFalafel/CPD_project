#include <stdio.h>
#include <stdlib.h>
#include "../LinkedListLib/linked_list.h"
#define MAX 256

int main(int argc, char const *argv[]) {
	item *root;
	data k;
	char line[256];

	root = list_init();

	printf("Introduza célula\n");
	fgets(line, MAX, stdin);
	sscanf(line,"%d %d %d", &k.x, &k.y, &k.z);
	while(k.x != -1){
		root = list_append( root, k);
		printf("Introduza célula\n");
		fgets(line, MAX, stdin);
		sscanf(line,"%d %d %d", &k.x, &k.y, &k.z);
	}

	printf("Agora remova\n");
	fgets(line, MAX, stdin);
	sscanf(line,"%d %d %d", &k.x, &k.y, &k.z);
	while(k.x != -1){
		root = list_remove( root, k);
		printf("outra:\n");
		fgets(line, MAX, stdin);
		sscanf(line,"%d %d %d", &k.x, &k.y, &k.z);
	}

	printf("Imprimindo: \n");
	list_print(root);
	list_free(root);

	hash
	return 0;
}
