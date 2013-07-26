PACKAGEONLY=1

CG_TOP := ../../../../..

include $(CG_TOP)/src/build_tools/cg/common.mk

# how to get to this directory from src/build directory
SOLNDIR  := ../examples/OpenGL/glew/include/GL

DEST_DIR=$(EXAMPLES_BUILTDIR)/OpenGL/glew/include/GL

PACKAGE_FILES :=
PACKAGE_FILES += glew.h
PACKAGE_FILES += glxew.h
PACKAGE_FILES += wglew.h

package::
	$(foreach file,$(PACKAGE_FILES),$(call PACKAGE_FILE,$(file),$(DEST_DIR)))
