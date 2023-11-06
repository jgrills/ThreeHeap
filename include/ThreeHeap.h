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
		THREEHEAP_DEFINE_FLAG(flag_malloc_calloc,           0b0000'0000'0000'0100'0000, IsMallocCalloc);
		THREEHEAP_DEFINE_FLAG(flag_malloc_valloc,           0b0000'0000'0000'1000'0000, isMallocValloc);

		THREEHEAP_DEFINE_FLAG(flag_report_allocation,       0b0000'0000'0001'0000'0000, isAllocate);
		THREEHEAP_DEFINE_FLAG(flag_report_free,             0b0000'0000'0010'0000'0000, isFree);

		THREEHEAP_DEFINE_FLAG(flag_guard_bands,             0b0001'0000'0000'0000'0000, useGuardBands);
		THREEHEAP_DEFINE_FLAG(flag_fill_guard_bands,        0b0010'0000'0000'0000'0000, fillGuardBands);
		THREEHEAP_DEFINE_FLAG(flag_fill_frees,              0b0100'0000'0000'0000'0000, fillFrees);
		THREEHEAP_DEFINE_FLAG(flag_fill_allocations,        0b1000'0000'0000'0000'0000, fillAllocations);

		THREEHEAP_DEFINE_FLAG(flag_validate_guard_bands,    0b0000'0001'0000'0000'0000, validateGuardBands);
		THREEHEAP_DEFINE_FLAG(flag_validate_free,           0b0000'0100'0000'0000'0000, validateFree);

		uint32_t flags;
	};

	THREEHEAP_DECLARE_FLAGS(heap_fast);
	THREEHEAP_DECLARE_FLAGS(heap_debug);

	THREEHEAP_DECLARE_FLAGS(zero);

	THREEHEAP_DECLARE_FLAGS(new_scalar);
	THREEHEAP_DECLARE_FLAGS(new_array);
	THREEHEAP_DECLARE_FLAGS(malloc);
	THREEHEAP_DECLARE_FLAGS(malloc_calloc);
	THREEHEAP_DECLARE_FLAGS(malloc_aligned);
	THREEHEAP_DECLARE_FLAGS(malloc_aligned_calloc);
	THREEHEAP_DECLARE_FLAGS(malloc_aligned_valloc);

	THREEHEAP_DECLARE_FLAGS(free_check);

	THREEHEAP_DECLARE_FLAGS(guard_bands);
	THREEHEAP_DECLARE_FLAGS(validate_guard_bands);
	THREEHEAP_DECLARE_FLAGS(validate_free);

	THREEHEAP_DECLARE_FLAGS(fill_guard_bands);
	THREEHEAP_DECLARE_FLAGS(fill_allocations);
	THREEHEAP_DECLARE_FLAGS(fill_frees);

	THREEHEAP_DECLARE_FLAGS(report_allocation);
	THREEHEAP_DECLARE_FLAGS(report_free);

	struct ErrorInfo
	{
		enum class Type
		{
			Unknown,
			Assert,
			MismatchedFree,
			GuardBandCorruption,
			FreeCorruption
		};

		Type type = Type::Unknown;
		void const * memory = nullptr;
		int64_t size = 0;

		char const * assert = nullptr;
		char const * file = nullptr;
		int line = 0;

		int pre_size = 0;
		int post_size = 0;
		bool pre_guard_band_corrupt[64] = {false};
		bool post_guard_band_corrupt[128] = {false};

		int free_corrupt_index = 0;

		Flags allocation_flags = ThreeHeap::zero;
		Flags free_flags = ThreeHeap::zero;

		// @TODO What else should we include with this
	};
	struct ExternalInterface
	{
		virtual void tree_fixed_nodes(int64_t * & sizes, int & count ) = 0;
		virtual void * system_allocator(int64_t & size) = 0;
		virtual void report_operation(const void * ptr, int64_t size, int alignment, const void * owner, Flags flags) = 0;
		virtual void report_allocations(const void * ptr, int64_t size, const void * owner, Flags flags) = 0;
		virtual void error(ErrorInfo const & info) = 0;
		virtual void terminate() = 0;
	};
	struct DefaultInterface : public ExternalInterface
	{
		void tree_fixed_nodes(int64_t * & sizes, int & count ) override;
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
	void * allocate(int64_t size, int alignment, Flags flags, void * owner=nullptr);
	void free(void * memory, Flags flags);

	// Reallocate only supports malloc, no alignment, no valloc, no clearing allowed on these blocks
	void * reallocate(void * memory, int64_t size);

	// Find out the size of an allocation
	int64_t getAllocationSize(void * memory) const;

	// Change the ownership of the memory to this caller
	void * own(void * memory, void * owner);

	// Verify the internal heap structures (optionally guard bands and free fills too)
	void verify(Flags flags = zero) const;

	// Print outstanding memory allocations
	void report_allocations() const;

private:

	struct Block;
	struct FreeBlock;
	struct AllocatedBlock;
	struct SentinelBlock;
	struct SystemAllocation;

private:

	static bool verifyGuardBand(void const * memory, int size, bool * corrupt);
	void verifyGuardBands(AllocatedBlock const * block) const;
	void verifyFree(void const * memory, int size) const;

	void allocateFromSystem(int64_t minimum_size);

	void verify(FreeBlock const * parent, FreeBlock const * node, int & number_of_free_blocks) const;
	void removeFromFreeList(FreeBlock * block);
	void addToFreeList(FreeBlock * block);
	FreeBlock * searchFreeList(int64_t size);

private:

	ExternalInterface & external;
	const Flags heap_flags;
	const int guard_band_size = 0;

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

	int fixed_nodes_count = 0;
	int64_t * fixed_node_sizes = nullptr;
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

inline ThreeHeap::Flags operator|(ThreeHeap::Flags const & lhs, ThreeHeap::Flags const & rhs)
{
	return ThreeHeap::Flags{lhs.flags | rhs.flags};
}

inline ThreeHeap::Flags& operator|=(ThreeHeap::Flags & lhs, ThreeHeap::Flags const & rhs)
{
	lhs.flags |= rhs.flags;
	return lhs;
}

inline ThreeHeap::Flags operator&(ThreeHeap::Flags const & lhs, ThreeHeap::Flags const & rhs)
{
	return ThreeHeap::Flags{lhs.flags & rhs.flags};
}

inline ThreeHeap::Flags operator&=(ThreeHeap::Flags & lhs, ThreeHeap::Flags const & rhs)
{
	lhs.flags &= rhs.flags;
	return lhs;
}

inline bool operator==(ThreeHeap::Flags const & lhs, ThreeHeap::Flags const & rhs)
{
	return lhs.flags == rhs.flags;
}

inline bool operator!=(ThreeHeap::Flags const & lhs, ThreeHeap::Flags const & rhs)
{
	return lhs.flags != rhs.flags;
}