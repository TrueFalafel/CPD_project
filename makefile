COMPFLAGS = gcc -Wall -g -c -o
LINKFLAGS = gcc -Wall -fopenmp -o
LIBFLAGS = ar -cvq
LIB_PATH = ./LinkedListLib
IN_FILES_PATH = ./InputFiles
#FAZER make PARA COMPILAR O PROGRAMA############################################
life3d: life3d.o
	$(LINKFLAGS) $@ $< -L$(LIB_PATH) -l_linked_list

life3d.o: life3d.c $(LIB_PATH)/linked_list.h
	$(COMPFLAGS) $@ $<
################################################################################
#FAZER make lib PARA CRIAR A BIBLIOTECA#########################################
lib: $(LIB_PATH)/linked_list.o
	$(LIBFLAGS) $(LIB_PATH)/lib_linked_list.a $<

linked_list.o: linked_list.c linked_list.h
	$(COMPFLAGS) $@ $<
################################################################################
#FAZER make run PARA CORRER O PROGRAMA##########################################
run:
	./life3d $(IN_FILES_PATH)/simple.in 1
################################################################################
#FAZER make clean PARA LIMPAR APENAS OS FICHEIROS DO PROGRAMA###################
clean:
	rm life3d *.o
#FAZER make clean_all PARA LIMPAR APP FILES E A BIBLIOTECA######################
clean_all:
	rm life3d *.o  $(LIB_PATH)/*.o $(LIB_PATH)/lib_linked_list.a
################################################################################
