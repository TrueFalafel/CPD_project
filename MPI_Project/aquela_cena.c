// SEND SIZE OF SECOND HASH LIST TO P - 1
/*MPI_Sendrecv(&list_size, 1, MPI_INT, id - 1 != -1 ? id - 1 : p - 1,
            TAG + 6, &incoming_lsize, 1, MPI_INT, (id + 1)%p, TAG + 6,
            new_world, &status);
// CONVERT LIST TO VECTOR
data *dsend = NULL;
if(list_size){
    dsend = malloc(list_size * sizeof(data));
    int k = 0;
    while(hashtable->table[my_index + 1] != NULL)
        dsend[k++] = hash_first(hashtable, my_index + 1)->K;
}
// ALLOC MEMORY TO RECEIVE THE SECOND HASH LIST FROM P + 1
data *drecv = NULL;
if(incoming_lsize)
    drecv = malloc(incoming_lsize * sizeof(data));

if(!list_size && !incoming_lsize)
    ; // SKIP
else if(list_size && !incoming_lsize)
    MPI_Send(dsend, list_size, MPI_DATA, id - 1 != -1 ? id - 1 : p - 1, TAG + 7, new_world); // SEND
else if(!list_size && incoming_lsize){
    MPI_Recv(drecv, incoming_lsize, MPI_DATA, (id + 1)%p, TAG + 7, new_world, &status); // MPI_RECEIVE;
    // CONVERT VECTOR TO LIST
    hashtable->table[(my_index + my_size + 1)%cube_size] = NULL; //TODO free da lista em vez de meter a NULL
    for(int k = 0; k != incoming_lsize; k++)
        hash_insert(hashtable, drecv[k]);
}
else{ // SEND AND RECEIVE first_list IN VECTOR FORM
    MPI_Sendrecv(dsend, list_size, MPI_DATA,
            id - 1 != -1 ? id - 1 : p - 1, TAG + 7, drecv,
            incoming_lsize, MPI_DATA, (id + 1)%p, TAG + 7,
            new_world, &status);
    // CONVERT VECTOR TO LIST
    hashtable->table[(my_index + my_size + 1)%cube_size] = NULL; //TODO free da lista em vez de meter a NULL
    for(int k = 0; k != incoming_lsize; k++)
        hash_insert(hashtable, drecv[k]);
}
if(dsend != NULL)
    free(dsend);
if(drecv != NULL)
    free(drecv);*/
/******************************************************************************/
