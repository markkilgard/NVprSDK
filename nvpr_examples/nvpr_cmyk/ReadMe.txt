
nvpr_cmyk

GPU-accelerated path rendering into CMYK (and RGB) color space.  In CMYK
color mode, two spot colors for dedicated ink colors (in addition to the
standard cyan/magenta/yellow/black "process" inks) are demonstrated.

For CMYK rendering, ARB_framebuffer_object+ARB_draw_buffers
functionality is used to orchestrate CMYK rendering.  A first color
buffer holds cyan/magneta/yellow/alpha while a second color buffer holds
black/spot1/spot2/alpha in the RGBA components respectively.  The alpha
channel is replicated in each color buffer.

When NV_framebuffer_mixed_samples is supported (on Maxwell 2 or later
NVIDIA GPUs), more quality settings are available.

Rotate and zooming is centered where you first left mouse click.
Hold down Ctrl at left mouse click to JUST SCALE.
Hold down Shift at left mouse click to JUST ROTATE.

Use middle mouse button to slide (translate).

'c' toggles CMYK vs. RGB color model rendering

'b' toggles sRGB vs. uncorrected RGB rendering (for RGB color model only)

'p' toggles use of spot colors (for CMYK color model only)

's' toggles stroking

'f' toggles filling

'x' toggles a checkboard vs. blue background

'v' cycles through visualizations of CMYK color channels (applies only
    to CMYK color mode)

    Normal -->
    Cyan only -->
    Magenta only -->
    Yellow only -->
    Black only -->
    Spot 1 (white) only -->
    Spot 2 (nose pink) only -->
    "Raw" RGB of first color buffer -->
    "Raw" RGB of second color buffer --> <beginning>

'V' cycles backwards through visualizations

'r' reset to Normal CMYK visualization mode

'1' when in Normal CMYK visualization mode, toggle masking of cyan
'2' " magenta channel
'3' " yellow channel
'4' " black channel
'5' " spot 1 channel
'6' " spot 2 channel

'i' print information on current configuration; includes estimate of
    total off-screen video memory used for the framebuffer configuration

'z' Request a 24-bit depth buffer + 8-bit stencil buffer

'Z' Request just an 8-bit stencil buffer (faster on Maxwell 2 GPUs)
