#include <ThreeHeap.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>

int constexpr number_of_allocations = 2048;
void* pointer[number_of_allocations];
std::vector<int> full;
std::vector<int> empty;

int main()
{
    ThreeHeap::DefaultInterface heap_interface;
    ThreeHeap::Flags heap_flags;
    ThreeHeap heap(heap_interface, heap_flags);

    srand(0);

    empty.reserve(number_of_allocations);
    full.reserve(number_of_allocations);

    for (int i = 0; i < number_of_allocations; ++i)
        empty.push_back(i);

    int constexpr loop_count = 10000;
    for (int loops = 0; loops < loop_count; ++loops)
    {
        int operation = rand() % number_of_allocations;
        if (operation <= empty.size())
        {
            if (empty.size())
            {
                int const slot = empty.back();
                empty.pop_back();
                int const size = rand() % (64 * 1024);
                void * result = heap.allocate(size, 0);
                pointer[slot] = result;
                printf("allocate[%4d] = %8p @ %8d\n", slot, result, size);
                full.push_back(slot);
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
                void * p = pointer[slot];
                heap.free(p);
                printf("allocate[%4d] = %8p free\n", slot, p);
            }
        }
    }
}
