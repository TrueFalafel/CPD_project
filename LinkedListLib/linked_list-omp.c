#include "linked_list-omp.h"

item* list_init(){
	return NULL;
}

//Removes first element of the list
item* list_first(item** root){
	item *first;

	first = (*root);
	if(root==NULL){
        perror("List already empty\n");
		exit(-1);
    }else if((*root)->next == (*root)){
        (*root) = NULL;
	}else{
        (*root) = (*root)->next;
		(*root)->prev = first->prev;
		first->prev->next = (*root);
		first->next = NULL;
        first->prev = NULL;
	}
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

//Removes element with data K from the list
item* list_remove(item* root, data K){

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
}

//Search for item with data K in the list
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

//Free all the elements of a list
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
            tail = root->prev;
            while(back != front && tail != NULL){
                back = tail;
                tail = tail->prev;
                free(back);
            }
        }
    }
}

//Print all the elements of a list
void list_print(item* root){
	item *aux;
	data k;

	print_data(root->K);
	aux = root->next;
	while(aux != root){
		k = aux->K;
		print_data(k);
		aux = aux->next;
	}
	return;
}

//Appends one list to the end of another list
item* lists_concatenate(item* list1, item* list2){

    list2->prev->next = list1;
	list1->prev->next = list2;
    list1->prev = list2->prev;
    list2->prev = list1->prev->next;

	return list1;
}
//Recursively divide the list in half and sort the sublists
//Needs to be called in a parallel section
void list_sort(item** root){
	item* first_half;
	item* second_half;
	item* tmphead = *root;

	/*Empty list or with only one element*/
	if((tmphead == NULL) || (tmphead->next == tmphead)){
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
	printf("aiai\n");
	item* slow;
	item* fast;
	item* begin = head;

	/*Empty list or with only one element*/
	if((head == NULL) || (head->next == head)){
		*first_half = head;
		*second_half = NULL;
	}else{
		slow = head;
		fast = head->next;

		while(fast != begin){
			fast = fast->next;
			if(fast != begin){
				slow = slow->next;
				fast = fast->next;
			}
		}

		/*slow is before the element in the middle*/
		slow->next->prev = begin->prev;
		head->prev->next = slow->next;
		*second_half = slow->next;
		head->prev = slow;
		slow->next = head;
		*first_half = head;

	}
}
