/*
 * File: testbench_demo.c
 * Description: Minimal sanity-check driver verifying that DP-1 (`knapsack_origin`),
 *              DP-with-Lists (`knapsack_list`), and DP-with-Upper-Bounds
 *              (`knapsack_list_with_bound`) return identical optimal profits on
 *              a 5-item toy instance (n = 5, c = 15, optimal profit P* = 18).
 */

#include "Knapsack_origin.h"
#include "Knapsack_List.h"
#include "Knapsack_List_with_bound.h"
#include "Knapsack_w3.h"
#include <stdio.h>

int main() {
    /* Toy 0/1 knapsack instance: optimal subset {1, 2, 3, 4} has w = 15, p = 18. */
    int number = 5;
    int capacity = 15;
    int profits[] = {6, 3, 5, 4, 2};
    int weights[] = {2, 2, 6, 5, 4};

    /* 1. Baseline dense-table dynamic programming (DP-1). */
    int max_profit = knapsack_origin(number, capacity, profits, weights);
    printf("Maximum profit (origin): %d\n", max_profit);

    /* 2. List-based dynamic programming with Pareto dominance pruning (Section 3.4). */
    int max_profit_list = knapsack_list(number, capacity, profits, weights);
    printf("Maximum profit (list): %d\n", max_profit_list);

    /* 3. List-based dynamic programming with fractional upper-bound fathoming (Section 3.5). */
    int max_profit_list_with_bound = knapsack_list_with_bound(number, capacity, profits, weights);
    printf("Maximum profit (list with bound): %d\n", max_profit_list_with_bound);

    /* 4. Capacity-independent proximity/SMAWK bonus solver. */
    int bonus = knapsack_w3(number, capacity, profits, weights);
    printf("Maximum profit (w3): %d\n", bonus);
    return !(max_profit == max_profit_list && max_profit == max_profit_list_with_bound
             && max_profit == bonus);
}
