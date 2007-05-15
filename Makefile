##
## Compiling jk_botti under CYGWIN:
##
## Get linux crosscompiler for cygwin: http://metamod-p.sourceforge.net/cross-compiling.on.windows.for.linux.html
##
##  for compiling linux: make OS=linux
##  for compiling win32: make
##

ifeq ($(OS),Windows_NT)
	CPP = gcc -mno-cygwin
	ARCHFLAG = -march=i686 -mtune=pentium4
	LINKFLAGS = -mdll -Xlinker --add-stdcall-alias -s
	DLLEND = .dll
else
	CPP = gcc-linux
	ARCHFLAG = -march=i686 -mcpu=pentium4 -fPIC
	LINKFLAGS = -fPIC -shared -ldl -s
	DLLEND = _i386.so
endif

TARGET = jk_botti_mm
BASEFLAGS = 
OPTFLAGS = -O3 -fomit-frame-pointer -ffast-math
#OPTFLAGS = -O0 -g
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
	bot_skill.cpp \
	bot_sound.cpp \
	bot_start.cpp \
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
	(cd zlib; CC="${CPP} ${ARCHFLAG}" ./configure; make; cd ..)

clean:
	rm -f *.o ${TARGET}${DLLEND} Rules.depend zlib/*.exe
	(cd zlib; make clean; cd ..)

distclean:
	rm -f Rules.depend ${TARGET}.dll ${TARGET}_i386.so addons/jk_botti/dlls/* zlib/*.exe
	(cd zlib; make distclean; cd ..)

%.o: %.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o: %.c
	${CPP} ${CFLAGS} -c $< -o $@

depend: Rules.depend

Rules.depend: Makefile $(SRC)
	$(CPP) -MM ${INCLUDES} $(SRC) > $@

include Rules.depend
