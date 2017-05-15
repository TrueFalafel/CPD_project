COMPFLAGS = gcc -std=c99 -Wall -c -o
COMPFLAGS_OPENMP = gcc -Wall -fopenmp -c -o
COMPFLAGS_MPI = mpicc -std=c99 -Wall -c -o
LINKFLAGS = gcc -Wall -o
LINKFLAGS_OPENMP = gcc -Wall -fopenmp -o
LINKFLAGS_MPI = mpicc -Wall -o
EXTRA_FLAGS = -lm

LL_LIB_PATH = ./LinkedListLib
HT_LIB_PATH = ./HashTableLib

IN1 = InputFiles/s5e50.in
IN2 = InputFiles/s20e400.in
IN3 = InputFiles/s50e5k.in
IN4 = InputFiles/s150e10k.in
IN5 = InputFiles/s200e50k.in
IN6 = InputFiles/s500e300k.in

OUT1 = OutputFiles/s5e50.10.out
OUT2 = OutputFiles/s20e400.500.out
OUT3 = OutputFiles/s50e5k.300.out
OUT4 = OutputFiles/s150e10k.1000.out
OUT5 = OutputFiles/s200e50k.1000.out
OUT6 = OutputFiles/s500e300k.2000.out

O_OUT1 = OutputFiles/s5e50.10_our.out
O_OUT2 = OutputFiles/s20e400.500_our.out
O_OUT3 = OutputFiles/s50e5k.300_our.out
O_OUT4 = OutputFiles/s150e10k.1000_our.out
O_OUT5 = OutputFiles/s200e50k.1000_our.out
O_OUT6 = OutputFiles/s500e300k.2000_our.out

#FAZER make PARA COMPILAR e LINKAR VERSAO PARALLEL##############################
life3d-mpi: life3d-mpi.o $(LL_LIB_PATH)/linked_list.o $(HT_LIB_PATH)/hashtable.o
	$(LINKFLAGS_MPI) $@ $^ $(EXTRA_FLAGS)

life3d-mpi.o: new_ver.c $(LL_LIB_PATH)/linked_list.h $(HT_LIB_PATH)/hashtable.h
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
#FAZER make run#################################################################
run1:
	time -p mpirun -np $(p) life3d-mpi $(IN1) $(i)
run2:
	time -p mpirun -np $(p) life3d-mpi $(IN2) $(i)
run3:
	time -p mpirun -np $(p) life3d-mpi $(IN3) $(i)
run4:
	time -p mpirun -np $(p) life3d-mpi $(IN4) $(i)
run5:
	time -p mpirun -np $(p) life3d-mpi $(IN5) $(i)
run6:
	time -p mpirun -np $(p) life3d-mpi $(IN6) $(i)
################################################################################
#FAZER make run&compare#############################################################
run_cmp1:
	mpirun -np $(p) life3d-mpi $(IN1) 10 > $(O_OUT1)
	diff $(OUT1) $(O_OUT1)
run_cmp2:
	mpirun -np $(p) life3d-mpi $(IN2) 500 > $(O_OUT2)
	diff $(OUT2) $(O_OUT2)
run_cmp3:
	mpirun -np $(p) life3d-mpi $(IN3) 300 > $(O_OUT3)
	diff $(OUT3) $(O_OUT3)
run_cmp4:
	mpirun -np $(p) life3d-mpi $(IN4) 1000 > $(O_OUT4)
	diff $(OUT4) $(O_OUT4)
run_cmp5:
	mpirun -np $(p) life3d-mpi $(IN5) 1000 > $(O_OUT5)
	diff $(OUT5) $(O_OUT5)
run_cmp6:
	mpirun -np $(p) life3d-mpi $(IN6) 2000 > $(O_OUT6)
	diff $(OUT6) $(O_OUT6)
################################################################################
#FAZER make clean PARA LIMPAR APENAS OS FICHEIROS DO PROGRAMA###################
clean:
	rm -f life3d-mpi *.o
#FAZER make clean_all PARA LIMPAR APP FILES E A BIBLIOTECAS#####################
clean_all:
	rm -f life3d-mpi life3d *.o  $(LL_LIB_PATH)/*.o $(HT_LIB_PATH)/*.o
################################################################################
