.PHONY: test unit docker-test clean

dhad : src/main.cpp $(wildcard src/*.h)
	g++ -Wall src/main.cpp -g -o dhad -std=c++20

# tokenizer and parser unit tests; they need no nasm, so they run on any OS
unit :
	g++ -Wall -std=c++20 tests/unit.cpp -o unit_tests && ./unit_tests

test : dhad unit
	tests/run.sh

# runs the tests in a Linux x64 container (works on macOS / Apple Silicon)
docker-test :
	docker build -q --platform linux/amd64 -t dhadlang . && docker run --rm --platform linux/amd64 dhadlang

clean:
	rm -f dhad out out.o out.asm unit_tests
