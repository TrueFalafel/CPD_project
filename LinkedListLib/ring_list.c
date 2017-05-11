#include "ring_list.h"
#include <omp.h>

item* list_init(){
	return NULL;
}

//Removes first element of the list
item* list_first(item** root){
	item *first;

	first = (*root);
	if((*root)==NULL){
		return first;
    }else if((*root)->next == (*root)){
        (*root) = NULL;
	}else{
        (*root) = (*root)->next;
		(*root)->prev = first->prev;
		first->prev->next = (*root);
	}
	first->next = NULL;
	first->prev = NULL;

	return first;
}

//Puts an element in the beginning of the list
item* list_push(item* root, item* other){

    if(root == NULL){
        root = other;
        root->next = root;
        root->prev = root;
    }else{
        other->prev = root->prev;
		root->prev->next = other;
    	other->next = root;
        root->prev = other;
    	root = other;
    }
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
	root = list_push(root, new);
	return root;
}


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
		root->prev = aux->prev;
		aux->prev->next = root;
		free(aux);
	}else{
		while(!equal_data(aux_seg->K, K)){
			if(aux_seg->next == root){
				perror("No data K found in remove!\n");
				exit(-1);
			}
			aux = aux->next;
			aux_seg = aux_seg->next;
		}
		aux->next = aux_seg->next;
		aux_seg->next->prev = aux;
		free(aux_seg);
	}
	return root;
}
//Removes element with data K from the list
/*item* list_remove(item* root, data K){

    if(root == NULL){
		perror("Already an empty list!\n");
		exit(-1);
	}

    if(root->next == root){
        if(equal_data(root->K, K)){
            free(root);
            return NULL;
        }else{
            perror("No data K found in remove(1 element)!\n");
            exit(-1);
        }
    }

    item *front = root, *back = root->prev;
    int front_found = 0, back_found = 0, not_found = 0;
    #pragma omp parallel sections shared(front, back, front_found, back_found, not_found)
    {
        #pragma omp section
        {
            while(1){
                if(!front_found && !back_found && !not_found){
                    if(equal_data(front->K, K)){
                        front_found = 1;
                    }else{
						//just on the front so it won't prevent from seeing all elements
						if(front != back){
                        	front = front->next;
							if(front == root){
								not_found = 1;
								break;
							}
						}else{
							//just on the front so it won't prevent from seeing all elements
							not_found = 1;
							break;
						}
                    }
                }else{
                    break;
                }
            }
        }
        #pragma omp section
        {
            while(1){
                if(!front_found && !back_found && !not_found){
                    if(equal_data(back->K, K)){
                        back_found = 1;
                    }else{
                        if(back != front && back->prev != front){
                            back = back->prev;
							if(back == root->prev){
								not_found = 1;
								break;
							}
						}else{
							break;
						}
                    }
                }else{
                    break;
                }
            }
        }
    }
    if(front_found)
        back = front;
    else if(back_found)
        front = back;
    else{
        perror("No data K found in remove (multiple elements)!\n");
        exit(-1);
    }

    back = back->prev;
    back->next = front->next;
    front->next->prev = back;
    free(front);
    return root;
}*/

//Search for item with data K in the list
/*Em principio ta MAL, só nao alterei pq nao usamos
item* list_search(item* root, data K){

    if(root == NULL){
		printf("Search cancelled, empty list, returning NULL\n");
		return NULL;
	}

    if(root->next == NULL){
        if(equal_data(root->K, K))
            return root;
        else{
            printf("Element not found, returning NULL\n");
    		return NULL;
        }
    }

    item *front = root, *back = root->prev;
    int front_found = 0, back_found = 0;
	#pragma omp parallel sections shared(front, back, front_found, back_found)
    {
        #pragma omp section
        {
            while(1){
                if(!front_found && !back_found){
                    if(equal_data(front->K, K)){
                        front_found = 1;
                    }else{
                        front = front->next;
                    }
                }else{
                    break;
                }
            }
        }
        #pragma omp section
        {
            while(1){
                if(!front_found && !back_found){
                    if(equal_data(back->K, K)){
                        back_found = 1;
                    }else{
                        if(back != front && back->prev != front)
                            back = back->prev;
                    }
                }else{
                    break;
                }
            }
        }
    }

    if(front_found){
        return front;
    }else if(back_found){
        return back;
    }else{
        printf("Element not found, returning NULL\n");
        return NULL;
    }
}
*/

//Free all the elements of a list
/*ACHO QUE NAO DÁ PARA PARALELO
void list_free(item* root){

    if(root == NULL){
		printf("Empty list already\n");
		return;
	}

    if(root->next == NULL){
        free(root);
        return;
    }

    item *tail = root->prev, *front = root, *back = tail;

    #pragma omp parallel sections shared(front, back, root, tail)
    {
        #pragma omp section
        {
            while(front != back && root != NULL){
                front = root;
                root = root->next;
                free(front);
            }
        }
        #pragma omp section
        {
            while(back != front && tail != NULL){
                back = tail;
                tail = tail->prev;
                free(back);
            }
        }
    }
}
*/

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
	item *aux;

	if(list1 == NULL && list2 == NULL){
		return NULL;
	}else if(list1 == NULL && list2 != NULL){
		return list2;
	}else if(list1 != NULL && list2 == NULL){
		return list1;
	}else{
	    list2->prev->next = list1;
		list1->prev->next = list2;
		aux = list1->prev;
	    list1->prev = list2->prev;
	    list2->prev = aux;
		return list1;
	}
}
//Recursively divide the list in half and sort the sublists
//Needs to be called in a parallel section
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

item* list_dering(item *root){

	if(root != NULL){
		root->prev->next = NULL;
		root->prev = NULL;
	}
	return root;
}
