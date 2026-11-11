.PHONY: test clean

dhad : src/main.cpp $(wildcard src/*.h)
	g++ -Wall src/main.cpp -g -o dhad -std=c++20

test : dhad
	tests/run.sh

clean:
	rm -f dhad out out.o out.asm
