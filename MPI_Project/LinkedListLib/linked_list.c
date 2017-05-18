#include "linked_list.h"

item* list_init(){
	return NULL;
}

//Removes first element of the list
item* list_first(item** root){
	item *first;

	first = (*root);
	if((*root)!=NULL){
		(*root) = (*root)->next;
		first->next = NULL;
	}else{
		printf("List already empty\n");
	}

	return first;
}

//Puts an element in the beginning of the list
item* list_push(item* root, item* other){

	other->next = root;
	root = other;

	return root;
}

//Creates an element and puts it in the beginning of the list
item* list_append(item* root, data K){
	item *new;

	new = (item*)malloc(sizeof(item));
	if(new == NULL){
		perror("Erro na alocação de new item\n");
		exit(-1);
	}

	new->K = K;
	new->next = root;
	root = new;

	return root;
}

//Removes element with data K from the list
item* list_remove(item* root, data K){
	item *aux, *aux_seg;

	if(root == NULL){
		perror("Already an empty list!\n");
		exit(-1);
	}

	aux = root;
	aux_seg = aux->next;
	if(equal_data(root->K, K)){
		root = root->next;
		free(aux);
	}else{
		while(!equal_data(aux_seg->K, K)){
			if(aux_seg->next == NULL){
				printf("[%d %d %d]\n", K.x, K.y, K.z);
				perror("No data K found in remove!\n");
				exit(-1);
			}
			aux = aux->next;
			aux_seg = aux_seg->next;
		}
		aux->next = aux_seg->next;
		free(aux_seg);
	}
	return root;
}

//Search for item with data K in the list
int list_search(item* root, data K){
	item *aux;

	if(root == NULL){
		//printf("Search cancelled, empty list, returning NULL\n");
		return 0;
	}

	aux = root;
	while(!equal_data(aux->K, K)){
		if(aux->next == NULL){
			//printf("No data K found in search!\n");
			return 0;
		}
		aux = aux->next;
	}

	if(aux == NULL){
		//printf("Element not found, returning NULL\n");
		return 0;
	}else{
		return 1;
	}

}

//Free all the elements of a list
void list_free(item* root){
	item *aux;

	while(root != NULL){
		aux = root;
		root = root->next;
		free(aux);
	}
	return;
}

//Print all the elements of a list
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

//Appends one list to the end of another list
item* lists_concatenate(item* list1, item* list2){
	item* aux=NULL;

	aux = list1;
	if(aux == NULL)
		list1 = list2;
	else{
		while(aux->next != NULL)
			aux = aux->next;
		aux->next = list2;
	}

	return list1;
}

int list_count_el(item *root){
	int i; item *aux = root;
	if(root == NULL)
		return 0;

	for(i = 0; aux != NULL; i++)
		aux = aux->next;
	return i;
}

//Recursively divide the list in half and sort the sublists
void list_sort(item** root){
	item* first_half;
	item* second_half;
	item* tmphead = *root;

	/*Empty list or with only one element*/
	if((tmphead == NULL) || (tmphead->next == NULL)){
		return;
	}

	/*create the 2 sublists*/
	list_split(tmphead, &first_half, &second_half);

	/*recursively sort the 2 sublists*/
	list_sort(&first_half);
	list_sort(&second_half);

	/*sort and merge the sublists together*/
	*root = sort(first_half, second_half);
	//return;
}

/*Divides the list in half
Uses 1 pointer that advances 2 elements (fast) and
1 pointer that only advances 1(slow)*/
void list_split(item* head, item** first_half, item** second_half){
	item* slow;
	item* fast;

	/*Empty list or with only one element*/
	if((head == NULL) || (head->next == NULL)){
		*first_half = head;
		*second_half = NULL;
	}else{
		slow = head;
		fast = head->next;

		while(fast != NULL){
			fast = fast->next;
			if(fast != NULL){
				slow = slow->next;
				fast = fast->next;
			}
		}

		/*slow is in before the element in the middle*/
		*first_half = head;
		*second_half = slow->next;
		slow->next = NULL;
	}
}
