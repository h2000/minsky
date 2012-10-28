# Use the TCL stub libraries to be consistent with TkTable

TCLLIBS+=$(shell grep TK_STUB_LIB_FLAG\
    $(call search,lib*/tkConfig.sh) | cut -f2 -d\')
TCLLIBS+=$(shell grep TCL_STUB_LIB_FLAG\
    $(call search,lib*/tclConfig.sh) | cut -f2 -d\')


# root directory for ecolab include files and libraries
ifeq ($(shell ls $(HOME)/usr/ecolab/include/ecolab.h),$(HOME)/usr/ecolab/include/ecolab.h)
ECOLAB_HOME=$(HOME)/usr/ecolab
else
ECOLAB_HOME=/usr/local/ecolab
endif

include $(ECOLAB_HOME)/include/Makefile

MODELS=minsky
# override MODLINK to remove tclmain.o, which allows us to provide a
# custom one that picks up its scripts from a relative library
# directory
MODLINK=$(LIBMODS:%=$(ECOLAB_HOME)/lib/%)
OTHER_OBJS=tclmain.o godley.o port.o portManager.o variable.o variableManager.o variableValue.o operation.o plotWidget.o cairoItems.o XGLItem.o godleyIcon.o groupIcon.o equations.o
MODLINK+=$(OTHER_OBJS)
FLAGS+=-DTR1 $(OPT) -UECOLAB_LIB -DECOLAB_LIB=\"library\"

.h.cd: $(CLASSDESC)
	$(CLASSDESC) -nodef -I $(CDINCLUDE) -I $(ECOLAB_HOME)/include  -i $< $(ACTIONS) >$@
# xml_pack/unpack need to -typeName option, as well as including privates
	$(CLASSDESC) -typeName -nodef -I $(CDINCLUDE) -I $(ECOLAB_HOME)/include  -i $< xml_pack xml_unpack >>$@
	$(CLASSDESC) -nodef -I $(CDINCLUDE) -I $(ECOLAB_HOME)/include \
	  -respect_private -i $< $(RPACTIONS) >>$@

TESTS=
ifdef AEGIS
TESTS=tests checkMissing
endif

ifeq ($(OS),MINGW)
LIBS+=-lTktable211
else
LIBS+=-lTktable2.11
endif

LIBS+=-lgsl -lgslcblas -lxgl -lxlib -lcairo -lpng -lz

#chmod command is to counteract AEGIS removing execute privelege from scripts
all: $(MODELS) $(TESTS)
	-$(CHMOD) a+x *.tcl *.sh *.pl

# This option removes the black window, but this also prevents being
# able to type TCL commands on the command line, so only use it for
# release builds

ifeq ($(OS),MINGW) 
ifndef DEBUG
FLAGS+=-mwindows
endif
endif

# This rule uses a header file of object descriptors
$(MODELS:=.o): %.o: %.cc 
	$(CPLUSPLUS) -c $(FLAGS)  $<

# how to build a model executable
$(MODELS): %: %.o $(MODLINK) 
	$(LINK) $(FLAGS) $(MODLINK) $*.o -L/opt/local/lib/db48 -L. $(LIBS) -o $@

include $(MODELS:=.d) $(OTHER_OBJS:.o=.d)

# ecolab doesn't normally provide XML definitions of things
plot.xmlcd: $(ECOLAB_HOME)/include/plot.h
	$(CLASSDESC) -nodef -I $(CDINCLUDE) -I $(ECOLAB_HOME)/include  -i $< xml_pack xml_unpack >>$@

tests: $(MODELS)
	cd test; $(MAKE)

clean:
	$(BASIC_CLEAN)
	rm -f $(MODELS) 
	cd test; $(BASIC_CLEAN)

# we want to build this target always when under AEGIS, otherwise only
# when non-existing
ifdef AEGIS
.PHONY: minskyVersion.h
endif

minskyVersion.h:
	rm -f minskyVersion.h
ifdef AEGIS
	echo '#define MINSKY_VERSION "'$(version)'"' >minskyVersion.h
else
	echo '#define MINSKY_VERSION "unknown"' >minskyVersion.h
endif

mac-dist:
# build objects in 32 bit mode, using custom static libraries
	$(MAKE) OPT="-m32 -O3 -DNDEBUG" UNURAN= minsky.o $(OTHER_OBJS)
# create executable in the app package directory. Make it 32 bit only
	mkdir -p minsky.app/Contents/MacOS
	$(LINK) -m32 $(FLAGS) $(MODLINK) minsky.o -L$(HOME)/usr/lib -L$(HOME)/usr/ecolab/lib -L/usr/X11/lib -lecolab $(HOME)/usr/lib/libpangocairo-1.0.a $(HOME)/usr/lib/libpango-1.0.a $(HOME)/usr/lib/libcairo.a $(HOME)/usr/lib/libgobject-2.0.a $(HOME)/usr/lib/libgmodule-2.0.a $(HOME)/usr/lib/libglib-2.0.a $(HOME)/usr/lib/libffi.a $(HOME)/usr/lib/libintl.a $(HOME)/usr/lib/libpixman-1.a $(HOME)/usr/lib/libpng15.a -liconv -lxgl -lxlib -lfreetype -Wl,-framework,Carbon -Wl,-framework,IOKit -Wl,-framework,CoreServices -Wl,-framework,ApplicationServices    -lXrender  -lSM -lICE  $(HOME)/usr/lib/libtk8.5.a $(HOME)/usr/lib/libtcl8.5.a -lX11  -lm -lgsl -lgslcblas -lz -lreadline -lncurses -ldb -o minsky.app/Contents/MacOS/minsky
	install_name_tool -change /usr/X11/lib/libfreetype.6.dylib @executable_path/libfreetype.6.dylib minsky.app/Contents/MacOS/minsky
	cp /usr/X11/lib/libfreetype.6.dylib minsky.app/Contents/MacOS/
#copy toplevel tcl script
	cp *.tcl minsky.app/Contents/MacOS
#copy library scripts and Tktable dylib
	cp -r library minsky.app/Contents/MacOS
	cp -r $(HOME)/usr/lib/tcl8.5 $(HOME)/usr/lib/tk8.5 library
	cp $(HOME)/usr/lib/libTktable2.11.dylib minsky.app/Contents/MacOS/library
	cp -r icons minsky.app/Contents/MacOS

checkMissing:
	chmod a+x test/checkMissing.sh
	test/checkMissing.sh
