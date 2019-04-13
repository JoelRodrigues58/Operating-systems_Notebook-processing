CC=gcc
CFLAGS =-Wall

procNotebook:
	$(CC) $(CFLAGS) procNotebook.c -o notebook

clean:
	-rm -f *.o procNotebook *~
	-rm *.log
	-rm *.txt
	-rm auxiliar.nb
