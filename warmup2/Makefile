warmup2: warmup2.o my402list.o
	gcc -g -Wall warmup2.o my402list.o -o warmup2 -lpthread -lrt -lm
warmup2.o: warmup2.c my402list.h
	gcc -g -Wall -c -Wall warmup2.c
my402list.o: my402list.c my402list.h
	gcc -g -Wall -c -Wall my402list.c
clean:
	rm -f *.o warmup2
