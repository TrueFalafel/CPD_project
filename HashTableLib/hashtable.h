#ifndef _HASHTABLE_LIB_H_
#define _HASHTABLE_LIB_H_
#include <stdio.h>
#include <stdlib.h>
#include "../LinkedListLib/linked_list.h"

typedef struct hashtable_s hashtable_s;

int hash_getSize(hashtable_s *hashtable);

hashtable_s *hash_create(int size, int (*hash_f)(data));

void hash_insert(hashtable_s *hashtable, data K);

void hash_print(hashtable_s *hashtable);

#endif //_HASHTABLE_LIB_H_