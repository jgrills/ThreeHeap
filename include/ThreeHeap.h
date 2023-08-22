#include <stdint.h>

#define THREEHEAP_DEFINE_FLAG(flag_name, flag_bit, function_name) \
		static inline constexpr uint32_t flag_name  = flag_bit; \
		inline bool function_name() const { return (flag_name & flags) != 0; }

#define THREEHEAP_DECLARE_FLAGS(flags_name) \
		static const ThreeHeap::Flags flags_name

#define THREEHEAP_DEFINE_FLAGS1(flags_name, flags0) \
		const ThreeHeap::Flags flags_name{ThreeHeap::Flags::flags0}

#define THREEHEAP_DEFINE_FLAGS2(flags_name, flags0, flags1) \
		const ThreeHeap::Flags flags_name{ThreeHeap::Flags::flags0 | ThreeHeap::Flags::flags1}

#define THREEHEAP_DEFINE_FLAGS3(flags_name, flags0, flags1, flags2) \
		const ThreeHeap::Flags flags_name{ThreeHeap::Flags::flags0 | ThreeHeap::Flags::flags1 | ThreeHeap::Flags::flags2}

#define THREEHEAP_DEFINE_FLAGS4(flags_name, flags0, flags1, flags2, flags3) \
		const ThreeHeap::Flags flags_name{ThreeHeap::Flags::flags0 | ThreeHeap::Flags::flags1 | ThreeHeap::Flags::flags2 | ThreeHeap::Flags::flags3}

class ThreeHeap
{
public:

	struct Flags;
	struct ExternalInterface
	{
		struct HeapError
		{
			// @todo what data should we report?
		};

		void* (*page_allocator)(int size);
		void (*report)(const void *ptr, int size, int alignment, Flags flags);
		void (*heap_error)(HeapError &heap_error);
	};

	ThreeHeap(ExternalInterface&& external_interface, Flags enabled);
	~ThreeHeap();

	int getTotalNumberOfAllocations();
	int getCurrentNumberOfAllocations();
	int getCurrentNumberOfBytesAllocated();
	int getMaximumNumberOfAllocations();
	int getMaximumNumberOfBytesAllocated();

	bool getConfigureFlag(Flags flag) const;
	void setConfigureFlag(Flags flag, bool enabled);

	// Interface for C & C++ depending upon the AllocationFlags that get passed in
	void* allocate(int size, int alignment, Flags flags);
	void free(void *memory, Flags flags);

	// Reallocate only supports malloc, no alignment, valloc, clearing allowed on these blocks
	void* reallocate(void *memory, int size);

	// Change the ownership of the memory to this caller
	void* own(void *memory);

	// Check functions for the heap and memory blocks
	void checkAllAllocations(Flags flags) const;

	// Check this one allocation
	void checkAllocation(const void *memory, Flags flags) const;

	struct Flags
	{
		THREEHEAP_DEFINE_FLAG(flag_from_new,                0b0000'0000'0000'0000'0001, isNew);
		THREEHEAP_DEFINE_FLAG(flag_new_scalar,              0b0000'0000'0000'0000'0010, isNewScalar);
		THREEHEAP_DEFINE_FLAG(flag_new_array,               0b0000'0000'0000'0000'0100, isNewArray);
		THREEHEAP_DEFINE_FLAG(flag_from_malloc,             0b0000'0000'0000'0001'0000, isMalloc);
		THREEHEAP_DEFINE_FLAG(flag_malloc_aligned,          0b0000'0000'0000'0010'0000, isMallocAligned);
		THREEHEAP_DEFINE_FLAG(flag_malloc_calloc,           0b0000'0000'0000'0100'0010, IsMallocCalloc);
		THREEHEAP_DEFINE_FLAG(flag_malloc_valloc,           0b0000'0000'0000'1000'0010, isMallocValloc);

		THREEHEAP_DEFINE_FLAG(flag_report_allocate,         0b0000'0000'0001'0000'0000, isAllocate);
		THREEHEAP_DEFINE_FLAG(flag_report_free,	            0b0000'0000'0010'0000'0000, isFree);

		THREEHEAP_DEFINE_FLAG(flag_validate_pre_guardband,  0b0000'0001'0000'0000'0000, validatePreGuardBand);
		THREEHEAP_DEFINE_FLAG(flag_validate_post_guardband, 0b0000'0010'0000'0000'0000, validatePostGuardBand);
		THREEHEAP_DEFINE_FLAG(flag_validate_free_pattern,   0b0000'0100'0000'0000'0000, validateFreePattern);

		THREEHEAP_DEFINE_FLAG(flag_fill_pre_guardband,      0b0001'0000'0000'0000'0000, fillPreGuardBand);
		THREEHEAP_DEFINE_FLAG(flag_fill_post_guardband,     0b0010'0000'0000'0000'0000, fillPostGuardBand);
		THREEHEAP_DEFINE_FLAG(flag_fill_free_pattern,       0b0100'0000'0000'0000'0000, fillFreePattern);
		THREEHEAP_DEFINE_FLAG(flag_fill_allocate_pattern,   0b1000'0000'0000'0000'0000, fillAllocatePatten);

		uint32_t flags;
	};

	THREEHEAP_DECLARE_FLAGS(malloc);
	THREEHEAP_DECLARE_FLAGS(new_scalar);
	THREEHEAP_DECLARE_FLAGS(new_array);
	THREEHEAP_DECLARE_FLAGS(malloc_calloc);
	THREEHEAP_DECLARE_FLAGS(malloc_aligned);
	THREEHEAP_DECLARE_FLAGS(malloc_aligned_calloc);
	THREEHEAP_DECLARE_FLAGS(malloc_aligned_valloc);

	THREEHEAP_DECLARE_FLAGS(validate_pre_guardband);
	THREEHEAP_DECLARE_FLAGS(validate_post_guardband);
	THREEHEAP_DECLARE_FLAGS(validate_guardbands);
	THREEHEAP_DECLARE_FLAGS(validate_free_patterns);
	THREEHEAP_DECLARE_FLAGS(validate_everything);

	THREEHEAP_DECLARE_FLAGS(fill_pre_guardband);
	THREEHEAP_DECLARE_FLAGS(fill_post_guardband);
	THREEHEAP_DECLARE_FLAGS(fill_guardbands);
	THREEHEAP_DECLARE_FLAGS(fill_free_patterns);
	THREEHEAP_DECLARE_FLAGS(fill_everything);

private:

	struct AllocationBlock;

	void checkAllocationBlock(AllocationBlock *block);

private:
	ExternalInterface external_interface;
	Flags configure_flags;

private:
	ThreeHeap(const ThreeHeap &) = delete;
	ThreeHeap& operator=(const ThreeHeap &) = delete;
	ThreeHeap(ThreeHeap &&) = delete;
	ThreeHeap& operator=(ThreeHeap &&) = delete;
};
