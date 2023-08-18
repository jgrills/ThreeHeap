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

const ThreeHeap::CheckFlags validate_pre_guardband{ThreeHeap::CheckFlags::flag_validate_pre_guardband};
const ThreeHeap::CheckFlags validate_post_guardband{ThreeHeap::CheckFlags::flag_validate_post_guardband};
const ThreeHeap::CheckFlags validate_guardbands{ThreeHeap::CheckFlags::flag_validate_pre_guardband | ThreeHeap::CheckFlags::flag_validate_post_guardband};
const ThreeHeap::CheckFlags validate_free_patterns{ThreeHeap::CheckFlags::flag_validate_free_pattern};
const ThreeHeap::CheckFlags validate_everything{ ThreeHeap::CheckFlags::flag_validate_pre_guardband | ThreeHeap::CheckFlags::flag_validate_post_guardband | ThreeHeap::CheckFlags::flag_validate_free_pattern};

const ThreeHeap::ConfigureFlags fill_pre_guardband{ThreeHeap::ConfigureFlags::flag_fill_pre_guardband};
const ThreeHeap::ConfigureFlags fill_post_guardband{ThreeHeap::ConfigureFlags::flag_fill_post_guardband};
const ThreeHeap::ConfigureFlags fill_guardbands{ThreeHeap::ConfigureFlags::flag_fill_pre_guardband | ThreeHeap::ConfigureFlags::flag_fill_post_guardband};
const ThreeHeap::ConfigureFlags fill_free_patterns{ThreeHeap::ConfigureFlags::flag_fill_free_pattern};
const ThreeHeap::ConfigureFlags fill_everything{ThreeHeap::ConfigureFlags::flag_fill_pre_guardband | ThreeHeap::ConfigureFlags::flag_fill_post_guardband | ThreeHeap::ConfigureFlags::flag_fill_free_pattern};

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
