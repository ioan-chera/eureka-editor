
Eureka README
=============


INTRODUCTION

Eureka is a map editor for the classic DOOM games, and a few related
games such as Heretic, Hexen and Strife.  The supported operating systems are
Linux (and other Unix-likes), Windows and macOS.


WEB SITE

http://eureka-editor.sourceforge.net/


FEATURES

-  Undo/Redo (multiple levels)
-  3D view with good lighting emulation
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
-  Hexen


REQUIREMENTS

-  128 MB of computer memory
-  1024x720 or higher screen resolution
-  3D accelerated graphics card
-  a keyboard and a two-button mouse
-  the data (iwad) file from a supported game


COMPILATION

See the INSTALL.txt document (in source code)


RUNNING


Command line:

You can run Eureka from the command line, or it can be run from
the desktop menu (on Linux: if your OS handles .desktop files as per the
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
* click in empty area to clear the selection
* click + drag in empty area to select a group of objects

RMB
* begin/continue line drawing (in vertex mode)
* merge sectors (in sectors mode)
* with CTRL pressed: bring up operation menu

MMB
* scroll the map around (by dragging)

wheel : zoom in or out

cursor keys : scroll the map

F1 : operation menu

TAB : toggle the 3D preview on or off
ESC : cancel the current operation

t : enter Thing mode
l : enter Linedef mode
s : enter Sector mode
v : enter Vertex mode

b : toggle the Browser on or off

1..9 : select the grid size (smallest to largest)

CTRL-Z : undo (can be used multiple times)
CTRL-Y : redo (i.e. undo the previous undo)

CTRL-A : select all
CTRL-I : invert the selection
CTRL-U : unselect all
` (backquote) : unselect all

HOME : zoom 2D viewport to show the whole map
END  : move 2D viewport to camera location
' (quote) : move 3D camera to position of mouse pointer

f : toggle free mode vs grid snapping
g : toggle grid on / off

N : open next map in the current wad
P : open previous map in the current wad

j : jump to object (by its numeric id)
J : toggle object number display

o : copy and paste the selected objects
c : copy properties from selected --> highlighted object
C : copy properties from highlighted --> selected objects

H : mirror objects horizontally
V : mirror objects vertically
R : rotate objects 90 degrees clockwise
W : rotate objects 90 degrees anti-clockwise

a : scroll map with the mouse
r : scale selected objects with the mouse
R : scale selected objects, allow stretching
CTRL-R : rotate the selected objects (with the mouse)
K : skew (shear) the selected objects

\ : toggle the RECENT category in the Browser

u  : popup menu to set ratio lock
z  : popup menu to set current scale
B  : popup menu to set browser mode
F8 : popup menu to set sector rendering mode

; : make the next key pressed META

META-N : load next file in given list
META-P : load previous file in given list

META-F : apply a fresh tag to the current objects
META-L : apply the last (highest) tag to the current objects


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
* begin/continue line drawing
* with SHIFT key: always continue line drawing
* with CTRL key: inhibit creation of sectors

d : disconnect all linedefs at the selected vertices
m : merge selected vertices into a single one
u : unlock any current ratio lock

I : reshape selected vertices into a line
O : reshape selected vertices into a circle
D : reshape selected vertices into a half-circle
C : reshape selected vertices into a 120-degree arc
Q : reshape selected vertices into a 240-degree arc


Linedef Mode
------------

e : select a chain of linedefs
E : select a chain of linedefs with same textures

w : flip linedefs
k : split linedefs in two
A : auto-align offsets on all selected linedefs

d : disconnect selected linedefs from the rest
m : merge two one-sided linedefs into a two-sided linedef


Sector Mode
-----------

SPACE
* add a new sector to area around the mouse pointer
* if a sector is selected, copy that sector instead of using defaults

d : disconnect sector(s) from their neighbors
m : merge all selected sectors into a single one

w : swap floor and ceiling textures
i : increase light level
I : decrease light level

e : select sectors with same floor height
E : select sectors with same floor texture
D : select sectors with same ceiling texture

, and < : lower floor heights
. and > : raise floor heights
[ and { : lower ceiling heights
] and } : raise ceiling heights


3D View
-------

(cursor keys will move forward and back, turn left and right)
(the WASD keys can also be used to move the camera)

LMB : select walls, floors or ceilings
MMB : turn or move the camera (by dragging the mouse)

wheel : move camera forwards or backwards

PGUP and PGDN : move camera up and down

g : toggle gravity (i.e. as if the player was on the ground)
e : popup menu to set edit mode
o : toggle objects on or off

META-v : drop to the ground
META-l : toggle lighting on or off
META-t : toggle texturing on or off

F11 : increase brightness (gamma)

r : adjust offsets on highlighted wall (with the mouse)
c : clear offsets on highlighted wall

x : align X offset with wall to the left
y : align Y offset with wall to the left
z : align both X + Y offsets

X : align X offset with wall to the right
Y : align Y offset with wall to the right
Z : align both X + Y offsets



COPYRIGHT and LICENSE

  Eureka DOOM Editor

  Copyright (C) 2014-2024 Ioan Chera
  Copyright (C) 2001-2020 Andrew Apted, et al
  Copyright (C) 1997-2003 Andre Majorel et al

  Eureka is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; either version 2 of the License,
  or (at your option) any later version.

  Eureka is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
