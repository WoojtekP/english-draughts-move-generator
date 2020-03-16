CC = g++
CFLAGS = -Wall -Wextra -Wpedantic -Ofast -march=native -std=c++17 -flto

all: benchmark

benchmark: reasoner.o simulation.cpp
	$(CC) $(CFLAGS) -o $@ $^

reasoner.o: reasoner.cpp reasoner.hpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f benchmark *.o