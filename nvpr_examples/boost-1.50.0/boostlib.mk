# boostlib.mk

include ../../../../cg/main/src/build_tools/cg/getprofile.mk

STATIC_LIBRARY    := boostlib_1.50.0
PROJ_DIR          := .

DEFINES += BOOST_USER_CONFIG=\"cgiNew.h\"

ifneq ($(PROFILE_OS), Windows)
FILES += libs/thread/src/pthread/once.cpp
FILES += libs/thread/src/pthread/thread.cpp
endif
FILES += libs/thread/src/tss_null.cpp
ifeq ($(PROFILE_OS), Windows)
FILES += libs/thread/src/win32/thread.cpp
FILES += libs/thread/src/win32/tss_dll.cpp
FILES += libs/thread/src/win32/tss_pe.cpp
endif

# boost System Library

FILES += libs/system/src/error_code.cpp

# Use STLport for all platforms except Solaris

ifneq ($(PROFILE_OS), SunOS)
INCLUDES      += $(STLPORT_INCLUDES)
CXXFLAGS      += $(STLPORT_CXXFLAGS)
DEBUG_DEFINES += _STLP_DEBUG=1
DEBUG_DEFINES += _STLP_DEBUG_ALLOC=1
DEBUG_DEFINES += _STLP_DEBUG_UNINITIALIZED=1
endif

# Use Boost for all platforms

INCLUDES  += $(BOOST_INCLUDES)
CXXFLAGS  += $(BOOST_CXXFLAGS)
DEFINES   += BOOST_THREAD_BUILD_LIB

include ../../../../cg/main/src/build_tools/cg/common.mk
