
Eureka 0.95 README
==================

by Andrew Apted  <ajapted@users.sf.net>   Feb 2013


INTRODUCTION

Eureka is a cross-platform map editor for the classic DOOM games.
The supported operating systems are Linux (and other Unix-likes),
Windows and MacOS X.

It started when I made a fork of the Yadex editor and attempted
to make it use a proper GUI toolkit, namely FLTK.  I also wanted
to implemented multiple Undo / Redo, which is something you greatly
miss when it's not there.  These changes required rewriting large
portions of the code, and the more I work on Eureka the more I
replace (or at least rework) the existing code.

While Eureka began as a fork of Yadex, it is NOT intended to be a
continuation of it, nor to be compatible with its file formats,
keyboard shortcuts, configuration items (etc).  Eureka is an
indepedent program now, and it has a different workflow than
Yadex or DEU.


FEATURES

-  Undo/Redo (multiple levels)
-  3D preview
-  Low system requirements, no 3D card needed
-  Editable panels for things, linedefs, sectors (etc)
-  Browser for textures, flats, things (etc)
-  Key binding system
-  Built-in nodes builder


SUPPORTED GAMES

-  DOOM
-  DOOM 2
-  Final Doom
-  FreeDoom
-  HacX
-  Heretic


REQUIREMENTS

-  128 MB of computer memory
-  800x600 or higher screen resolution
-  a keyboard and a two-button mouse
-  the data (iwad) file from a supported game


COMPILING / SETTING UP

See the INSTALL.txt document (in source code)


RUNNING

You can run Eureka from the command line, or it can be run from
the desktop menu (if your OS handles .desktop files as per the
XDG specs).  Eureka will need to be able to find an IWAD to run,
if it cannot find any then the "Manage Wads" dialog will open up,
allowing you to "Find" one (which is remembered for next time).

You can open a PWAD file using the File/Open menu command, or start
a new map with File/New command.

You can also specify the PWAD to edit on the command line, either
on its own or with the -file option:

   eureka -file masterpiece.wad

If that PWAD contains multiple maps, you may need to specify which
one to edit using the -warp option:

   eureka -file masterpiece.wad -warp 14

For a summary of useful command line options, type:

   eureka --help



KEYBOARD AND MOUSE CONTROLS

All Modes
---------

LMB
* select an object, drag to move the object(s)
* click in an empty area to clear the selection
* click in an empty area and drag to select a group of objects

RMB
* scroll the map around (by dragging)

MMB
* insert an object (same as SPACE or INSERT key)
* with SHIFT key: scale the selected objects
* with CTRL key: rotate objects

wheel : zoom in or out

1..9 : select the grid size (smallest to largest)

TAB : toggle the 3D preview on or off

t : enter Thing mode 
l : enter Linedef mode 
s : enter Sector mode 
v : enter Vertex mode 
R : enter RTS mode

b : toggle the Browser on or off

CTRL-Z : undo (can be used multiple times) 
CTRL-Y : redo (i.e. undo the previous undo)

CTRL-A : select all 
CTRL-I : invert the selection 
CTRL-U : clear the selection 
` (backquote) : clear the selection

HOME : move/zoom 2D viewport to show the whole map 
END  : move 2D viewport to camera location 
' (quote) : move 3D camera to position of mouse pointer

g : grid size adjustment : smaller 
G : grid size adjustment : larger 
h : grid mode toggle : on, simple, off 

f : toggle grid snapping on or off
J : toggle object number display
j : jump to object (by its numberic id)

o : copy and paste the selected objects
c : copy properties from the selection to the highlighted object
P : prune unused sectors, sidedefs and vertices

H : mirror objects horizontally 
V : mirror objects vertically
R : rotate objects 90 degrees clockwise 
W : rotate objects 90 degrees anti-clockwise


Things Mode
-----------

SPACE : add a new thing

d : disconnect things at the same location
m : move selected things to occupy the same location

w : rotate things 45 degrees anti-clockwise 
x : rotate things 45 degrees clockwise


Vertex Mode
-----------

SPACE
* add a new vertex
* if a vertex is already selected, adds a new linedef too

d : disconnect all linedefs at the selected vertices
m : merge selected vertices into a single one


Linedef Mode
------------

e : select a chain of linedefs 
E : select a chain of linedefs with same textures

w : flip linedefs 
k : split linedefs in two

d : disconnect selected linedefs from the rest
m : merge two one-sided linedefs into a two-sided linedef


Sector Mode
-----------

SPACE
* add a new sector to area around the mouse pointer
* fix broken sectoring in an area (use CTRL key to force a new sector)
* if a sector is selected, copy that sector instead of using defaults

d : disconnect sector(s) from their neighbors
m : merge all selected sectors into a single one

w : swap floor and ceiling textures

e : select sectors with same floor height 
E : select sectors with same floor texture

, and < : lower floor heights 
. and > : raise floor heights
[ and { : lower ceiling heights 
] and } : raise ceiling heights


3D Preview
----------

(Cursor keys will move forward and back, turn left and right)
(the WASD keys can also be used to move the camera)

LMB : does not do anything yet 
RMB : turn or move the camera (by dragging the mouse)
wheel : move camera forwards or backwards

PGUP and PGDN : move camera up and down

g : toggle gravity (i.e. as if the player was on the ground)

l : toggle lighting on or off
t : toggle texturing on or off
o : toggle objects on or off

F5  : toggle low detail / high detail
F11 : increase brightness (gamma)



COPYRIGHT and LICENSE

  Eureka DOOM Editor

  Copyright (C) 2001-2013 Andrew Apted
  Copyright (C) 1997-2003 Andre Majorel et al

  Eureka is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  Eureka is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.


