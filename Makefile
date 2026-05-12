##
## compiling under ubuntu:
##  for compiling linux: make
##  for compiling win32: make OSTYPE=win32
##
## Produces four artifacts per platform:
##   jk_botti_mm{.dll,_i386.so}           -- shim w/ runtime CPUID dispatch
##   jk_botti_mm_x87{.dll,_i386.so}       -- i686 / x87 fallback build
##   jk_botti_mm_sse3{.dll,_i386.so}      -- SSE3 build
##   jk_botti_mm_avx2fma{.dll,_i386.so}   -- AVX2+FMA build
##
## Build one variant only:
##   make VARIANT=shim
##   make VARIANT=x87
##   make VARIANT=sse3
##   make VARIANT=avx2fma
##
## Override per-variant flags via SHIM_TARGETFLAGS / X87_TARGETFLAGS /
## SSE3_TARGETFLAGS / AVX2FMA_TARGETFLAGS on the command line.
##

ifeq ($(OSTYPE),win32)
	CXX = i686-w64-mingw32-g++ -m32
	CC = i686-w64-mingw32-gcc -m32
	AR = i686-w64-mingw32-ar rc
	RANLIB = i686-w64-mingw32-ranlib
	LINKFLAGS = -mdll -lm -lwsock32 -lws2_32 -Xlinker --add-stdcall-alias -g
	DLLEND = .dll
	ZLIB_OSFLAGS = -DZ_PREFIX
	OS_DIR = win32
else
	CXX ?= g++ -m32
	CC ?= gcc -m32
	AR ?= ar rc
	RANLIB ?= ranlib
	ARCHFLAG = -fPIC
	LINKFLAGS = -fPIC -shared -ldl -lm -g
	DLLEND = _i386.so
	ZLIB_OSFLAGS = -DZ_PREFIX
	OS_DIR = linux
endif

# Override make's built-in defaults (origin 'default') which ?= won't catch
ifeq ($(origin CC),default)
CC = gcc -m32
endif
ifeq ($(origin AR),default)
AR = ar rc
endif

TARGET = jk_botti_mm

ifneq ($(findstring clang,$(shell $(firstword $(CXX)) --version 2>/dev/null)),)
COMPILER_IS_CLANG := 1
endif

SHIM_TARGETFLAGS    ?= -march=i686 -mtune=generic
X87_TARGETFLAGS     ?= -march=i686 -mtune=generic
ifeq ($(COMPILER_IS_CLANG),1)
SSE3_TARGETFLAGS    ?= -march=i686 -mtune=generic -msse3
AVX2FMA_TARGETFLAGS ?= -march=i686 -mtune=generic -mavx2 -mfma
else
SSE3_TARGETFLAGS    ?= -march=i686 -mtune=generic -msse3 -mfpmath=sse
AVX2FMA_TARGETFLAGS ?= -march=i686 -mtune=generic -mavx2 -mfma -mfpmath=sse
endif
VALGRIND_TARGETFLAGS ?= $(SSE3_TARGETFLAGS)

SHIM_OUT := $(TARGET)$(DLLEND)
X87_OUT  := $(TARGET)_x87$(DLLEND)
SSE3_OUT := $(TARGET)_sse3$(DLLEND)
AVX_OUT  := $(TARGET)_avx2fma$(DLLEND)

ifeq ($(VARIANT),)
# ============================================================================
# Top-level: dispatch variant builds via recursive make.
# ============================================================================

.PHONY: all variants shim x87 sse3 avx2fma test valgrind sanitize coverage clean distclean depend

all: variants

variants: shim x87 sse3 avx2fma

shim:
	+$(MAKE) VARIANT=shim _build

x87:
	+$(MAKE) VARIANT=x87 _build

sse3:
	+$(MAKE) VARIANT=sse3 _build

avx2fma:
	+$(MAKE) VARIANT=avx2fma _build

test:
	+$(MAKE) VARIANT=avx2fma _test

valgrind:
	$(MAKE) -C tests clean
	+$(MAKE) VARIANT=sse3 TARGETFLAGS_OVERRIDE="$(VALGRIND_TARGETFLAGS)" _valgrind

sanitize:
	+$(MAKE) VARIANT=avx2fma _sanitize

coverage:
	+$(MAKE) VARIANT=avx2fma _coverage

depend:
	+$(MAKE) VARIANT=avx2fma _depend

clean:
	rm -rf obj
	rm -f $(SHIM_OUT) $(X87_OUT) $(SSE3_OUT) $(AVX_OUT)
	$(MAKE) -C tests clean

distclean:
	rm -rf obj
	rm -f $(SHIM_OUT) $(X87_OUT) $(SSE3_OUT) $(AVX_OUT)
	rm -f $(TARGET).dll $(TARGET)_x87.dll $(TARGET)_sse3.dll $(TARGET)_avx2fma.dll
	rm -f $(TARGET)_i386.so $(TARGET)_x87_i386.so $(TARGET)_sse3_i386.so $(TARGET)_avx2fma_i386.so
	rm -rf addons/jk_botti/dlls/*

else
# ============================================================================
# Variant build (VARIANT is set).
# ============================================================================

ifeq ($(VARIANT),shim)
	TARGETFLAGS := $(SHIM_TARGETFLAGS)
	VARIANT_SUFFIX :=
else ifeq ($(VARIANT),x87)
	TARGETFLAGS := $(X87_TARGETFLAGS)
	VARIANT_SUFFIX := _x87
else ifeq ($(VARIANT),sse3)
	TARGETFLAGS := $(SSE3_TARGETFLAGS)
	VARIANT_SUFFIX := _sse3
else ifeq ($(VARIANT),avx2fma)
	TARGETFLAGS := $(AVX2FMA_TARGETFLAGS)
	VARIANT_SUFFIX := _avx2fma
else
	$(error Unknown VARIANT=$(VARIANT) (valid: shim, x87, sse3, avx2fma))
endif

# Expose the variant tag to the C++ code (Plugin_info.version string).
ifneq ($(VARIANT_SUFFIX),)
	VERFLAGS_EXTRA = -DVARIANT_VERSION_SUFFIX=\"$(VARIANT_SUFFIX)\"
endif

# Allow top-level targets (e.g. valgrind) to force alternate flags.
ifneq ($(TARGETFLAGS_OVERRIDE),)
	TARGETFLAGS := $(TARGETFLAGS_OVERRIDE)
endif

OBJDIR := obj/$(OS_DIR)-$(VARIANT)
OUTPUT := $(TARGET)$(VARIANT_SUFFIX)$(DLLEND)

VER_MAJOR ?= 0
VER_MINOR ?= 0
VER_NOTE  ?= git$(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)
VERFLAGS   = -DVER_MAJOR=$(VER_MAJOR) -DVER_MINOR=$(VER_MINOR) -DVER_NOTE=\"$(VER_NOTE)\" $(VERFLAGS_EXTRA)

BASEFLAGS  = -Wall -Wno-write-strings $(VERFLAGS)
BASEFLAGS += -fno-strict-aliasing -fno-strict-overflow
BASEFLAGS += -fvisibility=hidden
ifneq ($(COMPILER_IS_CLANG),1)
BASEFLAGS += -Wno-class-memaccess -mincoming-stack-boundary=2
endif
ARCHFLAG  += $(TARGETFLAGS)

ifeq ($(DBG_FLGS),1)
	OPTFLAGS = -O0 -g
else
	OPTFLAGS  = -O2 -fomit-frame-pointer -g
	OPTFLAGS += -fno-semantic-interposition
endif

INCLUDES = -I"./metamod" \
	-I"./common" \
	-I"./dlls" \
	-I"./engine" \
	-I"./pm_shared"

CFLAGS    = $(BASEFLAGS) $(OPTFLAGS) $(ARCHFLAG) $(INCLUDES)
CXXFLAGS  = -fno-rtti -fno-exceptions
CXXFLAGS += $(CFLAGS)

.PHONY: _build _test _valgrind _sanitize _coverage _depend

# ----------------------------------------------------------------------------
# Shim variant: single-file forwarder, no zlib, no HL SDK headers.
# ----------------------------------------------------------------------------
ifeq ($(VARIANT),shim)

SHIM_CFLAGS = -Wall -Wextra -Wno-unused-parameter $(OPTFLAGS) $(ARCHFLAG) -std=gnu99

_build: $(OUTPUT)

$(OUTPUT): shim.c | $(OBJDIR)
	$(CC) $(SHIM_CFLAGS) -o $@ $< $(LINKFLAGS)
	mkdir -p addons/jk_botti/dlls
	cp $@ addons/jk_botti/dlls/
ifneq ($(OSTYPE),win32)
	ln -sf $(OUTPUT) addons/jk_botti/dlls/$(TARGET).so
endif

$(OBJDIR):
	mkdir -p $@

else
# ----------------------------------------------------------------------------
# Full-plugin variants (sse, avx2fma): compile all C++ sources + zlib.
# ----------------------------------------------------------------------------

SRC = 	bot.cpp \
	bot_chat.cpp \
	bot_client.cpp \
	bot_combat.cpp \
	bot_trace.cpp \
	bot_config_init.cpp \
	bot_models.cpp \
	bot_navigate.cpp \
	bot_query_hook.cpp \
	bot_query_hook_linux.cpp \
	bot_query_hook_win32.cpp \
	bot_skill.cpp \
	bot_sound.cpp \
	bot_weapons.cpp \
	commands.cpp \
	dll.cpp \
	engine.cpp \
	h_export.cpp \
	random_num.cpp \
	safe_snprintf.cpp \
	util.cpp \
	waypoint.cpp

OBJ      = $(SRC:%.cpp=$(OBJDIR)/%.o)
ZLIB_LIB = $(OBJDIR)/zlib/libz.a
DEP_FILE = $(OBJDIR)/Rules.depend

_build: $(OUTPUT)

$(OUTPUT): $(ZLIB_LIB) $(OBJ)
	$(CC) -o $@ $(OBJ) $(ZLIB_LIB) $(LINKFLAGS)
	mkdir -p addons/jk_botti/dlls
	cp $@ addons/jk_botti/dlls/

$(ZLIB_LIB): | $(OBJDIR)/zlib
	(cd $(OBJDIR)/zlib; AR="$(AR)" ARFLAGS="" RANLIB="$(RANLIB)" CC="$(CC) $(OPTFLAGS) $(ARCHFLAG) $(ZLIB_OSFLAGS)" $(CURDIR)/zlib/configure --static; $(MAKE) libz.a CC="$(CC) $(OPTFLAGS) $(ARCHFLAG) $(ZLIB_OSFLAGS)" AR="$(AR)" ARFLAGS="" RANLIB="$(RANLIB)")

_test: $(ZLIB_LIB)
	$(MAKE) -C tests CXXFLAGS="$(CXXFLAGS)" ZLIB_LIB="../$(ZLIB_LIB)" run

_valgrind: $(ZLIB_LIB)
	$(MAKE) -C tests CXXFLAGS="$(CXXFLAGS)" ZLIB_LIB="../$(ZLIB_LIB)" valgrind

_sanitize: $(ZLIB_LIB)
	$(MAKE) -C tests CXXFLAGS="$(CXXFLAGS)" ZLIB_LIB="../$(ZLIB_LIB)" sanitize

_coverage: $(ZLIB_LIB)
	$(MAKE) -C tests CXXFLAGS="$(CXXFLAGS)" ZLIB_LIB="../$(ZLIB_LIB)" coverage

_depend: $(DEP_FILE)

$(DEP_FILE): Makefile $(SRC) | $(OBJDIR)
	$(CXX) -MM $(INCLUDES) $(SRC) | sed 's|^\([^ ]\)|$(OBJDIR)/\1|' > $@

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CXX) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/zlib:
	mkdir -p $@

-include $(DEP_FILE)

endif # VARIANT=shim

endif # VARIANT set
