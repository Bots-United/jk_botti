ifeq ($(OS),Windows_NT)
	CPP = gcc -mno-cygwin
	LINKFLAGS = -mdll -Xlinker --add-stdcall-alias -s
	DLLEND = .dll
else
	CPP = gcc-linux
	LINKFLAGS = -fPIC -shared -ldl -s
	DLLEND = _i386.so
endif

TARGET = jk_botti_mm
ARCHFLAG = i686
BASEFLAGS = 
OPTFLAGS = -O3 -ffast-math -fno-rtti -fno-exceptions -fomit-frame-pointer
#OPTFLAGS = -O0 -g -fno-rtti -fno-exceptions
INCLUDES = -I"./metamod" \
	-I"./common" \
	-I"./dlls" \
	-I"./engine" \
	-I"./pm_shared"
CPPFLAGS = -Wall ${BASEFLAGS} ${OPTFLAGS} -march=${ARCHFLAG} ${INCLUDES}

SRC = 	bot.cpp \
	bot_chat.cpp \
	bot_client.cpp \
	bot_combat.cpp \
	bot_models.cpp \
	bot_navigate.cpp \
	bot_skill.cpp \
	bot_start.cpp \
	bot_weapons.cpp \
	dll.cpp \
	engine.cpp \
	h_export.cpp \
	util.cpp \
	waypoint.cpp

OBJ = $(SRC:%.cpp=%.o)

${TARGET}${DLLEND}: ${OBJ}
	${CPP} -o $@ ${OBJ} ${LINKFLAGS}
	cp $@ Release

clean:
	rm -f *.o

distclean:
	rm -rf Release
	mkdir Release	

%.o: %.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o: %.c
	${CPP} ${CPPFLAGS} -c $< -o $@

depend: Rules.depend

Rules.depend: Makefile $(SRC)
	$(CPP) -MM ${INCLUDES} $(SRC) > $@

include Rules.depend
