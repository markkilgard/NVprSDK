
nvpr_text_shape

An NV_path_rendering example using HarfBuzz to render properly shaped text.

Rotate and zooming is centered where you first left mouse click.
Hold down Ctrl at left mouse click to JUST SCALE.
Hold down Shift at left mouse click to JUST ROTATE.

Use middle mouse button to slide (translate).

This demo expects to have access to the following fonts:
  fonts/DejaVuSerif.ttf
  fonts/amiri-0.104/amiri-regular.ttf
  fonts/fireflysung-1.3.0/fireflysung.ttf
  fonts/lohit_ta.ttf
  fonts/DDC_Uchen.ttf
  fonts/SILEOT.ttf

On Windows, this demo expects the executable to have access to
the GTK freetype6.dll on its Path so the GL_FILE_NAME_NV mode of
glPathGlyphIndexArrayNV and glPathGlyphIndexRangeNV works.  On Linux
and other platforms with FreeType pre-installed, that should be sufficient.

