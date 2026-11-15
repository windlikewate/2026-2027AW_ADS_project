#ifndef KNAPSACK_ORIGIN_H
#define KNAPSACK_ORIGIN_H

#include <stdio.h>
#include <stdlib.h>

// The implementation of the original dynamic programming algorithm for the knapsack problem (DP-1)
/**
 * @brief Solves the 0/1 knapsack problem using the original dynamic programming algorithm (DP-1).
 *
 * @param number The number of items.
 * @param capacity The capacity of the knapsack.
 * @param profits An array of item profits.
 * @param weights An array of item weights.
 * @return The maximum profit that can be obtained without exceeding the knapsack capacity.
 */
int knapsack_origin(int number, int capacity, int *profits, int *weights){
    int **best_solution = (int**) malloc((number+1)*sizeof(int*));
    for(int i=0; i<=number; i++){
        best_solution[i] = (int*) malloc((capacity+1)*sizeof(int));
    }
    
    // Initialize the base case: with 0 items, the maximum profit is 0 for any capacity
    for(int j=0; j<=capacity; j++){
        best_solution[0][j] = 0;
    }

    // Fill the table using the recurrence relation
    for(int i=1; i<=number; i++){
        for(int j=0; j<=capacity; j++){
            best_solution[i][j] = best_solution[i-1][j];
            if(j >= weights[i-1]){
                int candidate = best_solution[i-1][j-weights[i-1]] + profits[i-1];
                if(candidate > best_solution[i][j]){
                    best_solution[i][j] = candidate;
                }
            }
        }
    }

    int result = best_solution[number][capacity];

    // Free the allocated memory
    for(int i=0; i<=number; i++){
        free(best_solution[i]);
    }
    free(best_solution);

    return result;

}

#endif // KNAPSACK_ORIGIN_H