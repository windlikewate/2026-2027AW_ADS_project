#include "binary_min_heap.h"
#include "quick_sort.h"
#include "bench.h"
typedef struct {
    int price;
    int couponIndex;// the index we use to access the corresponding coupon in the coupons array
} Item;

Result solveShoppingCounted(int N, int D, int prices[], int coupons[], int *couponPurchases) {
    quickSort(coupons, 0, N - 1);
    int remainingMoney = D;
    int maxItems = 0;
    int k = 0; // Successful coupon purchases / heap extractions in this call.
    int minOriginalPrice = prices[0]; // do not use coupon
    BinaryMinHeap* heap = createBinaryMinHeap(N);
    Item* items = (Item*)malloc(N * sizeof(Item));
    for (int i = 0;i < N ;i++){
        items[i].price = prices[i];
        items[i].couponIndex = 0;
        if (prices[i] < minOriginalPrice) {
            minOriginalPrice = prices[i];
        } // find the minimum original price we can buy without using any coupon
        insertBinaryMinHeapEntry(heap, items[i].price - coupons[items[i].couponIndex], i);
    }

    int64_t minPrice;
    minPrice = getMin(heap);
    int itemIndex;
    while (remainingMoney > 0){
        if (isEmpty(heap) || getMin(heap) >= minOriginalPrice) {
            maxItems += remainingMoney / minOriginalPrice;
            remainingMoney %= minOriginalPrice;
            break;
        }// if the heap is empty or the minimum price with coupon is not cheaper than the minimum original price, buy as many items as possible without using coupons

        minPrice = getMin(heap);
        if(remainingMoney < minPrice) break;
        remainingMoney -= minPrice;
        maxItems++;
        extractMinBinaryEntry(heap, &minPrice, &itemIndex);
        ++k;
        items[itemIndex].couponIndex++;
        if(items[itemIndex].couponIndex < N){
            insertBinaryMinHeapEntry(heap, items[itemIndex].price - coupons[items[itemIndex].couponIndex], itemIndex);
        }
    }
    destroy(heap);
    free(items);
    if (couponPurchases) *couponPurchases = k;
    // printf("%d %d\n", maxItems, remainingMoney);
    return (Result){maxItems, remainingMoney};
}

/* Preserve the original solver interface for other callers. */
Result solveShopping(int N, int D, int prices[], int coupons[]) {
    return solveShoppingCounted(N, D, prices, coupons, NULL);
}
