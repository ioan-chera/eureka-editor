# Sacrificial Altar
![altar](user/ck_altar_004.png)

Using concentric circles we construct a sumptuous sacrificial altar.

## Method
In vertex edit mode, create a rectangle of points:

![rectangle of points](user/ck_altar_005.png)

This rectangle will be transformed into a circle using the arc shaping tools.

Select the centre pair of points and scale by 1.9 on the y plane:

![scaling](user/ck_altar_006.png)

Select the next pair of points on either side and scale by 1.8 on the y plane:

![scaling 2](user/ck_altar_007.png)

Select the next pair of points, scale by 1.7.

![scaling 3](user/ck_altar_008.png)

You can stop when you reach the edge vertices.

For each pair of points we scale by a fraction lower, this is to give the arc shape tool a hint as to the direction of the arc. If we tried to shape the points without this hinting we would instead see the "strange shape" status message.

Select all the top vertices and apply the arc shape 180 tool with `SHIFT-d`:

![shift-d](user/ck_altar_009.png)

Select the bottom vertices and `SHIFT-d` again:

![shift-d 2](user/ck_altar_010.png)

We now have the base for our altar:

![base of altar](user/ck_altar_011.png)

Switch to sector edit mode and select the circle sector. `CTRL-c` and `CTRL-v` to make a copy. Scale the copy to 1.2 on both x and y planes.

Switching to 3D view at this point will show the copied sector rendering with artifacts, this is because the copied sector needs merging into the room. Hover with the cursor inside the copied sector, the surrounding room highlights up to indicate this, and press `SPACE` to solidify the sector:

![solidify](user/ck_altar_012.png)

Repeat the above step two more times, scaling by 1.4 and 1.6 on both xy planes. You will end up with four concentric circles:

![concentric](user/ck_altar_014.png)

Select each circle in turn, lowering its floor and adjusting the sidedef textures to your liking.

## Downloads
[altar.wad](http://sourceforge.net/projects/eureka-editor/files/Misc/Samples/altar.wad/download)
