
/* nvpr_text_shape.cpp - render text with NV_path_rendering that is shaped by HarfBuzz */

// Copyright (c) NVIDIA Corporation. All rights reserved.

// Based on https://github.com/lxnt/ex-sdl-freetype-harfbuzz/blob/master/ex-sdl-freetype-harfbuzz.c

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#ifdef __APPLE__
# include <GLUT/glut.h>
#else
# include <GL/glut.h>
# ifdef _WIN32
#  include <windows.h>  // for wglGetProcAddress
# else
#  include <GL/glx.h>  // for glXGetProcAddress
# endif
#endif

#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftadvanc.h>
#include <freetype/ftsnames.h>
#include <freetype/tttables.h>
#include <freetype/ftoutln.h> // for FT_Outline_Decompose

#include <hb.h>
#include <hb-ft.h>
#include <hb-ucdn.h>  // Unicode Database and Normalization
//#include <hb-icu.h> // Alternatively you can use hb-glib.h

#include <Cg/double.hpp>
#include <Cg/vector/xyzw.hpp>
#include <Cg/matrix/1based.hpp>
#include <Cg/matrix/rows.hpp>
#include <Cg/vector.hpp>
#include <Cg/matrix.hpp>
#include <Cg/mul.hpp>
#include <Cg/round.hpp>
#include <Cg/iostream.hpp>

using namespace Cg;
using std::vector;

#include "sRGB_math.h"
#include "countof.h"
#include "request_vsync.h"
#include "showfps.h"
#include "cg4cpp_xform.hpp"
#include "read_file.hpp"

#ifndef GLAPIENTRYP
# ifdef _WIN32
#  define GLAPIENTRYP __stdcall *
# else
#  define GLAPIENTRYP *
# endif
#endif

#ifndef glPathGlyphIndexArrayNV
// New NV_path_rendering API version 1.2 command for getting path objects ordered by font glyph index
typedef GLenum (GLAPIENTRYP PFNGLPATHGLYPHINDEXARRAYNVPROC) (GLuint firstPathName, GLenum fontTarget, const GLvoid *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
typedef GLenum (GLAPIENTRYP PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC) (GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const GLvoid *fontData, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
typedef GLenum (GLAPIENTRYP PFNGLPATHGLYPHINDEXRANGENVPROC) (GLenum fontTarget, const GLvoid *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint *baseAndCount);
#endif
// Possible return tokens from glPathGlyphIndexRangeNV:
#define GL_FONT_GLYPHS_AVAILABLE_NV                         0x9368
#define GL_FONT_TARGET_UNAVAILABLE_NV                       0x9369
#define GL_FONT_UNAVAILABLE_NV                              0x936A
#define GL_FONT_CORRUPT_NV                                  0x936B
#define GL_STANDARD_FONT_FORMAT_NV                          0x936C
#define GL_FONT_NUM_GLYPH_INDICES_BIT_NV                    0x20000000

// Different names because GLEW has macros for the real names.
// We just used these to test if extension functions exist (are non-NULL).
static PFNGLPATHGLYPHINDEXARRAYNVPROC my_glPathGlyphIndexArrayNV = NULL;
static PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC my_glPathMemoryGlyphIndexArrayNV = NULL;
static PFNGLPATHGLYPHINDEXRANGENVPROC my_glPathGlyphIndexRangeNV = NULL;

bool stroking = true;
int hasPathRendering = 0;
int hasFramebufferSRGB = 0;
GLint sRGB_capable = 0;
const char *program_name = "nvpr_text_shape";
bool animating = false;
bool enable_vsync = true;
int canvas_width = 800, canvas_height = 600;

GLuint path_template = 0;

/* Scaling and rotation state. */
float anchor_x = 0,
      anchor_y = 0;  /* Anchor for rotation and scaling. */
int scale_y = 0, 
    rotate_x = 0;  /* Prior (x,y) location for scaling (vertical) or rotation (horizontal)? */
int zooming = 0;  /* Are we zooming currently? */
int scaling = 0;  /* Are we scaling (zooming) currently? */

/* Sliding (translation) state. */
float slide_x = 0,
      slide_y = 0;  /* Prior (x,y) location for sliding. */
int sliding = 0;  /* Are we sliding currently? */

float3x3 view;

/* Global variables */
int background = 2;

float point_size = 50;  // 50 point

FPScontext gl_fps_context;

enum FontType {  // Avoid "Font" since used by X headers
  ENGLISH=0,
  ARABIC,
  CHINESE,
  TAMIL,
  TIBETAN,
  HEBREW,
  NUM_FONTS
};

struct TextSystem {
  hb_direction_t direction;
  const char *language;
  hb_script_t script;
  FontType font;
};

// Text systems
const TextSystem latin_text_system = {
  HB_DIRECTION_LTR,
  "en",
  HB_SCRIPT_LATIN,
  ENGLISH
};
const TextSystem arabic_text_system = {
  HB_DIRECTION_RTL,
  "ar",
  HB_SCRIPT_ARABIC,
  ARABIC
};
const TextSystem vertical_chinese_text_system = {
  HB_DIRECTION_TTB,
  "ch",
  HB_SCRIPT_HAN,
  CHINESE
};
const TextSystem horizontal_chinese_text_system = {
  HB_DIRECTION_LTR,
  "ch",
  HB_SCRIPT_HAN,
  CHINESE
};
const TextSystem tamil_text_system = {
  HB_DIRECTION_RTL,
  "ta",
  HB_SCRIPT_TAMIL,
  TAMIL
};
const TextSystem tibetan_text_system = {
  HB_DIRECTION_LTR,
  "bo",  // bod/tib  bo    Tibetan
  HB_SCRIPT_TIBETAN,
  TIBETAN
};
const TextSystem hebrew_text_system = {
  HB_DIRECTION_RTL,
  "iw",  // Hebrew
  HB_SCRIPT_HEBREW,
  HEBREW
};

struct TextSample {
  const char *text;
  const TextSystem *text_system;
};

TextSample cat_text[] = {
   { "Ленивый рыжий кот",     // "Lazy ginger cat" in Russian
      &latin_text_system },
   { "كسول الزنجبيل القط",  // "Lazy ginger cat" in Arabic
     &arabic_text_system },
   { "懶惰的姜貓",             // "Lazy ginger cat" in Chinese
     &vertical_chinese_text_system }
};

TextSample some_text[] = {
   { "This is some english text",    // English
      &latin_text_system },
   { "هذه هي بعض النصوص العربي",  // "These are some of the Arab texts" in Arabic
     &arabic_text_system },
   { "這是一些中文",                  // "This is some Chinese" in Chinese
     &vertical_chinese_text_system }
};

TextSample cloud_text[] = {
   { "Every cloud is sky poetry",    // English
      &latin_text_system },
   { "Каждое облако небо поэзии",    // Russian
      &latin_text_system },
   { "كل سحابة السماء الشعر",      // Arabic
     &arabic_text_system },
   { "每個雲是天空的詩",               // Chinese
     &horizontal_chinese_text_system }
};

TextSample change_text[] = {
   { "Writing changes everything",    // English
      &latin_text_system },
   { "Написание все меняется",    // Russian
      &latin_text_system },
   { "كتابة كل شيء يتغير",      // Arabic
     &arabic_text_system },
   { "寫作改變了一切",               // Chinese
     &horizontal_chinese_text_system },
   { "எழுதுதல் மாற்றங்கள் எல்லாம்",    // Tamil
      &tamil_text_system },
   { "Pisanie wszystko się zmienia",    // Polish
      &latin_text_system },
   { "གསར་གནས།",  // Tibetan (don't know what it says); perhaps "News Flash"
     &tibetan_text_system },
   { "שינויי כתיבת הכל",  // Hebrew
     &hebrew_text_system },
};

FT_Library ft_library;

hb_font_t *hb_ft_font[NUM_FONTS];
GLuint nvpr_glyph_base[NUM_FONTS];
GLuint nvpr_glyph_count[NUM_FONTS];
const char *font_names[NUM_FONTS] = {
  "fonts/DejaVuSerif.ttf",
#if 0  // alternative font with Arabic glyphs
  "fonts/lateef.ttf",
#else
  "fonts/amiri-0.104/amiri-regular.ttf",
#endif
  "fonts/fireflysung-1.3.0/fireflysung.ttf",
  "fonts/lohit_ta.ttf",
  "fonts/DDC_Uchen.ttf",
  "fonts/SILEOT.ttf",
};

const float EM_SCALE = 2048;

#define PRE_glPathGlyphIndexRangeNV  // Uncomment for older drivers.
#ifdef PRE_glPathGlyphIndexRangeNV
struct PathGenState {
  bool needs_close;
  vector<char> cmds;
  vector<float> coords;
  const float glyph_scale;

  PathGenState(float glyph_scale_)
    : needs_close(false)
    , glyph_scale(glyph_scale_)
  {}
};

// Callbacks to populate FT_Outline_Funcs 
// http://freetype.sourceforge.net/freetype2/docs/reference/ft2-outline_processing.html#FT_Outline_Funcs

// FT_Outline_Funcs move_to callback
static int move_to(const FT_Vector* to,
                   void* user)
{
  PathGenState *info = reinterpret_cast<PathGenState*>(user);
  if (info->needs_close) {
    info->cmds.push_back('z');
  }
  info->cmds.push_back('M');
  const float scale = info->glyph_scale;
  info->coords.push_back(scale * float(to->x));
  info->coords.push_back(scale * float(to->y));
  info->needs_close = true;
  return 0;
}

// FT_Outline_Funcs line_to callback
static int line_to(const FT_Vector* to,
                   void* user)
{
  PathGenState *info = reinterpret_cast<PathGenState*>(user);
  info->cmds.push_back('L');
  const float scale = info->glyph_scale;
  info->coords.push_back(scale * float(to->x));
  info->coords.push_back(scale * float(to->y));
  return 0;
}

// FT_Outline_Funcs conic_to callback (for quadratic Bezier segments)
static int conic_to(const FT_Vector* control,
                    const FT_Vector* to,
                    void* user)
{
  PathGenState *info = reinterpret_cast<PathGenState*>(user);
  info->cmds.push_back('Q');
  const float scale = info->glyph_scale;
  info->coords.push_back(scale * float(control->x));
  info->coords.push_back(scale * float(control->y));
  info->coords.push_back(scale * float(to->x));
  info->coords.push_back(scale * float(to->y));
  return 0;
}

// FT_Outline_Funcs cubic_to callback (for cubic Bezier segments)
static int cubic_to(const FT_Vector* control1,
                    const FT_Vector *control2,
                    const FT_Vector* to,
                    void* user)
{
  PathGenState *info = reinterpret_cast<PathGenState*>(user);
  info->cmds.push_back('C');
  const float scale = info->glyph_scale;
  info->coords.push_back(scale * float(control1->x));
  info->coords.push_back(scale * float(control1->y));
  info->coords.push_back(scale * float(control2->x));
  info->coords.push_back(scale * float(control2->y));
  info->coords.push_back(scale * float(to->x));
  info->coords.push_back(scale * float(to->y));
  return 0;
}

void generateGlyph(GLuint path, FT_Face face, FT_UInt glyph_index)
{
  const float em_scale = EM_SCALE;
  const FT_UShort units_per_em = face->units_per_EM;

  FT_Error error;

  // http://freetype.sourceforge.net/freetype2/docs/reference/ft2-base_interface.html#FT_Load_Glyph
  error = FT_Load_Glyph(face, glyph_index,
                        FT_LOAD_NO_SCALE |   // Don't scale the outline glyph loaded, but keep it in font units.
                        FT_LOAD_NO_BITMAP);  // Ignore bitmap strikes when loading.
  assert(!error);
  FT_GlyphSlot slot = face->glyph;

  assert(slot->format == FT_GLYPH_FORMAT_OUTLINE);

  const float glyph_scale = em_scale / units_per_em;

  FT_Outline outline = slot->outline;

  static const FT_Outline_Funcs funcs = {
    move_to,
    line_to,
    conic_to,
    cubic_to,
    0,  // shift
    0   // delta
  };

  PathGenState info(glyph_scale);
  // http://freetype.sourceforge.net/freetype2/docs/reference/ft2-outline_processing.html#FT_Outline_Decompose
  FT_Outline_Decompose(&outline, &funcs, &info);
  if (info.needs_close) {
    info.cmds.push_back('z');
  }

  const GLsizei num_cmds = info.cmds.size() ? GLsizei(info.cmds.size()) : 0;
  const GLsizei num_coords = info.coords.size() ? GLsizei(info.coords.size()) : 0;
  const GLubyte *cmds = info.cmds.size() ? reinterpret_cast<GLubyte*>(&info.cmds[0]) : NULL;
  const GLfloat *coords = info.coords.size() ? &info.coords[0] : NULL;
  glPathCommandsNV(path,
    num_cmds, cmds, 
    num_coords, GL_FLOAT, coords);

  glPathParameterfNV(path, GL_PATH_STROKE_WIDTH_NV, 0.1*EM_SCALE);  // 10% of emScale
  glPathParameteriNV(path, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);
}
#endif // PRE_glPathGlyphIndexRangeNV

const int device_hdpi = 72;
const int device_vdpi = 72;

enum NVprAPImode_t {
  // NV_path_rendering revision 1.3
  GLYPH_INDEX_ARRAY,
  MEMORY_GLYPH_INDEX_ARRAY,
  // NV_path_rendering revision 1.2
  GLYPH_INDEX_RANGE,
} NVprAPImode = GLYPH_INDEX_ARRAY;

void reportMode()
{
  switch (NVprAPImode) {
  case GLYPH_INDEX_ARRAY:
    printf("Using glPathGlyphIndexArrayNV\n");
    break;
  case MEMORY_GLYPH_INDEX_ARRAY:
    printf("Using glPathMemoryGlyphIndexArrayNV\n");
    break;
  case GLYPH_INDEX_RANGE:
    printf("Using glPathGlyphIndexRangeNV\n");
    break;
  default:
    assert(!"bogus NVprAPImode");
    break;
  }
}

#if 0
void destroy_nop(void *user_data)
{
    assert(user_data == NULL);
}
#endif

// FreeType Error Enumerations
// http://www.freetype.org/freetype2/docs/reference/ft2-error_enumerations.html
#undef __FTERRORS_H__                                             
#define FT_ERRORDEF( e, v, s )  { e, s },                         
#define FT_ERROR_START_LIST     {                                 
#define FT_ERROR_END_LIST       { 0, NULL } };                    
                                                                
const struct _ft_errors {                                                                 
    int          err_code;                                          
    const char*  err_msg;                                           
} ft_errors[] =
// Preprocessor magic using FT_ERROR* macros above
#include FT_ERRORS_H                                              

static const char *FreeTypeErrorMessage(FT_Error err)
{
    for (size_t i=0; i<countof(ft_errors); i++) {
        if (err == ft_errors[i].err_code) {
            return ft_errors[i].err_msg;
        }
    }
    return "<unknown FreeType error code>";
}

void initHarfBuzz()
{
  int error_count = 0;
  const FT_F26Dot6 ptSize26Dot6 = FT_F26Dot6(round(point_size * 64));

  FT_Error err;

  /* Init freetype */
  err = FT_Init_FreeType(&ft_library);
  if (err) {
      printf("FT_Init_FreeType: %s\n", FreeTypeErrorMessage(err));
      exit(1);
  }

  /* Load our fonts */
  for (size_t ii=0; ii<countof(font_names); ii++) {
    FT_Face ft_face;

    assert(ii < NUM_FONTS);
    err = FT_New_Face(ft_library, font_names[ii], 0, &ft_face);
    if (err) {
        printf("FT_New_Face: %s for %s\n", FreeTypeErrorMessage(err), font_names[ii]);
        exit(1);
    }
    err = FT_Set_Char_Size(ft_face, 0, ptSize26Dot6, device_hdpi, device_vdpi );
    if (err) {
        printf("FT_Set_Char_Size: %s for %s\n", FreeTypeErrorMessage(err), font_names[ii]);
        exit(1);
    }

#ifdef PRE_glPathGlyphIndexRangeNV
    if (!my_glPathGlyphIndexRangeNV) {
      // XXX instead of glPathGlyphIndexRangeNV, just reserve glyphs and create them lazily
      const GLsizei num_glyphs = ft_face->num_glyphs;
      nvpr_glyph_base[ii] = glGenPathsNV(num_glyphs);
      nvpr_glyph_count[ii] = num_glyphs;
    } else
#endif // PRE_glPathGlyphIndexRangeNV
    {
      GLuint base_and_count[2] = { 0xdeadbeef, 0xdeadbabe };
      GLenum fontStatus = GL_FONT_TARGET_UNAVAILABLE_NV;

      switch (NVprAPImode) {
      case GLYPH_INDEX_ARRAY:
        nvpr_glyph_base[ii] = glGenPathsNV(ft_face->num_glyphs);
        nvpr_glyph_count[ii] = ft_face->num_glyphs;
        fontStatus = glPathGlyphIndexArrayNV(nvpr_glyph_base[ii],
          GL_FILE_NAME_NV, font_names[ii], /*fontStyle */0,
          /*firstGlyphIndex*/0, ft_face->num_glyphs,
          path_template, EM_SCALE);
        break;
      case MEMORY_GLYPH_INDEX_ARRAY:
        {
          nvpr_glyph_base[ii] = glGenPathsNV(ft_face->num_glyphs);
          nvpr_glyph_count[ii] = ft_face->num_glyphs;
          long file_size = 0;
          char *file_contents = read_binary_file(font_names[ii], &file_size);
          fontStatus = glPathMemoryGlyphIndexArrayNV(nvpr_glyph_base[ii],
            GL_STANDARD_FONT_FORMAT_NV, file_size, file_contents, /*fontStyle */0,
            /*firstGlyphIndex*/0, ft_face->num_glyphs,
            path_template, EM_SCALE);
        }
        break;
      case GLYPH_INDEX_RANGE:
        fontStatus = glPathGlyphIndexRangeNV(GL_FILE_NAME_NV, 
          font_names[ii], 0,
          path_template, EM_SCALE, base_and_count);
        nvpr_glyph_base[ii] = base_and_count[0];
        nvpr_glyph_count[ii] = base_and_count[1];
        break;
      default:
        assert(!"unknown NVprAPImode");
      }
      if (fontStatus == GL_FONT_GLYPHS_AVAILABLE_NV) {
          printf("Font <%s> is available @ %d for %d glyphs\n",
              font_names[ii], nvpr_glyph_base[ii], nvpr_glyph_count[ii]);
      } else {
        error_count++;
        // Driver should always write zeros on failure
        if (NVprAPImode == GLYPH_INDEX_RANGE) {
          assert(base_and_count[0] == 0);
          assert(base_and_count[1] == 0);
        }
        printf("Font glyphs could not be populated (0x%x)\n", fontStatus);
        switch (fontStatus) {
          case GL_FONT_TARGET_UNAVAILABLE_NV:
            printf("> Font target unavailable\n");
            break;
          case GL_FONT_UNAVAILABLE_NV:
            printf("> Font unavailable\n");
            break;
          case GL_FONT_CORRUPT_NV:
            printf("> Font corrupt\n");
            break;
          case GL_OUT_OF_MEMORY:
            printf("> Out of memory\n");
            break;
          case GL_INVALID_VALUE:
            printf("> Invalid value for glPathGlyphIndexRangeNV (should not happen)\n");
            break;
          case GL_INVALID_ENUM:
            printf("> Invalid enum for glPathGlyphIndexRangeNV (should not happen)\n");
            break;
          default:
            printf("> UNKNOWN reason (should not happen)\n");
            break;
        }
      }
    }

    /* Get our harfbuzz font/face structs */
    hb_ft_font[ii] = hb_ft_font_create(ft_face, hb_destroy_func_t(FT_Done_Face));
  }

#if 0 // XXX hb_blob_t work
  hb_blob_t *blob = NULL;
  {
      const char *font_data;
      unsigned int len;
      hb_destroy_func_t destroy;
      void *user_data;
      hb_memory_mode_t mm;

      const char *font_file = "fonts/DejaVuSerif.ttf";
      //const char *font_file = "c:\\windows\\fonts\\Arial.ttf";
      FILE *f = fopen (font_file, "rb");
      fseek (f, 0, SEEK_END);
      len = ftell (f);
      fseek (f, 0, SEEK_SET);
      font_data = (const char *) malloc (len);
      if (!font_data) len = 0;
      len = fread ((char *) font_data, 1, len, f);
      destroy = free;
      user_data = (void *) font_data;
      fclose (f);
      mm = HB_MEMORY_MODE_WRITABLE;

      blob = hb_blob_create (font_data, len, mm, user_data, destroy);
  }

#if 0
  const char *font_name = "Arial";
  hb_blob_t *blob = hb_blob_create(font_name, int(strlen(font_name)), HB_MEMORY_MODE_READONLY, NULL, destroy_nop);
#endif
  hb_face_t *face = hb_face_create(blob, 1);
  hb_font_t *win32_font = hb_font_create(face);
  hb_font_t *empty_font = hb_font_get_empty();

  int x_scale = 0;
  int y_scale = 0;
  hb_font_get_scale(win32_font, &x_scale, &y_scale);
  printf("font x_scale = %d\n", x_scale);
  printf("font y_scale = %d\n", y_scale);
  hb_font_set_scale(win32_font, 2048, 2048);
  unsigned int x_ppem = 0;
  unsigned y_ppem = 0;
  hb_font_get_ppem(win32_font, &x_ppem, &y_ppem);
  printf("font x_ppem = %u\n", x_ppem);
  printf("font y_ppem = %u\n", y_ppem);

  const char *demo_string = "This is some english text";
  int text_length(int(strlen(demo_string)));
  hb_buffer_t *buf = hb_buffer_create();
  hb_buffer_set_unicode_funcs(buf, hb_ucdn_make_unicode_funcs());
  hb_buffer_add_utf8(buf, demo_string, text_length, 0, text_length);

  hb_buffer_set_direction(buf, HB_DIRECTION_LTR); /* or LTR */
  hb_buffer_set_script(buf, HB_SCRIPT_LATIN); /* see hb-unicode.h */
  const char *language = "en";
  hb_buffer_set_language(buf, hb_language_from_string(language, int(strlen(language))));

  //hb_shape(win32_font, buf, NULL, 0);
  const char *shaper[2] = { "uniscribe", NULL };
  hb_shape_full(win32_font, buf, NULL, 0, shaper);

  unsigned int glyph_count = 0;
  hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

  for (unsigned int j=0; j < glyph_count; ++j) {
      printf("%d: '%c' %d @ (%d,%d) to (%d,%d)\n",
          j, demo_string[j], glyph_info[j].codepoint,
          glyph_pos[j].x_offset, glyph_pos[j].y_offset,
          glyph_pos[j].x_advance, glyph_pos[j].y_advance);
  }
#endif

  if (error_count > 0) {
    printf("NVpr font setup failed with %d errors\n", error_count);
#ifdef _WIN32
    printf("\nCould it be you don't have freetype6.dll in your current directory?\n");
#endif
    exit(1);
  }

#ifdef _WIN32
  if (glPathGlyphIndexRangeNV) {
      const GLbitfield styleMask = 0;
      const char *font_name = "Arial";
      GLuint base_and_count[2];
      GLenum fontStatus;
      fontStatus = glPathGlyphIndexRangeNV(GL_SYSTEM_FONT_NAME_NV, 
          font_name, styleMask,
          path_template, EM_SCALE, base_and_count);
      GLuint arial_base = base_and_count[0];
      GLuint arial_count = base_and_count[1];
      printf("arial_base = %d, count = %d\n", arial_base, arial_count);
      font_name = "Wingdings";
      fontStatus = glPathGlyphIndexRangeNV(GL_SYSTEM_FONT_NAME_NV, 
          font_name, styleMask,
          path_template, EM_SCALE, base_and_count);
      GLuint wingding_base = base_and_count[0];
      GLuint wingding_count = base_and_count[1];
      printf("wingding_base = %d, count = %d\n", wingding_base, wingding_count);
      font_name = "Arial Unicode MS";
      fontStatus = glPathGlyphIndexRangeNV(GL_SYSTEM_FONT_NAME_NV, 
          font_name, styleMask,
          path_template, EM_SCALE, base_and_count);
      GLuint arialuni_base = base_and_count[0];
      GLuint arialuni_count = base_and_count[1];
      printf("arialuni_base = %d, count = %d\n", arialuni_base, arialuni_count);
  }
#endif
}

void updatePointSize(float new_size)
{
  FT_Error err;

  point_size = new_size;

  const FT_F26Dot6 ptSize26Dot6 = FT_F26Dot6(round(point_size * 64));
  for (size_t ii=0; ii<countof(font_names); ii++) {
    err = FT_Set_Char_Size(hb_ft_font_get_face(hb_ft_font[ii]), 0, ptSize26Dot6, device_hdpi, device_vdpi );
    if (err) {
      printf("FT_Set_Char_Size: %s\n", FreeTypeErrorMessage(err));
      exit(1);
    }
  }
}

struct NVprShapedGlyphs {
  GLuint glyph_base;
  float scale;
  vector<GLuint> glyphs;
  vector<GLfloat> xy;

  NVprShapedGlyphs(GLuint glyph_base_, float scale_)
    : glyph_base(glyph_base_)
    , scale(scale_)
  {}
  void reset() {
    glyphs.clear();
    xy.clear();
  }
  void addGlyph(GLfloat x, GLfloat y, GLuint gindex) {
    glyphs.push_back(gindex);
    xy.push_back(x);
    xy.push_back(y);
  }
  void drawFilled(GLuint stencil_write_mask) {
    assert(2*glyphs.size() == xy.size());
    glMatrixPushEXT(GL_MODELVIEW); {
      glMatrixScalefEXT(GL_MODELVIEW, scale, -scale, 1);
      glStencilFillPathInstancedNV(
        GLsizei(glyphs.size()),
        GL_UNSIGNED_INT, &glyphs[0],
        glyph_base,
        GL_COUNT_UP_NV, stencil_write_mask, 
        GL_TRANSLATE_2D_NV, &xy[0]);
      glCoverFillPathInstancedNV(
        GLsizei(glyphs.size()),
        GL_UNSIGNED_INT, &glyphs[0],
        glyph_base,
        GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
        GL_TRANSLATE_2D_NV, &xy[0]);
    } glMatrixPopEXT(GL_MODELVIEW);
  }
  void drawStroked(GLuint stencil_write_mask) {
    assert(2*glyphs.size() == xy.size());
    glMatrixPushEXT(GL_MODELVIEW); {
      glMatrixScalefEXT(GL_MODELVIEW, scale, -scale, 1);
      glStencilStrokePathInstancedNV(
        GLsizei(glyphs.size()),
        GL_UNSIGNED_INT, &glyphs[0],
        glyph_base,
        /*reference*/0x1, stencil_write_mask, 
        GL_TRANSLATE_2D_NV, &xy[0]);
      glCoverStrokePathInstancedNV(
        GLsizei(glyphs.size()),
        GL_UNSIGNED_INT, &glyphs[0],
        glyph_base,
        GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV,
        GL_TRANSLATE_2D_NV, &xy[0]);
    } glMatrixPopEXT(GL_MODELVIEW);
  }
};

vector<NVprShapedGlyphs> shaped_text;

void shapeExampleText(int text_count, const TextSample text[], int width, int height)
{
  float scale = point_size/EM_SCALE;
  float inv_scale = 1/scale;
  float x = inv_scale*0;
  float y = -inv_scale*point_size;

  shaped_text.clear();

  /* Create a buffer for harfbuzz to use */
  hb_buffer_t *buf = hb_buffer_create();
  //alternatively you can use hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
  hb_buffer_set_unicode_funcs(buf, hb_ucdn_make_unicode_funcs());

  for (int i=0; i < text_count; ++i) {
    hb_buffer_clear_contents(buf);

    hb_buffer_set_direction(buf, text[i].text_system->direction); /* or LTR */
    hb_buffer_set_script(buf, text[i].text_system->script); /* see hb-unicode.h */
    const char *language = text[i].text_system->language;
    hb_buffer_set_language(buf, hb_language_from_string(language, int(strlen(language))));

    /* Layout the text */
    int text_length(int(strlen(text[i].text)));
    hb_buffer_add_utf8(buf, text[i].text, text_length, 0, text_length);
    FontType font = text[i].text_system->font;
    hb_shape(hb_ft_font[font], buf, NULL, 0);

    /* Hand the layout to cairo to render */
    unsigned int glyph_count;
    hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t *glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

#if 0
    for (unsigned int j=0; j < glyph_count; ++j) {
        printf("%d: %d @ (%d,%d) to (%d,%d)\n",
            j, glyph_info[j].codepoint,
            glyph_pos[j].x_offset, glyph_pos[j].y_offset,
            glyph_pos[j].x_advance, glyph_pos[j].y_advance);
    }
#endif

    int string_width_in_pixels = 0;
    for (unsigned int j=0; j < glyph_count; ++j) {
      string_width_in_pixels += glyph_pos[j].x_advance/64;
    }

    switch (text[i].text_system->direction) {
    case HB_DIRECTION_LTR:
      x = inv_scale*20; /* left justify */
      break;
    case HB_DIRECTION_RTL:
      x = inv_scale*(width - string_width_in_pixels -20); /* right justify */
      break;
    case HB_DIRECTION_TTB:
      x = inv_scale*(width/2 - string_width_in_pixels/2);  /* center */
      break;
    default:
      assert(!"unknown direction");
    }

    NVprShapedGlyphs a(nvpr_glyph_base[font], scale);

    for (unsigned int j=0; j < glyph_count; ++j) {
      GLuint path = nvpr_glyph_base[font] + glyph_info[j].codepoint;
#ifdef PRE_glPathGlyphIndexRangeNV
      if (!my_glPathGlyphIndexRangeNV) {
        generateGlyph(path, hb_ft_font_get_face(hb_ft_font[font]), glyph_info[j].codepoint);
      } else {
        // glPathGlyphIndexRangeNV has already generated the glyph.
        assert(glIsPathNV(path));
      }
#endif // PRE_glPathGlyphIndexRangeNV
      a.addGlyph(x + inv_scale*glyph_pos[j].x_offset/64,
                 y + inv_scale*glyph_pos[j].y_offset/64,
                 glyph_info[j].codepoint);
      x += inv_scale*glyph_pos[j].x_advance/64;
      y += inv_scale*glyph_pos[j].y_advance/64;  
    }

    shaped_text.push_back(a);

    y += -inv_scale*(point_size*1.5);
  }
  hb_buffer_destroy(buf);
}

void drawShapedExampleText()
{
  const GLuint stencil_write_mask = ~0;
  if (stroking) {
    for (size_t i=0; i<shaped_text.size(); i++) {
      glColor3f(0,0,0); // black
      shaped_text[i].drawStroked(stencil_write_mask);
      glColor3f(1,1,0); // yellow
      shaped_text[i].drawFilled(stencil_write_mask);
    }
  } else {
    glColor3f(1,1,0); // yellow
    for (size_t i=0; i<shaped_text.size(); i++) {
      shaped_text[i].drawFilled(stencil_write_mask);
    }
  }
}

void shutdownHarfBuzz()
{
  /* Cleanup */
  for (int i=0; i < NUM_FONTS; ++i) {
    // This will implicitly call FT_Done_Face on each hb_ft_font element's FT_Face
    // because FT_Done_Face is each hb_font_t's hb_destroy_func_t callback
    hb_font_destroy(hb_ft_font[i]);

#if !defined(PRE_glPathGlyphIndexRangeNV)
    assert(glIsPathNV(nvpr_glyph_base[i]));
    assert(glIsPathNV(nvpr_glyph_base[i]+nvpr_glyph_count[i]-1));
    glDeletePathsNV(nvpr_glyph_base[i], nvpr_glyph_count[i]);
    assert(!glIsPathNV(nvpr_glyph_base[i]));
    assert(!glIsPathNV(nvpr_glyph_base[i]+nvpr_glyph_count[i]-1));
#endif
  }

  FT_Done_FreeType(ft_library);
}

void setBackground()
{
  float r, g, b, a;

  switch (background) {
  default:
  case 0:
    r = g = b = 0.0;
    break;
  case 1:
    r = g = b = 1.0;
    break;
  case 2:
    r = 0.1;
    g = 0.3;
    b = 0.6;
    break;
  case 3:
    r = g = b = 0.5;
    break;
  }
  if (sRGB_capable) {
    r = convertSRGBColorComponentToLinearf(r);
    g = convertSRGBColorComponentToLinearf(g);
    b = convertSRGBColorComponentToLinearf(b);
  }
  a = 1.0;
  glClearColor(r,g,b,a);
}

static void fatalError(const char *message)
{
  fprintf(stderr, "%s: %s\n", program_name, message);
  exit(1);
}

void
initGraphics()
{
  if (hasFramebufferSRGB) {
    glGetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &sRGB_capable);
    if (sRGB_capable) {
      glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    }
  }

  setBackground();

  // Create a null path object to use as a parameter template for creating fonts.
  path_template = glGenPathsNV(1);
  glPathCommandsNV(path_template, 0, NULL, 0, GL_FLOAT, NULL);
  glPathParameterfNV(path_template, GL_PATH_STROKE_WIDTH_NV, 0.1*EM_SCALE);  // 10% of emScale
  glPathParameteriNV(path_template, GL_PATH_JOIN_STYLE_NV, GL_ROUND_NV);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_NOTEQUAL, 0, ~0);
  glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
}

void initViewMatrix()
{
  view = float3x3(1,0,0,
                  0,1,0,
                  0,0,1);
}

float window_width, window_height, aspect_ratio;
float view_width, view_height;
float3x3 win2obj;

void configureProjection()
{
  float3x3 iproj, viewport;

  viewport = ortho(0,window_width, 0,window_height);
  float left = 0, right = canvas_width, top = 0, bottom = canvas_height;
#if 0
  if (aspect_ratio > 1) {
    top *= aspect_ratio;
    bottom *= aspect_ratio;
  } else {
    left /= aspect_ratio;
    right /= aspect_ratio;
  }
#endif
  glMatrixLoadIdentityEXT(GL_PROJECTION);
  glMatrixOrthoEXT(GL_PROJECTION, left, right, bottom, top,
    -1, 1);
  iproj = inverse_ortho(left, right, top, bottom);
  view_width = right - left;
  view_height = bottom - top;
  win2obj = mul(iproj, viewport);
}

void reshape(int w, int h)
{
  reshapeFPScontext(&gl_fps_context, w, h);
  glViewport(0,0,w,h);
  window_width = w;
  window_height = h;
  aspect_ratio = window_height/window_width;

  configureProjection();
}

void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  MatrixLoadToGL(view);

  glEnable(GL_STENCIL_TEST);
  drawShapedExampleText();

  glDisable(GL_STENCIL_TEST);
  handleFPS(&gl_fps_context);

  glutSwapBuffers();
}

void
mouse(int button, int state, int mouse_space_x, int mouse_space_y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {

      float2 win = float2(mouse_space_x, mouse_space_y);
      float3 tmp = mul(win2obj, float3(win,1));
      anchor_x = tmp.x/tmp.z;
      anchor_y = tmp.y/tmp.z;

      rotate_x = mouse_space_x;
      scale_y = mouse_space_y;
      if (!(glutGetModifiers() & GLUT_ACTIVE_CTRL)) {
        scaling = 1;
      } else {
        scaling = 0;
      }
      if (!(glutGetModifiers() & GLUT_ACTIVE_SHIFT)) {
        zooming = 1;
      } else {
        zooming = 0;
      }
    } else {
      zooming = 0;
      scaling = 0;
    }
  }
  if (button == GLUT_MIDDLE_BUTTON) {
    if (state == GLUT_DOWN) {
      slide_x = mouse_space_x;
      slide_y = mouse_space_y;
      sliding = 1;
    } else {
      sliding = 0;
    }
  }
}

void
motion(int mouse_space_x, int mouse_space_y)
{
  if (zooming || scaling) {
    float3x3 t, r, s, m;
    float angle = 0;
    float zoom = 1;
    if (scaling) {
      angle = 0.3 * (rotate_x - mouse_space_x) * canvas_width/window_width;
    }
    if (zooming) {
      zoom = pow(1.003f, (mouse_space_y - scale_y) * canvas_height/window_height);
    }

    t = translate(anchor_x, anchor_y);
    r = rotate(angle);
    s = scale(zoom, zoom);

    r = mul(r, s);
    m = mul(t, r);
    t = translate(-anchor_x, -anchor_y);
    m = mul(m, t);
    view = mul(m, view);
    rotate_x = mouse_space_x;
    scale_y = mouse_space_y;
    glutPostRedisplay();
  }
  if (sliding) {
    float3x3 m;

    float x_offset = (mouse_space_x - slide_x) * view_width/window_width;
    float y_offset = (mouse_space_y - slide_y) * view_height/window_height;
    m = translate(x_offset, y_offset);
    view = mul(m, view);
    slide_y = mouse_space_y;
    slide_x = mouse_space_x;
    glutPostRedisplay();
  }
}

void idle()
{
  glutPostRedisplay();
}

int current_scene = 0;

void loadScene()
{
  switch (current_scene) {
  case 0:
    shapeExampleText(countof(some_text), some_text, canvas_width, canvas_height);
    break;
  case 1:
    shapeExampleText(countof(cat_text), cat_text, canvas_width, canvas_height);
    break;
  case 2:
    shapeExampleText(countof(cloud_text), cloud_text, canvas_width, canvas_height);
    break;
  case 3:
    shapeExampleText(countof(change_text), change_text, canvas_width, canvas_height);
    break;
  }
}

void setScene(int scene)
{
  current_scene = scene;
  loadScene();
}

void
keyboard(unsigned char c, int x, int y)
{
  switch (c) {
  case 27:  /* Esc quits */
    shutdownHarfBuzz();
    exit(0);
    return;
  case '1':
  case '2':
  case '3':
  case '4':
    setScene(c - '1');
    break;
  case '+':
  case '=':
    updatePointSize(point_size + 1.0f);
    printf("point_size = %g\n", point_size);
    loadScene();
    break;
  case '-':
    updatePointSize(point_size - 1.0f);
    printf("point_size = %g\n", point_size);
    loadScene();
    break;
  case 'f':
  case 'F':
    colorFPS(0,1,0);
    toggleFPS();
    break;
  case 'v':
    enable_vsync = !enable_vsync;
    requestSynchornizedSwapBuffers(enable_vsync);
    break;
  case ' ':
    animating = !animating;
    if (animating) {
        glutIdleFunc(idle);
    } else {
        glutIdleFunc(NULL);
    }
    return;
  case 'r':
    initViewMatrix();
    break;
  case 13:  /* Enter redisplays */
    break;
  case 's':
    stroking = !stroking;
    printf("stroking = %d\n", stroking);
    break;
  case 'i':
    initGraphics();
    break;
  case 'b':
    background = (background+1)%4;
    setBackground();
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

static void menu(int item)
{
  keyboard(char(item), 0,0);  // bogus (x,y) location
}

static void createMenu()
{
  glutCreateMenu(menu);
  glutAddMenuEntry("[+] Increase point size", '+');
  glutAddMenuEntry("[-] Decrease point size", '-');
  glutAddMenuEntry("[1] Scene 1", '1');
  glutAddMenuEntry("[2] Scene 2 (lazy cat)", '2');
  glutAddMenuEntry("[3] Scene 3 (cloud)", '3');
  glutAddMenuEntry("[4] Scene 4 (Writing changes everything)", '4');
  glutAddMenuEntry("[ ] Toggle animation", ' ');
  glutAddMenuEntry("[s] Toggle stroking", 's');
  glutAddMenuEntry("[r] Reset view", 'r');
  glutAddMenuEntry("[v] Toggle vsync", 'v');
  glutAddMenuEntry("[f] Toggle showing frame rate", 'F');
  glutAddMenuEntry("[Esc] Quit", '\027');
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

#if defined(linux) || defined(sun)
# define GET_PROC_ADDRESS(name)  glXGetProcAddressARB((const GLubyte *) name)
#elif defined(vxworks)
# define GET_PROC_ADDRESS(name)  rglGetProcAddress(name)
#elif defined(__APPLE__)
# define GET_PROC_ADDRESS(name)  /*nothing*/
#elif defined(_WIN32)
# define GET_PROC_ADDRESS(name)  wglGetProcAddress(name)
#else
# error unimplemented code!
#endif

#ifdef __APPLE__
#define LOAD_PROC(type, name, var)  /*nothing*/
#else
#define LOAD_PROC(type, name, var) \
  var = (type) GET_PROC_ADDRESS(#name); \
  if (!var) { \
    fprintf(stderr, "%s: failed to GetProcAddress for %s\n", program_name, #name); \
    /*exit(1);*/ \
  }
#endif

int
main(int argc, char **argv)
{
  GLenum status;
  GLboolean hasDSA;
  GLboolean has_NV_path_rendering;
  int samples = 0;

  glutInitWindowSize(canvas_width, canvas_height);
  glutInit(&argc, argv);
  for (int i=1; i<argc; i++) {
    if (!strcmp(argv[i], "-gia")) {
      NVprAPImode = GLYPH_INDEX_ARRAY;
      continue;
    }
    if (!strcmp(argv[i], "-mgia")) {
      NVprAPImode = MEMORY_GLYPH_INDEX_ARRAY;
      continue;
    }
    if (!strcmp(argv[i], "-gir")) {
      NVprAPImode = GLYPH_INDEX_RANGE;
      continue;
    }
    if (argv[i][0] == '-') {
      int value = atoi(argv[i]+1);
      if (value >= 1) {
        samples = value;
        continue;
      }
    }
    fprintf(stderr, "usage: %s [-#]\n       where # is the number of samples/pixel\n",
      program_name);
    exit(1);
  }

  if (samples > 0) {
    if (samples == 1) 
      samples = 0;
    printf("requesting %d samples\n", samples);
    char buffer[200];
    sprintf(buffer, "rgb stencil~4 double samples~%d", samples);
    glutInitDisplayString(buffer);
  } else {
    /* Request a double-buffered window with at least 4 stencil bits and
       8 samples per pixel. */
    glutInitDisplayString("rgb stencil~4 double samples~8");
  }
  if (!glutGet(GLUT_DISPLAY_MODE_POSSIBLE)) {
    glutInitDisplayString(NULL);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_STENCIL);
  }

  glutCreateWindow("Text shaped with Harfbuzz and rendered with NV_path_rendering");
  printf("vendor: %s\n", glGetString(GL_VENDOR));
  printf("version: %s\n", glGetString(GL_VERSION));
  printf("renderer: %s\n", glGetString(GL_RENDERER));
  printf("samples = %d\n", glutGet(GLUT_WINDOW_NUM_SAMPLES));
  printf("Executable: %d bit\n", (int)(8*sizeof(int*)));

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  createMenu();
  initViewMatrix();

  status = glewInit();
  if (status != GLEW_OK) {
    fatalError("OpenGL Extension Wrangler (GLEW) failed to initialize");
  }
  // Use glutExtensionSupported because glewIsSupported is unreliable for DSA.
  hasDSA = glutExtensionSupported("GL_EXT_direct_state_access");
  if (!hasDSA) {
    fatalError("OpenGL implementation doesn't support GL_EXT_direct_state_access (you should be using NVIDIA GPUs...)");
  }

  has_NV_path_rendering = glutExtensionSupported("GL_NV_path_rendering");
  if (!has_NV_path_rendering) {
    fatalError("required NV_path_rendering OpenGL extension is not present");
  }
  // GetProcAddress the new glyph index API for NV_path_rendering (old drivers return NULL).
  LOAD_PROC(PFNGLPATHGLYPHINDEXARRAYNVPROC, glPathGlyphIndexArrayNV, my_glPathGlyphIndexArrayNV);
  LOAD_PROC(PFNGLPATHMEMORYGLYPHINDEXARRAYNVPROC, glPathMemoryGlyphIndexArrayNV, my_glPathMemoryGlyphIndexArrayNV);
  LOAD_PROC(PFNGLPATHGLYPHINDEXRANGENVPROC, glPathGlyphIndexRangeNV, my_glPathGlyphIndexRangeNV);

  if (!my_glPathGlyphIndexArrayNV || !my_glPathMemoryGlyphIndexArrayNV || !my_glPathGlyphIndexRangeNV) {
#ifdef PRE_glPathGlyphIndexRangeNV
      printf("  glPathGlyphIndexArrayNV:       %s\n", my_glPathGlyphIndexArrayNV ? "supported" : "MISSING (upgrade your OpenGL driver)");
      printf("  glPathMemoryGlyphIndexArrayNV: %s\n", my_glPathMemoryGlyphIndexArrayNV ? "supported" : "MISSING (upgrade your OpenGL driver)");
      printf("  glPathGlyphIndexRangeNV:       %s\n", my_glPathGlyphIndexRangeNV ? "supported" : "MISSING (upgrade your OpenGL driver)");
      printf("Using emulation of glPath*GlyphIndex*NV commands...\n");
      NVprAPImode = GLYPH_INDEX_RANGE;
#else
      printf("Please upgrade your OpenGL driver to a newer version. (EXITING)\n");
      exit(1);
#endif
  }
  reportMode();
  initGraphics();
  disableFPS();

  initHarfBuzz();
  loadScene();

  initFPScontext(&gl_fps_context, FPS_USAGE_TEXTURE);

  glutMainLoop();
  return 0;
}

