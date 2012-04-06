YACC = bison
FLEX = flex
CXX = g++
SSH = ssh
RSYNC = rsync
ECHO = echo
RM = rm

TARGET = gesamt
SOURCE = Makefile scan.l gram.y common.hpp common.cpp lib include

HOST = ub-handin
REMOTEDIR = abgabe/$(TARGET)
REMOTETEST = /usr/ftp/pub/ublu/test/$(TARGET)/test

SFLAGS = -F "../.ssh-cfg"
RFLAGS = --recursive --progress \
		 --copy-links -e "$(SSH) $(SFLAGS)"
YFLAGS = -d
CXXFLAGS = -Wall -Wextra -pedantic -Wno-long-long -lstdc++ -g -O0 -std=gnu++98 \
		   -Iinclude -DNDEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS \
		   -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer \
		   -fno-exceptions -fPIC -Woverloaded-virtual -Wcast-qual
# LLVM needs to be compiled using --enable-targets=host
LDFLAGS = -lfl \
		  -Llib/llvm  -lpthread -lffi -ldl -lm \
		  -lLLVMTableGen -lLLVMMCJIT -lLLVMRuntimeDyld -lLLVMObject \
		  -lLLVMMCDisassembler -lLLVMLinker -lLLVMipo -lLLVMInterpreter \
		  -lLLVMInstrumentation -lLLVMJIT -lLLVMExecutionEngine -lLLVMDebugInfo \
		  -lLLVMBitWriter -lLLVMX86Disassembler -lLLVMX86AsmParser -lLLVMX86CodeGen \
		  -lLLVMX86Desc -lLLVMSelectionDAG -lLLVMX86AsmPrinter -lLLVMX86Utils \
		  -lLLVMX86Info -lLLVMAsmPrinter -lLLVMMCParser -lLLVMCodeGen -lLLVMScalarOpts \
		  -lLLVMInstCombine -lLLVMTransformUtils -lLLVMipa -lLLVMAsmParser \
		  -lLLVMArchive -lLLVMBitReader -lLLVMAnalysis -lLLVMTarget -lLLVMMC \
		  -lLLVMCore -lLLVMSupport

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
	$(SSH) $(SFLAGS) $(HOST) 'cd $(REMOTEDIR); $(REMOTETEST) 2>&1 | egrep "(Eingabe|\[Error|[0-9]+ Tests )"'

clean:
	rm -f lex.yy.cpp gram.tab.cpp gram.tab.hpp $(TARGET) \
		  common.o gram.output
