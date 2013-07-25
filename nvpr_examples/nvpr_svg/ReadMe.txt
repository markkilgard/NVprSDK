
nvpr_svg

This large NV_path_rendering example provides GPU-accelerated rendering
of Scalable Vector Graphics (SVG) content as well as font glyphs.

This example requires Cg Toolkit 3.0 libraries to be installed on your
system.  Cg Toolkit can be downloaded from:

  http://developer.nvidia.com/object/cg_download.html

For performance and quality comparisons between NV_path_rendering and
other major path rendering systems, the example includes five alternative
path renderers:

* Cairo 1.9.6 graphics
  used by Mozilla
  http://cairographics.org/
  XXX 1.9.6 has a bug in path clipping that didn't exist in 1.8.8

* Qt 4.5.2
  http://doc.trolltech.com/4.5/painting-painterpaths.html
  http://qt.nokia.com/doc/4.5/qt4-arthur.html

* Google's Skia 2D Graphics Library
  used by Google's Chrome web browser and Android operating system
  http://code.google.com/p/skia/

* OpenVG 1.1 reference implementation
  WARNING, VERY SLOW
  http://www.khronos.org/registry/vg/ri/openvg-1.1-ri.zip

* Direct2D
  used by Microsoft's Internet Explorer 9
  http://msdn.microsoft.com/en-us/library/dd370990%28VS.85%29.aspx
  Microsoft only exposes Direct2D on Vista and Windows 7 (not XP); on XP,
  gs_bezier cannot expose Direct2D renderer

Cairo, Qt, Skia, and OpenVG are all CPU-based path renderers.  Direct2D
uses the GPU through Direct3D; additionally, Direct3D's CPU-based WARP
(Windows Advanced Rasterization Platform) can be used as the rendering
backend for Direct2D.  For performance comparison purposes, gs_bezier
supports either the GPU or WARP Direct3D backends.  Not surpringly,
WARP is generally slower than the dedicated CPU-based path renderers.

Instructions:

*  Use the right menu to get a pop-up menu.

*  Left mouse click & drag zooms (up & down) and/or rotates (left & right)
   around the clicked point.

*  Middle mouse button (clicking the trackwheel) & dragging will pan.

*  Keyboad short-cuts are listed on the menu items.

   Quick summary:
     space bar animates and stops animation
     "f" cycles through SVG files
     "g" cycles through scalable fonts
     "n" shows the software rendering window

   When you left click on a window not animating when continuous animation
   is active, the clicked window begins animating (only one window--either the
   GPU rendered window or the software rendering window--will be animating
   at a time).

*  Use 'c' to toggle control points.  When control points are visible,
   click on control points with the left-mouse button and drag them around.

Command line options:

  -svg NAME          :: name of an SVG file
  -font NAME         :: name of a font
  -path #            :: number of a builtin path
  -geometry WxH      :: initial window size (must be first option),
                        default is 500x500: example -geometry 400x400
  -vsync             :: start with vertical retrace synchronization on (default is off)
  -novsync           :: request no vertical retrace synchronization (default)
  -aliased           :: don't use any antialiasing
  -1                 :: same as -aliased
  -2                 :: 2 sample multisampling
  -4                 :: 4 sample multisampling
  -8                 :: 8 sample multisampling
  -16                :: 16 sample multisampling (the default)
  -v                 :: start in verbose mode (not recommended)
  -topToBottom       :: render top-to-bottom layers with stencil testing,
                        effectively the Reverse Painter's algorithm, using
                        stencil to discard pixels already updated
  -benchmark         :: report best frames/second interval over 6 second intervals
  -accumPasses #     :: number of accumulation buffering passes to perform
                        (zero, the default, means no accumulation buffering)
  -noSpin            :: don't spin and scale the object when rendering
                        continuously and reporting a frame rate
  -cairo             :: show window with Cairo-rendered version of the scene
  -qt                :: show window with Qt-rendered version of the scene
  -skia              :: show window with Skia-rendered version of the scene
  -d2d               :: show window with Direct2D-rendered version of scene (Windows 7 & Vista only)
  -openvg            :: show window with OpenVG reference implementation-rendered version of the scene
  -waitForExit       :: don't exit benchmark mode until Return into in console window
  -animate           :: start up animating the (OpenGL) window
  -frameCount #      :: number of frames to render before calling exit()
  -dlist             :: use display lists
  -black             :: clear to a black background
  -white             :: clear to a white background
  -blue              :: clear to a blue background
  -gold              :: start with gold image window displaying
  -diff              :: start with difference image window displaying
  -regress           :: start running regression tests of SVG files
  -seed              :: start running a seeding that converts SVG files to gold TGAs
  -vprofile name     :: specify Cg vertex profile name (example: -vprofile arbvp1)
  -fprofile name     :: specify Cg fragment profile name (example: -fprofile arbfp1)
  -linearRGB         :: default to blending and filtering in linear RGB color space
                        (default is uncorrected sRGB)

There are a lot of keyboard controls...

A few key operations:

[f] = advance SVG file, [F] goes back an SVG file
      You can also pick SVG files "by name" from the "SVG files..." submenu
      NOTE: color gradients and stroking is ignored (for now)

[w] = advance compiled-in path, [W] goes back a path
      You can also pick paths "by name" from the "Path objects..." submenu
      Some of the paths: dragon, phone, star (non-zero), star (even-odd),
      quill, butterfly, letter Psi, quadratic curve cluster, music symbol,
      stylized swan, biohazard logo, biohazard sign with text, old bicylce,
      various "hard" test cases demonstrating cubic loops

[g] = advance scalable fonts. [G] goes back a font
      You can also pick fonts "by name" from the "Scalable fonts..." submenu.

      If looking at SVG files, [w] toggles back to the last compiled-in path.
      If looking at compiled-in paths, [f] toggles back to the last SVG file.

[p] = toggle on/off white reference points along path's outline

[d] = dump the current scene to an SVG file named "test.svg"
      use "firefox test.svg" to view

[s] = toggle visualizing stencil buffer

[c] = toggle on/off control points:
        green control points are on the curve,
        red control points are off the curve

[l] = (lowercase L):toggle on/off stippling mode, use with [2]

[1] = (the number one): reset any control point adjustments

[!] = reset to original configuration

[space] = run benchmark mode

[=] = stop the scaling & rotating during benchmark
      good for ascertaining a stable framerate

[v] = toggle monitor refresh synchronization

[3] = change clear color to black
[4] = change clear color to white
[5] = change color to blue (the default)
[6] = change color to light gray

[0] = (zero) toggles display list mode (off by default unless -dlist specified)

[7] = cycles through using 1 (default, fastest), 4, 9, or 16 rendering passes
      per path to accumulate coverage to increase quality

[F5] & [F6] = decrease the X or Y (respectively) multipass stepping

[F7] & [F8] = increase the X or Y (respectively) multipass stepping

[F1] & [F2] = scale down the X or Y (respectively) multipass spread

[F3] & [F4] = scale up the X or Y (respectively) multipass spread

[F9] = toggles stippling mode for multi-pass alpha accumulate; when
       stippling is enabled, only half as many pass samples on the grid
       are performed.  So a 3x3 grid uses 4 samples and a 5x4 grid uses
       10 samples.

[9] = toggles reverse Painter's Algorithm (generally slower, but could
      be faster)

[;] = increase number of accumulation buffer passes
      (default is zero, for no accumulation buffering)
[:] = decrease number of accumulation buffer passs
[P] = visualize the sample locations for the current accumulation buffering
      number of passes

[n] = toggle showing window with software-rendered (Qt or Cairo)
      version of the scene
[R] = toggle using Qt (default) or Cairo or Skia software renderer
[N] = run a benchmark of the current (Qt or Cairo) software renderer
      for current scene
[Ctrl R] = selects OpenVG reference implementation software renderer;
           warning this is VERY SLOW

[Page Up]     = zoom in
[Page Down]   = zoom out
[Down Arrow]  = decrease Y projection
[Up Arrow]    = increase Y projection
[Left Arrow]  = decrease X projection
[Right Arrow] = increase X projection

[F11] = dump a gold TGA file to an appropriate directory

[Left mouse] = dragging horizontally rotates around the clicked point
               (right = clockwise, left = counterclockwise) while
               dragging vertically zooms/scales (up = smaller, down =
               larger) at the clicked point.

[Shift Left mouse] = dragging ONLY does rotation.

[Ctrl Left mouse] = dragging ONLY does zooming/scaling.

[Left mouse] in 'c' mode = control point dragging mode: click and drag control points

[Alt Left mouse] in 'c' mode = interatively drags the control point in both the
                               GL and Software windows (instead of the default of just
                               dragging in the window you clicked in).

   (The 'c' mode toggles visualizes showing control points.)

[Middle mouse] = dragging shifts/translates/scrolls the scene.

   NOTE:  Rotating/zooming/shifting only interactively updates the window
   (GL or Software) where the drag was initiated.  When the drag stops
   (button release), the "other" window updates.  This allows you to
   determine the interactivity of GL or Software rendering individually.

[Alt Middle mouse] = inspects stencil value under mouse location.

[Right mouse] = pop-up menu

[Home] = Reset view to default

[Insert] = Toggle sRGB correct blending.

