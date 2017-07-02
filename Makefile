# SYNOPSIS:
#
#   make all    - makes everything.
#   make main   - makes the main executable.
#   make test   - makes tests.
#   make clean  - removes all files generated by make.

### Flags passed to the C++ compiler: common, include and linking flags.
CXXFLAGS += -std=c++14 -g -O0 -Wall -Wextra -pthread
LFLAGS += -lsfml-system -lsfml-window -lsfml-graphics -lGL
IFLAGS += -I.


### Source files, Object Files, Directories, Targets, ...
# Core object files to compile for every target.
ALL_SRCS = $(wildcard *.cpp)
SRCS     = $(filter-out main.cpp,$(ALL_SRCS))
OBJECTS  = $(SRCS:%.cpp=%.o)

# 'dear imgui,' object files to compile for the main target.
IMGUI_DIR = imgui
IMGUI_SRC = $(wildcard $(IMGUI_DIR)/*.cpp)
IMGUI_OBJ = $(IMGUI_SRC:$(IMGUI_DIR)/%.cpp=$(IMGUI_DIR)/%.o)

# Object files to compile for the main target.
MAIN    = main.o
MAIN   += $(OBJECTS)
MAIN   += $(IMGUI_OBJ)

# Main executable
TARGET = procgen.out

# Tests object file to compile for the test target.
TEST_DIR  = test
TESTS_SRC = $(wildcard $(TEST_DIR)/*.cpp)
TESTS_OBJ = $(TESTS_SRC:$(TEST_DIR)/%.cpp=$(TEST_DIR)/%.o)

# Complete Test Suite executable.
TEST_TARGET = $(TEST_DIR)/procgenTest.out


### Specific path, flags, source files for googletest
# Path the root of googletest
GTEST_DIR = /usr/src/googletest/googletest

# Flags passed to the preprocessor
GTEST_CPPFLAGS = -isystem $(GTEST_DIR)/include

# All Google Test headers.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# Internal googletest variable.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)




all : main test

# Cleans all intermediate compilation files.
clean :
	rm -f *.o *.a *.out \
	$(addprefix  $(TEST_DIR)/, *.o *.a *.out) \
	$(addprefix $(IMGUI_DIR)/, *.o *.a *.out)


# main: Links all the .o file from MAIN to TARGET.
main : $(MAIN)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $^ $(LFLAGS)

# test: Links all OBJECTS, TESTS files plus gtest_main.a into the test
#       suite TEST_TARGET.
test : $(OBJECTS) $(TESTS_OBJ) $(TEST_DIR)/gtest_main.a
	$(CXX) $(GTEST_CPPFLAGS) $(CXXFLAGS) -o $(TEST_TARGET) $^ $(IFLAGS) $(LFLAGS) -lpthread


# Each .o file is compiled with its associated *.cpp file.
# (Exception: Test files: see below)
%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@ $(IFLAGS)


# Compiles and archives googletest internals.
#   - gtest_main.a is used when test files are presented without the
#   main() function. gtest.a otherwise.
$(TEST_DIR)/gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(GTEST_CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc -o $@

$(TEST_DIR)/gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(GTEST_CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc -o $@

$(TEST_DIR)/gtest.a : $(TEST_DIR)/gtest-all.o
	$(AR) $(ARFLAGS)  $@ $^

$(TEST_DIR)/gtest_main.a : $(TEST_DIR)/gtest-all.o $(TEST_DIR)/gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

# Compiles every *Test.cpp.
$(TEST_DIR)/%Test.o : $(TEST_DIR)/%Test.cpp
	$(CXX) $(GTEST_CPPFLAGS) $(CXXFLAGS) -c $^ -o $@ $(IFLAGS)


