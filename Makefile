# Magic:
# g++ -std=c++11  extract_background.cpp -o app `pkg-config --cflags --libs opencv`

CC = g++
CFLAGS=-std=c++11 $(shell pkg-config --cflags --libs opencv)
TARGET=extract_car_game_background_and_car_trajectory

all: $(TARGET)
$(TARGET): $(TARGET).cpp
	$(CC) $(TARGET).cpp  -o $(TARGET) $(CFLAGS) 

clean:
	$(RM) $(TARGET)