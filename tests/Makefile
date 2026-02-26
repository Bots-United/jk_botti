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

# h_export tests (h_export.cpp is #included in test file, standalone)
test_h_export: test_h_export.o
	${CXX} $(COVERAGE_FLAGS) -o $@ test_h_export.o

# bot_config_init tests
BOT_CONFIG_INIT_OBJS = engine_mock.o test_bot_config_init.o \
	bot_config_init.o util.o safe_snprintf.o random_num.o

test_bot_config_init: $(BOT_CONFIG_INIT_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_CONFIG_INIT_OBJS) -lm

# bot_models tests
BOT_MODELS_OBJS = engine_mock.o test_bot_models.o \
	bot_models.o util.o safe_snprintf.o random_num.o

test_bot_models: $(BOT_MODELS_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_MODELS_OBJS) -lm

# bot_client tests
BOT_CLIENT_OBJS = engine_mock.o test_bot_client.o \
	bot_client.o bot_weapons.o util.o safe_snprintf.o random_num.o

test_bot_client: $(BOT_CLIENT_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_CLIENT_OBJS) -lm

# bot tests
BOT_OBJS = engine_mock.o test_bot.o \
	bot.o bot_config_init.o bot_weapons.o util.o safe_snprintf.o random_num.o

test_bot: $(BOT_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_OBJS) -lm

# commands tests
COMMANDS_OBJS = engine_mock.o test_commands.o \
	commands.o bot_weapons.o util.o safe_snprintf.o random_num.o

test_commands: $(COMMANDS_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(COMMANDS_OBJS) -lm

# dll tests (dll.cpp needs VER_NOTE as a proper C string literal)
dll.o: ../dll.cpp
	${CXX} ${TEST_CXXFLAGS} -UVER_NOTE -DVER_NOTE='"test"' -c $< -o $@

DLL_OBJS = engine_mock.o test_dll.o \
	dll.o bot_weapons.o bot_sound.o util.o safe_snprintf.o random_num.o

test_dll: $(DLL_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(DLL_OBJS) -lm

# engine tests
ENGINE_OBJS = engine_mock.o test_engine.o \
	engine.o bot_weapons.o bot_sound.o util.o safe_snprintf.o random_num.o

test_engine: $(ENGINE_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(ENGINE_OBJS) -lm

# bot_query_hook tests
BOT_QUERY_HOOK_OBJS = engine_mock.o test_bot_query_hook.o \
	bot_query_hook.o util.o safe_snprintf.o random_num.o

test_bot_query_hook: $(BOT_QUERY_HOOK_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_QUERY_HOOK_OBJS) -lm

# bot_query_hook_linux tests
BOT_QUERY_HOOK_LINUX_OBJS = engine_mock.o test_bot_query_hook_linux.o \
	bot_query_hook_linux.o util.o safe_snprintf.o random_num.o

test_bot_query_hook_linux: $(BOT_QUERY_HOOK_LINUX_OBJS)
	${CXX} $(COVERAGE_FLAGS) -o $@ $(BOT_QUERY_HOOK_LINUX_OBJS) -lm -lpthread

# bot_query_hook_win32 tests (source #included with -D_WIN32, mock Win32 headers)
test_bot_query_hook_win32.o: test_bot_query_hook_win32.cpp ../bot_query_hook_win32.cpp ../bot_query_hook.h windows.h winsock2.h
	${CXX} -Wall -D_WIN32 -I. -I".." $(COVERAGE_FLAGS) -c $< -o $@

test_bot_query_hook_win32: test_bot_query_hook_win32.o
	${CXX} $(COVERAGE_FLAGS) -o $@ test_bot_query_hook_win32.o

ALL_TESTS = test_name_sanitize test_posdata_list test_bot_combat test_bot_weapons test_util test_bot_chat test_safe_snprintf test_random_num test_neuralnet test_geneticalg test_waypoint test_bot_navigate test_bot_skill test_bot_sound test_h_export test_bot_config_init test_bot_models test_bot_client test_bot test_commands test_dll test_engine test_bot_query_hook test_bot_query_hook_linux test_bot_query_hook_win32

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
	./test_h_export
	./test_bot_config_init
	./test_bot_models
	./test_bot_client
	./test_bot
	./test_commands
	./test_dll
	./test_engine
	./test_bot_query_hook
	./test_bot_query_hook_linux
	./test_bot_query_hook_win32

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
	$(VALGRIND) ./test_h_export
	$(VALGRIND) ./test_bot_config_init
	$(VALGRIND) ./test_bot_models
	$(VALGRIND) ./test_bot_client
	$(VALGRIND) ./test_bot
	$(VALGRIND) ./test_commands
	$(VALGRIND) ./test_dll
	$(VALGRIND) ./test_engine
	$(VALGRIND) ./test_bot_query_hook
	$(VALGRIND) ./test_bot_query_hook_linux
	$(VALGRIND) ./test_bot_query_hook_win32

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
