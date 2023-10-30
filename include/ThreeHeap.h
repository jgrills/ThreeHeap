#pragma once

#include <stddef.h>
#include <stdint.h>

// ======================================================================

#define THREEHEAP_DEFINE_FLAG(flag_name, flag_bit, function_name) \
	static inline constexpr uint32_t flag_name  = flag_bit; \
	inline bool function_name() const { return (flag_name & flags) != 0; }

#define THREEHEAP_DECLARE_FLAGS(flags_name) \
	static const ThreeHeap::Flags flags_name

// ======================================================================

class ThreeHeap
{
public:

	struct Flags
	{
		THREEHEAP_DEFINE_FLAG(flag_from_new,                0b0000'0000'0000'0000'0001, isNew);
		THREEHEAP_DEFINE_FLAG(flag_new_scalar,              0b0000'0000'0000'0000'0010, isNewScalar);
		THREEHEAP_DEFINE_FLAG(flag_new_array,               0b0000'0000'0000'0000'0100, isNewArray);
		THREEHEAP_DEFINE_FLAG(flag_from_malloc,             0b0000'0000'0000'0001'0000, isMalloc);
		THREEHEAP_DEFINE_FLAG(flag_malloc_aligned,          0b0000'0000'0000'0010'0000, isMallocAligned);
		THREEHEAP_DEFINE_FLAG(flag_malloc_calloc,           0b0000'0000'0000'0100'0010, IsMallocCalloc);
		THREEHEAP_DEFINE_FLAG(flag_malloc_valloc,           0b0000'0000'0000'1000'0010, isMallocValloc);

		THREEHEAP_DEFINE_FLAG(flag_report_allocation,       0b0000'0000'0001'0000'0000, isAllocate);
		THREEHEAP_DEFINE_FLAG(flag_report_free,             0b0000'0000'0010'0000'0000, isFree);

		THREEHEAP_DEFINE_FLAG(flag_validate_pre_guardband,  0b0000'0001'0000'0000'0000, validatePreGuardBand);
		THREEHEAP_DEFINE_FLAG(flag_validate_post_guardband, 0b0000'0010'0000'0000'0000, validatePostGuardBand);
		THREEHEAP_DEFINE_FLAG(flag_validate_free_pattern,   0b0000'0100'0000'0000'0000, validateFreePattern);

		THREEHEAP_DEFINE_FLAG(flag_fill_pre_guardband,      0b0001'0000'0000'0000'0000, fillPreGuardBand);
		THREEHEAP_DEFINE_FLAG(flag_fill_post_guardband,     0b0010'0000'0000'0000'0000, fillPostGuardBand);
		THREEHEAP_DEFINE_FLAG(flag_fill_free_pattern,       0b0100'0000'0000'0000'0000, fillFreePattern);
		THREEHEAP_DEFINE_FLAG(flag_fill_allocate_pattern,   0b1000'0000'0000'0000'0000, fillAllocatePatten);

		uint32_t flags;
	};

	THREEHEAP_DECLARE_FLAGS(zero);

	THREEHEAP_DECLARE_FLAGS(new_scalar);
	THREEHEAP_DECLARE_FLAGS(new_array);
	THREEHEAP_DECLARE_FLAGS(malloc);
	THREEHEAP_DECLARE_FLAGS(malloc_calloc);
	THREEHEAP_DECLARE_FLAGS(malloc_aligned);
	THREEHEAP_DECLARE_FLAGS(malloc_aligned_calloc);
	THREEHEAP_DECLARE_FLAGS(malloc_aligned_valloc);

	THREEHEAP_DECLARE_FLAGS(free_check);

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

	THREEHEAP_DECLARE_FLAGS(report_allocation);
	THREEHEAP_DECLARE_FLAGS(report_free);

	struct ErrorInfo
	{
		enum class Type
		{
			Unknown,
			Assert,
			MismatchedFree,
			//TODO GuardBandCorruption,
			//TODO FreePatternCorruption,
		};

		Type type = Type::Unknown;
		void const * memory = nullptr;
		int64_t size = 0;

		char const * assert = nullptr;
		char const * file = nullptr;
		int line = 0;

		Flags allocation_flags = ThreeHeap::zero;
		Flags free_flags = ThreeHeap::zero;

		// @TODO What else should we include with this
	};
	struct ExternalInterface
	{
		virtual void * system_allocator(int64_t & size) = 0;
		virtual void report_operation(const void * ptr, int64_t size, int alignment, const void * owner, Flags flags) = 0;
		virtual void report_allocations(const void * ptr, int64_t size, const void * owner, Flags flags) = 0;
		virtual void error(ErrorInfo const & info) = 0;
		virtual void terminate() = 0;
	};
	struct DefaultInterface : public ExternalInterface
	{
		void * system_allocator(int64_t & size) override;
		void report_operation(const void * ptr, int64_t size, int alignment, const void * owner, Flags flags) override;
		void report_allocations(const void * ptr, int64_t size, const void * owner, Flags flags) override;
		void error(ErrorInfo const & info) override;
		void terminate() override;
	};

	ThreeHeap(ExternalInterface & external_interface, Flags enabled);
	~ThreeHeap();

	int getTotalNumberOfFrees() const;
	int getTotalNumberOfAllocations() const;
	int getCurrentNumberOfAllocations() const;
	int getMaximumNumberOfAllocations() const;

	int64_t getTotalNumberOfBytesAllocated() const;
	int64_t getCurrentNumberOfBytesAllocated() const;
	int64_t getMaximumNumberOfBytesAllocated() const;

	int64_t getCurrentNumberOfBytesFree() const;
	int64_t getTotalNumberOfBytesUsed() const;
	int64_t getCurrentNumberOfBytesUsed() const;
	int64_t getMaximumNumberOfBytesUsed() const;

	// bool getConfigureFlag(Flags flag) const;
	// void setConfigureFlag(Flags flag, bool enabled);

	// Interface for C & C++ depending upon the AllocationFlags that get passed in
	void * allocate(int64_t size, int alignment, Flags flags, void const * owner=nullptr);
	void free(void * memory, Flags flags);

	// Reallocate only supports malloc, no alignment, no valloc, no clearing allowed on these blocks
	void * reallocate(void * memory, int64_t size);

	// Find out the size of an allocation
	int64_t getAllocationSize(void * memory) const;

	// Change the ownership of the memory to this caller
	// void * own(void * memory);

	// Verify the internal heap structures
	void verify() const;

	// Print outstanding memory allocations
	void report_allocations() const;

private:

	ExternalInterface & external;
	Flags configure_flags;

private:

	struct Block;
	struct FreeBlock;
	struct AllocatedBlock;
	struct SentinelBlock;
	struct SystemAllocation;

private:

	void allocateFromSystem(int64_t minimum_size);

	void verify(FreeBlock const * parent, FreeBlock const * node, int & number_of_free_blocks) const;

	void removeFromFreeList(FreeBlock * block);
	void addToFreeList(FreeBlock * block);
	FreeBlock * searchFreeList(int64_t size);

private:

	FreeBlock * free_list = nullptr;
	SystemAllocation * first_system_allocation = nullptr;
	SystemAllocation * last_system_allocation = nullptr;

	int total_number_of_allocations = 0;
	int total_number_of_frees = 0;
	int current_number_of_allocations = 0;
	int maximum_number_of_allocations = 0;

	int total_bytes_allocated = 0;
	int current_bytes_allocated = 0;
	int maximum_bytes_allocated = 0;
	int current_bytes_free = 0;

	int total_bytes_used = 0;
	int current_bytes_used = 0;
	int maximum_bytes_used = 0;

private:

	ThreeHeap(const ThreeHeap &) = delete;
	ThreeHeap& operator=(const ThreeHeap &) = delete;
	ThreeHeap(ThreeHeap &&) = delete;
	ThreeHeap& operator=(ThreeHeap &&) = delete;
};

// ======================================================================

inline int ThreeHeap::getTotalNumberOfFrees() const
{
	return total_number_of_frees;
}

inline int ThreeHeap::getTotalNumberOfAllocations() const
{
	return total_number_of_allocations;
}

inline int ThreeHeap::getCurrentNumberOfAllocations() const
{
	return current_number_of_allocations;
}

inline int ThreeHeap::getMaximumNumberOfAllocations() const
{
	return maximum_number_of_allocations;
}

inline int64_t ThreeHeap::getTotalNumberOfBytesAllocated() const
{
	return total_bytes_allocated;
}

inline int64_t ThreeHeap::getCurrentNumberOfBytesAllocated() const
{
	return current_bytes_allocated;
}

inline int64_t ThreeHeap::getCurrentNumberOfBytesFree() const
{
	return current_bytes_free;
}

inline int64_t ThreeHeap::getMaximumNumberOfBytesAllocated() const
{
	return maximum_bytes_allocated;
}

inline int64_t ThreeHeap::getTotalNumberOfBytesUsed() const
{
	return total_bytes_used;
}

inline int64_t ThreeHeap::getCurrentNumberOfBytesUsed() const
{
	return current_bytes_used;
}

inline int64_t ThreeHeap::getMaximumNumberOfBytesUsed() const
{
	return maximum_bytes_used;
}
