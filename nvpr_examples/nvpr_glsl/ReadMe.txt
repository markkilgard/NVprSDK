
nvpr_glsl

An NV_path_rendering example demonstrating how programmable fragment
shaders can be used during the "cover" step to bump map text.  The
fractal fragment shader is written in GLSL.

The glProgramPathFragmentInputGenNV command generates varying fragment
inputs for the GLSL fragment shader.

Arrow keys shift fractal pattern.

'c' cycles through color pairs

's' toggles stroking.

'f' toggles filling.

'u' cycles through underling styles.

Rotate and zooming is centered where you first left mouse click.
Hold down Ctrl at left mouse click to JUST SCALE.
Hold down Shift at left mouse click to JUST ROTATE.

Use middle mouse button to slide (translate).

