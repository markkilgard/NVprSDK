
TARGETS = nvpr_svg

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
  GLEW  := /Developer/NVIDIA/Cg/examples/OpenGL/glew
else
  GLEW  := ../glew
endif

SKIA  := ../skia

CG4CPP  := ../cg4cpp
BOOST   := ../boost-1.50.0

GS_SIMPLE_C = \
  ../common/showfps.c \
  stb/stb_image.c \
  stb/stb_image_write.c \
  ../glew/src/glew.c \
  ../common/nvpr_glew_init.c \
  ../common/sRGB_math.c \
  ../common/request_vsync.c \
  $(NULL)

GS_SIMPLE_CPP = \
  color_names.cpp \
  freetype2_loader.cpp \
  nvpr_svg.cpp \
  glmatrix.cpp \
  path.cpp \
  path_data.cpp \
  path_process.cpp \
  path_parse_svg.cpp \
  scene.cpp \
  renderer.cpp \
  ActiveControlPoint.cpp \
  sRGB_vector.cpp \
  stc/renderer_stc.cpp  \
  stc/scene_stc.cpp \
  openvg/renderer_openvg.cpp  \
  openvg/scene_openvg.cpp \
  tinyxml/tinystr.cpp \
  tinyxml/tinyxml.cpp \
  tinyxml/tinyxmlerror.cpp \
  tinyxml/tinyxmlparser.cpp \
  svg_files.cpp \
  svg_loader.cpp \
  nvpr/renderer_nvpr.cpp \
  nvpr/renderer_nvpr_path.cpp \
  nvpr/renderer_nvpr_shader.cpp \
  ../common/dsa_emulate.cpp \
  cairo/renderer_cairo.cpp \
  cairo/scene_cairo.cpp \
  qt/renderer_qt.cpp \
  qt/scene_qt.cpp \
  skia/renderer_skia.cpp \
  skia/scene_skia.cpp \
  d2d/init_d2d.cpp \
  d2d/renderer_d2d.cpp \
  d2d/scene_d2d.cpp \
  ../cg4cpp/src/inverse.cpp \
  $(NULL)

GS_SIMPLE_OBJS = $(GS_SIMPLE_C:.c=.o) $(GS_SIMPLE_CPP:.cpp=.o)

OBJS = $(GS_SIMPLE_OBJS)

#DEBUG_OPT = -g -DSK_DEBUG
OPT_OPT = -O2 -DNDEBUG

CC         = gcc
CXX        = g++
CFLAGS     += -Wall $(DEBUG_OPT) $(OPT_OPT)
CXXFLAGS   += -Wall $(DEBUG_OPT) $(OPT_OPT)
CFLAGS     += -DGLEW_STATIC
CFLAGS     += -I.
CXXFLAGS   += -I.
CFLAGS     += -I../common
CXXFLAGS   += -I../common
CXXFLAGS   += -I../boost-1.38.0
CXXFLAGS   += -I../openvg-1_1-ri/ri_package/ri/include
CFLAGS     += -Iinclude
CXXFLAGS   += -Iinclude
CXXFLAGS   += -Itinyxml
CFLAGS     += -I"$(GLEW)/include"
CXXFLAGS   += -I"$(GLEW)/include"
CFLAGS     += -I"$(CG4CPP)/include"
CXXFLAGS   += -I"$(CG4CPP)/include"
CFLAGS     += -I"$(BOOST)"
CXXFLAGS   += -I"$(BOOST)"
CXXFLAGS   += -I"$(SKIA)/include/core" -I"$(SKIA)/include/config" -I"$(SKIA)/include/effects" -I"$(SKIA)/src/core"
CXXFLAGS   += -DGLEW_STATIC
ifdef CG_INC_PATH
  CFLAGS   += -I"$(CG_INC_PATH)"
  CXXFLAGS += -I"$(CG_INC_PATH)"
endif

ifdef CG_LIB_PATH
    CLINKFLAGS += -L"$(CG_LIB_PATH)"
endif

DEPEND_FILES = $(OBJS:%.o=%.d)
DEPEND_OPTS = -MMD

SKIA_LIB_NAMES = core effects gr opts opts_ssse3 ports skgr utils core
SKIA_LIB_OPTS = $(SKIA_LIB_NAMES:%=-l%)
SKIA_LIB_OPTS += -L../skia/out/Debug/obj.target/gyp/

ifeq ($(UNAME), Darwin)
  #CLINKFLAGS += -L/usr/X11/lib
  CLINKFLAGS += -L/Developer/SDKs/MacOSX10.5.sdk/usr/X11/lib
  CLINKFLAGS += -lfreetype
  CLINKFLAGS += -framework Cg
  CLINKFLAGS += -framework GLUT
  CLINKFLAGS += -framework OpenGL
  CXXFLAGS   += -I/usr/X11/include
  CXXFLAGS   += -I/usr/X11/include/freetype2
  CXXFLAGS   += -I/usr/include/freetype2
  CLINKFLAGS += -Wl,-dylib_file,/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib
  CFLAGS     += $(DEPEND_OPTS)
  CXXFLAGS   += $(DEPEND_OPTS)
else
  ifeq ($(findstring CYGWIN, $(UNAME)), CYGWIN)
    CFLAGS     += -D_WIN32
    CXXFLAGS   += -D_WIN32
    CFLAGS     += -DGLUT_DISABLE_ATEXIT_HACK
    CXXFLAGS   += -DGLUT_DISABLE_ATEXIT_HACK

    CFLAGS     += -I"C:/Program Files/NVIDIA Corporation/Cg/include"
    CXXFLAGS   += -I"C:/Program Files/NVIDIA Corporation/Cg/include"
    CLINKFLAGS += -L"C:/Program Files/NVIDIA Corporation/Cg/lib"
    # XXX assume Visual Studio built Release/cg4cpp.lib
    CLINKFLAGS += -LRelease
    CLINKFLAGS += -lcg4cpp
    CLINKFLAGS += -lcgGL -lcg
    CLINKFLAGS += -lglut32
    CLINKFLAGS += -lglu32 -lopengl32 -lm
    EXE = .exe
  else
    ifeq ($(UNAME), SunOS)
      CLINKFLAGS += -L"$(SKIA)/out"
      CLINKFLAGS += $(SKIA_LIB_OPTS)
      CLINKFLAGS += -lcairo
      CLINKFLAGS += -lfreetype
      CLINKFLAGS += -lCgGL -lCg
      CLINKFLAGS += -L../glut/lib/glut
      CLINKFLAGS += -L/usr/X11R6/lib64 -L/usr/X11R6/lib
      CLINKFLAGS += -lglut -lXi -lX11 -lm
      CLINKFLAGS += -lGLU -lGL
      CLINKFLAGS += -lpthread
      CXXFLAGS   += -I/usr/include/cairo
    else
      CLINKFLAGS += -L"$(SKIA)/out"
      CLINKFLAGS += $(SKIA_LIB_OPTS)
      CLINKFLAGS += -lcairo
#      CLINKFLAGS += -lQtGui -lQtCore
      CLINKFLAGS += -lfreetype
      CLINKFLAGS += -lCgGL -lCg
      CLINKFLAGS += -L../glut/lib/glut
      CLINKFLAGS += -L/usr/X11R6/lib64 -L/usr/X11R6/lib
      CLINKFLAGS += -lglut -lXi -lX11 -lm
      CLINKFLAGS += -lGLU -lGL
      CLINKFLAGS += -lpthread
      CFLAGS     += $(DEPEND_OPTS)
      CXXFLAGS   += $(DEPEND_OPTS)
      CXXFLAGS   += -I/usr/include/freetype2
      CXXFLAGS   += -I/usr/include/cairo
      CXXFLAGS   += -I/usr/include/qt4
      CXXFLAGS   += -I/usr/include/qt4/QtCore
      CXXFLAGS   += -I/usr/include/qt4/QtGui
      CXXFLAGS   += -I/usr/include/QtCore
      CXXFLAGS   += -I/usr/include/QtGui
    endif
  endif
endif

BINARIES := $(TARGETS:=$(EXE))

all: $(BINARIES)

run: $(TARGET)$(EXE)
	./$(TARGET)$(EXE)

SKIA_LIB = $(SKIA)/out/libskia.a

LIBRARIES_TO_BUILD = $(SKIA_LIB)

$(SKIA_LIB):
	$(MAKE) -C '$(SKIA)' -f Makefile

nvpr_svg$(EXE): $(LIBRARIES_TO_BUILD) $(GS_SIMPLE_OBJS)
	$(CXX) $(CFLAGS) $(GS_SIMPLE_OBJS) -o $@ $(CLINKFLAGS)

clean:
	$(RM) $(BINARIES) $(GS_SIMPLE_OBJS) 
	$(MAKE) -C '$(SKIA)' -f Makefile clean

clobber: clean
	$(RM) *.bak *.o *~ $(DEPEND_FILES)

.PHONY: all run clean clobber run

sinclude $(DEPEND_FILES)
