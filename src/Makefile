# Variables
OUTPUT_DATA = ../public/shortest_path.geojson
CXX = g++
FLAGS = -Wall -Wextra -O2
LFLAGS = -ljsoncpp
TARGET = ./process
BIN = ./*.bin

debug: 
	g++ *.cpp -Wall -Wextra -O0 -I/usr/include/ -L/usr/lib/x86_64-linux-gnu/ -ljsoncpp -o process -g

cleanp:
	rm ./process

cleanall:
	rm ./process ./*.bin 
