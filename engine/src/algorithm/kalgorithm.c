#include "kalgorithm.h"

void swap(void* a, void* b, u32 type_size)
{
	void* p = (void*)kallocate(type_size, MEMORY_TAG_ALGORITHM);
	KASSERT(p != NULL);

	kcopy_memory(p, a, type_size);
	kcopy_memory(a, b, type_size);
	kcopy_memory(b, p, type_size);

	kfree(p, type_size, MEMORY_TAG_ALGORITHM);
}

// nlogn
void _quick_sort(i32 q[], i32 l, i32 r)
{
	if (l >= r) return;
	u32 rnd_idx = l + rand() % (r - l + 1);
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

// nlogn
void _merge_sort(i32 q[], i32 tmp[], i32 l, i32 r)
{
	if (l >= r) return;
    u32 mid = (l + r) >> 1;

    _merge_sort(q, tmp, l, mid);
    _merge_sort(q, tmp, mid + 1, r);

    int k = 0, i = l, j = mid + 1;
    while (i <= mid && j <= r)
    {
        if (q[i] <= q[j]) tmp[k ++] = q[i ++];
        else tmp[k ++] = q[j ++];
    }
    while (i <= mid) tmp[k ++] = q[i ++];
    while (j <= r) tmp[k ++] = q[j ++];

    for (int i = l, j = 0; i <= r; i++, j++) q[i] = tmp[j];
}