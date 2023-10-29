.PHONY: build

build: output/threeheap

OPTFLAGS := -g3 -O3
CXXFLAGS := -Wall -Wno-sign-compare -std=c++17 -I include ${OPTFLAGS}

output/ThreeHeap.o: src/ThreeHeap.cpp include/ThreeHeap.h Makefile
	@mkdir -p output
	g++ -o $@ -c $< ${CXXFLAGS}

output/GlobalHeap.o: src/GlobalHeap.cpp include/GlobalHeap.h include/ThreeHeap.h Makefile
	@mkdir -p output
	g++ -o $@ -c $< ${CXXFLAGS}

output/main.o: src/main.cpp include/ThreeHeap.h include/GlobalHeap.h Makefile
	@mkdir -p output
	g++ -o $@ -c $< ${CXXFLAGS}

output/threeheap: output/ThreeHeap.o output/GlobalHeap.o output/main.o Makefile
	g++ -o $@ ${OPTFLAGS} output/ThreeHeap.o output/GlobalHeap.o output/main.o

