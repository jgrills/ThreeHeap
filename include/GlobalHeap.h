#pragma once

#include <ThreeHeap.h>

class HeapInterface : public ThreeHeap::DefaultInterface
{
public:
	void tree_fixed_nodes(int64_t * & sizes, int & count ) override;
void report_operation(const void * memory, int64_t size, int alignment, const void * owner, ThreeHeap::Flags flags) override;
	void terminate() override;

	bool permissive = false;
	bool report_operations = false;
};

extern HeapInterface g_heapInterface;
extern ThreeHeap g_heap;

void * HeapOwn(void * memory);
