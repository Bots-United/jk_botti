# tests/Makefile

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

ALL_TESTS = test_name_sanitize test_posdata_list test_bot_combat test_util test_bot_chat test_safe_snprintf test_random_num test_neuralnet

all: $(ALL_TESTS)

run: $(ALL_TESTS)
	./test_name_sanitize
	./test_posdata_list
	./test_bot_combat
	./test_util
	./test_bot_chat
	./test_safe_snprintf
	./test_random_num
	./test_neuralnet

VALGRIND = valgrind --leak-check=full --error-exitcode=1

valgrind: $(ALL_TESTS)
	$(VALGRIND) ./test_name_sanitize
	$(VALGRIND) ./test_posdata_list
	$(VALGRIND) ./test_bot_combat
	$(VALGRIND) ./test_util
	$(VALGRIND) ./test_bot_chat
	$(VALGRIND) ./test_safe_snprintf
	$(VALGRIND) ./test_random_num
	$(VALGRIND) ./test_neuralnet

clean:
	rm -f $(ALL_TESTS) *.o
