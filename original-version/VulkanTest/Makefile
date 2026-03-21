CFLAGS = -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

main: main.cpp
	g++ $(CFLAGS) -o main main.cpp $(LDFLAGS)

.PHONY: test clean

test: main
	./main

clean:
	rm -f main
