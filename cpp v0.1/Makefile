CXX = /opt/rk-linux/linux-x86/rk3588/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LDFLAGS = -pthread
SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
EXECUTABLE = app
.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXECUTABLE)