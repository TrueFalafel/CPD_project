#include <stdio.h>
#include <stdlib.h>
#include "../LinkedListLib/linked_list.h"
#include "../HashTableLib/hashtable.h"

int hashfunction (struct data k){
    return k.x + k.y + k.z;
}

int main(){
    hashtable_s *hashtable;
    int i, size = 4;

    hashtable = hash_create(size, hashfunction);

    data K;

    K.x=0; K.y=1; K.z=1;
    hash_insert(hashtable, K);

    K.x=1; K.y=1; K.z=1;
    hash_insert(hashtable, K);

    K.x=1; K.y=1; K.z=0;
    hash_insert(hashtable, K);

    K.x=1; K.y=0; K.z=1;
    hash_insert(hashtable, K);

    K.x=0; K.y=0; K.z=0;
    hash_insert(hashtable, K);

    K.x=2; K.y=0; K.z=0;
    hash_insert(hashtable, K);

    K.x=0; K.y=1; K.z=0 ;
    hash_insert(hashtable, K);

    hash_print(hashtable);

    return 0;
}
