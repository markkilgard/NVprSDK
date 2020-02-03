# miniDXUT.mk

include ../../../src/build_tools/cg/getprofile.mk

STATIC_LIBRARY    := miniDXUT
PROJ_DIR          := .

# DX9

INCLUDES += $(DX9_INCLUDES)
LIBDIRS  += $(DX9_LIBDIRS)

INCLUDES += .

DEFINES += _UNICODE UNICODE MINI_DXUT

FILES += DXUT.cpp
FILES += DXUTenum.cpp
FILES += DXUTmisc.cpp

INSTALL_FILES += DXUT.h
INSTALL_FILES += DXUTenum.h
INSTALL_FILES += DXUTmisc.h
INSTALL_FILES += ReadMe.txt
INSTALL_FILES += dxstdafx.cpp
INSTALL_FILES += dxstdafx.h

NO_PACKAGE_BINARIES := 1

ifneq (Windows,$(OS))
BUILD_PROJECT := 0
endif

include ../../../src/build_tools/cg/common.mk

DEST_DIR=$(EXAMPLES_BUILTDIR)/Direct3D9/miniDXUT
PACKAGE_FILES=\
    $(FILES) \
    $(INSTALL_FILES) \
    $(NULL)
package::
	$(foreach file,$(PACKAGE_FILES),$(call PACKAGE_FILE,$(file),$(DEST_DIR)))
