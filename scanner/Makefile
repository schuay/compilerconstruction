TARGET=scanner

FLEX=flex
GCC=gcc

LDFLAGS=-lfl

all: $(TARGET)

$(TARGET): lex.yy.c
	$(GCC) -o $@ $< $(LDFLAGS) 

lex.yy.c: scanner.l
	$(FLEX) $<

check: $(TARGET)
	cat test.u | ./$(TARGET)
	cat edgecases.u | ./$(TARGET)

clean:
	rm -f $(TARGET) lex.yy.c
