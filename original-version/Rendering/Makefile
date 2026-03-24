CXX = g++
CXXFLAGS = -std=c++23 -O3 #-g
LDFLAGS = -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

main: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp $(LDFLAGS) -o main

.PHONY: test clean

test: main
	./main

clean:
	rm -f main