#include <ThreeHeap.h>

#include <stdlib.h>
#include <stdio.h>
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

#define assert(a) assertHandler(__FILE__, __LINE__, #a, a)

void * ThreeHeap::DefaultInterface::page_allocator(int64_t size) {
	return ::malloc(size);
}

void ThreeHeap::DefaultInterface::report(const void * ptr, int64_t size, int alignment, Flags flags) {
}

void ThreeHeap::DefaultInterface::heap_error(HeapError & heap_error) {
	printf("abort\n");
	abort();
}

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
	/* 2 */ BlockStatus status = BlockStatus::Unknown;
	/* 2 */ mutable int16_t verification_stamp = 0;
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

struct ThreeHeap::SentinelBlock : public ThreeHeap::Block
{
};

void ThreeHeap::assertHandler(const char * const file, int line, const char * const text, const bool everything_is_okay) const {
	static_assert(sizeof(Block) <= HeaderSize);
	static_assert(sizeof(FreeBlock) <= HeaderSize);
	static_assert(sizeof(AllocatedBlock) <= HeaderSize);
	if (!everything_is_okay) {
		printf("%s:%d\nassert failed: %s\n", file, line, text);
		HeapError error;
		external_interface.heap_error(error);
	}
}

void ThreeHeap::addToFreeList(FreeBlock * const block)
{
	assert(block->less == nullptr);
	assert(block->equal == nullptr);
	assert(block->greater == nullptr);
	assert(block->parent == nullptr);

	// Handle inserting into an empty tree
	FreeBlock * node = free_list;
	if (!node) {
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

void * ThreeHeap::allocate(const int64_t size, const int alignment /* AllocationFlags flags */)
{
	// (void)flags;

	// calculate the total size of the allocation block
	const int64_t padding = (AllocationPad - (size & (AllocationPad - 1))) & (AllocationPad - 1);
	const int64_t additional_alignment = (alignment > Alignment) ? alignment : 0;
	const int64_t block_size = HeaderSize + size + padding + additional_alignment;

	// Search for best node to use for this allocation
	// @TODO handle `node` being nullptr here and allocate a new block from the system and retry
	FreeBlock * free_block = searchFreeList(block_size);
	if (!free_block)
	{
		allocateFromSystem();
		free_block = searchFreeList(block_size);
		assert(free_block);
	}

	removeFromFreeList(free_block);
	assert(free_block->less == nullptr);
	assert(free_block->equal == nullptr);
	assert(free_block->greater == nullptr);
	assert(free_block->parent == nullptr);
	
	AllocatedBlock* allocated_block = static_cast<AllocatedBlock *>(static_cast<Block *>(free_block));
	allocated_block->status = BlockStatus::Allocated;
	allocated_block->allocation_size = size;
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
	free_block->verification_stamp = 0;
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
		Block* const next_next = next->next;
		free_block->size += next->size;
		free_block->next = next_next;
		next_next->previous = free_block;

		// Destroy the node we are collapsing into this node
		next->marker = 0;
		next->size = 0;
		next->verification_stamp = 0;
		next->previous = nullptr;
		next->next = nullptr;

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

		// Destroy the current free block that was collapsed into the previous
		free_block->marker = 0;
		free_block->size = 0;
		free_block->verification_stamp = 0;
		free_block->previous = nullptr;
		free_block->next = nullptr;

		// The free block is now the previous free block
		free_block = free_previous;
	}


	free_block->status = BlockStatus::Free;
	addToFreeList(free_block);
}

int64_t ThreeHeap::getAllocationSize(void * memory) const
{
	Block const * const block = reinterpret_cast<Block *>(reinterpret_cast<intptr_t>(memory) - HeaderSize);
	assert(block->marker == Block::Marker);
	assert(block->status == BlockStatus::Allocated);

	AllocatedBlock const * const allocated_block = static_cast<AllocatedBlock const *>(block);
	return allocated_block->allocation_size;
}

ThreeHeap::ThreeHeap(ExternalInterface& ei, Flags flags)
: external_interface(ei), configure_flags(flags)
{
}

void ThreeHeap::allocateFromSystem()
{
	int64_t constexpr memory_size = 64 * 4 * 1024;
	void * memory = ::malloc(memory_size);
	intptr_t const m = reinterpret_cast<intptr_t>(memory);

	SystemAllocation * const allocation = new(reinterpret_cast<void*>(m)) SystemAllocation();

	SentinelBlock* const start_sentinel = new(reinterpret_cast<void*>(m + HeaderSize)) SentinelBlock();
	start_sentinel->status = BlockStatus::Sentinel;
	start_sentinel->size = HeaderSize;

	SentinelBlock* const end_sentinel = new(reinterpret_cast<void*>(m + memory_size - HeaderSize)) SentinelBlock();
	end_sentinel->status = BlockStatus::Sentinel;
	end_sentinel->size = HeaderSize;

	FreeBlock * const free_block = new(reinterpret_cast<void*>(m + HeaderSize + HeaderSize)) FreeBlock();
	free_block->status = BlockStatus::Free;
	free_block->size = memory_size - (3 * HeaderSize);

	// Set up the initial state of the doubly linked memory blocks
	// First the start sentinal, then free block, then the end sentinal
	start_sentinel->next = free_block;
	free_block->next = end_sentinel;
	end_sentinel->previous = free_block;
	free_block->previous = start_sentinel;

	allocation->start = start_sentinel;
	allocation->end = end_sentinel;
	if (last_system_allocation)
		last_system_allocation->next = allocation;
	else
		first_system_allocation = allocation;
	last_system_allocation = allocation;

	addToFreeList(free_block);
}

void ThreeHeap::verify(FreeBlock const * const parent, FreeBlock const * const node) const
{
	++free_tree;

	assert(node->marker == Block::Marker);
	assert(node->status == BlockStatus::Free);
	assert(node->size >= (HeaderSize + HeaderSize));
	assert((node->size % Alignment) == 0);

	if (FreeBlock const * less = node->less; less)
	{
		assert(less->size < node->size);
		assert(less->parent == node);
		verify(node, less);
	}
	if (FreeBlock const * greater = node->greater; greater)
	{
		assert(greater->size > node->size);
		assert(greater->parent == node);
		verify(node, greater);
	}
	if (FreeBlock const * equal = node->equal; equal)
	{
		assert(equal->parent == node);
		assert(equal->size == node->size);
		verify(node, equal);
	}
	assert(node->parent == parent);

	node->verification_stamp = verification_stamp;
}

void ThreeHeap::verify() const
{
	// Check all the system allocation doubly linked list
	for (SystemAllocation * allocation = first_system_allocation; allocation; allocation = allocation->next)
	{
		// Linearly scan all the blocks in the system allocation
		for (Block * block = allocation->start->next; block != allocation->end; block = block->next)
		{
			Block * previous = block->previous;
			Block * next = block->next;

			assert(block->marker == Block::Marker);
			assert(block->status == BlockStatus::Unknown || block->status == BlockStatus::Sentinel || block->status == BlockStatus::Free || block->status == BlockStatus::Allocated);
			assert((block->size & (AllocationPad - 1)) == 0);
			assert(reinterpret_cast<intptr_t>(block->next) == (reinterpret_cast<intptr_t>(block) + block->size));
			assert(next->previous == block);
			assert(previous->next == block);
		}
	}

	// Verify all the free nodes can be found in the free list
	if (free_list)
	{
		++verification_stamp;
		free_tree = 0;
		verify(nullptr, free_list);
		int free_linear = 0;
		for (SystemAllocation * allocation = first_system_allocation; allocation; allocation = allocation->next)
			for (Block * block = allocation->start->next; block != allocation->end; block = block->next)
				if (block->status == BlockStatus::Free)
				{
					FreeBlock * const free_block = static_cast<FreeBlock *>(block);
					assert(free_block->verification_stamp == verification_stamp);
					++free_linear;
				}

		assert(free_tree == free_linear);
	}
}
