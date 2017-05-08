COMPFLAGS = gcc -Wall -c -o
COMPFLAGS_OPENMP = gcc -Wall -fopenmp -c -o
COMPFLAGS_MPI = mpicc -Wall -c -o
LINKFLAGS = gcc -Wall -o
LINKFLAGS_OPENMP = gcc -Wall -fopenmp -o
LINKFLAGS_MPI = mpicc -Wall -o
EXTRA_FLAGS = -lm

LL_LIB_PATH = ./LinkedListLib
HT_LIB_PATH = ./HashTableLib

#FAZER make PARA COMPILAR e LINKAR VERSAO PARALLEL##############################
life3d-mpi: life3d-mpi.o $(LL_LIB_PATH)/linked_list.o $(HT_LIB_PATH)/hashtable.o
	$(LINKFLAGS_MPI) $@ $^ $(EXTRA_FLAGS)

life3d-mpi.o: life3d-mpi.c $(LL_LIB_PATH)/linked_list.h $(HT_LIB_PATH)/hashtable.h
	$(COMPFLAGS_MPI) $@ $<
################################################################################
#FAZER make serial PARA COMPILAR E LINKAR VERSAO SERIAL#########################
serial: life3d

life3d: life3d.o $(LL_LIB_PATH)/linked_list.o $(HT_LIB_PATH)/hashtable.o
	$(LINKFLAGS) $@ $^

life3d.o: life3d.c $(LL_LIB_PATH)/linked_list.h $(HT_LIB_PATH)/hashtable.h
	$(COMPFLAGS) $@ $<
################################################################################
#FAZER make hash PARA COMPILAR HASHTABLE########################################
hash: $(HT_LIB_PATH)/hashtable.o

$(HT_LIB_PATH)/hashtable.o: $(HT_LIB_PATH)/hashtable.c $(HT_LIB_PATH)/hashtable.h
	$(COMPFLAGS) $@ $<
################################################################################
#FAZER make linked_list PARA COMPILAR LINKED LIST###############################
linked_list: $(LL_LIB_PATH)/linked_list.o

$(LL_LIB_PATH)/linked_list.o: $(LL_LIB_PATH)/linked_list.c $(LL_LIB_PATH)/linked_list.h
	$(COMPFLAGS) $@ $<
################################################################################
#FAZER make clean PARA LIMPAR APENAS OS FICHEIROS DO PROGRAMA###################
clean:
	rm -f life3d-mpi *.o
#FAZER make clean_all PARA LIMPAR APP FILES E A BIBLIOTECAS#####################
clean_all:
	rm -f life3d-omp life3d *.o  $(LL_LIB_PATH)/*.o $(HT_LIB_PATH)/*.o
################################################################################
