#
#  --- Eureka Editor ---
#
#  Makefile for Unixy system-wide install.
#  Requires GNU make.
#

PROGRAM=eureka

# prefix choices: /usr  /usr/local  /opt
PREFIX ?= /usr/local

# CXX=clang++-6.0

# flags controlling the dialect of C++
# [ the code is old-school C++ without modern features ]
CXX_DIALECT=-std=c++03 -fno-strict-aliasing -fwrapv

WARNINGS=-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
OPTIMISE=-O2 -g
STRIP_FLAGS=--strip-unneeded

# default flags for compiler, preprocessor and linker
CXXFLAGS ?= $(OPTIMISE) $(WARNINGS)
CPPFLAGS ?=
LDFLAGS ?= $(OPTIMISE)
LIBS ?=

# general things needed by Eureka
CXXFLAGS += $(CXX_DIALECT)
LIBS += -lz -lm

# FLTK flags (this assumes a system-wide FLTK installation)
CXXFLAGS += $(shell fltk-config --use-images --cxxflags)
LDFLAGS += $(shell fltk-config --use-images --ldflags)

# NOTE: the following is commented out since it does not work as expected.
#       the --libs option gives us static libraries, but --ldflags option
#       gives us dynamic libraries (and we use --ldflags above).
# LIBS += $(shell fltk-config --use-images --libs)

OBJ_DIR=obj_linux

DUMMY=$(OBJ_DIR)/zzdummy


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
	$(OBJ_DIR)/m_editlump.o  \
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
	$(OBJ_DIR)/ui_editor.o  \
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
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<


#----- Targets -----------------------------------------------

all: $(DUMMY) $(PROGRAM)

clean:
	rm -f $(PROGRAM) $(OBJ_DIR)/*.[oa]
	rm -f ERRS LOG.txt update.log core core.*

$(PROGRAM): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LIBS)

# this is used to create the OBJ_DIR directory
$(DUMMY):
	mkdir -p $(OBJ_DIR)
	@touch $@

stripped: all
	strip $(STRIP_FLAGS) $(PROGRAM)

# note that DESTDIR is usually left undefined, and is mainly
# useful when making packages for Debian/RedHat/etc...

INSTALL_DIR=$(DESTDIR)$(PREFIX)/share/eureka

install: all
	install -o root -m 755 $(PROGRAM) $(DESTDIR)$(PREFIX)/bin/
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

full-install: install
	xdg-desktop-menu  install --novendor misc/eureka.desktop
	xdg-icon-resource install --novendor --size 32 misc/eureka.xpm

uninstall:
	rm -v $(DESTDIR)$(PREFIX)/bin/$(PROGRAM)
	rm -Rv $(INSTALL_DIR)

full-uninstall: uninstall
	xdg-desktop-menu  uninstall --novendor misc/eureka.desktop
	xdg-icon-resource uninstall --novendor --size 32 eureka

.PHONY: all clean stripped

.PHONY: install uninstall full-install full-uninstall

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
