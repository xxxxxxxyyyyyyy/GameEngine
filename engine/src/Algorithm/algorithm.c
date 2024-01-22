#include "algorithm.h"

void swap(void* a, void* b, u32 type_size)
{
	void* p = (void*)malloc(type_size);
	KASSERT(p != NULL);

	memcpy(p, a, type_size);
	memcpy(a, b, type_size);
	memcpy(b, p, type_size);

	free(p);
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