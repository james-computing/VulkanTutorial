CXX = g++
CXXFLAGS = -std=c++23 -O3
LDFLAGS = -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = .
SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
EXE = $(BIN_DIR)/main

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ) $(BIN_DIR)
	$(CXX) -o $(EXE) $(OBJ) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< $(LDFLAGS) -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -r $(EXE) $(OBJ_DIR)

-include: $(OBJ:.o=.d)