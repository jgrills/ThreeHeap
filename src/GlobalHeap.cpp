#include <GlobalHeap.h>
#include <ThreeHeap.h>

#include <new>

#include <unistd.h>

HeapInterface heap_interface;
ThreeHeap g_heap(heap_interface, ThreeHeap::zero);

void HeapInterface::report(const void * ptr, int64_t size, int alignment, const void * const owner, ThreeHeap::Flags flags)
{
	if (report_operations || !flags.isOperation())
		DefaultInterface::report(ptr, size, alignment, owner, flags);
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
	void * allocate_new_scalar(int64_t const size, void const * const owner);
	void * allocate_new_array(int64_t const size, void const * const owner);
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
void * allocate_new_scalar(int64_t const size, void const * const owner)
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
void * allocate_new_array(int64_t const size, void const * const owner)
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
