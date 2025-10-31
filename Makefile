GCC = gcc
FLAGS = 
RM = rm -rf

taller_procesos:
	$(GCC) $(FLAGS) $@.c -o $@

clear:
	$(RM) taller_procesos