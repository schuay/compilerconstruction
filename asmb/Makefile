all: asmb.o

asmb: asmb.o main.c
	gcc -o asmb asmb.o main.c

asmb.o: asmb.s
	gcc -c asmb.s

check: asmb
	@./asmb

clean:
	rm -f asmb.o asmb
