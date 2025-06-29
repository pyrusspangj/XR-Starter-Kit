CXX = g++
CXXFLAGS = -O3 -std=c++17 -g -Wno-psabi $(shell pkg-config --cflags opencv4) \
	-I/usr/include \
    -I/usr/include/python3.10 \
    -I/usr/include/boost \
    -I/usr/include/CL \

LDFLAGS = $(shell pkg-config --libs opencv4) \
    -L/usr/lib/python3.10/config-3.10-aarch64-linux-gnu \
    -lpython3.10 -lboost_system -lboost_filesystem \
    -lgflags -lglog \
    -lX11 -lXtst -lOpenCL \

TARGET = xrhands

SRCS = main.cpp Generics.cpp handtracking.cpp hand.cpp ../../../Helper-Scripts/xrimaging.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
clean:
	rm -f $(OBJS) $(TARGET)
	
.PHONY: all clean
