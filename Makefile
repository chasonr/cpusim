OFILES = \
main.o \
cpu6502.o \
cpu.o \
disasm.o \
flags.o \
load.o \
memdump.o \
memory.o \
registers.o \

EXE = cpusim

CXX = $(shell wx-config --cxx)
CXXFLAGS = -Wall -g $(shell wx-config --cxxflags)

$(EXE) : $(OFILES)
	$(shell wx-config --ld) $(EXE) $(OFILES) $(shell wx-config --libs)

cpu6502.o: cpu6502.cpp cpu6502.h cpu.h memory.h

cpu.o: cpu.cpp cpu.h memory.h

disasm.o: disasm.cpp disasm.h

flags.o: flags.cpp cpu.h

load.o: load.cpp

main.o: main.cpp cpu6502.h disasm.h events.h load.h memdump.h memory.h \
        registers.h open.xpm into.xpm over.xpm return.xpm goto.xpm mgoto.xpm

memdump.o: memdump.cpp events.h memdump.h memory.h

memory.o: memory.cpp memory.h

registers.o: registers.cpp flags.h registers.h

clean:
	rm -f *.o $(EXE)
