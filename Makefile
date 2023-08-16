.PHONY: build

build:
	@mkdir -p output
	g++ -Wall -Wno-sign-compare -c -o output/ThreeHeap.o  -g3 -std=c++17 -I include src/ThreeHeap.cpp 
