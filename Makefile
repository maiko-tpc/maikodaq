Target	= MAIKo_DAQ
Module	= TCPclient GigaIwaki vme-gbe-comm

ObjSuf	= .o
SrcSuf	= .cxx
HdrSuf	= .h

#Compiler flags
CFLAGS	= -Wall -O2 -std=c++11

#Linker flags
LDFLAGS	= -lpthread

TargetSrc = $(Target:%=%$(SrcSuf))
TargetObj = $(Target:%=%$(ObjSuf))
ModuleSrc = $(Module:%=%$(SrcSuf))
ModuleHrd = $(Module:%=%$(HdrSuf))
ModuleObj = $(Module:%=%$(ObjSuf))


all	: $(Target)

$(Target) : $(TargetObj) $(ModuleObj) $(ModuleHdr)
	$(CXX) -o $@ $@.o $(ModuleObj) $(LDFLAGS)

.cxx.o	:
	$(CXX) $(CFLAGS) -c -o $@ $<

.SUFFIXES: .o .cxx

clean	:
	rm -f *.o
	rm -f *~
	rm -f $(Target)
