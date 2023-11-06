#include <GlobalHeap.h>
#include <ThreeHeap.h>

#include <new>

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

HeapInterface g_heapInterface;
ThreeHeap g_heap(g_heapInterface, ThreeHeap::heap_debug);

int64_t fixed_sizes[1024];

void HeapInterface::tree_fixed_nodes(int64_t * & sizes, int & count)
{
	int64_t size = 32 * 1024;
	int c = 0;
	fixed_sizes[c++] = size;
	printf("%d\n", (int)(fixed_sizes[c-1]));

	int base = 0;

	for (int i = 0; i < 2; ++i)
	{
		size >>= 1;
		int const top = c;
		for (int j = base; j < top; ++j)
		{
			fixed_sizes[c++] = fixed_sizes[j] - size;
			printf("%d\n", (int)fixed_sizes[c-1]);
			fixed_sizes[c++] = fixed_sizes[j] + size;
			printf("%d\n", (int)fixed_sizes[c-1]);
		}
		base = top;
	}
	sizes = fixed_sizes;
	count = c;
}

void HeapInterface::report_operation(const void * ptr, int64_t size, int alignment, const void * const owner, ThreeHeap::Flags flags)
{
	if (!report_operations)
		return;
	DefaultInterface::report_operation(ptr, size, alignment, owner, flags);
}

void HeapInterface::terminate()
{
	// Ignore errors if asked to
	if (permissive)
		return;
	DefaultInterface::terminate();
}

extern "C"
{
	void * heap_own(void * memory, void * owner);
	void * allocate_new_scalar(int64_t size, void * owner);
	void * allocate_new_array(int64_t size, void * owner);
}

// Thin assembly function to grab the return address off the stack
// and use it for the owner for the allocation.
__attribute__((naked))
void * HeapOwn(void * const memory)
{
	__asm__("mov (%rsp),%rsi");
	__asm__("jmp heap_own");
}

// This function is intentionally immediately after its caller so it'll be hot in the cache
void *heap_own(void * const memory, void * const owner)
{
	return g_heap.own(memory, owner);
}

// Thin assembly function to grab the return address off the stack
// and use it for the owner for the allocation.
__attribute__((naked))
void * operator new(std::size_t const size)
{
	__asm__("mov (%rsp),%rsi");
	__asm__("jmp allocate_new_scalar");
}

// This function is intentionally immediately after its caller so it'll be hot in the cache
void * allocate_new_scalar(int64_t const size, void * const owner)
{
	return g_heap.allocate(size, 0, ThreeHeap::new_scalar, owner);
}

void operator delete(void * const ptr) throw()
{
	g_heap.free(ptr, ThreeHeap::new_scalar);
}

// Thin assembly function to grab the return address off the stack
// and use it for the owner for the allocation.
__attribute__((naked))
void * operator new[](std::size_t const size)
{
	__asm__("mov (%rsp),%rsi");
	__asm__("jmp allocate_new_array");
}

// This function is intentionally immediately after its caller so it'll be hot in the cache
void * allocate_new_array(int64_t const size, void * const owner)
{
	return g_heap.allocate(size, 0, ThreeHeap::new_array, owner);
}

void operator delete[](void * const ptr) throw()
{
	g_heap.free(ptr, ThreeHeap::new_array);
}

#if 0
#include <string_view>

// @TODO These need to be pushing into a library and linked
void * malloc(size_t const size) {
	std::string_view message { "malloc\n" };
	(void)write(1, message.data(), message.size());
	return g_heap.allocate(size, 0, ThreeHeap::malloc);
}

void free(void * const ptr) {
	std::string_view message { "free\n" };
	(void)write(1, message.data(), message.size());
	g_heap.free(ptr, ThreeHeap::malloc);
}

void * calloc(size_t const elements, size_t const size)
{
	std::string_view message { "calloc\n" };
	(void)write(1, message.data(), message.size());
	return g_heap.allocate(elements * size, 0, ThreeHeap::malloc_calloc);
}

void * realloc(void * const ptr, size_t const size)
{
	std::string_view message { "realloc\n" };
	(void)write(1, message.data(), message.size());
	return g_heap.reallocate(ptr, size);
}

#endif
