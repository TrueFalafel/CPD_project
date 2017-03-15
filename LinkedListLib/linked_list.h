#ifndef _LINKED_LIST_LIB_H_
#define _LINKED_LIST_LIB_H_
#include <stdio.h>
#include <stdlib.h>

/***Definir estrutura que contem dados de cada nó**/
struct data{
	int x;
	int y;
	int z;
};
/**************************************************/

struct item;

typedef struct data data;
typedef struct item item;

item* list_init();

item* list_append(item* root, data K);

item* list_remove(item* root, data K);

item* list_search(item* root, data K);

void list_free(item* root);

void list_print(item* root);

/***** funções abstratas (falta implementação)***/
data *set_data(int x, int y, int z);

int equal_data(data K1, data K2); //sucesso=1, insucesso=0

void print_data(data K);
/*************************************************/
#endif //_LINKED_LIST_LIB_H_
