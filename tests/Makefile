# tests/Makefile

ZLIB_LIB ?= ../zlib/libz.a
COVERAGE_FLAGS ?=
GCOV ?= $(subst g++,gcov,$(firstword $(CXX)))

PARENT_INCLUDES = -I"../metamod" -I"../common" -I"../dlls" -I"../engine" -I"../pm_shared"
TEST_CXXFLAGS = ${CXXFLAGS} ${PARENT_INCLUDES} -I".." $(COVERAGE_FLAGS)

# Simple self-contained tests
test_name_sanitize: test_name_sanitize.cpp ../bot_name_sanitize.h
	${CXX} -Wall $(COVERAGE_FLAGS) -o $@ $<

test_posdata_list: test_posdata_list.cpp ../posdata_list.h
	${CXX} -Wall $(COVERAGE_FLAGS) -o $@ $<

# Engine-mock based tests
BOT_COMBAT_OBJS = engine_mock.o test_bot_combat.o \
	bot_weapons.o bot_sound.o util.o safe_snprintf.o random_num.o

test_bot_combat: $(BOT_COMBAT_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_COMBAT_OBJS) -lm

%.o: %.cpp
	${CXX} ${TEST_CXXFLAGS} -c $< -o $@

%.o: ../%.cpp
	${CXX} ${TEST_CXXFLAGS} -c $< -o $@

# safe_snprintf test
SAFE_SNPRINTF_OBJS = test_safe_snprintf.o safe_snprintf.o

test_safe_snprintf: $(SAFE_SNPRINTF_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(SAFE_SNPRINTF_OBJS)

# Random number generator test
RANDOM_NUM_OBJS = test_random_num.o random_num.o

test_random_num: $(RANDOM_NUM_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(RANDOM_NUM_OBJS) -lm

# Util bug fix tests
UTIL_TEST_OBJS = engine_mock.o test_util.o util.o safe_snprintf.o random_num.o

test_util: $(UTIL_TEST_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(UTIL_TEST_OBJS) -lm

# Bot chat tests (bot_chat.cpp is #included in test file, not linked separately)
BOT_CHAT_OBJS = engine_mock.o test_bot_chat.o util.o safe_snprintf.o random_num.o

test_bot_chat: $(BOT_CHAT_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_CHAT_OBJS) -lm

# Neural net training test
NEURALNET_OBJS = test_neuralnet.o neuralnet.o geneticalg.o random_num.o

test_neuralnet: $(NEURALNET_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(NEURALNET_OBJS) -lm

# Genetic algorithm unit tests (geneticalg.cpp is #included in test file)
GENETICALG_OBJS = test_geneticalg.o random_num.o

test_geneticalg: $(GENETICALG_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(GENETICALG_OBJS) -lm

# Bot weapons tests
BOT_WEAPONS_OBJS = engine_mock.o test_bot_weapons.o \
	bot_weapons.o util.o safe_snprintf.o random_num.o

test_bot_weapons: $(BOT_WEAPONS_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_WEAPONS_OBJS) -lm

# Bot navigate tests (bot_navigate.cpp is #included in test file, not linked separately)
BOT_NAVIGATE_OBJS = engine_mock.o test_bot_navigate.o \
	bot_weapons.o util.o safe_snprintf.o random_num.o

test_bot_navigate: $(BOT_NAVIGATE_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_NAVIGATE_OBJS) -lm

# Bot skill tests (bot_skill.cpp is #included in test file, not linked separately)
test_bot_skill: test_bot_skill.o
	${CXX} $(COVERAGE_FLAGS) -o $@ test_bot_skill.o

# Bot sound tests
BOT_SOUND_OBJS = engine_mock.o test_bot_sound.o \
	bot_sound.o util.o safe_snprintf.o random_num.o

test_bot_sound: $(BOT_SOUND_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_SOUND_OBJS) -lm

# Waypoint tests (waypoint.cpp is #included in test file, not linked separately)
WAYPOINT_OBJS = engine_mock.o test_waypoint.o bot_weapons.o util.o safe_snprintf.o random_num.o

test_waypoint: $(WAYPOINT_OBJS) $(ZLIB_LIB)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(WAYPOINT_OBJS) $(ZLIB_LIB) -lm

ALL_TESTS = test_name_sanitize test_posdata_list test_bot_combat test_bot_weapons test_util test_bot_chat test_safe_snprintf test_random_num test_neuralnet test_geneticalg test_waypoint test_bot_navigate test_bot_skill test_bot_sound

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
	./test_geneticalg
	./test_waypoint
	./test_bot_navigate
	./test_bot_skill
	./test_bot_sound

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
	$(VALGRIND) ./test_geneticalg
	$(VALGRIND) ./test_waypoint
	$(VALGRIND) ./test_bot_navigate
	$(VALGRIND) ./test_bot_skill
	$(VALGRIND) ./test_bot_sound

coverage:
	$(MAKE) clean
	rm -f *.gcda *.gcno coverage.info
	rm -rf coverage_report
	$(MAKE) COVERAGE_FLAGS="--coverage" run
	lcov --capture --directory . --output-file coverage.info \
	    --gcov-tool $(GCOV) --branch-coverage \
	    --ignore-errors inconsistent,deprecated
	lcov --remove coverage.info '*/tests/*' '*/zlib/*' '*/engine/*' \
	    '*/metamod/*' '*/common/*' '*/dlls/*' '*/pm_shared/*' \
	    '/usr/*' \
	    --output-file coverage.info --branch-coverage \
	    --ignore-errors inconsistent,deprecated,unused
	genhtml coverage.info --output-directory coverage_report \
	    --branch-coverage --ignore-errors inconsistent,deprecated
	lcov --summary coverage.info --branch-coverage \
	    --ignore-errors inconsistent,deprecated

coverage-clean:
	rm -f *.gcda *.gcno coverage.info
	rm -rf coverage_report

clean:
	rm -f $(ALL_TESTS) *.o *.gcda *.gcno *.gcov coverage.info
	rm -rf coverage_report
