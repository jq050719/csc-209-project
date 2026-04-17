FLAGS = -Wall -Wextra -g
.PHONY = clean all

all: football_sim

football_sim: main.o worker.o
	gcc $(FLAGS) -o football_sim main.o worker.o

main.o: main.c common.h
	gcc $(FLAGS) -c main.c

worker.o: worker.c common.h
	gcc $(FLAGS) -c worker.c

clean:
	rm -f main.o worker.o football_sim