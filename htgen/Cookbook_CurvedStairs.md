# Curved Stairs

![curve1](user/ck_curve_001.jpg)

## Layout

![curve2](user/ck_curve_002.png)

## Method

Draw the outline of the stairs in vertex edit mode:

![curve3](user/ck_curve_003.png)

Use the `LMB` to insert vertices along the edges, to define where the steps will be:

![curve4](user/ck_curve_004.png)

Press `f` to toggle free mode (no grid snapping) and drag the step vertices *roughly* into the arc shape.

![curve5](user/ck_curve_005.png)

Deselect all vertices (`` ` ``) and only select those on one side of the stairs:

![curve6](user/ck_curve_006.png)

Press `SHIFT`-`c` (shape arc to 120 degrees):

![curve7](user/ck_curve_007.png)

Now deselect all vertices, select only the other side and apply the shape arc operation again. Next use the `RMB` to join the step vertices:

![curve9](user/ck_curve_009.png)

Switch to sector edit mode, hold `SHIFT` and drag-select the stair sectors:

![curve10](user/ck_curve_010.png)

Press the raise floor shortcut (`.`) twice. Using the `LMB`, deselect the left-most step, and deselect the right-most step:

![curve11](user/ck_curve_011.png)

Repeat raising the floor and deselecting a step from either side, until you reach the center step. Your stairs are now done:
![curve12](user/ck_curve_012.png)

## Notes

The shape arc operation is also available by pressing `F1` to bring up the operations menu. However this menu does not have the 120 degree arc, so we used the `SHIFT`-`c` keyboard shortcut.

![curve8](user/ck_curve_008.png)

## Downloads
[curved-stairs.wad](http://sourceforge.net/projects/eureka-editor/files/Misc/Samples/curved-stairs.wad/download)
