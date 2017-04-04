#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./LinkedListLib/linked_list.h"
#include "./HashTableLib/hashtable.h"

void usage();
int hashfunction (struct data k);

int main(int argc, char *argv[]){

	if(argc != 2){
		printf("Incorrect number of arguments\n");
	    usage();
	    exit(1);
	}
	FILE *read = fopen(argv[1], "r");
	if(!read){
		printf("File doesn't exist\n");
	    exit(1);
	}

	int size;
	data k;

	hashtable_s *hashtable;
	fscanf(read, "%d", &size);
	hashtable = hash_create( size, &hashfunction);
	while(fscanf(read, "%d %d %d", &k.x, &k.y, &k.z)!=EOF){
		hash_insert( hashtable, k);
	}
	fclose(read);

	int i;
	strsep(&argv[1], "/");
	char title[100] = "Densities/density_";
	strcat(title ,argv[1]);
	printf("%s\n", title);
	FILE *fd = fopen(title, "w");
	if(!fd){
		printf("Error creating file\n");
	    exit(1);
	}
	item *aux;
	for(i=0; i < size; i++){
		int count=0;
		while((aux = hash_first(hashtable, i))!=NULL){
			count ++;
			free(aux);
		}

		fprintf(fd, "Slice %d has %d elements: density = %0.2f\n", i, count, (float)count/(size*size));
	}
	fclose(fd);
}

/*##############################################################################################*/
/*##############################################################################################*/



void usage(){
    printf("Usage: ./density <input file>\n");
}

int hashfunction (struct data k){
    return k.x;
}

int equal_data(data K1, data K2){
	return K1.x == K2.x && K1.y == K2.y && K1.z == K2.z;
}

void print_data(data K){
	printf("%d %d %d\n", K.x, K.y, K.z);
}

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
