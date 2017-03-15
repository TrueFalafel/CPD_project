#ifndef _HASHTABLE_LIB_H_
#define _HASHTABLE_LIB_H_
#include <stdio.h>
#include <stdlib.h>

typedef struct hashtable_s hashtable_s;

int hash_function(data k);

int hash_getSize(hashtable_s hashtable);

hashtable_s *hash_create(int size);

#endif //_HASHTABLE_LIB_H_
