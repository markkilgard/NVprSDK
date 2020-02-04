
nvpr_stroke_edging

This NV_path_rendering builds on the nvpr_basic example to demonstrate
how to use NV_path_rendering to stroke the "internal edging" of a stroke
(the portion of the stroke region that is also in the fill region) and
the "external edging" of a stroking (the portion of the stroke region
that is NOT in the fill region).

Key bindings:

'i' toggles the magenta internal edging.

'x' toggles the light blue external edging.

's' toggles the yellow (internal and external) stroking.

'f' toggles the green filling.

'e' toggles non-zero (the default) vs. non-zero filling.

'p' loads the heart & star path from a different representation (SVG
    string, PostScript string, explicit data).

Enter forces a redraw.

Esc exits the demo.
