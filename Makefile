CXX = g++
CXXFLAGS = -Wall -Wextra -Iinclude -std=c++17

SRC_DIR = src
TEST_DIR = test
OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/main
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: all
	$(MAKE) -C $(TEST_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run: all
	$(shell ./bin/main)

help:
	@echo "Makefile targets:"
	@echo "  make all     - Build the project (default)"
	@echo "  make test    - Build and run tests (requires test/Makefile)"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make run     - Runs the exe"
	@echo "  make help    - Show this help message"

.PHONY: all clean test
