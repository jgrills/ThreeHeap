#include <ThreeHeap.h>

#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <new>

// ======================================================================

// CPP defines that control the code compilation for these features,
// this should be kept private in this one translation unit
#define USE_ASSERT                       1
#define USE_REPORT_OPERATION             1

#define USE_FILL_ALLOCATIONS             1
#define USE_FILL_FREES                   1

#define USE_VERIFY_GUARD_BANDS           1
#define USE_VERIFY_FREES                 1

// #define USE_ALLOCATION_STACK_DEPTH       0

#if USE_ASSERT

	#define assert(a) \
		do \
		{ \
			const bool result = static_cast<bool>(a); \
			if (!result) \
			{ \
				ErrorInfo info; \
				info.type = ErrorInfo::Type::Assert; \
				info.assert = #a; \
				info.file = __FILE__; \
				info.line = __LINE__; \
				external.error(info); \
			} \
		} while (false)

#else

	#define assert(a) static_cast<void>(0)

#endif

#if USE_REPORT_OPERATION
	#define REPORT_OPERATION(...) external.report_operation(__VA_ARGS__)
#else
	#define REPORT_OPERATION(...) static_cast<void>(0)
#endif

#define THREEHEAP_DEFINE_FLAGS1(flags_name, flags0) \
	const ThreeHeap::Flags ThreeHeap::flags_name{ThreeHeap::Flags::flags0}

#define THREEHEAP_DEFINE_FLAGS2(flags_name, flags0, flags1) \
	const ThreeHeap::Flags ThreeHeap::flags_name{ThreeHeap::Flags::flags0 | ThreeHeap::Flags::flags1}

#define THREEHEAP_DEFINE_FLAGS3(flags_name, flags0, flags1, flags2) \
	const ThreeHeap::Flags ThreeHeap::flags_name{ThreeHeap::Flags::flags0 | ThreeHeap::Flags::flags1 | ThreeHeap::Flags::flags2}

#define THREEHEAP_DEFINE_FLAGS4(flags_name, flags0, flags1, flags2, flags3) \
	const ThreeHeap::Flags ThreeHeap::flags_name{ThreeHeap::Flags::flags0 | ThreeHeap::Flags::flags1 | ThreeHeap::Flags::flags2 | ThreeHeap::Flags::flags3}

const ThreeHeap::Flags ThreeHeap::zero{0};
const ThreeHeap::Flags ThreeHeap::heap_fast = zero;
const ThreeHeap::Flags ThreeHeap::heap_debug = guard_bands | validate_guard_bands | fill_allocations | fill_guard_bands | fill_frees;

THREEHEAP_DEFINE_FLAGS2(new_scalar, flag_from_new, flag_new_scalar);
THREEHEAP_DEFINE_FLAGS2(new_array, flag_from_new, flag_new_array);
THREEHEAP_DEFINE_FLAGS1(malloc, flag_from_malloc);
THREEHEAP_DEFINE_FLAGS2(malloc_calloc, flag_from_malloc, flag_malloc_calloc);
THREEHEAP_DEFINE_FLAGS2(malloc_aligned, flag_from_malloc, flag_malloc_aligned);
THREEHEAP_DEFINE_FLAGS2(malloc_aligned_calloc, flag_from_malloc, flag_malloc_calloc);
THREEHEAP_DEFINE_FLAGS2(malloc_aligned_valloc, flag_from_malloc, flag_malloc_valloc);

THREEHEAP_DEFINE_FLAGS4(free_check, flag_from_new, flag_new_scalar, flag_new_array, flag_from_malloc);
THREEHEAP_DEFINE_FLAGS1(guard_bands, flag_guard_bands);

THREEHEAP_DEFINE_FLAGS1(validate_guard_bands,flag_validate_guard_bands);
THREEHEAP_DEFINE_FLAGS1(validate_free	, flag_validate_free);

THREEHEAP_DEFINE_FLAGS1(fill_guard_bands, flag_fill_guard_bands);
THREEHEAP_DEFINE_FLAGS1(fill_allocations, flag_fill_allocations);
THREEHEAP_DEFINE_FLAGS1(fill_frees, flag_fill_frees);

THREEHEAP_DEFINE_FLAGS1(report_allocation, flag_report_allocation);
THREEHEAP_DEFINE_FLAGS1(report_free, flag_report_free);

// ======================================================================

namespace
{
	constexpr static size_t Alignment = 64;
	constexpr static size_t AllocationPad = Alignment;
	constexpr static size_t HeaderSize = Alignment;
	constexpr static size_t SplitSize = HeaderSize * 2;
	constexpr static size_t GuardBandSize = 64;

	constexpr static char GuardBandFillChar = 0xab;
	constexpr static char AllocationFillChar = 0xcd;
	constexpr static char FreeFillChar = 0xef;

	// Alignment needs to be a power of 2
	static_assert((Alignment & (Alignment-1)) == 0);

	enum class BlockStatus : int16_t
	{
		Unknown,
		Free,
		Allocated,
		Sentinel
	};

	const char *GetAllocFlags(ThreeHeap::Flags flags)
	{
		flags = flags & ThreeHeap::free_check;
		if (flags == ThreeHeap::malloc) {
			return "malloc";
		}
		if (flags == ThreeHeap::new_scalar) {
			return "new";
		}
		if (flags == ThreeHeap::new_array) {
			return "new[]";
		}
		return "invalid";
	}

	const char *GetFreeFlags(ThreeHeap::Flags flags)
	{
		flags = flags & ThreeHeap::free_check;
		if (flags == ThreeHeap::malloc) {
			return "free";
		}
		if (flags == ThreeHeap::new_scalar) {
			return "delete";
		}
		if (flags == ThreeHeap::new_array) {
			return "delete[]";
		}
		return "invalid";
	}

	int Padding(int const size, int const alignment)
	{
		int const mask = alignment - 1;
		return (alignment - (size & mask)) & mask; 
	}
}

// ======================================================================

struct ThreeHeap::SystemAllocation
{
	SentinelBlock * start = nullptr;
	SentinelBlock * end = nullptr;
	SystemAllocation * next = nullptr;
	SentinelBlock * first_sentinel = nullptr;
	SentinelBlock * last_sentinel = nullptr;
};

struct ThreeHeap::Block
{
	static constexpr uint32_t Marker = ('3' << 24) | ('H' << 16) | ('P' << 8) | ('B' << 0);
	/* 4 */ uint32_t marker = Marker;
	/* 2 */ BlockStatus status = BlockStatus::Unknown;
	/* 2 */ int16_t fixed = 0;
	/* 8 */ int64_t size = 0;

	// doubly linked list in memory order for block coalescing
	/* 8 */ Block * previous = nullptr;
	/* 8 */ Block * next = nullptr;
	/* 4 */ // BlockFlags flags;
};

struct ThreeHeap::FreeBlock : public ThreeHeap::Block
{
	// ternary search free of free nodes
	/* 8 */ FreeBlock * less = nullptr;
	/* 8 */ FreeBlock * equal = nullptr;
	/* 8 */ FreeBlock * greater = nullptr;

	// parent for fast removal from the tree
	/* 8 */ FreeBlock * parent = nullptr;
};

struct ThreeHeap::AllocatedBlock : public ThreeHeap::Block
{
	/* 8 */ int64_t allocation_size = 0;
	/* 8 */ void * owner = nullptr;
	/* 4 */ int alignment = 0;
	/* 4 */ Flags flags = zero;
};

struct ThreeHeap::SentinelBlock : public ThreeHeap::Block
{
};

// ======================================================================

void ThreeHeap::DefaultInterface::tree_fixed_nodes(int64_t * & sizes, int & count)
{
	sizes = nullptr;
	count = 0;
}

void * ThreeHeap::DefaultInterface::system_allocator(int64_t & size)
{
	if (size == 0)
		size = 1;

	// Allocate 16mb blocks from the underlying system
	const int64_t system_allocation_size = 16 * 1024 * 1024;
	size = size + system_allocation_size - 1;
	size = size - (size % system_allocation_size);

	void * result = sbrk(size);
	return result;
}

void ThreeHeap::DefaultInterface::report_operation(void const * const memory, int64_t const size, int const alignment, void const * const owner, Flags const flags)
{
	if (flags.isAllocate())
	{
		printf("op=alloc result=%p size=%d align=%d owner=%p flags=%x\n", memory, (int)size, alignment, owner, flags.flags);
	}
	else
	{
		printf("op=free memory=%p size=%d owner=%p flags=%x\n", memory, (int)size, owner, flags.flags);
	}
}

void ThreeHeap::DefaultInterface::report_allocations(void const * const memory, int64_t const size, void const * const owner, Flags const flags)
{
	printf("allocation memory=%p size=%d owner=%p flags=%x\n", memory, (int)size, owner, flags.flags);
}

void ThreeHeap::DefaultInterface::error(ErrorInfo const & error)
{
	printf("ERROR!\n");
	switch (error.type)
	{
		case ErrorInfo::Type::Assert:
			printf("  %s:%d\n", error.file, error.line);
			printf("  assert failed: %s\n", error.assert);
			break;

		case ErrorInfo::Type::MismatchedFree:
			printf("  mismatched allocation/free %p %d\n", error.memory, (int)error.size);
			printf("  alloc was %s, but free is %s\n", GetAllocFlags(error.allocation_flags), GetFreeFlags(error.free_flags));
			break;

		case ErrorInfo::Type::GuardBandCorruption:
			printf("  guard band corruption %p %d\n", error.memory, (int)error.size);
			printf("  prefix byte map:\n    ");
			for (int i = 0; i < error.pre_size; ++i)
				putchar(error.pre_guard_band_corrupt[i] ? 'X' : '.');
			printf("<alloc>\n  postfix byte map:\n    ");
			for (int i = 0; i < error.post_size; ++i)
				putchar(error.post_guard_band_corrupt[i] ? 'X' : '.');
			printf("<alloc>\n");
			break;

		case ErrorInfo::Type::FreeCorruption:
			printf("  free memory corruption %p %d\n", error.memory, (int)error.size);
			printf("  at byte %d\n", error.free_corrupt_index);
			break;

		default:
			printf("  unknown error %d\n", static_cast<int>(error.type));
			break;
	};

	terminate();
}

void ThreeHeap::DefaultInterface::terminate()
{
	abort();
}

// ======================================================================

ThreeHeap::ThreeHeap(ExternalInterface& external_interface, Flags const flags)
:
	external(external_interface),
	heap_flags(flags),
	guard_band_size(flags.useGuardBands() ? GuardBandSize : 0)
{
	static_assert(sizeof(ThreeHeap::Block) <= HeaderSize);
	static_assert(sizeof(ThreeHeap::FreeBlock) <= HeaderSize);
	static_assert(sizeof(ThreeHeap::AllocatedBlock) <= HeaderSize);

	// fixed nodes will sometimes fail allocation, disable for now
	// external_interface.tree_fixed_nodes(fixed_node_sizes, fixed_nodes_count);

	if (fixed_nodes_count)
		allocateFromSystem(fixed_nodes_count * HeaderSize);
}

void ThreeHeap::allocateFromSystem(int64_t const minimum_size)
{
	// Ask the system for memory, it may resize the allocation
	// Add space in the allocation for the sentinel nodes
	int64_t allocation_size = HeaderSize + HeaderSize + minimum_size + HeaderSize;
	void * const memory = external.system_allocator(allocation_size);
	intptr_t m = reinterpret_cast<intptr_t>(memory);

	bool const fixed = !first_system_allocation && fixed_nodes_count;

	// Create the system allocation object
	SystemAllocation * const allocation = new(reinterpret_cast<void *>(m)) SystemAllocation();
	m += HeaderSize;

	// sentinels to remove special cases from the code
	SentinelBlock * const start_sentinel = new(reinterpret_cast<void *>(m)) SentinelBlock();
	m += HeaderSize;
	allocation->first_sentinel = start_sentinel;
	start_sentinel->status = BlockStatus::Sentinel;
	start_sentinel->size = HeaderSize;

	Block * previous = start_sentinel;
	if (fixed)
	{
		for (int i = 0; i < fixed_nodes_count; ++i)
		{
			FreeBlock * const fixed_block = new(reinterpret_cast<void *>(m)) FreeBlock();
			m += HeaderSize;
			fixed_block->status = BlockStatus::Free;
			previous->next = fixed_block;
			fixed_block->previous = previous;
			previous = fixed_block;
			fixed_block->fixed = 1;
			fixed_block->size = fixed_node_sizes[i];
		}
	}

	// Create the free block from the heap
	FreeBlock * const free_block = new(reinterpret_cast<void *>(m)) FreeBlock();
	int64_t const free_size = allocation_size - ((3 + fixed_nodes_count) * HeaderSize);

#if USE_FILL_FREES
	if (heap_flags.fillFrees())
		memset(reinterpret_cast<void*>(m + HeaderSize), FreeFillChar, free_size - HeaderSize);
#endif

	m += free_size;
	free_block->status = BlockStatus::Free;
	free_block->size = free_size;
	current_bytes_free += free_size;
	free_block->previous = previous;
	previous->next = free_block;

	SentinelBlock * const end_sentinel = new(reinterpret_cast<void *>(m)) SentinelBlock();
	allocation->last_sentinel = end_sentinel;
	end_sentinel->status = BlockStatus::Sentinel;
	end_sentinel->size = HeaderSize;
	end_sentinel->previous = free_block;
	free_block->next = end_sentinel;

	// Hook the system allocation in the system allocation list
	allocation->start = start_sentinel;
	allocation->end = end_sentinel;
	if (last_system_allocation)
		last_system_allocation->next = allocation;
	else
		first_system_allocation = allocation;
	last_system_allocation = allocation;

	// Add all the free blocks
	for (Block * add_free_block = start_sentinel->next; add_free_block->status == BlockStatus::Free; add_free_block = add_free_block->next)
		addToFreeList(reinterpret_cast<FreeBlock *>(add_free_block));
}

ThreeHeap::~ThreeHeap()
{
}

void ThreeHeap::addToFreeList(FreeBlock * const block)
{
	assert(block->status == BlockStatus::Free);
	assert(block->fixed == 0 || block->fixed == 1);
	assert(block->less == nullptr);
	assert(block->equal == nullptr);
	assert(block->greater == nullptr);
	assert(block->parent == nullptr);

	// Handle inserting into an empty tree
	FreeBlock * node = free_list;
	if (!node)
	{
		free_list = block;
		return;
	}

	int64_t const block_size = block->size;
	for (;;)
	{
		if (block_size < node->size)
		{
			// Traverse down the less branch
			FreeBlock * const less = node->less;
			if (less)
			{
				node = less;
				continue;
			}

			// Add the block as less child
			node->less = block;
			block->parent = node;
			return;
		}

		if (block_size > node->size)
		{
			// Traverse down the greater branch
			FreeBlock * const greater = node->greater;
			if (greater)
			{
				node = greater;
				continue;
			}

			// Add the block as greater child
			node->greater = block;
			block->parent = node;
			return;
		}

		// Add the block as first equal child
		FreeBlock * const equal = node->equal;
		node->equal = block;
		block->parent = node;
		if (equal)
		{
			block->equal = equal;
			equal->parent = block;
		}
		return;
	}
}

void ThreeHeap::removeFromFreeList(FreeBlock * const block)
{
	assert(block->status == BlockStatus::Free);

	// We can more cases with less code by keeping track of the parent pointer to update with a second level of indirection
	FreeBlock * const parent = block->parent;
	FreeBlock * * pparent = nullptr;
	if (parent)
	{
		block->parent = nullptr;

		// The equal subtree is a special case in the ternary search tree, it's a simple linked list
		if (parent->equal == block)
		{
			FreeBlock * block_equal = block->equal;
			if (block_equal)
				block_equal->parent = parent;
			parent->equal = block_equal;
			block->equal = nullptr;
			return;
		}

		// Figure out which pointer is the parent pointer and remember it
		if (parent->less == block)
			pparent = &parent->less;
		else if (parent->greater == block)
			pparent = &parent->greater;
		else
			assert(false);
	}
	else
	{
		// Removing the root, so the parent pointer is the root's pointer
		assert(block == free_list);
		pparent = &free_list;
	}

	// If this block has an equal sized child, replace the block with that to keep the tree structure
	FreeBlock * const block_equal = block->equal;
	FreeBlock * const block_less = block->less;
	FreeBlock * const block_greater = block->greater;
	if (block_equal)
	{
		block->equal = nullptr;
		block_equal->parent = parent;
		*pparent = block_equal;

		// Move the less child up to the replacement
		if (block_less)
		{
			block->less = nullptr;
			block_equal->less = block_less;
			block_less->parent = block_equal;
		}

		// Move the greater child up to the replacement
		if (block_greater)
		{
			block->greater = nullptr;
			block_equal->greater = block_greater;
			block_greater->parent = block_equal;
		}
		return;
	}

	// If the block has just one child, use that child as the replacement
	if (block_less)
	{
		block->less = nullptr;
		if (!block_greater)
		{
			// Block only has the less child, use it as the replacement
			block_less->parent = parent;
			*pparent = block_less;
			return;
		}
		else
		{
			block->greater = nullptr;
		}
	}
	else
	{
		if (block_greater)
		{
			// Block only has the greater child, use it as the replacement
			block->greater = nullptr;
			block_greater->parent = parent;
			*pparent = block_greater;
			return;
		}

		// Block is a leaf, nip it off the tree
		*pparent = nullptr;
		return;
	}


	// Block has smaller and greater but no equal. This is the worst case.
	// We are going to use the larger child as the replacement, and place
	// the smaller child into the larger child, attached to the smallest
	// element in the larger subtree.
	FreeBlock * smallest_larger_block = block_greater;
	for (;;)
	{
		FreeBlock * less = smallest_larger_block->less;
		if (less)
			smallest_larger_block = less;
		else
			break;
	}

	// connect the current block's less child to the smallest larger free block on its less side
	smallest_larger_block->less = block_less;
	block_less->parent = smallest_larger_block;

	// replace the free block with the greater child node (which now has the less child attached)
	block_greater->parent = parent;
	*pparent = block_greater;
}

ThreeHeap::FreeBlock * ThreeHeap::searchFreeList(const int64_t size)
{
	// iterative descent through the tree looking for the best fit
	FreeBlock * best_fit = nullptr;
	for (FreeBlock * block = free_list; block; )
	{
		if (size < block->size)
		{
			// Update best fitting block and descend
			best_fit = block;
			block = block->less;
			continue;
		}

		if (size > block->size)
		{
			// Can't fit so just descend
			block = block->greater;
			continue;
		}

		// return the right size block
		best_fit = block;
		break;
	}

	// Bail if we couldn't find a node to fit
	if (!best_fit)
		return nullptr;

	// If the best fitting node has an equally sized child, use that as it'll be faster to remove from the tree
	FreeBlock * const equal = best_fit->equal;
	if (equal)
		return equal;

	return best_fit;
}

void * ThreeHeap::own(void * const memory, void * const owner)
{
	intptr_t block_address = reinterpret_cast<intptr_t>(memory) - HeaderSize - guard_band_size;
	AllocatedBlock * allocated_block = reinterpret_cast<AllocatedBlock *>(block_address);
	assert(allocated_block->marker == Block::Marker);
	assert(allocated_block->previous->next == allocated_block);
	assert(allocated_block->next->previous == allocated_block);
	assert(allocated_block->status == BlockStatus::Allocated);
	allocated_block->owner = owner;
	return memory;
}

void * ThreeHeap::allocate(int64_t const size, int const alignment, Flags const oflags, void * const owner)
{
	Flags combined_flags = oflags | heap_flags;

	// calculate the total size of the allocation block
	const int64_t padding = Padding(size, Alignment);
	const int64_t additional_alignment = (alignment > Alignment) ? alignment : 0;
	assert(additional_alignment == 0);
	const int64_t block_size = HeaderSize + guard_band_size + size + padding + additional_alignment + guard_band_size; 

	// Search for best node to use for this allocation
	// @TODO handle `node` being nullptr here and allocate a new block from the system and retry
	FreeBlock * free_block = searchFreeList(block_size);
	if (!free_block)
		free_block = searchFreeList(block_size);
	if (!free_block)
	{
		allocateFromSystem(block_size);
		free_block = searchFreeList(block_size);
		assert(free_block);
	}

	removeFromFreeList(free_block);
	assert(free_block->less == nullptr);
	assert(free_block->equal == nullptr);
	assert(free_block->greater == nullptr);
	assert(free_block->parent == nullptr);

	AllocatedBlock * allocated_block = static_cast<AllocatedBlock *>(static_cast<Block *>(free_block));
	allocated_block->status = BlockStatus::Allocated;
	allocated_block->allocation_size = size;
	allocated_block->flags = combined_flags;
	allocated_block->owner = owner;
	free_block = nullptr;

	// Check if there's sufficient size left over to split this block
	const int64_t remainder_size = allocated_block->size - block_size;
	if (remainder_size >= SplitSize)
	{
		// Trim this block
		allocated_block->size = block_size;

		// Create the new free block
		intptr_t const remainder_address = reinterpret_cast<intptr_t>(allocated_block) + block_size;
		FreeBlock * const remainder_block = new(reinterpret_cast<void *>(remainder_address)) FreeBlock();

		// Link the new free block into the doubly linked list
		Block * allocated_block_next = allocated_block->next;
		allocated_block->next = remainder_block;
		allocated_block_next->previous = remainder_block;
		remainder_block->previous = allocated_block;
		remainder_block->next = allocated_block_next;

		// Update the remainder block stats and add it to the free list
		remainder_block->size = remainder_size;
		remainder_block->status = BlockStatus::Unknown;

		remainder_block->status = BlockStatus::Free;
		addToFreeList(remainder_block);
	}

	intptr_t const allocated_address = reinterpret_cast<intptr_t>(allocated_block);

	if (guard_band_size)
	{
		memset(reinterpret_cast<void*>(allocated_address + HeaderSize), GuardBandFillChar, guard_band_size);
		int const post_size = Padding(size, guard_band_size) + guard_band_size;
		memset(reinterpret_cast<void*>(allocated_address + HeaderSize + guard_band_size + size), GuardBandFillChar, post_size);
	}

	// Update metrics
	++total_number_of_allocations;
	++current_number_of_allocations;
	if (current_number_of_allocations > maximum_number_of_allocations)
		maximum_number_of_allocations = current_number_of_allocations;

	total_bytes_allocated += size;
	current_bytes_allocated += size;
	if (current_bytes_allocated > maximum_bytes_allocated)
		maximum_bytes_allocated = current_bytes_allocated;

	current_bytes_free -= block_size;
	total_bytes_used += block_size;
	current_bytes_used += block_size;
	if (current_bytes_used > maximum_bytes_used)
		maximum_bytes_used = current_bytes_used;

	// Return a pointer to the client memory
	void * const result = reinterpret_cast<void *>(allocated_address + HeaderSize + guard_band_size);

#if USE_FILL_ALLOCATIONS
	if (combined_flags.fillAllocations())
		memset(result, AllocationFillChar, size);
#endif

	REPORT_OPERATION(result, size, alignment, owner, combined_flags | report_allocation);
	return result;
}

void ThreeHeap::verifyFree(void const * const memory, int const size) const
{
	for (int i = 0; i < size; ++i)
		if (reinterpret_cast<const char*>(memory)[i] != FreeFillChar)
		{
			ErrorInfo info;
			info.type = ErrorInfo::Type::FreeCorruption;
			info.memory = memory;
			info.size = size;
			info.free_corrupt_index = i;
			external.error(info);
			return;
		}
}

bool ThreeHeap::verifyGuardBand(void const * const memory, int const size, bool * corrupt)
{
	bool result = true;
	for (int i = 0; i < size; ++i)
		if (reinterpret_cast<const char*>(memory)[i] != GuardBandFillChar)
		{
			corrupt[i] = true;
			result = false;
		}

	return result;
}

void ThreeHeap::verifyGuardBands(AllocatedBlock const * const block) const
{
	ErrorInfo info;
	intptr_t const block_address = reinterpret_cast<intptr_t>(block);
	int64_t const allocated_size = block->allocation_size;
	int const post_size = Padding(allocated_size, guard_band_size) + guard_band_size;
	bool const pre = verifyGuardBand(reinterpret_cast<void*>(block_address + HeaderSize), guard_band_size, info.pre_guard_band_corrupt);
	bool const post = verifyGuardBand(reinterpret_cast<void*>(block_address + HeaderSize + guard_band_size + allocated_size), post_size, info.post_guard_band_corrupt);
	if (!(pre && post))
	{
		info.type = ErrorInfo::Type::GuardBandCorruption;
		info.memory = reinterpret_cast<void*>(block_address + HeaderSize + guard_band_size);
		info.size = allocated_size;
		info.pre_size = guard_band_size;
		info.post_size = guard_band_size + Padding(allocated_size, Alignment);
		external.error(info);
		return;
	}
}

void ThreeHeap::free(void * const memory, Flags const flags)
{	
	if (!memory)
		return;

	intptr_t const block_address = reinterpret_cast<intptr_t>(memory) - HeaderSize - guard_band_size;
	AllocatedBlock * allocated_block = reinterpret_cast<AllocatedBlock *>(block_address);
	assert(allocated_block->marker == Block::Marker);
	assert(allocated_block->previous->next == allocated_block);
	assert(allocated_block->next->previous == allocated_block);
	assert(allocated_block->status == BlockStatus::Allocated);

	int64_t const allocated_block_size = allocated_block->size;
	int64_t const allocated_size = allocated_block->allocation_size;
	// Make sure the allocation flags match the delete flags
	Flags const allocated_flags = allocated_block->flags;
	Flags const free_check_allocation = allocated_flags & free_check;
	Flags const free_check_free = flags & free_check;
	if (free_check_allocation != free_check_free)
	{
		ErrorInfo info;
		info.type = ErrorInfo::Type::MismatchedFree;
		info.memory = memory;
		info.size = allocated_size;
		info.allocation_flags = allocated_flags;
		info.free_flags = flags;
		external.error(info);
		return;
	}

	// Update metrics
	++total_number_of_frees;
	--current_number_of_allocations;
	current_bytes_allocated -= allocated_size;
	current_bytes_used -= allocated_block_size;
	current_bytes_free += allocated_block_size;

	// Report the operation
	REPORT_OPERATION(memory, allocated_size, 0, allocated_block->owner, allocated_block->flags | report_free);

#if USE_VERIFY_GUARD_BANDS
	if (guard_band_size)
		verifyGuardBands(allocated_block);
#endif

#if USE_FILL_FREES
	Flags const combined_flags = flags | heap_flags;
	if (combined_flags.fillFrees())
	{
		intptr_t user = reinterpret_cast<intptr_t>(allocated_block) + HeaderSize;
		memset(reinterpret_cast<void *>(user), FreeFillChar, allocated_block_size - HeaderSize);
	}
#endif

	// Convert this previously allocated block to a free block
	FreeBlock * free_block = static_cast<FreeBlock *>(static_cast<Block *>(allocated_block));
	allocated_block->allocation_size = 0;
	allocated_block->owner = nullptr;
	allocated_block = nullptr;
	free_block->fixed = 0;
	free_block->status = BlockStatus::Unknown;
	free_block->less = nullptr;
	free_block->equal = nullptr;
	free_block->greater = nullptr;
	free_block->parent = nullptr;

	// Coalesce with the next block if it is free
	Block * next = free_block->next;
	if (next->status == BlockStatus::Free)
	{
		// Remove the next free block from the free list
		FreeBlock * const free_next = static_cast<FreeBlock *>(next);
		
		removeFromFreeList(free_next);
		assert(free_next->less == nullptr);
		assert(free_next->equal == nullptr);
		assert(free_next->greater == nullptr);
		assert(free_next->parent == nullptr);

		// Collapse it into the current block
		Block * const next_next = next->next;
		free_block->size += next->size;
		free_block->next = next_next;
		next_next->previous = free_block;

#if USE_FILL_FREES
		if (combined_flags.fillFrees())
			memset(reinterpret_cast<void *>(next), FreeFillChar, HeaderSize);
		else
#endif
		{
			// Destroy the node we are collapsing into this node
			next->marker = 0;
			next->size = 0;
			next->fixed = 0;
			next->previous = nullptr;
			next->next = nullptr;
		}

		// Our next is the next
		next = next_next;
	}

	// Coalesce with the previous block if it was free
	Block * const previous = free_block->previous;
	if (previous->status == BlockStatus::Free)
	{
		const int64_t additional_size = free_block->size;

		FreeBlock * const free_previous = static_cast<FreeBlock *>(previous);
		removeFromFreeList(free_previous);
		assert(free_previous->less == nullptr);
		assert(free_previous->equal == nullptr);
		assert(free_previous->greater == nullptr);
		assert(free_previous->parent == nullptr);

		// Collapse the current free block into the previous
		free_previous->status = BlockStatus::Unknown;
		free_previous->size += additional_size;
		free_previous->next = next;
		next->previous = free_previous;

#if USE_FILL_FREES
		if (combined_flags.fillFrees())
			memset(reinterpret_cast<void *>(free_block), FreeFillChar, HeaderSize);
		else
#endif
		{
			// Destroy the current free block that was collapsed into the previous
			free_block->marker = 0;
			free_block->size = 0;
			free_block->fixed = 0;
			free_block->previous = nullptr;
			free_block->next = nullptr;
		}

		// The free block is now the previous free block
		free_block = free_previous;
	}

	free_block->status = BlockStatus::Free;
	addToFreeList(free_block);
}

int64_t ThreeHeap::getAllocationSize(void * const memory) const
{
	if (!memory)
		return 0;

	Block const * const block = reinterpret_cast<Block const *>(reinterpret_cast<intptr_t>(memory) - HeaderSize);
	assert(block->marker == Block::Marker);
	assert(block->status == BlockStatus::Allocated);

	AllocatedBlock const * const allocated_block = static_cast<AllocatedBlock const *>(block);
	return allocated_block->allocation_size;
}

void ThreeHeap::verify(FreeBlock const * const parent, FreeBlock const * const node, int & number_of_free_blocks) const
{
	++number_of_free_blocks;

	assert(node->marker == Block::Marker);
	assert(node->status == BlockStatus::Free);
	assert(node->size >= (HeaderSize + HeaderSize));
	assert((node->size % Alignment) == 0);

	if (FreeBlock const * less = node->less; less)
	{
		assert(less->size < node->size);
		assert(less->parent == node);
		verify(node, less, number_of_free_blocks);
	}
	if (FreeBlock const * greater = node->greater; greater)
	{
		assert(greater->size > node->size);
		assert(greater->parent == node);
		verify(node, greater, number_of_free_blocks);
	}
	if (FreeBlock const * equal = node->equal; equal)
	{
		assert(equal->parent == node);
		assert(equal->size == node->size);
		verify(node, equal, number_of_free_blocks);
	}
	assert(node->parent == parent);
}

void ThreeHeap::verify(Flags flags) const
{
#if USE_ASSERT
	// Limit the verification to features supported in the heap
	bool const check_free = flags.validateFree() && heap_flags.fillFrees();

	// Check all the system allocation doubly linked list
	int free_blocks_linear = 0;
	for (SystemAllocation * allocation = first_system_allocation; allocation; allocation = allocation->next)
	{
		// Linearly scan all the blocks in the system allocation
		for (Block const * block = allocation->start->next; block != allocation->end; block = block->next)
		{
			Block const * const previous = block->previous;
			Block const * const next = block->next;

			if (block->status == BlockStatus::Free)
				++free_blocks_linear;

			assert(block->marker == Block::Marker);
			assert(block->status == BlockStatus::Unknown || block->status == BlockStatus::Sentinel || block->status == BlockStatus::Free || block->status == BlockStatus::Allocated);
			assert((block->size & (AllocationPad - 1)) == 0);
			assert(reinterpret_cast<intptr_t>(block->next) == (reinterpret_cast<intptr_t>(block) + (block->fixed ? HeaderSize : block->size)));
			assert(next->previous == block);
			assert(previous->next == block);

#if USE_VERIFY_GUARD_BANDS
			if (guard_band_size && flags.validateGuardBands() && block->status == BlockStatus::Allocated)
				verifyGuardBands(static_cast<AllocatedBlock const *>(block));
#endif

#if USE_FILL_FREES && USE_VERIFY_FREES
			if (check_free && block->status == BlockStatus::Free && !block->fixed)
			{
				intptr_t const memory = reinterpret_cast<intptr_t>(block) + HeaderSize;
				verifyFree(reinterpret_cast<void *>(memory), block->size - HeaderSize);
			}
#endif
		}
	}

	// Verify all the free nodes can be found in the free list
	if (free_list)
	{
		int free_blocks_tree = 0;
		verify(nullptr, free_list, free_blocks_tree);
		assert(free_blocks_linear == free_blocks_tree);
	}
#endif
}

void ThreeHeap::report_allocations() const
{
	// Check all the system allocation doubly linked list
	for (SystemAllocation const * allocation = first_system_allocation; allocation; allocation = allocation->next)
	{
		// Linearly scan all the blocks in the system allocation
		for (Block const * block = allocation->start->next; block != allocation->end; block = block->next)
			if (block->status == BlockStatus::Allocated)
			{
				AllocatedBlock const * const allocated_block = static_cast<AllocatedBlock const *>(block);
				void const * const mem = reinterpret_cast<void *>(reinterpret_cast<intptr_t>(allocated_block) + HeaderSize);
				external.report_allocations(mem, allocated_block->allocation_size, allocated_block->owner, allocated_block->flags);

#if 0
				// Look up the source location and report where the leak came from
				// char ** backtrace_symbols (void *const *buffer, int size)
				void * owner = const_cast<void*>(allocated_block->owner);
				char * * result = nullptr;
				result = backtrace_symbols(&owner, 1);
				printf("  address %p %s\n", mem, result[0]);
#endif
			}
	}
}

void * ThreeHeap::reallocate(void * const memory, int64_t const size)
{
	// @TODO size the block in situ if possible
	int64_t const previous = getAllocationSize(memory);
	int64_t const least = (previous < size) ? previous : size;
	void * const result = allocate(size, 0, malloc);
	memcpy(result, memory, least);
	return result;
}
