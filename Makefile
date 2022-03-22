SRC_DIR = src
OBJ_DIR = obj

DIRS = $(shell find $(SRC_DIR) -type d)
SRCS = $(shell find $(SRC_DIR) -name '*.cpp')
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

CXX = g++
CXXFLAGS = -lpthread -std=c++17 -g

cemu: $(OBJS)
	$(CXX) $(OBJS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(addprefix -I,${DIRS}) -c $< -o $@