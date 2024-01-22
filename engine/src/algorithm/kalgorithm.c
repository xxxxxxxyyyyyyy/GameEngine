#include "kalgorithm.h"

void swap(void* a, void* b, u32 type_size)
{
	void* p = (void*)kallocate(type_size, MEMORY_TAG_UNKNOWN);
	KASSERT(p != NULL);

	kcopy_memory(p, a, type_size);
	kcopy_memory(a, b, type_size);
	kcopy_memory(b, p, type_size);

	kfree(p, type_size, MEMORY_TAG_UNKNOWN);
}

void _quick_sort(i32 q[], i32 l, i32 r)
{
	if (l >= r) return;
	int rnd_idx = l + rand() % (r - l + 1);
	i32 x = q[l], i = l - 1, j = r + 1;
	swap(&q[l], &q[rnd_idx], sizeof(i32));
	while (i < j)
	{
		do i++; while (q[i] < x);
		do j--; while (q[j] > x);
		if (i < j) swap(&q[i], &q[j], sizeof(i32));
	}

	_quick_sort(q, l, j);
	_quick_sort(q, j + 1, r);	
}