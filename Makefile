CPP = gcc-2.95.3
TARGET = hpb_bot_mm
ARCHFLAG = i586
BASEFLAGS = -Dstricmp=strcasecmp -Dstrcmpi=strcasecmp
OPTFLAGS = 
CPPFLAGS = ${BASEFLAGS} ${OPTFLAGS} -march=${ARCHFLAG} -O2 -w -I"../metamod" -I"../../devtools/hlsdk-2.3/multiplayer/common" -I"../../devtools/hlsdk-2.3/multiplayer/dlls" -I"../../devtools/hlsdk-2.3/multiplayer/engine" -I"../../devtools/hlsdk-2.3/multiplayer/pm_shared"

OBJ = 	bot.o \
	bot_chat.o \
	bot_client.o \
	bot_combat.o \
	bot_models.o \
	bot_navigate.o \
	bot_start.o \
	dll.o \
	engine.o \
	h_export.o \
	util.o \
	waypoint.o

${TARGET}_i386.so: ${OBJ}
	${CPP} -fPIC -shared -o $@ ${OBJ} -Xlinker -Map -Xlinker ${TARGET}.map -ldl
	mv *.o Release
	mv *.map Release
	mv $@ Release

clean:
	rm -f Release/*.o
	rm -f Release/*.map

distclean:
	rm -rf Release
	mkdir Release	

%.o:	%.cpp
	${CPP} ${CPPFLAGS} -c $< -o $@

%.o:	%.c
	${CPP} ${CPPFLAGS} -c $< -o $@
