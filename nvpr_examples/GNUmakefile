
# GNUmakefile for GNU make

# broadcast making a specified target to all SUBDIRS

SUBDIRS =\
  nvpr_basic \
  nvpr_cursive \
  nvpr_font_file \
  nvpr_gradient \
  nvpr_hello_world \
  nvpr_korean \
  nvpr_pick \
  nvpr_shaders \
  nvpr_svg \
  nvpr_text_wheel \
  nvpr_tiger \
  nvpr_tiger3d \
  nvpr_warp_tiger \
  nvpr_welsh_dragon \
  nvpr_whitepaper \
  $(NULL)

# use + prefix for rules using this macro so GNU make knows to look for $(MAKE) being spawned for parallel builds
define SPAWN_MAKE
	$(MAKE) -C $(1) -f GNUmakefile $@

endef

.DEFAULT all clean:
	+$(foreach dir,$(SUBDIRS),$(call SPAWN_MAKE,$(dir)))

.PHONY: all clean release
