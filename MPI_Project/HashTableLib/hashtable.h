#ifndef _HASHTABLE_LIB_H_
#define _HASHTABLE_LIB_H_
#include <stdio.h>
#include <stdlib.h>
#include "../LinkedListLib/linked_list.h"

struct hashtable_s {
	int size;
	struct item **table;
	int (*hash_function)(data);
};

typedef struct hashtable_s hashtable_s;

int hash_getSize(hashtable_s *hashtable);

hashtable_s *hash_create(int size, int (*hash_f)(data));

int get_index(hashtable_s *hashtable, data K);

void hash_insert(hashtable_s *hashtable, data K);

void hash_print(hashtable_s *hashtable);

void hash_print_chunk(hashtable_s *hashtable, int begin, int size);

//item *hash_search(hashtable_s *hashtable, data K);

void hash_remove(hashtable_s *hashtable, data K);

item* hash_first(hashtable_s *hashtable, int entry);

void hash_free(hashtable_s *hashtable);

void hash_sort(hashtable_s *hashtable);

void hash_sort_chunk(hashtable_s *hashtable, int begin, int size);

#endif //_HASHTABLE_LIB_H_
