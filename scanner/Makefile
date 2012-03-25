TARGET=scanner

FLEX=flex
GCC=gcc

SOURCE   = Makefile scanner.l

HOST = ub-handin
REMOTEDIR = abgabe/$(TARGET)
REMOTETEST = /usr/ftp/pub/ublu/test/$(TARGET)/test

SSH = ssh
RSYNC = rsync
ECHO = echo
RM = rm

SFLAGS = -F "../.ssh-cfg"
RFLAGS = --chmod=o-rwxs,Dug+x,u+rw,g+r -e "$(SSH) $(SFLAGS)"
LDFLAGS=-lfl

all: $(TARGET)

$(TARGET): lex.yy.c
	$(GCC) -o $@ $< $(LDFLAGS) 

lex.yy.c: scanner.l
	$(FLEX) $<

test: $(TARGET)
	./$(TARGET) < test.u
	./$(TARGET) < edgecases.u

hand-in: $(SOURCE)
	@$(ECHO) "Handing in $(SOURCE)..."
	$(RSYNC) $(RFLAGS) $(SOURCE) $(HOST):$(REMOTEDIR)
	$(SSH) $(SFLAGS) $(HOST) 'cd $(REMOTEDIR); $(REMOTETEST)'

clean:
	rm -f $(TARGET) lex.yy.c
