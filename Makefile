CC = g++
FLAGS = -W -Wall
FILES = valley.cpp noiseutils.cpp
INCLUDES = -I .
LINKTO = -lSDL -lGL -lnoise
TARGET = valley

main: valley.cpp
	$(CC) $(AUTO) $(FILES) $(INCLUDES) -o $(TARGET) $(LINKTO)
