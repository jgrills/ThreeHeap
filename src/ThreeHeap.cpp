#include <ThreeHeap.h>

// CPP defines that control the code compilation for these features,
// this should be kept private in this one translation unit
#define USE_ALLOCATION_STACK_DEPTH       0
#define USE_PRE_GUARD_BAND_VALIDATION    0
#define USE_POST_GUARD_BAND_VALIDATION   0
#define USE_INITIALIZE_PATTERN_FILL      0
#define USE_FREE_PATTERN_FILL            0
#define USE_FREE_PATTERN_VALIDATION      0

THREEHEAP_DEFINE_FLAGS1(malloc, flag_from_malloc);
THREEHEAP_DEFINE_FLAGS2(malloc_calloc, flag_from_malloc, flag_malloc_calloc);
THREEHEAP_DEFINE_FLAGS2(malloc_aligned, flag_from_malloc, flag_malloc_aligned);
THREEHEAP_DEFINE_FLAGS2(malloc_aligned_calloc, flag_from_malloc, flag_malloc_calloc);
THREEHEAP_DEFINE_FLAGS2(malloc_aligned_valloc, flag_from_malloc, flag_malloc_valloc);
THREEHEAP_DEFINE_FLAGS2(new_scalar, flag_from_new, flag_new_scalar);
THREEHEAP_DEFINE_FLAGS2(new_array, flag_from_new, flag_new_array);

THREEHEAP_DEFINE_FLAGS1(validate_pre_guardband, flag_validate_pre_guardband);
THREEHEAP_DEFINE_FLAGS1(validate_post_guardband, flag_validate_post_guardband);
THREEHEAP_DEFINE_FLAGS2(validate_guardbands,flag_validate_pre_guardband, flag_validate_post_guardband);
THREEHEAP_DEFINE_FLAGS1(validate_free_patterns, flag_validate_free_pattern);
THREEHEAP_DEFINE_FLAGS3(validate_everything, flag_validate_pre_guardband, flag_validate_post_guardband, flag_validate_free_pattern);

THREEHEAP_DEFINE_FLAGS1(fill_pre_guardband, flag_fill_pre_guardband);
THREEHEAP_DEFINE_FLAGS1(fill_post_guardband, flag_fill_post_guardband);
THREEHEAP_DEFINE_FLAGS2(fill_guardbands, flag_fill_pre_guardband, flag_fill_post_guardband);
THREEHEAP_DEFINE_FLAGS1(fill_free_patterns, flag_fill_free_pattern);
THREEHEAP_DEFINE_FLAGS3(fill_everything, flag_fill_pre_guardband, flag_fill_post_guardband, flag_fill_free_pattern);

ThreeHeap::ThreeHeap(ExternalInterface&& ei, Flags flags)
: external_interface(ei), configure_flags(flags)
{
}

ThreeHeap::~ThreeHeap()
{
}

void* ThreeHeap::allocate(int size, int alignment, Flags flags)
{
    (void)size;
    (void)alignment;
    (void)flags;
    return nullptr;
}
void ThreeHeap::free(void *memory, Flags flags)
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
