#include <ThreeHeap.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <assert.h>

int constexpr number_of_allocations = 2048;
void* pointer[number_of_allocations];
std::vector<int> full;
std::vector<int> empty;

int main()
{
	ThreeHeap::DefaultInterface heap_interface;
	ThreeHeap::Flags heap_flags;
	ThreeHeap heap(heap_interface, heap_flags);

	int16_t & watchpoint = *reinterpret_cast<int16_t *>(0x7ffff3a84956);
	(void)watchpoint;

	srand(0);

	empty.reserve(number_of_allocations);
	full.reserve(number_of_allocations);

	for (int i = 0; i < number_of_allocations; ++i)
		empty.push_back(i);

	int constexpr loop_count = 100'000'000;
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
				void * result = heap.allocate(size, 0);
				printf("%6d alloc[%4d] (%8d) %8p\n", loops, slot, (int)size, result);
				pointer[slot] = result;
				full.push_back(slot);

				// Verify the heap after every operation
				heap.verify();
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
				int64_t const size = heap.getAllocationSize(p);
				heap.free(p);
				printf("%6d free [%4d] (%8d) %8p\n", loops, slot, (int)size, p);

				// Verify the heap after every operation
				heap.verify();
			}
		}
	}
}
