.PHONY: build

build: output/threeheap

output/ThreeHeap.o: src/ThreeHeap.cpp include/ThreeHeap.h Makefile
	@mkdir -p output
	g++ -Wall -Wno-sign-compare -c -o output/ThreeHeap.o -g3 -std=c++17 -I include src/ThreeHeap.cpp

output/GlobalHeap.o: src/GlobalHeap.cpp include/GlobalHeap.h include/ThreeHeap.h Makefile
	@mkdir -p output
	g++ -Wall -Wno-sign-compare -c -o output/GlobalHeap.o -g3 -std=c++17 -I include src/GlobalHeap.cpp

output/main.o: src/main.cpp include/ThreeHeap.h include/GlobalHeap.h Makefile
	@mkdir -p output
	g++ -Wall -Wno-sign-compare -c -o output/main.o -g3 -std=c++17 -I include src/main.cpp

output/threeheap: output/ThreeHeap.o output/GlobalHeap.o output/main.o Makefile
	g++ -Wall -Wno-sign-compare -o output/threeheap -g3 -std=c++17 output/ThreeHeap.o output/GlobalHeap.o output/main.o

