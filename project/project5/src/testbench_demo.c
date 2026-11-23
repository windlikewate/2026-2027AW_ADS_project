/*
 * File: testbench_demo.c
 * Description: Interactive standard-input CLI driver for "Shopping with Coupons".
 *              Reads one test case formatted per the assignment specification
 *              (N, D, N item prices, N coupon discounts) from `stdin` and prints
 *              `<maxItems> <remainingMoney>` to `stdout`.
 *
 * Build & Run Example:
 *   gcc -O2 -std=c11 -Wall -Wextra testbench_demo.c -o testbench_demo
 *   ./testbench_demo < ../data/sample.in    # Expected output: 8 2
 *
 * Input Format:
 *   Line 1: N D          (1 <= N <= 10^5, 1 <= D <= 10^6)
 *   Line 2: P_1 .. P_N   (positive original item prices)
 *   Line 3: C_1 .. C_N   (positive coupon values, max C_j < min P_i)
 */

#include <stdio.h>
#include <stdlib.h>
#include "shopping_with_cunpons.h"

int main(void) {
    int N, D;
    /* Read problem size N and available budget D from standard input. */
    if (scanf("%d %d", &N, &D) != 2 || N <= 0 || D < 0) {
        fprintf(stderr, "Invalid header: expected positive N and non-negative D.\n");
        return 1;
    }

    /* Heap-allocate input arrays so N up to 10^5 never overflows the call stack. */
    int *prices = (int *)malloc((size_t)N * sizeof(int));
    int *coupons = (int *)malloc((size_t)N * sizeof(int));
    if (!prices || !coupons) {
        free(prices);
        free(coupons);
        return 1;
    }

    /* Read N item prices followed by N coupon discount values. */
    for (int i = 0; i < N; i++) {
        if (scanf("%d", &prices[i]) != 1) { free(prices); free(coupons); return 1; }
    }
    for (int i = 0; i < N; i++) {
        if (scanf("%d", &coupons[i]) != 1) { free(prices); free(coupons); return 1; }
    }

    /* Run greedy frontier solver and print optimal (Q*, R*) pair. */
    Result ans = solveShopping(N, D, prices, coupons);
    printf("%d %d\n", ans.count, ans.money);

    free(prices);
    free(coupons);
    return 0;
}
