#include <GlobalHeap.h>

#include <string_view>
#include <vector>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define USE_THREEHEAP 1

int constexpr number_of_allocations = 16 * 1024;
void * pointer[number_of_allocations];
std::vector<int> full;
std::vector<int> empty;

int main()
{
	srand(0);

#if USE_THREEHEAP
	printf("threeheap\n");
#else
	printf("malloc/free\n");
#endif

	empty.reserve(number_of_allocations);
	full.reserve(number_of_allocations);
	for (int i = 0; i < number_of_allocations; ++i)
		empty.push_back(i);

	constexpr bool verify = false;

	int constexpr loop_count = 1'000'000;
	for (int loops = 0; loops < loop_count; ++loops)
	{
		assert(full.size() + empty.size() == number_of_allocations);

		if ((loops % 10'000) == 1)
			printf("loop %d\n", loops);

		int const operation = rand() % number_of_allocations;
		if (operation <= empty.size())
		{
			if (empty.size())
			{
				int const slot = empty.back();
				empty.pop_back();
				int const size = rand() % (64 * 1024);
#if USE_THREEHEAP == 1
				void * result = operator new(size);
#else
				void * const result = malloc(size);
#endif
				memset(result, 1, size);
				pointer[slot] = result;
				full.push_back(slot);

				// Verify the heap after every operation
				if constexpr (verify)
					g_heap.verify(ThreeHeap::validate_free | ThreeHeap::validate_guard_bands);
				if (size & 1)
					HeapOwn(result);
			}
		}
		else
		{
			if (full.size())
			{
				int const allocation = rand() % full.size();
				int const slot = full[allocation];
				full[allocation] = full.back();
				full.pop_back();
				void * const p = pointer[slot];
				pointer[slot] = nullptr;
				empty.push_back(slot);
#if USE_THREEHEAP == 1
				operator delete(p);
#else
				free(p);
#endif

				// Verify the heap after every operation
				if constexpr (verify)
					g_heap.verify(ThreeHeap::validate_free | ThreeHeap::validate_guard_bands);
			}
		}
	}

#if 1
	// test new & delete
	delete new int;
	delete[] new int[10];

	// These should be error cases
	g_heapInterface.permissive = true;
	delete[] new char;
	delete new int[10];
	g_heap.free(new int, ThreeHeap::malloc);
	g_heap.free(new int[10], ThreeHeap::malloc);
	delete static_cast<char*>(g_heap.allocate(10, 0, ThreeHeap::malloc));
	delete[] static_cast<char*>(g_heap.allocate(10, 0, ThreeHeap::malloc));

	unsigned char* q = new unsigned char[65];
	printf("q[-1] guard band %x\n", (int)q[-1]);
	printf("q[0] init %x\n", (int)q[0]);
	delete [] q;
	printf("q[0] freed %x\n", (int)q[0]);

	// Test free pattern corruption
	char const save = q[0];
	q[0] = 1;
	g_heap.verify(ThreeHeap::validate_free | ThreeHeap::validate_guard_bands);
	q[0] = save;
	g_heap.verify(ThreeHeap::validate_free | ThreeHeap::validate_guard_bands);

	// Test guard band corruption
	char* r = new char[65];
	r[-1] = 0;
	r[65] = 0;
	r[65 + 126] = 0;
	delete [] r;
#endif

	// Print memory leaks
	printf("memory leaks:\n");
	g_heap.report_allocations();
}
