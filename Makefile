.PHONY: build

build:
	@mkdir -p output
	g++ -Wall -Wno-sign-compare -c -o output/ThreeHeap.o -g3 -std=c++17 -I include src/ThreeHeap.cpp
	g++ -Wall -Wno-sign-compare -c -o output/main.o -g3 -std=c++17 -I include src/main.cpp
	g++ -Wall -Wno-sign-compare -o output/threeheap -g3 -std=c++17 output/ThreeHeap.o output/main.o
