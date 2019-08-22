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

all: Application all_tests

Application: $(OBJ_FILES) $(MAIN_OBJ)
	$(CXX) -o $@ $^ $(LD_FLAGS)

run_singlefailure: Application
	./Application ./testcases/singlefailure_mli.conf

all_tests: $(OBJ_FILES)
	$(MAKE) -C $(TEST_DIR) all_tests HEADER_DIR=$(abspath $(HEADER_DIR)) SRC_OBJ_DIR=$(abspath $(OBJ_DIR))

run_test: all_tests
	cd ./test && ./all_tests

run: clean
	$(MAKE) all
	./Application

gen_submit_ver:
	@rm -rf ./submit
	mkdir ./submit
	cp submit.mk ./submit/Makefile
	python merge_all_deps.py
	mv MP1Node.merged.cpp ./submit/MP1Node.cpp
	mv MP1Node.merged.h ./submit/MP1Node.h
	mv Application.merged.cpp ./submit/Application.cpp
	cp Application.h EmulNet.* Log.* Member.* Params.* Queue.h stdincludes.h ./submit/
	cp *.sh ./submit/
	cp submit.py ./submit/
	cp -rf testcases ./submit/

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
