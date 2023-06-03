SRC_DIR = src
OBJ_DIR = obj

DIRS = $(shell find $(SRC_DIR) -type d)
SRCS = $(shell find $(SRC_DIR) -name '*.cpp')
HEADERS = $(shell find $(SRC_DIR) -name '*.hpp')
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CXX = g++
CC = gcc
CFLAGS = -lpthread -O3
CXXFLAGS = -std=c++17

.PHONY: clean

cemu: obj $(OBJS)
	$(CXX) obj/main.o $(CFLAGS) $(CXXFLAGS) -o $@

cemu_lib_main: obj $(OBJS)
	$(CXX) src/cemu_lib_main.c obj/cemu_lib.o $(CFLAGS) -o $@

obj:
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(addprefix -I,${DIRS}) -c $< -o $@

clean:
	rm $(OBJS)