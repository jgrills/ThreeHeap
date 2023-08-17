#include <ThreeHeap.h>

// CPP defines that control the code compilation for these features,
// this should be kept private in this one translation unit
#define USE_ALLOCATION_STACK_DEPTH       0
#define USE_PRE_GUARD_BAND_VALIDATION    0
#define USE_POST_GUARD_BAND_VALIDATION   0
#define USE_INITIALIZE_PATTERN_FILL      0
#define USE_FREE_PATTERN_FILL            0
#define USE_FREE_PATTERN_VALIDATION      0

const ThreeHeap::AllocationFlags malloc{ThreeHeap::AllocationFlags::flag_from_malloc};
const ThreeHeap::AllocationFlags new_scalar{ThreeHeap::AllocationFlags::flag_from_new | ThreeHeap::AllocationFlags::flag_new_scalar};
const ThreeHeap::AllocationFlags new_array{ThreeHeap::AllocationFlags::flag_from_new | ThreeHeap::AllocationFlags::flag_new_array};
const ThreeHeap::AllocationFlags malloc_calloc{ThreeHeap::AllocationFlags::flag_from_malloc | ThreeHeap::AllocationFlags::flag_malloc_calloc};
const ThreeHeap::AllocationFlags malloc_aligned{ThreeHeap::AllocationFlags::flag_from_malloc | ThreeHeap::AllocationFlags::flag_malloc_aligned};
const ThreeHeap::AllocationFlags malloc_aligned_calloc{ThreeHeap::AllocationFlags::flag_from_malloc | ThreeHeap::AllocationFlags::flag_malloc_aligned | ThreeHeap::AllocationFlags::flag_malloc_calloc};
const ThreeHeap::AllocationFlags malloc_aligned_valloc{ThreeHeap::AllocationFlags::flag_from_malloc | ThreeHeap::AllocationFlags::flag_malloc_aligned | ThreeHeap::AllocationFlags::flag_malloc_valloc};

ThreeHeap::ThreeHeap(SystemAllocator system_allocator)
{
}

ThreeHeap::~ThreeHeap()
{
}

void* ThreeHeap::allocate(int size, int alignment, AllocationFlags flags)
{
    (void)size;
    (void)alignment;
    (void)flags;
    return nullptr;
}
void ThreeHeap::free(void *memory, AllocationFlags flags)
{
    (void)memory;
    (void)flags;
}

// Reallocate only supports malloc, no alignment, valloc, clearing allowed on these blocks
void* ThreeHeap::reallocate(void *memory, int size)
{
    (void)memory;
    (void)size;
    return nullptr;
}

// Change the ownership of the memory to this caller
void* ThreeHeap::own(void *memory)
{
    (void)memory;
    return memory;
}

// Check functions for the heap and memory blocks
void ThreeHeap::checkEverything() const
{
}

// Check this one allocation
void ThreeHeap::check(void *memory) const
{
    (void)memory;
}

// check guard bands on this one allocation
void ThreeHeap::check_guard_bands(void *memory) const
{
    (void)memory;
}

void ThreeHeap::check_pre_guard_band(void *memory) const
{
    (void)memory;
}

void ThreeHeap::check_post_guard_band(void *memory) const
{
    (void)memory;
}
