CPP = gcc-linux
TARGET = jk_botti_mm
ARCHFLAG = i686
BASEFLAGS = 
OPTFLAGS = -O3 -ffast-math -fno-rtti -fno-exceptions
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
	bot_start.cpp \
	dll.cpp \
	engine.cpp \
	h_export.cpp \
	util.cpp \
	waypoint.cpp

OBJ = $(SRC:%.cpp=%.o)

${TARGET}_i386.so: ${OBJ}
	${CPP} -fPIC -shared -o $@ ${OBJ} -ldl -s
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
