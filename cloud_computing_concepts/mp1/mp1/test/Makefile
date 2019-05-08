CXX = g++

# GTEST_DIR needs to be set in bashrc using:
# export GTEST_DIR=<path to GTEST_DIR>
# on my linux machine: 
# /home/mengwen/Desktop/googletest-master/googletest
# on my mac machine:
# /Users/mli/Desktop/gtest/googletest

# OBJ_DIR will be created by libgtest.mk if it does not exist.
TEST_OBJ_DIR = ./obj
TEST_SRC_DIR = .

ifndef HEADER_DIR
	HEADER_DIR = ../
endif

ifndef SRC_OBJ_DIR
	SRC_OBJ_DIR = ../obj
endif

# source files and headers.
TEST_SRC_FILES = $(notdir $(wildcard $(TEST_SRC_DIR)/*.cpp))
HEADER_FILES = $(wildcard $(HEADER_DIR)/*.h) 

TEST_OBJ_FILES := $(addprefix $(TEST_OBJ_DIR)/, $(TEST_SRC_FILES:.cpp=.o))
SRC_OBJ_FILES = $(wildcard $(SRC_OBJ_DIR)/*.o)
SRC_OBJ_FILES := $(filter-out $(SRC_OBJ_DIR)/main.o,$(SRC_OBJ_FILES))

TEST_DEP_FILES = $(TEST_OBJ_FILES:.o=.d)

CPP_FLAGS += -isystem $(GTEST_DIR)/include
CXX_FLAGS += -g -Wall -Wextra -pthread -I$(HEADER_DIR) -std=c++11
CXX_FLAGS += -Wno-unused-parameter -Wno-unused-variable -Wno-unused-private-field
LD_FLAGS = -lpthread

GTEST_LIB_MAIN = $(TEST_OBJ_DIR)/gtest_main.a
GTEST_LIB = $(TEST_OBJ_DIR)/gtest.a

.PHONY: libgtest clean

all_tests: $(TEST_OBJ_FILES) $(SRC_OBJ_FILES)
	$(MAKE) libgtest
	$(MAKE) -C .. main
	$(CXX) $(CPP_FLAGS) $(CXX_FLAGS) $(LD_FLAGS) $^ $(GTEST_LIB_MAIN) -o $@ 

libgtest:
	$(MAKE) -f libgtest.mk OBJ_DIR=$(TEST_OBJ_DIR)

$(TEST_OBJ_DIR)/%.o: $(TEST_SRC_DIR)/%.cpp
	$(CXX) -MMD -MP -MF $(@:.o=.d) -MT $@ -c $(CPP_FLAGS) $(CXX_FLAGS) $< -o $@

clean:
	@rm $(TEST_OBJ_DIR)/*.o $(TEST_OBJ_DIR)/*.a 2>/dev/null || true 
	@rm all_tests 2>/dev/null || true 

$(TEST_OBJ_FILES): | $(TEST_OBJ_DIR)

$(TEST_OBJ_DIR):
	mkdir -p $(TEST_OBJ_DIR)

-include $(TEST_DEP_FILES)