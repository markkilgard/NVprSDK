
# how to get to this directory from cglang directory
SOLNDIR  := ../../examples/Direct3D9/miniDXUT

PROJECTS += miniDXUT

include ../../../src/build_tools/cg/getprofile.mk

include ../../../src/build_tools/cg/common.mk

# package macros
DEST_DIR=$(EXAMPLES_BUILTDIR)/Direct3D9/miniDXUT

PACKAGE_FILES += Direct3D9_miniDXUT_2003.sln
PACKAGE_FILES += Direct3D9_miniDXUT_2005.sln
PACKAGE_FILES += Direct3D9_miniDXUT_2008.sln
PACKAGE_FILES += Direct3D9_miniDXUT_2010.sln

PACKAGE_FILES += miniDXUT_2003.vcproj
PACKAGE_FILES += miniDXUT_2005.vcproj
PACKAGE_FILES += miniDXUT_2008.vcproj
PACKAGE_FILES += miniDXUT_2010.vcxproj

package::
	$(foreach file,$(PACKAGE_FILES),$(call PACKAGE_FILE,$(file),$(DEST_DIR)))
