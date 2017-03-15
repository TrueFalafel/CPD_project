#include "hashtable.h"
#include "../LinkedListLib/linked_list.h"

struct hashtable_s {
	int size;
	struct item **table;
	int (*hash_function)(data);
};

int hash_getSize(hashtable_s *hashtable){
	return hashtable->size;
}

hashtable_s *hash_create(int size, int(*hash_f)(data)){

	hashtable_s *hashtable = NULL;
	int i;

	if( size < 1 ){
		printf("Invalid hashtable size\n");
		return NULL;
	}
	//Alocação da estrutura da hashtable
	if( ( hashtable = malloc(sizeof(hashtable_s))) == NULL ) {
		printf("Error allocating hashtable structure\n");
		return NULL;
	}
	//Alocação da tabela da hashtable
	if( ( hashtable->table = malloc(sizeof(item*) * size)) == NULL ) {
		printf("Error allocating hashtable table\n");
		return NULL;
	}
	//Inicialização das entradas da hashtable
	for( i = 0; i < size; i++ ) {
		hashtable->table[i] = NULL;
	}

	hashtable->size = size;
	hashtable->hash_function = hash_f;

	return hashtable;
}

void hash_insert(hashtable_s *hashtable, data K){
	int index = hashtable->hash_function(K);

	if(index > hashtable->size-1){
		printf("Hashtable index out of bounds\n");
		exit(20);
	}
	hashtable->table[index] = list_append(hashtable->table[index], K);
	return;
}

void hash_print(hashtable_s *hashtable){
	int i;

	for(i=0; i < hashtable->size; i++){
		printf("Position %d:\n", i);
		list_print(hashtable->table[i]);
	}
	return;
}
