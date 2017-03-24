#include "linked_list.h"

item* list_init(){
	return NULL;
}

item* list_first(item** root){
	item *first;

	first = (*root);
	if(root!=NULL){
		(*root) = (*root)->next;
		first->next = NULL;
	}

	return first;
}

item* list_push(item* root, item* other){

	other->next = root;
	root = other;

	return root;
}

item* list_append(item* root, data K){
	item *new, *aux;

	new = (item*)malloc(sizeof(item));
	if(new == NULL){
		printf("Erro na alocação\n");
		exit(-1);
	}

	new->K = K;
	new->next = root;
	root = new;

	return root;
}

item* list_remove(item* root, data K){
	item *aux, *aux_seg;

	if(root == NULL){
		printf("Already an empty list!\n");
		return root;
	}

	aux = root;
	aux_seg = aux->next;
	if(equal_data(root->K, K)){
		root = root->next;
		free(aux);
	}else{
		while(!equal_data(aux_seg->K, K)){
			if(aux_seg->next == NULL){
				printf("No data K found in remove!\n");
				return root;
			}
			aux = aux->next;
			aux_seg = aux_seg->next;
		}
		aux->next = aux_seg->next;
		free(aux_seg);
	}
	return root;
}

item* list_search(item* root, data K){
	item *aux;

	if(root == NULL){
		printf("Search cancelled, empty list, returning NULL\n");
		return NULL;
	}

	aux = root;
	while(!equal_data(aux->K, K)){
		if(aux->next == NULL){
			printf("No data K found in search!\n");
			return NULL;
		}
		aux = aux->next;
	}

	if(aux == NULL){
		printf("Element not found, returning NULL\n");
		return NULL;
	}else{
		return aux;
	}

}

void list_free(item* root){
	item *aux;

	while(root != NULL){
		aux = root;
		root = root->next;
		free(aux);
	}
	return;
}

void list_print(item* root){
	item *aux;
	data k;

	aux = root;
	while(aux != NULL){
		k = aux->K;
		print_data(k);
		aux = aux->next;
	}
	return;
}

item* lists_concatenate(item* list1, item* list2){
	item* aux=NULL;

	aux = list1;
	while(aux->next != NULL)
		aux = aux->next;

	aux->next = list2;
	return list1;
}
