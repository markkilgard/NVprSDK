
TARGET = nvpr_shaders

UNAME := $(shell uname)

CSRCS = \
  ../common/sRGB_math.c \
  ../common/nvpr_glew_init.c \
  ../common/xform.c \
  ../glew/src/glew.c \
  $(NULL)
CPPSRCS = $(TARGET:=.cpp) \
  ../common/cg4cpp_xform.cpp \
  ../cg4cpp/src/inverse.cpp \
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

ifeq ($(UNAME), Darwin)
  CLINKFLAGS += -framework OpenGL -framework GLUT
else
  ifeq ($(findstring CYGWIN, $(UNAME)), CYGWIN)
    CFLAGS += -D_WIN32 -DGLUT_DISABLE_ATEXIT_HACK -DGLEW_STATIC
    # Cg Toolkit includes <GL/glut.h>
    CFLAGS += -I"C:\Program Files\NVIDIA Corporation\Cg\include"
    CLINKFLAGS += -lcgGL -lcg -lglut32 -lglu32 -lopengl32
    EXE = .exe
  else
    CLINKFLAGS += -L../glut/lib/glut
    CLINKFLAGS += -L/usr/X11R6/lib64 -L/usr/X11R6/lib
    CLINKFLAGS += -lCgGL -lCg -lglut -lGLU -lGL -lXi -lX11 -lm -lpthread
  endif
endif

CXXFLAGS=$(CFLAGS)

BINARY := $(TARGET:=$(EXE))

all: $(BINARY)

run: $(TARGET)$(EXE)
	./$(TARGET)$(EXE)

release:
	$(MAKE) RELEASE=1

$(BINARY) : $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(CLINKFLAGS)

clean:
	$(RM) $(BINARY) $(OBJS)

RMDIR = -rm -rf
clobber: clean
	$(RM) *.bak *.o *~
	$(RMDIR) Release/ Debug/

.PHONY: clean clobber release run
