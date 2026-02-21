##
## compiling under ubuntu:
##  for compiling linux: make
##  for compiling win32: make OSTYPE=win32
##

ifeq ($(OSTYPE),win32)
	CXX = i686-w64-mingw32-g++ -m32
	CC = i686-w64-mingw32-gcc -m32
	AR = i686-w64-mingw32-ar rc
	RANLIB = i686-w64-mingw32-ranlib
	LINKFLAGS = -mdll -lm -lwsock32 -lws2_32 -Xlinker --add-stdcall-alias -g
	DLLEND = .dll
	ZLIB_OSFLAGS = -DZ_PREFIX
else
	CXX ?= g++ -m32
	CC ?= gcc -m32
	AR ?= ar rc
	RANLIB ?= ranlib
	ARCHFLAG = -fPIC
	LINKFLAGS = -fPIC -shared -ldl -lm -g
	DLLEND = _i386.so
	ZLIB_OSFLAGS = -DZ_PREFIX
endif

VER_MAJOR ?= 0
VER_MINOR ?= 0
VER_NOTE ?= git$(shell git rev-parse --short HEAD 2>/dev/null || echo unknown)

VERFLAGS = -DVER_MAJOR=$(VER_MAJOR) -DVER_MINOR=$(VER_MINOR) -DVER_NOTE=\"$(VER_NOTE)\"

TARGET = jk_botti_mm
BASEFLAGS = -Wall -Wno-write-strings -Wno-class-memaccess ${VERFLAGS}
BASEFLAGS += -fno-strict-aliasing -fno-strict-overflow
BASEFLAGS += -fvisibility=hidden
ARCHFLAG += -march=i686 -mtune=generic -msse -msse2 -msse3 -mincoming-stack-boundary=2

ifeq ($(DBG_FLGS),1)
	OPTFLAGS = -O0 -g
else
	OPTFLAGS = -O2 -fomit-frame-pointer -g
	OPTFLAGS += -fno-semantic-interposition
endif

INCLUDES = -I"./metamod" \
	-I"./common" \
	-I"./dlls" \
	-I"./engine" \
	-I"./pm_shared"

CFLAGS = ${BASEFLAGS} ${OPTFLAGS} ${ARCHFLAG} ${INCLUDES}
CXXFLAGS = -fno-rtti -fno-exceptions
CXXFLAGS += ${CFLAGS}

SRC = 	bot.cpp \
	bot_chat.cpp \
	bot_client.cpp \
	bot_combat.cpp \
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
	safe_snprintf.cpp \
	util.cpp \
	waypoint.cpp

OBJ = $(SRC:%.cpp=%.o)

${TARGET}${DLLEND}: zlib/libz.a ${OBJ}
	${CC} -o $@ ${OBJ} zlib/libz.a ${LINKFLAGS}
	cp $@ addons/jk_botti/dlls/

zlib/libz.a:
	(cd zlib; AR="${AR}" ARFLAGS="" RANLIB="${RANLIB}" CC="${CC} ${OPTFLAGS} ${ARCHFLAG} ${ZLIB_OSFLAGS}" ./configure --static; $(MAKE) libz.a CC="${CC} ${OPTFLAGS} ${ARCHFLAG} ${ZLIB_OSFLAGS}" AR="${AR}" ARFLAGS="" RANLIB="${RANLIB}"; cd ..)

test:
	$(MAKE) -C tests CXXFLAGS="$(CXXFLAGS)" run

valgrind:
	$(MAKE) -C tests CXXFLAGS="$(CXXFLAGS)" valgrind

clean:
	rm -f *.o ${TARGET}${DLLEND} Rules.depend zlib/*.exe
	$(MAKE) -C tests clean
	(cd zlib; $(MAKE) clean; cd ..)
	rm -f zlib/Makefile

distclean:
	rm -f Rules.depend ${TARGET}.dll ${TARGET}_i386.so addons/jk_botti/dlls/* zlib/*.exe
	(cd zlib; $(MAKE) distclean; cd ..)

#waypoint.o: waypoint.cpp
#	${CXX} ${CXXFLAGS} -funroll-loops -c $< -o $@

#safe_snprintf.o: safe_snprintf.cpp
#	${CXX} ${CXXFLAGS} -funroll-loops -c $< -o $@

%.o: %.cpp
	${CXX} ${CXXFLAGS} -c $< -o $@

%.o: %.c
	${CXX} ${CFLAGS} -c $< -o $@

depend: Rules.depend

Rules.depend: Makefile $(SRC)
	$(CXX) -MM ${INCLUDES} $(SRC) > $@

include Rules.depend
