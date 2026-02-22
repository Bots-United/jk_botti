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
	bot_combat.o bot_weapons.o util.o safe_snprintf.o

test_bot_combat: $(BOT_COMBAT_OBJS)
	${CXX} -o $@ $(BOT_COMBAT_OBJS) -lm

%.o: %.cpp
	${CXX} ${TEST_CXXFLAGS} -c $< -o $@

%.o: ../%.cpp
	${CXX} ${TEST_CXXFLAGS} -c $< -o $@

# Neural net training test
NEURALNET_OBJS = test_neuralnet.o neuralnet.o geneticalg.o random_num.o

test_neuralnet: $(NEURALNET_OBJS)
	${CXX} -o $@ $(NEURALNET_OBJS) -lm

ALL_TESTS = test_name_sanitize test_posdata_list test_bot_combat test_neuralnet

all: $(ALL_TESTS)

run: $(ALL_TESTS)
	./test_name_sanitize
	./test_posdata_list
	./test_bot_combat
	./test_neuralnet

VALGRIND = valgrind --leak-check=full --error-exitcode=1

valgrind: $(ALL_TESTS)
	$(VALGRIND) ./test_name_sanitize
	$(VALGRIND) ./test_posdata_list
	$(VALGRIND) ./test_bot_combat
	$(VALGRIND) ./test_neuralnet

clean:
	rm -f $(ALL_TESTS) *.o
