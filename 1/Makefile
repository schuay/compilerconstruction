all: asma.o

asma: asma.o main.c
	gcc -o asma asma.o main.c

asma.o: asmatiny.s
	gcc -c asmatiny.s -o asma.o

check: asma
	@./asma 0 0
	@./asma 0 1
	@./asma 0 3
	@./asma 0 -1
	@./asma 1 0
	@./asma 1 1
	@./asma 1 3
	@./asma 1 -1
	@./asma 3 0
	@./asma 3 1
	@./asma 3 3
	@./asma 3 -1

clean:
	rm -f asma.o asma
