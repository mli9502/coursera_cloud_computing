#**********************
#*
#* Progam Name: MP1. Membership Protocol.
#*
#* Current file: Makefile
#* About this file: Build Script.
#* 
#***********************
CXX = g++
LD_FLAGS = 
CXX_FLAGS = -std=c++11 -g

TEST_DIR = ./test

SRC_DIR = .
HEADER_DIR = .
OBJ_DIR = ./obj

MAIN_CPP = ./main.cpp
MAIN_OBJ = ./main.o

CXX_FLAGS += -I$(HEADER_DIR)
CXX_FLAGS += -Wall -Wno-unused-variable -Wno-format-security -Wno-sign-compare

SRC_FILES = $(notdir $(wildcard $(SRC_DIR)/*.cpp))
HEADER_FILES = $(wildcard $(HEADER_DIR)/*.h)
OBJ_FILES = $(addprefix $(OBJ_DIR)/, $(SRC_FILES:.cpp=.o))
OBJ_FILES := $(filter-out $(OBJ_DIR)/main.o,$(OBJ_FILES))
DEP_FILES = $(OBJ_FILES:.o=.d)

.PHONY: all main all_tests run_test clean

all: main all_tests

main: $(OBJ_FILES) $(MAIN_OBJ)
	$(CXX) -o $@ $^ $(LD_FLAGS)

run_singlefailure: main
	./main ./testcases/singlefailure_mli.conf

all_tests: $(OBJ_FILES)
	$(MAKE) -C $(TEST_DIR) all_tests HEADER_DIR=$(abspath $(HEADER_DIR)) SRC_OBJ_DIR=$(abspath $(OBJ_DIR))

run_test: all_tests
	cd ./test && ./all_tests

$(MAIN_OBJ): $(MAIN_CPP)
	$(CXX) -MMD -MP -MF $(@:.o=.d) -MT $@ -c $(CXX_FLAGS) $< -o $@

# -MMD: No -E (only do preprocess) implied.
#       Meaning that dependency files are generated as a side effect.
# https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html
# http://scottmcpeak.com/autodepend/autodepend.html
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -MMD -MP -MF $(@:.o=.d) -MT $@ -c $(CXX_FLAGS) $< -o $@

$(OBJ_FILES): | $(OBJ_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	$(MAKE) -C $(TEST_DIR) clean
	@rm main *.o $(OBJ_DIR)/*.o 2>/dev/null || true

-include $(DEP_FILES)

# CFLAGS =  -Wall -g -std=c++11
# CFLAGS += -Wno-unused-variable -Wno-format-security -Wno-sign-compare

# all: Application

# Application: MP1Node.o EmulNet.o Application.o Log.o Params.o Member.o  
# 	g++ -o Application MP1Node.o EmulNet.o Application.o Log.o Params.o Member.o ${CFLAGS}

# MP1Node.o: MP1Node.cpp MP1Node.h Log.h Params.h Member.h EmulNet.h Queue.h
# 	g++ -c MP1Node.cpp ${CFLAGS}

# EmulNet.o: EmulNet.cpp EmulNet.h Params.h Member.h
# 	g++ -c EmulNet.cpp ${CFLAGS}

# Application.o: Application.cpp Application.h Member.h Log.h Params.h Member.h EmulNet.h Queue.h 
# 	g++ -c Application.cpp ${CFLAGS}

# Log.o: Log.cpp Log.h Params.h Member.h
# 	g++ -c Log.cpp ${CFLAGS}

# Params.o: Params.cpp Params.h 
# 	g++ -c Params.cpp ${CFLAGS}

# Member.o: Member.cpp Member.h
# 	g++ -c Member.cpp ${CFLAGS}

# clean:
# 	rm -rf *.o Application dbg.log msgcount.log stats.log machine.log

# run: clean
# 	$(MAKE) all
# 	./Application
