DEBUG=1

# root directory for ecolab include files and libraries
ifeq ($(shell ls $(HOME)/usr/ecolab/include/ecolab.h),$(HOME)/usr/ecolab/include/ecolab.h)
ECOLAB_HOME=$(HOME)/usr/ecolab
else
ECOLAB_HOME=/usr/local/ecolab
endif

include $(ECOLAB_HOME)/include/Makefile

UNITTESTOBJS=main.o testMinsky.o testGroup.o testGeometry.o
MINSKYOBJS=$(filter-out ../tclmain.o,$(wildcard ../*.o))
FLAGS+=-I..
LIBS+=-lUnitTest++ -lgsl -lgslcblas  -lxgl -lxlib

all: unittests cmpFp

unittests: $(UNITTESTOBJS) $(MINSKYOBJS)
	$(CPLUSPLUS) $(FLAGS)  -o $@ $(UNITTESTOBJS) $(MINSKYOBJS) $(LIBS)

cmpFp: cmpFp.o
	$(CPLUSPLUS) -o $@ $<

include $(UNITTESTOBJS:.o=.d)
