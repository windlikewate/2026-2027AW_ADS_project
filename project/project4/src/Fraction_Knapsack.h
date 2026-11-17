/*
 * File: Fraction_Knapsack.h
 * Description: Profit-to-weight efficiency sorting and greedy LP-relaxation
 *              evaluator (LKP) used to compute valid integer upper bounds
 *              U_j(w, p) inside DP-with-Upper-Bounds (Section 3.5).
 */

#ifndef FRACTION_KNAPSACK_H
#define FRACTION_KNAPSACK_H

#include<stdio.h>
#include<stdlib.h>

/**
 * @brief Pairs an original 0-based item index with its unit profit ratio p_i / w_i.
 */
typedef struct {
    int index;    /* Original item subscript in [0, number - 1]. */
    double ratio; /* Unit profit density (double)profits[i] / weights[i]. */
} ItemRatio;

/**
 * @brief Sorts an array of ItemRatio records in non-increasing order of `ratio`.
 *
 * @details Uses in-place selection sort in Theta(n^2) time as a one-time
 *          preprocessing step prior to state-list generation.
 *
 * @param ratio  Array of `number` ItemRatio entries to sort in place.
 * @param number Total number of items n.
 * @return Pointer to the sorted `ratio` array.
 */
ItemRatio* selection_sort(ItemRatio *ratio, int number){
    for(int i = 0;i < number - 1;i++){
        int max_index = i;
        /* Locate the highest profit-to-weight ratio in suffix [i + 1 .. number - 1]. */
        for(int j = i+1;j < number;j++){
            if(ratio[j].ratio > ratio[max_index].ratio){
                max_index = j;
            }
        }
        /* Swap maximum-efficiency entry into position i. */
        ItemRatio temp = ratio[i];
        ratio[i] = ratio[max_index];
        ratio[max_index] = temp;
    }
    return ratio;
}

/**
 * @brief Evaluates the floored LP-relaxation (fractional knapsack) upper bound.
 *
 * @details Assumes `weights` and `values` are already ordered by non-increasing
 *          value/weight ratio. Packs whole items greedily while residual capacity
 *          permits, then adds the floored fractional contribution of the critical
 *          (break) item s: floor(p_s * remaining_capacity / w_s). Since all item
 *          profits are integers, flooring the LP optimum preserves a valid upper
 *          bound for the 0/1 integer subproblem.
 *
 * @param number   Number of remaining items (n - j).
 * @param capacity Residual knapsack capacity (c - w).
 * @param weights  Remaining item weights sorted by descending p/w.
 * @param values   Remaining item profits sorted by descending p/w.
 * @return Floored fractional-knapsack upper bound in O(number) time.
 */
int fraction_knapsack(int number, int capacity, int weights[], int values[]){
    int total_value = 0;
    int remaining_capacity = capacity;
    /* Greedily scan remaining items in non-increasing order of unit profit. */
    for(int i = 0;i < number;i++){
        if(remaining_capacity >= weights[i]){
            /* Whole item fits into residual capacity. */
            total_value += values[i];
            remaining_capacity -= weights[i];
        }else{
            /* Critical item: pack fraction remaining_capacity / weights[i] and stop. */
            total_value += (int)((values[i] * (double)remaining_capacity) / weights[i]);
            break;
        }
    }
    return total_value;
}

#endif // FRACTION_KNAPSACK_H
