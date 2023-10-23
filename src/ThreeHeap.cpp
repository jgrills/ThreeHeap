#include <ThreeHeap.h>

#include <stdlib.h>
#include <unistd.h>
#include <new>

// CPP defines that control the code compilation for these features,
// this should be kept private in this one translation unit
#define USE_ALLOCATION_STACK_DEPTH       0
#define USE_PRE_GUARD_BAND_VALIDATION    0
#define USE_POST_GUARD_BAND_VALIDATION   0
#define USE_INITIALIZE_PATTERN_FILL      0
#define USE_FREE_PATTERN_FILL            0
#define USE_FREE_PATTERN_VALIDATION      0

#if 0
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

#endif

ThreeHeap::~ThreeHeap()
{
}

// Reallocate only supports malloc, no alignment, valloc, clearing allowed on these blocks
void * ThreeHeap::reallocate(void * memory, int64_t size)
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

struct ThreeHeap::Block
{
	static constexpr uint32_t Marker = ('3' << 24) | ('H' << 16) | ('P' << 8) | ('B' << 0);
	/* 4 */ uint32_t marker = Marker;
	/* 4 */ BlockStatus status = BlockStatus::Unknown;
	/* 8 */ int64_t size = 0;

	// doubly linked list in memory order for block coalescing
	/* 8 */ Block* previous = nullptr;
	/* 8 */ Block* next = nullptr;
	/* 4 */ // BlockFlags flags;
};

struct ThreeHeap::FreeBlock : public ThreeHeap::Block
{
	// ternary search free of free nodes
	/* 8 */ FreeBlock* less = nullptr;
	/* 8 */ FreeBlock* equal = nullptr;
	/* 8 */ FreeBlock* greater = nullptr;

	// parent for fast removal from the tree
	/* 8 */ FreeBlock* parent = nullptr;
};

struct ThreeHeap::AllocatedBlock : public ThreeHeap::Block
{
	/* 8 */ int64_t allocation_size = 0;
};

void ThreeHeap::assert(const bool everything_is_okay) {
	static_assert(sizeof(Block) <= HeaderSize);
	static_assert(sizeof(FreeBlock) <= HeaderSize);
	static_assert(sizeof(AllocatedBlock) <= HeaderSize);
	if (!everything_is_okay) abort();
}

void ThreeHeap::addToFreeList(FreeBlock * const block)
{
	assert(block->less == nullptr);
	assert(block->equal == nullptr);
	assert(block->greater == nullptr);
	assert(block->parent == nullptr);

	int64_t const block_size = block->size;
	for (FreeBlock * node = free_list; node; )
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
		FreeBlock * const previous = node->equal;
		if (previous)
		{
			previous->parent = block;
			block->equal = previous;
		}
		node->equal = block;
		block->parent = node;
		return;
	}

	// Handle putting the first element on the list
	free_list = block;
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
		*pparent = block_equal;

		// Move the less child up to the replacement
		if (block_less) {
			block->less = nullptr;
			block_equal->less = block_less;
			block_less->parent = block_equal;
		}

		// Move the greater child up to the replacement
		if (block_greater) {
			block->greater = nullptr;
			block_equal->greater = block_greater;
			block_greater->parent = block_equal;
		}

		return;
	}


	// If the block has just one child, use that child as the replacement
	if (block_less)
	{
		if (!block_greater)
		{
			// Block only has the less child, use it as the replacement
			*pparent = block->less;
			block->parent = nullptr;
			block->less = nullptr;
			return;
		}
	}
	else
	{
		if (block_greater)
		{
			// Block only has the greater child, use it as the replacement
			*pparent = block_greater;
			block->parent = nullptr;
			block->greater = nullptr;
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
	do
	{
		FreeBlock * less = smallest_larger_block->less;
		if (less)
		{
			smallest_larger_block = less;
			continue;
		}
	} while (false);

	// connect the current block's less child to the smallest larger free block on its less side
	smallest_larger_block->less = block_less;
	block_less->parent = smallest_larger_block;

	*pparent = block_greater;
	block_greater->parent = parent;
}

ThreeHeap::FreeBlock * ThreeHeap::searchFreeList(const int64_t size) {
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

	// If the best fitting node has an equally sized child, use that as it'll be faster to remove from the tree
	FreeBlock * const equal = best_fit->equal;
	if (equal)
		best_fit = equal;

	return best_fit;
}

void * ThreeHeap::allocate(int64_t size, int alignment /* AllocationFlags flags */)
{
	// (void)flags;

	// calculate the total size of the allocation block
	int64_t block_size = 0;
	block_size += HeaderSize;
	block_size += (size + (AllocationPad - 1)) & (~AllocationPad);

	// Only align if higher than our default alignment
	if (alignment > Alignment)
	{
		// @TODO implement alignment
		assert(false);
		block_size += alignment;
	}

	// Search for best node to use for this allocation
	// @TODO handle `node` being nullptr here and allocate a new block from the system and retry
	FreeBlock * free_block = searchFreeList(block_size);

	removeFromFreeList(free_block);

	// Check if there's sufficient size left over to split this block
	const int64_t remainder_size = free_block->size - block_size;
	if (remainder_size >= SplitSize)
	{
		// Trim this block
		free_block->size = block_size;

		// Create the new free block
		intptr_t const remainder_address = reinterpret_cast<intptr_t>(free_block) + block_size;
		FreeBlock * const remainder_block = new(reinterpret_cast<void *>(remainder_address)) FreeBlock();

		// Link the new free block into the doubly linked list
		Block * free_block_next = free_block->next;
		free_block->next = remainder_block;
		free_block_next->previous = remainder_block;
		remainder_block->previous = free_block;
		remainder_block->next = free_block_next;

		// Update the remainder block stats and add it to the free list
		remainder_block->size = remainder_size;
		remainder_block->status = BlockStatus::Free;
		addToFreeList(remainder_block);
	}

	// Convert to an allocated block
	free_block->less = nullptr;
	free_block->equal = nullptr;
	free_block->greater = nullptr;
	free_block->parent = nullptr;
	AllocatedBlock * allocated_block = static_cast<AllocatedBlock *>(static_cast<Block *>(free_block));
	free_block = nullptr;
	allocated_block->status = BlockStatus::Allocated;
	allocated_block->allocation_size = size;

	// Return a pointer to the client memory
	void * const result = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(allocated_block) + HeaderSize);
	return result;
}

void ThreeHeap::free(void * memory /*, AllocationFlags flags*/)
{
	//(void)flags;

	Block * block = reinterpret_cast<Block *>(reinterpret_cast<intptr_t>(memory) - HeaderSize);
	assert(block->marker == Block::Marker);
	assert(block->previous->next == block);
	assert(block->next->previous == block);
	assert(block->status == BlockStatus::Allocated);

	// Convert this previously allocated block to a free block
	FreeBlock * free_block = static_cast<FreeBlock *>(block);
	free_block->status = BlockStatus::Free;
	free_block->less = nullptr;
	free_block->equal = nullptr;
	free_block->greater = nullptr;
	free_block->parent = nullptr;

	// Coalesce with the next block if it is free
	Block * next = free_block->next;
	if (next->status == BlockStatus::Free)
	{
		// Remove the next free block from the free list
		removeFromFreeList(static_cast<FreeBlock *>(next));

		// Collapse it into the current block
		Block* next_next = next->next;
		free_block->size += next->size;
		free_block->next = next_next;
		next_next->previous = free_block;
		next = next_next;
	}

	// Coalesce with the previous block if it was free
	Block * previous = free_block->previous;
	if (previous->status == BlockStatus::Free)
	{
		const int64_t additional_size = free_block->size;
		free_block = static_cast<FreeBlock *>(previous);
		removeFromFreeList(free_block);

		previous->size += additional_size;
		previous->next = next;
		next->previous = previous;
	}

	addToFreeList(free_block);
}

ThreeHeap::ThreeHeap(ExternalInterface& ei, Flags flags)
: external_interface(ei), configure_flags(flags)
{
	int64_t const memory_size = 64 * 1024 * 1024;
	void * memory = ::malloc(memory_size);
	intptr_t const m = reinterpret_cast<intptr_t>(memory);

	Block* start_sentinel = new(reinterpret_cast<void*>(m)) Block();
	start_sentinel->status = BlockStatus::Sentinel;
	start_sentinel->size = HeaderSize;

	Block* end_sentinel = new(reinterpret_cast<void*>(m + memory_size - HeaderSize)) Block();
	end_sentinel->status = BlockStatus::Sentinel;
	end_sentinel->size = HeaderSize;

	FreeBlock * free_block = new(reinterpret_cast<void*>(m + HeaderSize)) FreeBlock();
	free_block->status = BlockStatus::Free;
	free_block->size = memory_size - (2 * HeaderSize);

	// Set up the initial state of the doubly linked memory blocks
	// First the start sentinal, then free block, then the end sentinal
	start_sentinel->next = free_block;
	free_block->next = end_sentinel;
	end_sentinel->previous = free_block;
	free_block->previous = start_sentinel;

	addToFreeList(free_block);
}

void * ThreeHeap::DefaultInterface::page_allocator(int64_t size) {
	return ::malloc(size);
}

void ThreeHeap::DefaultInterface::report(const void * ptr, int64_t size, int alignment, Flags flags) {
}

void ThreeHeap::DefaultInterface::heap_error(HeapError & heap_error) {
	abort();
}
