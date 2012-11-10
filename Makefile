#
#   Makefile for Eureka  [Unixy system-wide install]
#

PROGRAM=eureka

# prefix choices: /usr  /usr/local  /opt
INSTALL_PREFIX=/usr/local

OBJ_DIR=obj_linux

OPTIMISE=-O0 -g3

# operating system choices: UNIX WIN32
OS=UNIX


#--- Internal stuff from here -----------------------------------

INSTALL_DIR=$(INSTALL_PREFIX)/share/eureka

CXXFLAGS=$(OPTIMISE) -Wall -D$(OS)  \
         -D_THREAD_SAFE -D_REENTRANT

LDFLAGS=-L/usr/X11R6/lib

LIBS=-lm -lz  \
     -lfltk_images -lfltk_gl -lfltk  \
     -lX11 -lXext -lXft -lfontconfig -lXinerama  \
     -lpng -ljpeg -lGL


#----- Object files ----------------------------------------------

OBJS = \
	$(OBJ_DIR)/editloop.o  \
	$(OBJ_DIR)/editobj.o  \
	$(OBJ_DIR)/e_basis.o   \
	$(OBJ_DIR)/e_checks.o   \
	$(OBJ_DIR)/e_cutpaste.o  \
	$(OBJ_DIR)/e_linedef.o  \
	$(OBJ_DIR)/e_loadsave.o  \
	$(OBJ_DIR)/e_path.o  \
	$(OBJ_DIR)/e_sector.o  \
	$(OBJ_DIR)/e_things.o  \
	$(OBJ_DIR)/e_vertex.o  \
	$(OBJ_DIR)/im_color.o  \
	$(OBJ_DIR)/im_img.o   \
	$(OBJ_DIR)/levels.o  \
	$(OBJ_DIR)/lib_adler.o  \
	$(OBJ_DIR)/lib_file.o  \
	$(OBJ_DIR)/lib_util.o  \
	$(OBJ_DIR)/main.o  \
	$(OBJ_DIR)/m_bitvec.o  \
	$(OBJ_DIR)/m_config.o  \
	$(OBJ_DIR)/m_dialog.o  \
	$(OBJ_DIR)/m_game.o  \
	$(OBJ_DIR)/m_select.o  \
	$(OBJ_DIR)/m_strings.o  \
	$(OBJ_DIR)/objects.o  \
	$(OBJ_DIR)/r_grid.o  \
	$(OBJ_DIR)/r_misc.o  \
 	$(OBJ_DIR)/r_render.o  \
	$(OBJ_DIR)/selectn.o  \
	$(OBJ_DIR)/s_misc.o  \
	$(OBJ_DIR)/sys_debug.o \
	$(OBJ_DIR)/ui_about.o  \
	$(OBJ_DIR)/ui_browser.o  \
	$(OBJ_DIR)/ui_canvas.o  \
	$(OBJ_DIR)/ui_dialog.o  \
	$(OBJ_DIR)/ui_hyper.o  \
	$(OBJ_DIR)/ui_infobar.o  \
	$(OBJ_DIR)/ui_linedef.o  \
	$(OBJ_DIR)/ui_menu.o  \
	$(OBJ_DIR)/ui_nombre.o  \
	$(OBJ_DIR)/ui_pic.o  \
	$(OBJ_DIR)/ui_radius.o  \
	$(OBJ_DIR)/ui_sector.o  \
	$(OBJ_DIR)/ui_scroll.o  \
	$(OBJ_DIR)/ui_sidedef.o  \
	$(OBJ_DIR)/ui_thing.o  \
	$(OBJ_DIR)/ui_tile.o   \
	$(OBJ_DIR)/ui_vertex.o  \
	$(OBJ_DIR)/ui_window.o  \
	$(OBJ_DIR)/w_loadpic.o  \
	$(OBJ_DIR)/w_flats.o  \
	$(OBJ_DIR)/w_sprite.o  \
	$(OBJ_DIR)/w_texture.o  \
	$(OBJ_DIR)/w_wad.o   \
	$(OBJ_DIR)/x_hover.o  \
	$(OBJ_DIR)/x_loop.o  \
	$(OBJ_DIR)/x_mirror.o  \
	$(OBJ_DIR)/ymemory.o

$(OBJ_DIR)/%.o: src/%.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<


#----- Targets -----------------------------------------------

all: $(PROGRAM)

clean:
	rm -f $(PROGRAM) $(OBJ_DIR)/*.* core core.*
	rm -f ERRS LOG.txt update.log

$(PROGRAM): $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

bin: $(PROGRAM)
	strip --strip-unneeded $(PROGRAM)

install: bin
	install -o root -m 755 $(PROGRAM) $(INSTALL_PREFIX)/bin/
	install -d $(INSTALL_DIR)/games
	install -d $(INSTALL_DIR)/common
	install -d $(INSTALL_DIR)/ports
	install -d $(INSTALL_DIR)/mods
	install -o root -m 755 games/*.* $(INSTALL_DIR)/games
	install -o root -m 755 common/*.* $(INSTALL_DIR)/common
	install -o root -m 755 ports/*.* $(INSTALL_DIR)/ports
#	install -o root -m 755 mods/*.*  $(INSTALL_DIR)/mods
	xdg-desktop-menu  install --novendor misc/eureka.desktop
	xdg-icon-resource install --novendor --size 32 misc/eureka.xpm

uninstall:
	rm -v $(INSTALL_PREFIX)/bin/$(PROGRAM)
	rm -Rv $(INSTALL_DIR) 

.PHONY: all clean bin

#--- editor settings ------------
# vi:ts=8:sw=8:noexpandtab
