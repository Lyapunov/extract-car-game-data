# Magic:
# g++ -std=c++11  extract_background.cpp -o app `pkg-config --cflags --libs opencv`

CC = g++
CFLAGS=-std=c++11
CVFLAGS=$(shell pkg-config --cflags --libs opencv)
GLFLAGS=-lGL -lglut

TARGET_EXTRACT=extract_car_game_background_and_car_trajectory
TARGET_CAR_TEST=car_physic_test

#all: $(TARGET_CAR_TEST)
all: $(TARGET_EXTRACT) $(TARGET_CAR_TEST)

$(TARGET_EXTRACT): $(TARGET_EXTRACT).cpp
	$(CC) $(TARGET_EXTRACT).cpp  -o $(TARGET_EXTRACT) $(CFLAGS) $(CVFLAGS)

$(TARGET_CAR_TEST): $(TARGET_CAR_TEST).cpp
	$(CC) $(TARGET_CAR_TEST).cpp  -o $(TARGET_CAR_TEST) $(CFLAGS) $(GLFLAGS)

clean:
	$(RM) $(TARGET_EXTRACT) $(TARGET_CAR_TEST)
