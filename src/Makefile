# Variables
OUTPUT_DATA = ../public/shortest_path.geojson
CXX = g++
FLAGS = -Wall -Wextra -O2
LFLAGS = -ljsoncpp -lboost_system -lpthread
TARGET = ./process
BIN = ./*.bin

start: $(TARGET) 
	$(TARGET)

$(TARGET):release

release:
	g++ *.cpp -Wall -Wextra -O2 -I/usr/include/ -L/usr/lib/x86_64-linux-gnu/ $(LFLAGS) -o process

debug: 
	g++ *.cpp -Wall -Wextra -O0 -I/usr/include/ -L/usr/lib/x86_64-linux-gnu/ $(LFLAGS) -o process -g

cleanp:
	rm ./process

cleanall:
	rm ./process ./*.bin 
