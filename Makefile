CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pedantic
INCLUDES = -Iinclude

all: riscvsim

riscvsim: src/main.cpp src/parser.cpp src/simulator.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

run: riscvsim
	./riscvsim

clean:
	rm -f riscvsim

.PHONY: all run clean
