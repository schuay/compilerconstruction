YACC = bison
FLEX = flex
CXX = g++
SSH = ssh
RSYNC = rsync
ECHO = echo
RM = rm

TARGET = ag
SOURCE = Makefile scan.l gram.y common.hpp common.cpp

HOST = ub-handin
REMOTEDIR = abgabe/$(TARGET)
REMOTETEST = /usr/ftp/pub/ublu/test/$(TARGET)/test

SFLAGS = -F "../.ssh-cfg"
RFLAGS = --chmod=o-rwxs,Dug+x,u+rw,g+r -e "$(SSH) $(SFLAGS)"
YFLAGS = -d
CXXFLAGS = -Wall -Wextra -pedantic -lstdc++ -g -O0
LDFLAGS = -lfl

all: $(TARGET)

$(TARGET): lex.yy.cpp gram.tab.cpp common.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) 

lex.yy.cpp: scan.l gram.tab.hpp
	$(FLEX) -olex.yy.cpp $<

gram.tab.cpp gram.tab.hpp: gram.y common.hpp
	$(YACC) $(YFLAGS) $< -o gram.tab.cpp

common.o: common.cpp common.hpp gram.tab.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

hand-in: $(SOURCE)
	@$(ECHO) "Handing in $(SOURCE)..."
	$(RSYNC) $(RFLAGS) $(SOURCE) $(HOST):$(REMOTEDIR)
	$(SSH) $(SFLAGS) $(HOST) 'cd $(REMOTEDIR); $(REMOTETEST) | egrep "(Test|Error)"'

clean:
	rm -f lex.yy.cpp gram.tab.cpp gram.tab.hpp $(TARGET) \
		  common.o gram.output
