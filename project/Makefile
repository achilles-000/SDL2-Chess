# Makefile for Chess Game (Windows)

CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = -lSDL2
TARGET = chess.exe
SRC = chess.c

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Clean build artifacts
clean:
	del /Q $(TARGET) 2>nul || true

# Run the game
run: $(TARGET)
	.\$(TARGET)

.PHONY: all clean run
