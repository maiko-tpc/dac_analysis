Target = DAC_Analysis2 Vth_Analysis2

ObjSuf = .o
SrcSuf = .cxx

CXX = g++
LD = g++

# Compiler flags
CXXFLAGS  = -O2 -W -Wall -ansi -pedantic -pipe -I.
CXXFLAGS += -Wno-long-long -Woverloaded-virtual
CXXFLAGS +=  -Wpointer-arith -Wno-non-virtual-dtor
CXXFLAGS += $(shell root-config --cflags)

LDFLAGS   = -O2 -W -Wall -ansi -pedantic -pipe
LDFLAGS  += -Wno-long-long -Woverloaded-virtual
LDFLAGS  +=  -Wpointer-arith -Wno-non-virtual-dtor
LDFLAGS  += $(shell root-config --ldflags)

LIBS      = $(shell root-config --libs)
LIBS     += $(shell root-config --glibs)

TargetSrc = $(Target:%=%$(SrcSuf))
TargetObj = $(Target:%=%$(ObjSuf))

all	: $(Target)

$(Target): $(TargetObj)
	$(LD) $(LDFLAGS) -o $@ $@.o $(LIBS)

.cxx.o	:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.SUFFIXES: .o .cxx

clean	:
	rm -f *.o
	rm -f *~
	rm -f $(Target)
