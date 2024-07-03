# Doors

![doors1](user/doors-01.png)

Manual, remote and locked doors.

> **Note:**
>
> Door textures come in two sizes: 64 and 128. When making your doorway, opt for a passage width to match.

## Building a door
Use this method to make a basic door. It can be used as a base for a manual or a remote door.

* Join two sectors together with a passage

![doors2](user/doors-02.png)

* Increase the grid detail as needed by pressing `3`..`5`
* Enter vertices edit mode (`v`)
* Add vertices for the door inside the passage

![doors3](user/doors-03.png)

* Enter linedef edit mode (`l`) and select the front/rear sides of the door

![doors5](user/doors-05.png)

* The door raises into the ceiling, set the front upper sidedef texture to the ICKDOOR1 texture.

![doors6](user/doors-06.png)

* Enter sector edit mode (`s`) and select the door sector

![doors4](user/doors-04.png)

* Lower the ceiling all the way to the floor so that the door is closed. Use the **Ceiling** +- buttons or the `[` `]` keys.

![doors7](user/doors-07.png)

## Manual Doors
Manual doors open when the player performs the USE action on the door.

* Enter linedef edit mode (`l`)
* Select both linedefs of the door

![doors8](user/doors-08.png)

* Choose the **Type** of the linedefs as 1 DR Open Door

To make the door open on a fixed track (the sides stay still while opening and closing):

* Select the track linedefs
* Check the **upper unpeg** and **lower unpeg** options

![doors9](user/doors-09.png)

> **Note:**
>
> The door line specials indicates that the sector facing the back of the linedef is a door, this special does not need a tag either.
>
> The DR special can be opened repeatedly, while D1 can only be opened once.

## Locked Doors
Doors that require a blue, yellow or red key to open are created similarly to manual doors. When choosing the door linedef type, pick one of the specials that target the keys:

![locked1](user/locked-01.png)

## Remote Doors
Remote doors are opened through a switch.

* Enter vertice edit mode (`v`)
* Use the `LMB` to insert vertices along the wall, make the linedef 64 units long

![remote1](user/remote-01.png)

* Enter linedef edit mode (`l`), select the new linedef
* Choose the SW1GRAY switch texture

![remote2](user/remote-02.png)

* Choose the linedef **Type** as 63 SR Door Open
* Move the mouse cursor over the grid to ensure focus is not stolen by the **Line Specials** panel
* Press `;` then `f` to apply a fresh tag to the linedef

![remote5](user/remote-05.png)

* Enter sector edit mode (`s`), select the door sector

![remote3](user/remote-03.png)

* Press `;` then `l` to apply the last tag to the door sector

![remote6](user/remote-06.png)

* Eureka highlights both the sector and the linedef that share the same tag. This shows us the two are linked:

![remote4](user/remote-04.png)

> **Note:**
>
> The SR line special indicates a switch that can be toggled repeatedly, while S1 is a switch that can only be toggled once.

## Downloads

[doors.wad](http://sourceforge.net/projects/eureka-editor/files/Misc/Samples/doors.wad/download)
