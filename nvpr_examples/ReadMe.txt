
Welcome to the NVIDIA Path Rendering SDK examples!

July 2011

Here you'll source code for more than a dozen GPU-accelerated path
rendering examples using the NV_path_rendering OpenGL extension.

Each example directory contains its own ReadMe.txt explaining the example.

This SDK contains some fonts, resolution-independent clip art in SVG
and other forms, and several open source projects.  This content is
intended to aid the demonstration of NV_path_rendering's GPU-acceleration.
See the respective licenses for non-NVIDIA software components, fonts,
and art work.

Are you looking for pre-compiled Windows versions of these examples?
Try the NVprDEMOs.zip instead.

Questions?  Issues?  Send email to nvpr-support@nvidia.com

== NVIDIA Driver Requirements ==

To use NV_path_rendering examples, you need an NVIDIA GPU (GeForce 8 or
later) with appropriate driver (Release 275.33 or later).  Please use the
most recent released GPU.  Developers may benefit from using beta drivers.
Obtain drivers from:

  http://www.nvidia.com/drivers

For best performance, use a Fermi-based GPU (GeForce 400 or later,
Quadro 4000 or later) though earlier (back to GeForce 8) GPUs will
work well.  Path rendering performance scales with GPU performance so
higher performance GPUs are advised.

== Build Requirements and Instructions ==

=== Windows ===

*  Visual Studio 2008

   http://www.microsoft.com/express/Downloads/#2008-Visual-CPP

*  Cg Toolkit 3.0

   http://developer.download.nvidia.com/cg/Cg_3.0/Cg-3.0_February2011_Setup.exe
   http://http.developer.nvidia.com/Cg/index_releases.html
   http://developer.nvidia.com/cg-toolkit-download

*  DirectX SDK 

   http://www.microsoft.com/download/en/details.aspx?id=6812

Open the nvpr_examples solution (nvpr_examples.sln) in Visual Studio
2008 and build.

If you are missing <d2d1.h> (the Direct2D header), this indicates you
don't have the DirectX SDK installed.  Once you install the DirectX SDK,
exit and restart Visual Studio to pick up the DXSDK_DIR environment
variable.

Support for Direct2D in the nvpr_svg example requires Vista SP1 or
Windows 7.  The identical nvpr_svg executable works on XP, but simply
cannot demonstrate the Direct2D back-end.

=== Linux, etc. ===

*  GNU make

*  g++ 4.1 or higher

*  FreeType 2

*  Cg Toolkit 3.0

   http://developer.download.nvidia.com/cg/Cg_3.0/Cg-3.0_February2011_x86.tgz
   http://developer.download.nvidia.com/cg/Cg_3.0/Cg-3.0_February2011_x86.rpm
   http://developer.download.nvidia.com/cg/Cg_3.0/Cg-3.0_February2011_x86_64.deb

   http://developer.download.nvidia.com/cg/Cg_3.0/Cg-3.0_February2011_x86_64.tgz
   http://developer.download.nvidia.com/cg/Cg_3.0/Cg-3.0_February2011_x86_64.rpm
   http://developer.download.nvidia.com/cg/Cg_3.0/Cg-3.0_February2011_x86_64.deb

   http://developer.download.nvidia.com/cg/Cg_3.0/Cg-3.0_February2011_Solaris.pkg.tar.gz

   http://developer.nvidia.com/cg-toolkit-download

Run "make" in the nvpr_examples directory.

== Included Software ==

In order for the nvpr_svg example to provide comparison results with Skia,
Cairo, Qt, Direct2D and OpenVG, these software components are necessary.
If you don't have one of the components, try unsetting the USE_SKIA,
USE_CARIO, USE_QT, USE_D2D, or USE_OPENVG compiler defines respectively.

These software components are included for comparison purposes only.
You can use the backends for various path rendering APIs as a kind of
"Rosetta stone" to see how a particular path rendering API translates
to the equivalent NV_path_rendering usage.

=== Cairo ===

The Cairo 1.96. source code distribution is included.  You can download
the latest version of Cairo from:

  http://cairographics.org/download/

=== Skia ===

The Skia source code included is circa May 2010.  The latest version
can be obtained from:

  http://code.google.com/p/skia/

=== Qt ===

The Qt headers and compiled Windows libraries are from Qt 4.5.3
(September 2009).  The latest version can be obtained from:

  http://qt.nokia.com/downloads/

Linux users should build with Qt libraries on their system to build
nvpr_svg; or else disable Qt support by undefining the USE_QT flag
in nvpr_svg/GNUmakefile.

=== OpenVG ===

The OpenVG 1.1 Reference Implementation source code distribution is
included.  The latest version can be obtained from:

  http://www.khronos.org/registry/vg/ri/openvg-1.1-ri.zip
  http://www.khronos.org/registry/vg/

== Scalable Vector Graphics (SVG) Support ==

The nvpr_svg example contains an incomplete SVG implementation.

What works:

*  paths
*  gradients
*  clipping
*  images
*  transformations
*  viewbox

What doesn't work (currently):

*  patterns
*  markers
*  text
*  animation
*  JavaScript and DOM manipulation

