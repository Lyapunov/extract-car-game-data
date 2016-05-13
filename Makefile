# Magic:
# g++ -std=c++11  extract_background.cpp -o app `pkg-config --cflags --libs opencv`

CC = g++
CFLAGS=-Wall -std=c++11 -c
LFLAGS=-Wall -std=c++11
CVFLAGS=$(shell pkg-config --cflags --libs opencv)
GLFLAGS=-lGL -lglut
CAR_TEST_OBJS = Drawable.o Positioned.o $(TARGET_CAR_TEST).o

TARGET_EXTRACT=extract_car_game_background_and_car_trajectory
TARGET_CAR_TEST=car_physic_test

#all: $(TARGET_CAR_TEST)
all: $(TARGET_EXTRACT) $(TARGET_CAR_TEST)

$(TARGET_EXTRACT): $(TARGET_EXTRACT).cpp
	$(CC) $(TARGET_EXTRACT).cpp  -o $(TARGET_EXTRACT) $(LFLAGS) $(CVFLAGS)

Drawable.o : Drawable.h Drawable.cpp
	$(CC) Drawable.cpp $(CFLAGS) 

Positioned.o : Positioned.h Positioned.cpp
	$(CC) Positioned.cpp $(CFLAGS) 

$(TARGET_CAR_TEST).o : $(TARGET_CAR_TEST).cpp
	$(CC) $(TARGET_CAR_TEST).cpp $(CFLAGS) 

$(TARGET_CAR_TEST): $(CAR_TEST_OBJS)
	$(CC) $(CAR_TEST_OBJS)  -o $(TARGET_CAR_TEST) $(LFLAGS) $(GLFLAGS)

clean:
	$(RM) $(TARGET_EXTRACT) $(TARGET_CAR_TEST) $(CAR_TEST_OBJS)
