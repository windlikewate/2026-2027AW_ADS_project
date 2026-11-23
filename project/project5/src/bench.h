/*
 * File: bench.h
 * Description: Common solver result interface shared between the greedy
 *              priority-queue solver (`shopping_with_cunpons.h`) and the
 *              independent verification & timing harness (`testbench.c`).
 *
 * Contract:
 *   - `count` : Maximum number of items Q* purchasable within budget D.
 *   - `money` : Maximum remaining money R* among all plans buying Q* items.
 *   - Solvers sort `coupons[]` in place and return {-1, -1} on memory failure.
 */

#ifndef SHOPPING_BENCH_H
#define SHOPPING_BENCH_H

#include <stdint.h>

/* Result tuple (Q*, R*) returned by solveShopping / solveShoppingCounted / oracle. */
typedef struct {
    int count; /* Optimal number of items purchased (Q*). */
    int money; /* Optimal remaining dollars after buying Q* items (R*). */
} Result;

#endif /* SHOPPING_BENCH_H */
