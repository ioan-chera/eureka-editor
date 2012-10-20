
Eureka 0.74 README
==================

by Andrew Apted    February 2012


INTRODUCTION

Eureka is a map editor for the game DOOM, and the main target is
Linux operating systems.  It started with me trying to port YADEX
to use a proper GUI toolkit, namely FLTK, but ended up becoming a
full rewrite, and this rewrite is only partially complete.

This editor is still "alpha" quality and not ready for serious use.
Lots of command and menu items are not implemented and will just
beep at you or do nothing at all -- not very user friendly! :-)


COMPILING

See the INSTALL.txt document for more information


RUNNING

This program expects to be run from the directory where it was
unpacked (where the compiled executable ends up, in the top level).
A proper "Unixy" installation is not supported yet.

You will need to either:
   (a) copy the DOOM 2 IWAD into the same directory
   (b) use the -iwad option with the full pathname of the IWAD
       for example: -iwad /usr/share/games/doom/doom2.wad

The IWAD filename must be lowercase, e.g. "doom2.wad"

To edit a PWAD at a particular map, use the following options
(which mimic the parameters used to run DOOM) :

   ./Eureka -file blah.wad -warp MAP03

Without the -file option, eureka will load a map from the IWAD.
The File/Save function (CTRL-S) will then be equivalent to the
File/Export function and prompt for a new output wad.

Without the -warp option, MAP01 is assumed.



MOUSE USAGE

right click : select an object, drag to move selected objects
              click in empty spot to clear selection
              click in empty spot and drag to select a group of objects

left click : scroll map around (by dragging)
             in 3D preview: turn camera left/right or move up/down

middle click : drag to scale the selected objects
               with SHIFT key: scale each axis independently
               with CTRL key: rotate and scale objects

mouse wheel : zoom in / out
              in 3D preview: move camera forward/back



KEYBOARD USAGE

1-9  : various grid sizes

HOME or 0: zoom out and centre map
END:       jump to camera location

t : enter Thing mode
l : enter Linedef mode
s : enter Sector mode
v : enter Vertex mode
r : enter RTS mode

TAB : toggle 3D preview

b : toggle browser 

CTRL-A     : select all
CTRL-U or `: clear the selection
CTRL-I     : invert the selection

CTRL-Z : undo
CTRL-Y : redo

g : grid step smaller
G : grid step larger

h : toggle grid on/simple/off

f : grid snapping (free mode) on/off

j : jump to specified object

o : copy and paste the selected objects

H : mirror objects horizontally
V : mirror objects vertically

m : scale objects by half
M : scale objects 2x

R : rotate objects 90^ clockwise
W : rotate objects 90^ anti-clockwise

' : move 3D camera to position at cursor


THINGS MODE:

SPACE : add new thing

w : rotate things 45^ anti-clockwise
x : rotate things 45^ clockwise


SECTORS MODE:

SPACE : add new sector to area around the mouse pointer
        if a sector is selected, copy that sector instead of default
        with CTRL key: set area to same sector as selected one

c : correct sector at mouse pointer

e : select sectors with same floor height
E : select sectors with same floor texture

w : swap floor and ceiling textures

[ { : lower floor heights
] } : raise floor heights

, < : lower ceiling heights
. > : raise ceiling heights


LINEDEFS MODE:

e : select a chain of linedefs
E : select a chain of linedefs with same textures

w : flip linedefs
x : split linedefs in two

d : disconnect selected linedefs from the rest


VERTEX MODE:

SPACE : add new vertex
        if a vertex is already selected, adds a new linedef

d : disconnect all linedefs at the selected vertices


BROWSER:

C : cycle category

CTRL-K : clear the search box


3D PREVIEW:

l  : toggle lighting  on / off
t  : toggle texturing on / off
s  : toggle sprites   on / off

w  : walk, drop camera to ground + player's view height

F5 : toggle low detail / high detail

F11: increase gamma



COPYRIGHT and LICENSE

  Eureka DOOM Editor

  Copyright (C) 2001-2012 Andrew Apted
  Copyright (C) 1997-2003 Andre Majorel et al

  Eureka is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  Eureka is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.


