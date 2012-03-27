YACC = bison
FLEX = flex
GCC = gcc
SSH = ssh
RSYNC = rsync
ECHO = echo
RM = rm

TARGET = parser
SOURCE = Makefile $(TARGET).l $(TARGET).y

HOST = ub-handin
REMOTEDIR = abgabe/$(TARGET)
REMOTETEST = /usr/ftp/pub/ublu/test/$(TARGET)/test

SFLAGS = -F "../.ssh-cfg"
RFLAGS = --chmod=o-rwxs,Dug+x,u+rw,g+r -e "$(SSH) $(SFLAGS)"
YFLAGS = -vd
CFLAGS = -Wall -Wextra -pedantic
LDFLAGS = -lfl

all: $(TARGET)

$(TARGET): lex.yy.c $(TARGET).tab.c
	$(GCC) $(CFLAGS) -o $@ $^ $(LDFLAGS) 

lex.yy.c: $(TARGET).l $(TARGET).tab.h
	$(FLEX) $<

$(TARGET).tab.c $(TARGET).tab.h: $(TARGET).y
	$(YACC) $(YFLAGS) $<

hand-in: $(SOURCE)
	@$(ECHO) "Handing in $(SOURCE)..."
	$(RSYNC) $(RFLAGS) $(SOURCE) $(HOST):$(REMOTEDIR)
	$(SSH) $(SFLAGS) $(HOST) 'cd $(REMOTEDIR); $(REMOTETEST)'

clean:
	rm -f $(TARGET) lex.yy.c $(TARGET).tab.c $(TARGET).tab.h $(TARGET).output
