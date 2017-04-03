COMPFLAGS = gcc -Wall -g -ggdb -c -o
COMPFLAGS_OPENMP = gcc -Wall -g -fopenmp -ggdb -c -o
LINKFLAGS = gcc -Wall -o
LINKFLAGS_OPENMP = gcc -Wall -fopenmp -o

LL_LIB_PATH = ./LinkedListLib
HT_LIB_PATH = ./HashTableLib

#FAZER make PARA COMPILAR e LINKAR VERSAO PARALLEL##############################
life3d-omp: life3d-omp.o $(LL_LIB_PATH)/linked_list-omp.o $(HT_LIB_PATH)/hashtable-omp.o
	$(LINKFLAGS_OPENMP) $@ $^ -lm

life3d-omp.o: life3d-omp.c $(LL_LIB_PATH)/linked_list-omp.h $(HT_LIB_PATH)/hashtable-omp.h
	$(COMPFLAGS_OPENMP) $@ $<
################################################################################
#FAZER make serial PARA COMPILAR E LINKAR VERSAO SERIAL#########################
serial: life3d

life3d: life3d.o $(LL_LIB_PATH)/linked_list.o $(HT_LIB_PATH)/hashtable.o
	$(LINKFLAGS) $@ $^

life3d.o: life3d.c $(LL_LIB_PATH)/linked_list.h $(HT_LIB_PATH)/hashtable.h
	$(COMPFLAGS) $@ $<
################################################################################
#FAZER make clean PARA LIMPAR APENAS OS FICHEIROS DO PROGRAMA###################
clean:
	rm life3d-omp *.o
#FAZER make clean_all PARA LIMPAR APP FILES E A BIBLIOTECAS#####################
clean_all:
	rm life3d-omp life3d *.o  $(LL_LIB_PATH)/*.o $(HT_LIB_PATH)/*.o
################################################################################
