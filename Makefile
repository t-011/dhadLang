.PHONY: test docker-test clean

dhad : src/main.cpp $(wildcard src/*.h)
	g++ -Wall src/main.cpp -g -o dhad -std=c++20

test : dhad
	tests/run.sh

# runs the tests in a Linux x64 container (works on macOS / Apple Silicon)
docker-test :
	docker build -q --platform linux/amd64 -t dhadlang . && docker run --rm --platform linux/amd64 dhadlang

clean:
	rm -f dhad out out.o out.asm
