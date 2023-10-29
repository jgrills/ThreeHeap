#include <ThreeHeap.h>

#include <new>
#include <string_view>

#include <unistd.h>

class HeapInterface : public ThreeHeap::DefaultInterface
{
public:
	void report(const void * ptr, int64_t size, int alignment, ThreeHeap::Flags flags) override;
	void terminate() override;
};

static HeapInterface heap_interface;
ThreeHeap g_heap(heap_interface, ThreeHeap::Flags{0});
bool g_heap_permissive = false;
bool g_heap_report_operations = false;

void HeapInterface::report(const void * ptr, int64_t size, int alignment, ThreeHeap::Flags flags)
{
	if (g_heap_report_operations || !flags.isOperation())
		DefaultInterface::report(ptr, size, alignment, flags);
}

void HeapInterface::terminate()
{
	// Ignore errors if asked to
	if (g_heap_permissive)
		return;
	DefaultInterface::terminate();
}

#if 0

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

void * operator new(std::size_t const size)
{
	return g_heap.allocate(size, 0, ThreeHeap::new_scalar);
}

void operator delete(void * const ptr) throw()
{
	g_heap.free(ptr, ThreeHeap::new_scalar);
}

void * operator new[](std::size_t const size)
{
	return g_heap.allocate(size, 0, ThreeHeap::new_array);
}

void operator delete[](void * const ptr) throw()
{
	g_heap.free(ptr, ThreeHeap::new_array);
}
