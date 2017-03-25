COMPFLAGS = gcc -Wall -g -ggdb -c -o
LINKFLAGS = gcc -Wall -o
#Fazer link flags diferentes pra omp (-fopenmp)
LIBFLAGS = ar -cvq
LL_LIB_PATH = ./LinkedListLib
HT_LIB_PATH = ./HashTableLib
IN_FILES_PATH = ./InputFiles
#FAZER make PARA COMPILAR O PROGRAMA############################################
life3d: life3d.o $(LL_LIB_PATH)/linked_list.o $(HT_LIB_PATH)/hashtable.o
	$(LINKFLAGS) $@ $^ 

life3d.o: life3d.c $(LL_LIB_PATH)/linked_list.h $(HT_LIB_PATH)/hashtable.h
	$(COMPFLAGS) $@ $<
################################################################################
#FAZER make linked_list_lib PARA CRIAR A BIBLIOTECA#############################
linked_list_lib: $(LL_LIB_PATH)/linked_list.o
	$(LIBFLAGS) $(LL_LIB_PATH)/lib_linked_list.a $<

linked_list.o: $(LL_LIB_PATH)/linked_list.c $(LL_LIB_PATH)/linked_list.h
	$(COMPFLAGS) $(LL_LIB_PATH)/$@ $<
################################################################################
#FAZER make hash_table_lib PARA CRIAR A BIBLIOTECA##############################
hash_table_lib: $(HT_LIB_PATH)/hashtable.o
	$(LIBFLAGS) $(HT_LIB_PATH)/lib_hash_table.a $<

hashtable.o: $(HT_LIB_PATH)/hashtable.c $(HT_LIB_PATH)/hashtable.h
	$(COMPFLAGS) $(HT_LIB_PATH)/$@ $<
################################################################################
#FAZER make serial PARA CORRER O PROGRAMA ######################################
run:
	./life3d $(IN_FILES_PATH)/simple.in 1
################################################################################
#FAZER make clean PARA LIMPAR APENAS OS FICHEIROS DO PROGRAMA###################
clean:
	rm life3d *.o
#FAZER make clean_all PARA LIMPAR APP FILES E A BIBLIOTECA######################
clean_all:
	rm life3d *.o  $(LL_LIB_PATH)/*.o $(LL_LIB_PATH)/lib_linked_list.a $(HT_LIB_PATH)/*.o $(HT_LIB_PATH)/lib_hash_table.a
################################################################################
