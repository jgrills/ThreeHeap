#include <GlobalHeap.h>

#include <string_view>
#include <vector>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define USE_THREEHEAP 1

int constexpr number_of_allocations = 64;
void* pointer[number_of_allocations];
std::vector<int> full;
std::vector<int> empty;

int main()
{
	srand(0);

	// test new & delete
	delete new int;
	delete[] new int[10];

	empty.reserve(number_of_allocations);
	full.reserve(number_of_allocations);
	for (int i = 0; i < number_of_allocations; ++i)
		empty.push_back(i);

	constexpr bool verify = false;

	int constexpr loop_count = 100'000;
	for (int loops = 0; loops < loop_count; ++loops)
	{
		assert(full.size() + empty.size() == number_of_allocations);

		int operation = rand() % number_of_allocations;
		if (operation <= empty.size())
		{
			if (empty.size())
			{
				int const slot = empty.back();
				empty.pop_back();
				int const size = rand() % (64 * 1024);
#if USE_THREEHEAP == 1
				void * result = g_heap.allocate(size, 0, ThreeHeap::malloc);
#else
				void * result = malloc(size);
#endif

				pointer[slot] = result;
				full.push_back(slot);
				// printf("%6d alloc[%4d] (%8d) %8p\n", loops, slot, (int)size, result);

				// Verify the heap after every operation
				if constexpr (verify)
					g_heap.verify();
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
				// int64_t const size = heap.getAllocationSize(p);
				// printf("%6d free [%4d] (%8d) %8p\n", loops, slot, (int)size, p);
#if USE_THREEHEAP == 1
				g_heap.free(p, ThreeHeap::malloc);
#else
				free(p);
#endif

				// Verify the heap after every operation
				if constexpr (verify)
					g_heap.verify();
			}
		}
	}

#if 0
	// These should be error cases
	g_heap_permissive = true;
	delete[] new char;
	delete new int[10];
	g_heap.free(new int, ThreeHeap::malloc);
	g_heap.free(new int[10], ThreeHeap::malloc);
	delete static_cast<char*>(g_heap.allocate(10, 0, ThreeHeap::malloc));
	delete[] static_cast<char*>(g_heap.allocate(10, 0, ThreeHeap::malloc));
#endif

	printf("memory leaks:\n");
	g_heap.report_allocations();
}
