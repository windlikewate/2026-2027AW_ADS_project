#include <stdio.h>

#define THRESHOLD 16

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// selection sort for small arrays
void selectionSort(int a[], int left, int right) {
    for (int i = left; i < right; ++i) {
        int maxIndex = i;

        for (int j = i + 1; j <= right; ++j) {
            if (a[j] > a[maxIndex]) {
                maxIndex = j;
            }
        }

        swap(&a[i], &a[maxIndex]);
    }
}


void quickSort(int a[], int left, int right) {
    while (left < right) {
        // if the array size is small, use selection sort
        if (right - left + 1 <= THRESHOLD) {
            selectionSort(a, left, right);
            return;
        }

        // choose the median of the first, middle, and last elements as the pivot
        int mid = left + (right - left) / 2;

        if (a[left] < a[mid])
            swap(&a[left], &a[mid]);
        if (a[left] < a[right])
            swap(&a[left], &a[right]);
        if (a[mid] < a[right])
            swap(&a[mid], &a[right]);

        int pivot = a[mid];

        // three-way partition
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
                // the swapped element has not been checked yet, so do not increment i
            } else {
                ++i;
            }
        }

        // after partitioning:
        // a[left ... lt-1]  > pivot
        // a[lt   ... gt]   == pivot
        // a[gt+1 ... right] < pivot
        //
        // recursively sort the smaller partition, iteratively sort the larger partition,
        // to avoid deep recursion.
        if (lt - left < right - gt) {
            quickSort(a, left, lt - 1);
            left = gt + 1;
        } else {
            quickSort(a, gt + 1, right);
            right = lt - 1;
        }
    }
}
