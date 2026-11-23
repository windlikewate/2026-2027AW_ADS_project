/*
 * File: shopping_with_cunpons.h
 * Description: Exact greedy priority-queue solver for ADS Project 5
 *              ("Shopping with Coupons") achieving O((N + K) log N) average-case
 *              time and Theta(N) auxiliary space via an implicit sorted-matrix
 *              min-heap frontier and an O(1) original-price bulk cutoff.
 *
 * Algorithmic Overview:
 *   1. Sort `coupons[0..N-1]` descending: C_(0) >= C_(1) >= ... >= C_(N-1).
 *      Each item `i` then defines an implicit sorted row of net prices
 *      P_i - C_(0) <= P_i - C_(1) <= ... <= P_i - C_(N-1).
 *   2. Record minimum full price B = min_i P_i and seed a capacity-N binary
 *      min-heap with the N row heads `(P_i - C_(0), i)`.
 *   3. Repeatedly pop the cheapest unredeemed pair `(minPrice, itemIndex)` and
 *      push its row successor `(P_v - C_(idx_v + 1), v)` while `minPrice < B`
 *      and `remainingMoney >= minPrice`.
 *   4. As soon as the heap empties or `getMin(heap) >= B`, buy all remaining
 *      affordable units at full price B via one O(1) integer division/modulo.
 */

#ifndef SHOPPING_WITH_CUNPONS_H
#define SHOPPING_WITH_CUNPONS_H

#include "binary_min_heap.h"
#include "quick_sort.h"
#include "bench.h"

/* Per-item state tracking base price P_i and next unredeemed coupon cursor. */
typedef struct {
    int price;       /* Original uncouponed price P_i of item i. */
    int couponIndex; /* Next index in descending `coupons[]` to apply to item i. */
} Item;

/*
 * @brief Compute optimal `(maxItems, remainingMoney)` and optionally record the
 *        exact number of couponed heap extractions `K` in `*couponPurchases`.
 * @param N               Number of item types and coupon types (1 <= N <= 10^5).
 * @param D               Initial budget in dollars (1 <= D <= 10^6).
 * @param prices          Input array of N positive item prices P_i.
 * @param coupons         Input array of N positive coupon discounts C_j (sorted in place).
 * @param couponPurchases Optional output pointer receiving K (may be NULL).
 * @return `Result{maxItems, remainingMoney}`, or `{-1, -1}` on allocation failure.
 */
static inline Result solveShoppingCounted(int N, int D, int prices[], int coupons[], int *couponPurchases) {
    /* Phase 1: Sort coupons in non-increasing order C_(0) >= ... >= C_(N-1). */
    quickSort(coupons, 0, N - 1);

    int remainingMoney = D;
    int maxItems = 0;
    int k = 0; /* Counts successful couponed purchases (heap extractions). */
    int minOriginalPrice = prices[0]; /* Minimum full price B = min_i P_i. */

    /* Phase 2: Allocate capacity-N frontier min-heap and per-item cursor table. */
    BinaryMinHeap* heap = createBinaryMinHeap(N);
    Item* items = (Item*)malloc((size_t)N * sizeof(Item));
    if (!heap || !items) {
        destroy(heap);
        free(items);
        return (Result){-1, -1};
    }

    /* Seed the heap with each item's cheapest discounted option P_i - C_(0). */
    for (int i = 0; i < N; i++){
        items[i].price = prices[i];
        items[i].couponIndex = 0;
        if (prices[i] < minOriginalPrice) {
            minOriginalPrice = prices[i];
        }
        insertBinaryMinHeapEntry(heap, items[i].price - coupons[items[i].couponIndex], i);
    }

    /* Phase 3: Stream cheapest couponed pairs until reaching B or exhausting budget. */
    int64_t minPrice = getMin(heap);
    int itemIndex;
    while (remainingMoney > 0){
        /*
         * O(1) Bulk-Cutoff Gate:
         * Once no couponed pair is strictly cheaper than B = minOriginalPrice,
         * unlimited full-price units at cost B dominate all remaining options.
         */
        if (isEmpty(heap) || getMin(heap) >= minOriginalPrice) {
            maxItems += remainingMoney / minOriginalPrice;
            remainingMoney %= minOriginalPrice;
            break;
        }

        /* Check whether the globally cheapest unredeemed pair is affordable. */
        minPrice = getMin(heap);
        if(remainingMoney < minPrice) break;

        /* Redeem `(itemIndex, couponIndex)` and advance that item's row cursor. */
        remainingMoney -= (int)minPrice;
        maxItems++;
        extractMinBinaryEntry(heap, &minPrice, &itemIndex);
        ++k;
        items[itemIndex].couponIndex++;
        if(items[itemIndex].couponIndex < N){
            insertBinaryMinHeapEntry(
                heap,
                items[itemIndex].price - coupons[items[itemIndex].couponIndex],
                itemIndex
            );
        }
    }

    /* Phase 4: Release auxiliary structures and return (Q*, R*). */
    destroy(heap);
    free(items);
    if (couponPurchases) *couponPurchases = k;
    return (Result){maxItems, remainingMoney};
}

/*
 * @brief Standard solver entry point preserving the 4-argument signature.
 */
static inline Result solveShopping(int N, int D, int prices[], int coupons[]) {
    return solveShoppingCounted(N, D, prices, coupons, NULL);
}

#endif /* SHOPPING_WITH_CUNPONS_H */
