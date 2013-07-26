# how to get to this directory from cglang directory

SOLNDIR  := ../../../../tools/cg/Common/boost-1.50.0

PROJECTS :=
PROJECTS += boostlib

include ../../../../cg/main/src/build_tools/cg/common.mk

# Targets used for DVS builds

.phony: dvs dvs.release dvs.debug dvs.both

dvs:
ifeq ($(OS),Darwin)
	@$(MAKE) -f $(MAKEFILE) TARGET=Darwin_i386
	@$(MAKE) -f $(MAKEFILE) TARGET=Darwin_ppc
	@$(MAKE) -f $(MAKEFILE) TARGET=Darwin_x86_64
	@$(MAKE) -f $(MAKEFILE) TARGET=Darwin_universal lipo
else
ifeq ($(OS),Windows)
	@$(MAKE) -f $(MAKEFILE) TARGET=Windows_vc7
	@$(MAKE) -f $(MAKEFILE) TARGET=Windows_vc7_64
	@$(MAKE) -f $(MAKEFILE) TARGET=Windows_vc8
	@$(MAKE) -f $(MAKEFILE) TARGET=Windows_vc8_64
	@$(MAKE) -f $(MAKEFILE) TARGET=Windows_vc9
	@$(MAKE) -f $(MAKEFILE) TARGET=Windows_vc9_64
	@$(MAKE) -f $(MAKEFILE) TARGET=Windows_vc10
	@$(MAKE) -f $(MAKEFILE) TARGET=Windows_vc10_64
else
ifeq ($(OS),Linux)
	@$(MAKE) -f $(MAKEFILE) TARGET=Linux_gcc4
	@$(MAKE) -f $(MAKEFILE) TARGET=Linux_gcc4_64
else
ifeq ($(OS),SunOS)
	@$(MAKE) -f $(MAKEFILE) TARGET=SunOS_studio12
	@$(MAKE) -f $(MAKEFILE) TARGET=SunOS_studio12_64
else
	@$(MAKE) -f $(MAKEFILE)
endif
endif
endif
endif

dvs.release:
	@$(MAKE) -f $(MAKEFILE) RELEASE=1 dvs

dvs.debug:
	@$(MAKE) -f $(MAKEFILE) RELEASE= dvs

dvs.both: dvs.debug dvs.release

