# tests/Makefile

ZLIB_LIB ?= ../zlib/libz.a

PARENT_INCLUDES = -I"../metamod" -I"../common" -I"../dlls" -I"../engine" -I"../pm_shared"
TEST_CXXFLAGS = ${CXXFLAGS} ${PARENT_INCLUDES} -I".."

# Simple self-contained tests
test_name_sanitize: test_name_sanitize.cpp ../bot_name_sanitize.h
	${CXX} -Wall -o $@ $<

test_posdata_list: test_posdata_list.cpp ../posdata_list.h
	${CXX} -Wall -o $@ $<

# Engine-mock based tests
BOT_COMBAT_OBJS = engine_mock.o test_bot_combat.o \
	bot_combat.o bot_weapons.o util.o safe_snprintf.o random_num.o

test_bot_combat: $(BOT_COMBAT_OBJS)
	${CXX} -o $@ $(BOT_COMBAT_OBJS) -lm

%.o: %.cpp
	${CXX} ${TEST_CXXFLAGS} -c $< -o $@

%.o: ../%.cpp
	${CXX} ${TEST_CXXFLAGS} -c $< -o $@

# safe_snprintf test
SAFE_SNPRINTF_OBJS = test_safe_snprintf.o safe_snprintf.o

test_safe_snprintf: $(SAFE_SNPRINTF_OBJS)
	${CXX} -o $@ $(SAFE_SNPRINTF_OBJS)

# Random number generator test
RANDOM_NUM_OBJS = test_random_num.o random_num.o

test_random_num: $(RANDOM_NUM_OBJS)
	${CXX} -o $@ $(RANDOM_NUM_OBJS) -lm

# Util bug fix tests
UTIL_TEST_OBJS = engine_mock.o test_util.o util.o safe_snprintf.o random_num.o

test_util: $(UTIL_TEST_OBJS)
	${CXX} -o $@ $(UTIL_TEST_OBJS) -lm

# Bot chat tests (bot_chat.cpp is #included in test file, not linked separately)
BOT_CHAT_OBJS = engine_mock.o test_bot_chat.o util.o safe_snprintf.o random_num.o

test_bot_chat: $(BOT_CHAT_OBJS)
	${CXX} -o $@ $(BOT_CHAT_OBJS) -lm

# Neural net training test
NEURALNET_OBJS = test_neuralnet.o neuralnet.o geneticalg.o random_num.o

test_neuralnet: $(NEURALNET_OBJS)
	${CXX} -o $@ $(NEURALNET_OBJS) -lm

# Bot weapons tests
BOT_WEAPONS_OBJS = engine_mock.o test_bot_weapons.o \
	bot_weapons.o util.o safe_snprintf.o random_num.o

test_bot_weapons: $(BOT_WEAPONS_OBJS)
	${CXX} -o $@ $(BOT_WEAPONS_OBJS) -lm

# Bot navigate tests
BOT_NAVIGATE_OBJS = engine_mock.o test_bot_navigate.o \
	bot_navigate.o bot_weapons.o util.o safe_snprintf.o random_num.o

test_bot_navigate: $(BOT_NAVIGATE_OBJS)
	${CXX} -o $@ $(BOT_NAVIGATE_OBJS) -lm

# Waypoint tests (waypoint.cpp is #included in test file, not linked separately)
WAYPOINT_OBJS = engine_mock.o test_waypoint.o bot_weapons.o util.o safe_snprintf.o random_num.o

test_waypoint: $(WAYPOINT_OBJS) $(ZLIB_LIB)
	${CXX} -o $@ $(WAYPOINT_OBJS) $(ZLIB_LIB) -lm

ALL_TESTS = test_name_sanitize test_posdata_list test_bot_combat test_bot_weapons test_util test_bot_chat test_safe_snprintf test_random_num test_neuralnet test_waypoint test_bot_navigate

all: $(ALL_TESTS)

run: $(ALL_TESTS)
	./test_name_sanitize
	./test_posdata_list
	./test_bot_combat
	./test_bot_weapons
	./test_util
	./test_bot_chat
	./test_safe_snprintf
	./test_random_num
	./test_neuralnet
	./test_waypoint
	./test_bot_navigate

VALGRIND = valgrind --leak-check=full --error-exitcode=1

valgrind: $(ALL_TESTS)
	$(VALGRIND) ./test_name_sanitize
	$(VALGRIND) ./test_posdata_list
	$(VALGRIND) ./test_bot_combat
	$(VALGRIND) ./test_bot_weapons
	$(VALGRIND) ./test_util
	$(VALGRIND) ./test_bot_chat
	$(VALGRIND) ./test_safe_snprintf
	$(VALGRIND) ./test_random_num
	$(VALGRIND) ./test_neuralnet
	$(VALGRIND) ./test_waypoint
	$(VALGRIND) ./test_bot_navigate

clean:
	rm -f $(ALL_TESTS) *.o
