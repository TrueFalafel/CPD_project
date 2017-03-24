#include "hashtable.h"

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
		hashtable->table[i] = list_init();
	}

	hashtable->size = size;
	hashtable->hash_function = hash_f;

	return hashtable;
}

int get_index(hashtable_s *hashtable, data K){
	int index = hashtable->hash_function(K);
	return index;
}

void hash_insert(hashtable_s *hashtable, data K){
	int index = get_index(hashtable, K);

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

item *hash_search(hashtable_s *hashtable, data K){
	item *aux;

	int index = get_index(hashtable, K);

	aux = list_search(hashtable->table[index], K);
	return aux;
}

void hash_remove(hashtable_s *hashtable, data K){
	int index = get_index(hashtable, K);

	hashtable->table[index] = list_remove(hashtable->table[index], K);
	return;
}

item* hash_first(hashtable_s *hashtable, int entry){
	return list_first(&hashtable->table[entry]);
}

void hash_free(hashtable_s *hashtable){
	int size = hashtable->size;
	int i;

	for(i=0;i<size;i++){
		list_free(hashtable->table[i]);
	}
	free(hashtable->table);
	free(hashtable);
}
