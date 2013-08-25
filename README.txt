
Eureka 1.00 README
==================

by Andrew Apted  <ajapted@users.sf.net>   AUG 2013


INTRODUCTION

Eureka is a cross-platform map editor for the classic DOOM games.
The supported operating systems are Linux (and other Unix-likes),
Windows and MacOS X.

It started when I ported the Yadex editor to a proper GUI toolkit,
namely FLTK, and implemented a system for multi-level Undo / Redo.
These and other features have required rewriting large potions of
the existing code, and adding lots of new code too.  Eureka is now
an indepedent program with its own workflow and quirks :)


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

; : make the next key pressed META

t : enter Thing mode 
l : enter Linedef mode 
s : enter Sector mode 
v : enter Vertex mode 

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

N : load next file in list
P : load previous file in list

g : grid size adjustment : smaller 
G : grid size adjustment : larger 
h : grid mode toggle : on, simple, off 

f : toggle grid snapping on or off
J : toggle object number display
j : jump to object (by its numberic id)

o : copy and paste the selected objects
c : copy properties from the selection to the highlighted object
p : prune unused sectors, sidedefs and vertices

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

LMB : not implemented yet 
RMB : turn or move the camera (by dragging the mouse)
MMB : adjust sidedef offsets (drag the mouse)

wheel : move camera forwards or backwards

PGUP and PGDN : move camera up and down

g : toggle gravity (i.e. as if the player was on the ground)

l : toggle lighting on or off
t : toggle texturing on or off
o : toggle objects on or off

F5  : toggle low detail / high detail
F11 : increase brightness (gamma)

c : clear offsets on highlighted sidedef
x : align X offset with wall to the left
y : align Y offset with wall to the left
z : align both X + Y offsets

X : align X offset with wall to the right
Y : align Y offset with wall to the right
Z : align both X + Y offsets



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


