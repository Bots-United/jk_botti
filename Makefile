##
## compiling under ubuntu:
##  for compiling linux: make 
##  for compiling win32: make OSTYPE=win32
##

ifeq ($(OSTYPE),win32)
	CPP = /usr/bin/i586-mingw32msvc-gcc
	LINKFLAGS = -mdll -lwsock32 -Xlinker --add-stdcall-alias -s
	DLLEND = .dll
	ZLIB_OSFLAGS = 
else
	CPP = gcc -m32
	ARCHFLAG = -fPIC
	LINKFLAGS = -fPIC -shared -ldl -s
	DLLEND = _i386.so
	ZLIB_OSFLAGS = -DNO_UNDERLINE 
endif

TARGET = jk_botti_mm
BASEFLAGS = 
ARCHFLAG += -march=i686 -mtune=pentium4

ifeq ($(DBG_FLGS),1)
	OPTFLAGS = -O0 -g
else
	OPTFLAGS = -O2 -ffast-math -fomit-frame-pointer -g
endif

INCLUDES = -I"./metamod" \
	-I"./common" \
	-I"./dlls" \
	-I"./engine" \
	-I"./pm_shared"

CFLAGS = -Wall ${BASEFLAGS} ${OPTFLAGS} ${ARCHFLAG} ${INCLUDES}
CPPFLAGS = -fno-rtti -fno-exceptions ${CFLAGS} 

SRC = 	bot.cpp \
	bot_chat.cpp \
	bot_client.cpp \
	bot_combat.cpp \
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
	${CPP} -o $@ ${OBJ} zlib/libz.a ${LINKFLAGS}
	cp $@ addons/jk_botti/dlls/

zlib/libz.a:
	(cd zlib; CC="${CPP} ${OPTFLAGS} ${ARCHFLAG} ${ZLIB_OSFLAGS} -DASMV" ./configure; make OBJA=match.o; cd ..)

clean:
	rm -f *.o ${TARGET}${DLLEND} Rules.depend zlib/*.exe 
	(cd zlib; make clean; cd ..)
	rm -f zlib/Makefile

distclean:
	rm -f Rules.depend ${TARGET}.dll ${TARGET}_i386.so addons/jk_botti/dlls/* zlib/*.exe
	(cd zlib; make distclean; cd ..)

waypoint.o: waypoint.cpp
	${CPP} ${CPPFLAGS} -funroll-loops -c $< -o $@

safe_snprintf.o: safe_snprintf.cpp
	${CPP} ${CPPFLAGS} -funroll-loops -c $< -o $@

%.o: %.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o: %.c
	${CPP} ${CFLAGS} -c $< -o $@

depend: Rules.depend

Rules.depend: Makefile $(SRC)
	$(CPP) -MM ${INCLUDES} $(SRC) > $@

include Rules.depend
