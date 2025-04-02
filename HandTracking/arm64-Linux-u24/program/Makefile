CXX = g++
CXXFLAGS = -O3 -std=c++17 -g -Wno-psabi $(shell pkg-config --cflags opencv4) \
	-I/usr/include \
    -I/home/orangepi/SOURCE/tensorflow \
    -I/home/orangepi/SOURCE/tensorflow/tensorflow/lite \
    -I/home/orangepi/SOURCE/tensorflow/bazel-bin/tensorflow/lite \
    -I/home/orangepi/SOURCE/vcpkg/installed/arm64-linux/include \
    -I/usr/include/python3.10 \
    -I/home/orangepi/.local/lib/python3.10/site-packages/numpy/core/include \
    -I/usr/include/boost \
    -I/home/orangepi/SOURCE/mediapipe \
    -I/home/orangepi/.cache/bazel/_bazel_orangepi/8dcaf30565e9b1b5c987034ed6524dbc/external/flatbuffers/include \
    -I/usr/include/CL \

LDFLAGS = $(shell pkg-config --libs opencv4) \
    -L/home/orangepi/SOURCE/tensorflow/bazel-bin/tensorflow/lite \
    -L/usr/lib/python3.10/config-3.10-aarch64-linux-gnu \
    -lpython3.10 -lboost_system -lboost_filesystem \
    -lgflags -lglog \
    -lX11 -lXtst -lOpenCL \

TARGET = VMC

SRCS = main.cpp Parts/Fingers.cpp Parts/Hand.cpp HandProc.cpp \
		InterfaceController.cpp Desktop/Desktop.cpp Desktop/Scanner.cpp \
		Generics.cpp VXMath.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)
	
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
	
clean:
	rm -f $(OBJS) $(TARGET)
	
.PHONY: all clean
