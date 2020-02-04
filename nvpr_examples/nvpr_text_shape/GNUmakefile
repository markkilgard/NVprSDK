
TARGET = nvpr_text_shape

UNAME := $(shell uname)

BUILD_FREETYPE := yes

# FreeType releated macros
ifeq ($(BUILD_FREETYPE), yes)
FREETYPE_DIR = ../freetype-2.4.9
FREETYPE_LIB_FILE = $(FREETYPE_DIR)/objs/.libs/libfreetype.a
endif

# HarfBuzz -- OpenType text shaping engine
HARFBUZZ_DIR = ../harfbuzz
HARFBUZZ_LIB_FILE = $(HARFBUZZ_DIR)/src/.libs/libharfbuzz.a

UCDN_DIR = ../ucdn

CSRCS = \
  ../common/sRGB_math.c \
  ../common/nvpr_glew_init.c \
  ../common/showfps.c \
  ../common/request_vsync.c \
  ../glew/src/glew.c \
  ../cg4cpp/src/inverse.c \
  $(UCDN_DIR)/ucdn.c \
  $(NULL)
CPPSRCS = \
  nvpr_text_shape.cpp \
  ../common/read_file.cpp \
  ../common/cg4cpp_xform.cpp \
  $(NULL)
OBJS = $(CSRCS:.c=.o) $(CPPSRCS:.cpp=.o)

CC = gcc
CXX = g++
CFLAGS += -Wall
ifdef RELEASE
CFLAGS += -O2
else
CFLAGS += -g
endif
CFLAGS += -I../common
CFLAGS += -I../glew/include
CFLAGS += -I../cg4cpp/include
CFLAGS += -I$(HARFBUZZ_DIR)/src
CFLAGS += -I$(UCDN_DIR)

ifeq ($(UNAME), Darwin)
  ifeq ($(BUILD_FREETYPE), yes)
    CXXFLAGS   += -I$(FREETYPE_DIR)/include
    CLINKFLAGS += -lfreetype -L$(dir $(FREETYPE_LIB_FILE))
  else
    CXXFLAGS   += -I/usr/X11/include/freetype2
    CLINKFLAGS += -lfreetype
    #CLINKFLAGS += -L/Developer/SDKs/MacOSX10.5.sdk/usr/X11/lib
    #CLINKFLAGS += -L/Developer/SDKs/MacOSX10.6.sdk/usr/X11/lib
    CLINKFLAGS += -L/Developer/SDKs/MacOSX10.7.sdk/usr/X11/lib
  endif
  CLINKFLAGS += -framework OpenGL -framework GLUT
else
  GLUT_DIR := ../glut/lib/glut
  GLUT_LIB := $(GLUT_DIR)/libglut.a
  ifeq ($(findstring CYGWIN, $(UNAME)), CYGWIN)
    CFLAGS += -D_WIN32 -DGLUT_DISABLE_ATEXIT_HACK -DGLEW_STATIC
    # Cg Toolkit includes <GL/glut.h>
    CFLAGS += -I"C:\Program Files\NVIDIA Corporation\Cg\include"
    CLINKFLAGS += -lglut32 -lglu32 -lopengl32
    EXE = .exe
  else
    CLINKFLAGS += -L$(GLUT_DIR)
    CLINKFLAGS += -lharfbuzz -L$(dir $(HARFBUZZ_LIB_FILE))
    ifeq ($(BUILD_FREETYPE), yes)
      CFLAGS     += -I$(FREETYPE_DIR)/include
      CLINKFLAGS += -lfreetype -L$(dir $(FREETYPE_LIB_FILE))
    else
      CLINKFLAGS += -lfreetype
    endif
    CLINKFLAGS += -lglib-2.0
    CLINKFLAGS += -lz
    CLINKFLAGS += -lbz2
    CLINKFLAGS += -L/usr/X11R6/lib64 -L/usr/X11R6/lib
    CLINKFLAGS += -lglut -lGLU -lGL -lXi -lX11 -lm -lpthread
  endif
endif

CXXFLAGS=$(CFLAGS)

BINARY := $(TARGET:=$(EXE))

all: $(BINARY)

ifeq ($(BUILD_FREETYPE), yes)
FREETYPE_CONFIGURE_OPTIONS = --enable-shared=no --enable-static

LIBRARIES_TO_BUILD += $(FREETYPE_LIB_FILE)
$(FREETYPE_LIB_FILE) : FORCE
	@echo Building FreeType
	( export CFG='$(FREETYPE_CONFIGURE_OPTIONS)'; $(MAKE) -C '$(FREETYPE_DIR)' -f Makefile )
	$(MAKE) -C '$(FREETYPE_DIR)' -f Makefile
endif

$(HARFBUZZ_DIR)/Makefile :
	( cd $(HARFBUZZ_DIR) && ./autogen.sh )

LIBRARIES_TO_BUILD += $(HARFBUZZ_LIB_FILE)
$(HARFBUZZ_LIB_FILE) : FORCE $(HARFBUZZ_DIR)/Makefile
	@echo Building Harfbuzz
	$(MAKE) -C '$(HARFBUZZ_DIR)' -f Makefile

run: $(TARGET)$(EXE)
	./$(TARGET)$(EXE)

release:
	$(MAKE) RELEASE=1

$(BINARY) : $(OBJS) | GNUmakefile $(LIBRARIES_TO_BUILD)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(CLINKFLAGS)

clean:
	$(RM) $(BINARY) $(OBJS)

RMDIR = -rm -rf
clobber: clean
	-$(RM) *.bak *.o *~
	-$(RMDIR) Release/ Debug/
	-$(MAKE) -C '$(HARFBUZZ_DIR)' -f Makefile clean
	-$(MAKE) -C '$(FREETYPE_DIR)' -f Makefile clean

LIBRARIES_TO_BUILD += $(GLUT_LIB)
$(GLUT_LIB): FORCEglut
	$(MAKE) -C '$(GLUT_DIR)' -f GNUmakefile

.PHONY: clean clobber release run FORCEglut FORCE
