#include <stdint.h>

class ThreeHeap
{
public:

	// Function call interface to get a new block of system memory
	typedef void* (*SystemAllocator)(int size);

	ThreeHeap(SystemAllocator system_allocator);
	~ThreeHeap();

	struct AllocationFlags
	{
		static inline constexpr uint8_t flag_from_malloc      = 0b0000'0001;
		static inline constexpr uint8_t flag_from_new         = 0b0000'0010;
		static inline constexpr uint8_t flag_new_scalar       = 0b0000'0100;
		static inline constexpr uint8_t flag_new_array        = 0b0000'1000;
		static inline constexpr uint8_t flag_malloc_aligned   = 0b0001'0000;
		static inline constexpr uint8_t flag_malloc_calloc    = 0b0010'0000;
		static inline constexpr uint8_t flag_malloc_valloc    = 0b0100'0000;

		static const AllocationFlags malloc;
		static const AllocationFlags new_scalar;
		static const AllocationFlags new_array;
		static const AllocationFlags malloc_calloc;
		static const AllocationFlags malloc_aligned;
		static const AllocationFlags malloc_aligned_calloc;
		static const AllocationFlags malloc_aligned_valloc;

		bool isMalloc() const;
		bool isNew() const;
		bool isScalar() const;
		bool isArray() const;
		bool isAligned() const;
		bool isCalloc() const;
		bool isValloc() const;

		uint8_t allocation_flags;
	};


	int getTotalNumberOfAllocations();
	int getCurrentNumberOfAllocations();
	int getCurrentNumberOfBytesAllocated();
	int getMaximumNumberOfAllocations();
	int getMaximumNumberOfBytesAllocated();

	struct CheckFlags
	{
		static inline constexpr uint16_t flag_validate_pre_guardband   = 0b0000'0000'0001;
		static inline constexpr uint16_t flag_validate_post_guardband  = 0b0000'0000'0010;
		static inline constexpr uint16_t flag_validate_free_pattern    = 0b0000'0000'0100;

		uint16_t flags;

		static const CheckFlags validate_pre_guardband;
		static const CheckFlags validate_post_guardband;
		static const CheckFlags validate_guardbands;
		static const CheckFlags validate_free_patterns;
		static const CheckFlags validate_everything;

		bool validatePreGuardBands() const;
		bool validatePostGuardBands() const;
		bool validateFreePatterns() const;
	};

	struct ConfigureFlags : public CheckFlags
	{
		static inline constexpr uint16_t flag_fill_pre_guardband    = 0b0000'0001'0000;
		static inline constexpr uint16_t flag_fill_post_guardband   = 0b0000'0010'0000;
		static inline constexpr uint16_t flag_fill_free_pattern     = 0b0000'0100'0000;
		static inline constexpr uint16_t flag_fill_allocate_pattern = 0b0000'1000'0000;

		bool fillPreGuardBands() const;
		bool fillPostGuardBands() const;
		bool fillFreePatterns() const;
		bool fillAllocatePatterns() const;

		static const ConfigureFlags fill_pre_guardband;
		static const ConfigureFlags fill_post_guardband;
		static const ConfigureFlags fill_guardbands;
		static const ConfigureFlags fill_free_patterns;
		static const ConfigureFlags fill_everything;
	};

	bool getConfigureFlag(ConfigureFlags flag) const;
	void setConfigureFlag(ConfigureFlags flag, bool enabled);

    // Interface for C & C++ depending upon the AllocationFlags that get passed in
	void* allocate(int size, int alignment, AllocationFlags flags);
	void free(void *memory, AllocationFlags flags);

    // Reallocate only supports malloc, no alignment, valloc, clearing allowed on these blocks
	void* reallocate(void *memory, int size);

	// Change the ownership of the memory to this caller
	void* own(void *memory);

    // Check functions for the heap and memory blocks
	void checkAllAllocations(CheckFlags flags) const;

    // Check this one allocation
	void checkAllocation(const void *memory, CheckFlags flags) const;

private:

	struct AllocationBlock;

	void checkAllocationBlock(AllocationBlock *block);

private:
	ConfigureFlags configure_flags;

private:
	ThreeHeap(const ThreeHeap &) = delete;
	ThreeHeap& operator=(const ThreeHeap &) = delete;
	ThreeHeap(ThreeHeap &&) = delete;
	ThreeHeap& operator=(ThreeHeap &&) = delete;
};

inline bool ThreeHeap::AllocationFlags::isMalloc() const { return (allocation_flags & flag_from_malloc) != 0; }
inline bool ThreeHeap::AllocationFlags::isNew() const { return (allocation_flags & flag_from_new) != 0; }
inline bool ThreeHeap::AllocationFlags::isScalar() const { return (allocation_flags & flag_new_scalar) != 0; }
inline bool ThreeHeap::AllocationFlags::isArray() const { return (allocation_flags & flag_new_array) != 0; }
inline bool ThreeHeap::AllocationFlags::isAligned() const { return (allocation_flags & flag_malloc_aligned) != 0; }
inline bool ThreeHeap::AllocationFlags::isCalloc() const { return (allocation_flags & flag_malloc_calloc) != 0; }
inline bool ThreeHeap::AllocationFlags::isValloc() const { return (allocation_flags & flag_malloc_valloc) != 0; }
