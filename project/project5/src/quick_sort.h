/*
 * File: quick_sort.h
 * Description: In-place descending QuickSort with three engineering safeguards
 *              for ordering coupon discounts C[0..N-1] in non-increasing order:
 *                1) Small-subarray cutoff (`THRESHOLD = 16`) to selection sort.
 *                2) Median-of-three pivot selection on (left, mid, right).
 *                3) Three-way partitioning (`> pivot`, `== pivot`, `< pivot`)
 *                   so heavy duplicate keys finish in linear O(N) time, paired
 *                   with smaller-branch recursion to cap stack depth at O(log N).
 */

#ifndef QUICK_SORT_H
#define QUICK_SORT_H

#include <stdio.h>

/* Subarrays of length <= THRESHOLD switch from recursion to selectionSort. */
#define THRESHOLD 16

/*
 * @brief Swap two 32-bit integers in place.
 */
static inline void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

/*
 * @brief Sort subarray `a[left..right]` in non-increasing (descending) order
 *        using in-place selection sort; invoked only when subarray size <= 16.
 */
static inline void selectionSort(int a[], int left, int right) {
    for (int i = left; i < right; ++i) {
        int maxIndex = i;

        /* Scan suffix `a[i+1..right]` for the largest remaining element. */
        for (int j = i + 1; j <= right; ++j) {
            if (a[j] > a[maxIndex]) {
                maxIndex = j;
            }
        }

        swap(&a[i], &a[maxIndex]);
    }
}

/*
 * @brief Sort subarray `a[left..right]` in non-increasing (descending) order
 *        via median-of-three three-way QuickSort with tail-call elimination.
 * @param a     Target integer array (coupon discounts).
 * @param left  Inclusive lower index.
 * @param right Inclusive upper index.
 */
static inline void quickSort(int a[], int left, int right) {
    while (left < right) {
        /* 1. Small-subarray cutoff: finish tiny segments without recursion. */
        if (right - left + 1 <= THRESHOLD) {
            selectionSort(a, left, right);
            return;
        }

        /* 2. Median-of-three pivot: sort (a[left], a[mid], a[right]) descending. */
        int mid = left + (right - left) / 2;

        if (a[left] < a[mid])
            swap(&a[left], &a[mid]);
        if (a[left] < a[right])
            swap(&a[left], &a[right]);
        if (a[mid] < a[right])
            swap(&a[mid], &a[right]);

        /* Central median now sits at `a[mid]`; use it as the 3-way splitter. */
        int pivot = a[mid];

        /*
         * 3. Three-way partition:
         *    Invariant during scan:
         *      a[left .. lt-1] >  pivot
         *      a[lt   .. i-1]  == pivot
         *      a[i    .. gt]   uninspected
         *      a[gt+1 .. right] < pivot
         */
        int lt = left;
        int i = left;
        int gt = right;

        while (i <= gt) {
            if (a[i] > pivot) {
                swap(&a[lt], &a[i]);
                ++lt;
                ++i;
            } else if (a[i] < pivot) {
                swap(&a[i], &a[gt]);
                --gt;
                /* Element swapped from `a[gt]` is uninspected; do not advance `i`. */
            } else {
                ++i;
            }
        }

        /*
         * 4. Tail-call elimination:
         *    `a[lt..gt]` already equals `pivot` and needs no further sorting.
         *    Recurse on the smaller strict partition and loop on the larger one,
         *    guaranteeing worst-case recursion depth <= floor(log2(N)).
         */
        if (lt - left < right - gt) {
            quickSort(a, left, lt - 1);
            left = gt + 1;
        } else {
            quickSort(a, gt + 1, right);
            right = lt - 1;
        }
    }
}

#endif /* QUICK_SORT_H */
