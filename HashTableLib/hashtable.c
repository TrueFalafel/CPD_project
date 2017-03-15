#include "hashtable.h"
#include "linked_list.h"

struct hashtable_s {
	int size;
	item **table;
};

int hash_function(data k){
	return k.x
}

int hash_getSize(hashtable_s hashtable){
	return hashtable->size;
}

hashtable_s *hash_create(int size){

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

	return hashtable;
}
