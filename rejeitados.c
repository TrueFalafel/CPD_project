//Leitura paralela do ficheiro!	
	hashtable_s *hashtable;
    data k;
    hashtable = hash_create( cube_size, &hashfunction);
	item *pool = list_init();
	int finished = 0;
	#pragma omp parallel sections
	{
		#pragma omp section
		{
		    while(fscanf(pf, "%d %d %d", &k.x, &k.y, &k.z) != EOF){
				#pragma omp critical (pool)
				{
		    		pool = list_append(pool, k);
				}
			}
			finished = 1;
		}
		#pragma omp section
		{
			item *aux = NULL;
			while(!finished){
				#pragma omp critical (pool)
				{
					aux = list_first(&pool);
				}
				if(aux != NULL){
					hash_insert(hashtable, aux);
				}
			}
		}
	}
/***************************************************************************/
