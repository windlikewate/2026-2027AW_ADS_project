/*
 * File: Knapsack_origin.h
 * Description: Baseline dynamic programming solver (DP-1) for the 0/1 Knapsack
 *              Problem using an explicit (n + 1) x (c + 1) profit table.
 *              Evaluates every (item, capacity) subproblem in Theta(n * c)
 *              worst-case and best-case time and Theta(n * c) heap space.
 */

#ifndef KNAPSACK_ORIGIN_H
#define KNAPSACK_ORIGIN_H

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Solves the 0/1 knapsack problem using standard dynamic programming (DP-1).
 *
 * @details Maintains a 2D table P[0..number][0..capacity] where P[i][j] = z_i(j)
 *          stores the maximum profit achievable using a subset of the first i
 *          items within residual capacity j.
 *
 * @param number   Total number of items n.
 * @param capacity Knapsack capacity c.
 * @param profits  0-indexed array of item profits p_1, ..., p_n.
 * @param weights  0-indexed array of positive item weights w_1, ..., w_n.
 * @return Optimal total profit P[number][capacity].
 */
int knapsack_origin(int number, int capacity, int *profits, int *weights){
    /* Allocate row pointer directory for rows 0 through number. */
    int **best_solution = (int**) malloc((number+1)*sizeof(int*));
    /* Allocate (capacity + 1) integer cells for each subproblem row i. */
    for(int i=0; i<=number; i++){
        best_solution[i] = (int*) malloc((capacity+1)*sizeof(int));
    }

    /* Base case i = 0: choosing from 0 items yields profit 0 for all capacities. */
    for(int j=0; j<=capacity; j++){
        best_solution[0][j] = 0;
    }

    /* Bottom-up Bellman recurrence: P[i][j] = max(P[i-1][j], P[i-1][j-w_i] + p_i). */
    for(int i=1; i<=number; i++){
        for(int j=0; j<=capacity; j++){
            /* Exclusion transition: skip the i-th item (0-based index i - 1). */
            best_solution[i][j] = best_solution[i-1][j];
            /* Inclusion transition: feasible only when capacity j covers weight w_i. */
            if(j >= weights[i-1]){
                int candidate = best_solution[i-1][j-weights[i-1]] + profits[i-1];
                /* Keep the larger profit between excluding and including item i. */
                if(candidate > best_solution[i][j]){
                    best_solution[i][j] = candidate;
                }
            }
        }
    }

    /* Optimal 0/1 knapsack value resides at the bottom-right entry P[n][c]. */
    int result = best_solution[number][capacity];

    /* Release every dynamically allocated DP row and the row-pointer array. */
    for(int i=0; i<=number; i++){
        free(best_solution[i]);
    }
    free(best_solution);

    return result;
}

#endif // KNAPSACK_ORIGIN_H
