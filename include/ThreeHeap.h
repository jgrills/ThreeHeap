#include <stddef.h>
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
	struct HeapError
	{
		// @todo what data should we report?
	};
	struct ExternalInterface
	{
		virtual void* page_allocator(int64_t size) = 0;
		virtual void report(const void *ptr, int64_t size, int alignment, Flags flags) = 0;
		virtual void heap_error(HeapError &heap_error) = 0;
	};
	struct DefaultInterface : public ExternalInterface
	{
		void * page_allocator(int64_t size) override;
		void report(const void * ptr, int64_t size, int alignment, Flags flags) override;
		void heap_error(HeapError & heap_error) override;
	};

	ThreeHeap(ExternalInterface& external_interface, Flags enabled);
	~ThreeHeap();

	int getTotalNumberOfAllocations() const;
	int getCurrentNumberOfAllocations() const;
	int getMaximumNumberOfAllocations() const;

	int64_t getTotalNUmberOfBytesAllocated() const;
	int64_t getCurrentNumberOfBytesAllocated() const;
	int64_t getMaximumNumberOfBytesAllocated() const;

	bool getConfigureFlag(Flags flag) const;
	void setConfigureFlag(Flags flag, bool enabled);

	// Interface for C & C++ depending upon the AllocationFlags that get passed in
	void* allocate(int64_t size, int alignment /* , Flags flags */);
	void free(void * memory /* , Flags flags */);

	int64_t getAllocationSize(void * memory) const;

	void verify() const;

	// Reallocate only supports malloc, no alignment, valloc, clearing allowed on these blocks
	void* reallocate(void *memory, int64_t size);

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
	ExternalInterface& external_interface;
	Flags configure_flags;

private:
	constexpr static size_t Alignment = 64;
	constexpr static size_t AllocationPad = Alignment;
	constexpr static size_t HeaderSize = Alignment;
	constexpr static size_t SplitSize = Alignment * 2;

	// Alignment needs to be a power of 2
	static_assert((Alignment & (Alignment-1)) == 0);

	struct Block;
	struct FreeBlock;
	struct AllocatedBlock;
	struct SentinelBlock;

	enum class BlockStatus : int16_t
	{
		Unknown,
		Free,
		Allocated,
		Sentinel
	};

	struct SystemAllocation
	{
		SentinelBlock * start = nullptr;
		SentinelBlock * end = nullptr;
		SystemAllocation * next = nullptr;
	};

private:

	void check_free_pattern(void *memory);
	// bool option_flag_values[static_cast<int>(OptionFlags::total_count)];

	void verify(FreeBlock const *parent, FreeBlock const *node) const;
	void assertHandler(const char * const file, int line, const char * const text, const bool everything_is_okay) const;

	void allocateFromSystem();

	void removeFromFreeList(FreeBlock *block);
	void addToFreeList(FreeBlock *block);
	FreeBlock *searchFreeList(int64_t size);

private:

	FreeBlock *free_list = nullptr;
	SystemAllocation * first_system_allocation = nullptr;
	SystemAllocation * last_system_allocation = nullptr;
	mutable int16_t verification_stamp = 0;
	mutable int free_tree = 0;

private:

	ThreeHeap(const ThreeHeap &) = delete;
	ThreeHeap& operator=(const ThreeHeap &) = delete;
	ThreeHeap(ThreeHeap &&) = delete;
	ThreeHeap& operator=(ThreeHeap &&) = delete;
};
