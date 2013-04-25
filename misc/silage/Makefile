#
#  --- Silage : SLIGE + GUI ---
#
#  Makefile for Unixy system-wide install
#
#  NOTE: Run from top level: make -f misc/silage/Makefile
#

PROGRAM=silage

# prefix choices: /usr  /usr/local  /opt
INSTALL_PREFIX=/usr/local

OBJ_DIR=obj_linux

OPTIMISE=-O0 -g3 -fno-strict-aliasing

# operating system choices: UNIX WIN32
OS=UNIX


#--- Internal stuff from here -----------------------------------

INSTALL_DIR=$(INSTALL_PREFIX)/share/silage

CXXFLAGS=$(OPTIMISE) -Wall -D$(OS)  \
         -Iglbsp_src  \
         -D_THREAD_SAFE -D_REENTRANT

LDFLAGS=-L/usr/X11R6/lib

LIBS= \
     -lfltk_images -lfltk_gl -lfltk  \
     -lX11 -lXext -lXft -lfontconfig -lXinerama  \
     -lpng -ljpeg -lGL -lz -lm


# support for a non-standard install of FLTK
ifneq ($(FLTK_PREFIX),)
CXXFLAGS += -I$(FLTK_PREFIX)/include
LDFLAGS += -L$(FLTK_PREFIX)/lib -Wl,-rpath,$(FLTK_PREFIX)/lib
endif

# support for statically linking FLTK (no GL, local JPEG and PNG)
ifneq ($(FLTK_STATIC),)
LIBS= \
     -lfltk_images -lfltk  \
     -lfltk_png -lfltk_jpeg \
     -lX11 -lXext -lXft -lfontconfig -lXinerama \
     -lz -lm
endif


#----- Object files ----------------------------------------------

OBJS = \
	$(OBJ_DIR)/silage/window.o

$(OBJ_DIR)/silage/%.o: misc/silage/%.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<


#----- glBSP Objects ------------------------------------------------

GLBSP_OBJS= \
	$(OBJ_DIR)/glbsp/analyze.o  \
	$(OBJ_DIR)/glbsp/blockmap.o \
	$(OBJ_DIR)/glbsp/glbsp.o    \
	$(OBJ_DIR)/glbsp/level.o    \
	$(OBJ_DIR)/glbsp/node.o     \
	$(OBJ_DIR)/glbsp/reject.o   \
	$(OBJ_DIR)/glbsp/seg.o      \
	$(OBJ_DIR)/glbsp/system.o   \
	$(OBJ_DIR)/glbsp/util.o     \
	$(OBJ_DIR)/glbsp/wad.o

GLBSP_CXXFLAGS=$(OPTIMISE) -Wall -DINLINE_G=inline

$(OBJ_DIR)/glbsp/%.o: glbsp_src/%.cc
	$(CXX) $(GLBSP_CXXFLAGS) -o $@ -c $< 


#----- Targets -----------------------------------------------

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) $(OBJ_DIR)/silage/*.*
	rm -f $(OBJ_DIR)/glbsp/*.*
	rm -f ERRS LOG.txt update.log

$(PROGRAM): $(OBJS) $(GLBSP_OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LIBS)

stripped: $(PROGRAM)
	strip --strip-unneeded $(PROGRAM)


.PHONY: all clean stripped

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
