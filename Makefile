#
#  --- Eureka Editor ---
#
#  Makefile for Unixy system-wide install
#

PROGRAM=eureka

# prefix choices: /usr  /usr/local  /opt
PREFIX=/usr/local

OBJ_DIR=obj_linux

OPTIMISE=-O2 -fno-strict-aliasing

STRIP_FLAGS=--strip-unneeded

# operating system choices: UNIX WIN32
OS=UNIX


#--- Internal stuff from here -----------------------------------

INSTALL_DIR=$(PREFIX)/share/eureka

CXXFLAGS=$(OPTIMISE) -Wall -D$(OS)  \
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
	$(OBJ_DIR)/e_basis.o   \
	$(OBJ_DIR)/e_checks.o   \
	$(OBJ_DIR)/e_commands.o  \
	$(OBJ_DIR)/e_cutpaste.o  \
	$(OBJ_DIR)/e_hover.o  \
	$(OBJ_DIR)/e_linedef.o   \
	$(OBJ_DIR)/e_main.o  \
	$(OBJ_DIR)/e_objects.o  \
	$(OBJ_DIR)/e_path.o  \
	$(OBJ_DIR)/e_sector.o  \
	$(OBJ_DIR)/e_things.o  \
	$(OBJ_DIR)/e_vertex.o  \
	$(OBJ_DIR)/im_color.o  \
	$(OBJ_DIR)/im_img.o   \
	$(OBJ_DIR)/lib_adler.o  \
	$(OBJ_DIR)/lib_file.o  \
	$(OBJ_DIR)/lib_tga.o   \
	$(OBJ_DIR)/lib_util.o  \
	$(OBJ_DIR)/main.o  \
	$(OBJ_DIR)/m_bitvec.o  \
	$(OBJ_DIR)/m_config.o  \
	$(OBJ_DIR)/m_events.o  \
	$(OBJ_DIR)/m_files.o  \
	$(OBJ_DIR)/m_game.o  \
	$(OBJ_DIR)/m_keys.o  \
	$(OBJ_DIR)/m_loadsave.o  \
	$(OBJ_DIR)/m_nodes.o  \
	$(OBJ_DIR)/m_select.o  \
	$(OBJ_DIR)/m_strings.o  \
	$(OBJ_DIR)/m_testmap.o  \
	$(OBJ_DIR)/r_grid.o  \
	$(OBJ_DIR)/r_render.o  \
	$(OBJ_DIR)/sys_debug.o \
	$(OBJ_DIR)/ui_about.o  \
	$(OBJ_DIR)/ui_browser.o  \
	$(OBJ_DIR)/ui_canvas.o  \
	$(OBJ_DIR)/ui_default.o  \
	$(OBJ_DIR)/ui_dialog.o  \
	$(OBJ_DIR)/ui_file.o  \
	$(OBJ_DIR)/ui_hyper.o  \
	$(OBJ_DIR)/ui_infobar.o  \
	$(OBJ_DIR)/ui_linedef.o  \
	$(OBJ_DIR)/ui_menu.o  \
	$(OBJ_DIR)/ui_misc.o  \
	$(OBJ_DIR)/ui_nombre.o  \
	$(OBJ_DIR)/ui_pic.o  \
	$(OBJ_DIR)/ui_prefs.o  \
	$(OBJ_DIR)/ui_replace.o  \
	$(OBJ_DIR)/ui_sector.o  \
	$(OBJ_DIR)/ui_scroll.o  \
	$(OBJ_DIR)/ui_sidedef.o  \
	$(OBJ_DIR)/ui_thing.o  \
	$(OBJ_DIR)/ui_tile.o   \
	$(OBJ_DIR)/ui_vertex.o  \
	$(OBJ_DIR)/ui_window.o  \
	$(OBJ_DIR)/w_loadpic.o  \
	$(OBJ_DIR)/w_texture.o  \
	$(OBJ_DIR)/w_wad.o   \
	\
	$(OBJ_DIR)/bsp_level.o \
	$(OBJ_DIR)/bsp_node.o \
	$(OBJ_DIR)/bsp_util.o

$(OBJ_DIR)/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<


#----- Targets -----------------------------------------------

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) $(OBJ_DIR)/*.* core core.*
	rm -f ERRS LOG.txt update.log

$(PROGRAM): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LIBS)

stripped: $(PROGRAM)
	strip $(STRIP_FLAGS) $(PROGRAM)

install: stripped
	install -o root -m 755 $(PROGRAM) $(PREFIX)/bin/
	install -d $(INSTALL_DIR)/games
	install -d $(INSTALL_DIR)/common
	install -d $(INSTALL_DIR)/ports
	rm -f $(INSTALL_DIR)/games/freedoom.ugh
	install -o root -m 644 bindings.cfg $(INSTALL_DIR)/bindings.cfg
	install -o root -m 644 defaults.cfg $(INSTALL_DIR)/defaults.cfg
	install -o root -m 644 operations.cfg $(INSTALL_DIR)/operations.cfg
	install -o root -m 644 misc/about_logo.png $(INSTALL_DIR)/about_logo.png
	install -o root -m 644 games/*.* $(INSTALL_DIR)/games
	install -o root -m 644 common/*.* $(INSTALL_DIR)/common
	install -o root -m 644 ports/*.* $(INSTALL_DIR)/ports
	xdg-desktop-menu  install --novendor misc/eureka.desktop
	xdg-icon-resource install --novendor --size 32 misc/eureka.xpm

uninstall:
	rm -v $(PREFIX)/bin/$(PROGRAM)
	rm -Rv $(INSTALL_DIR)
	xdg-desktop-menu  uninstall --novendor misc/eureka.desktop
	xdg-icon-resource uninstall --novendor --size 32 eureka

.PHONY: all clean stripped install uninstall

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
