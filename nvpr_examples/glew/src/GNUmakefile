PACKAGEONLY=1

CG_TOP := ../../../..

include $(CG_TOP)/src/build_tools/cg/common.mk

# how to get to this directory from src/build directory
SOLNDIR  := ../examples/OpenGL/glew/src

DEST_DIR=$(EXAMPLES_BUILTDIR)/OpenGL/glew/src

PACKAGE_FILES :=
PACKAGE_FILES += glew.c

package::
	$(foreach file,$(PACKAGE_FILES),$(call PACKAGE_FILE,$(file),$(DEST_DIR)))
