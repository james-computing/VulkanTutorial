CXXFLAGS = -std=c++23 -O3 #-g
LDFLAGS = -lglfw -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

main: main.cpp
	g++ $(CXXFLAGS) $(LDFLAGS) -o main main.cpp

.PHONY: test clean

test: main
	./main

clean:
	rm -f main