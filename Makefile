# Variables
SRC_DIR = src
OUTPUT_DATA = ./public/shortest_path.geojson
CXX = g++
FLAGS = -Wall -Wextra -O2
LFLAGS = -ljsoncpp
TARGET = $(SRC_DIR)/process
BIN = $(SRC_DIR)/*.bin

# Default target: build and run
all: $(TARGET) run

# Compile the executable
$(TARGET): $(SRC_DIR)/*.cpp
	$(CXX) -o $@ $^ $(FLAGS) $(LFLAGS)

# Run the executable and Node.js server
run: $(TARGET)
	cd $(SRC_DIR) && ./process && cd ..
	node app.js

# Clean up generated files
clean:
	rm -f $(TARGET) $(OUTPUT_DATA) $(BIN)

.PHONY: all run clean
