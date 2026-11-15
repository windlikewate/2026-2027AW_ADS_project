#include "Knapsack_origin.h"
#include "Knapsack_List.h"
#include "Knapsack_List_with_bound.h"
#include <stdio.h>

int main() {
    int number = 5;
    int capacity = 15;
    int profits[] = {6, 3, 5, 4, 2};
    int weights[] = {2, 2, 6, 5, 4};

    int max_profit = knapsack_origin(number, capacity, profits, weights);
    printf("Maximum profit (origin): %d\n", max_profit);

    int max_profit_list = knapsack_list(number, capacity, profits, weights);
    printf("Maximum profit (list): %d\n", max_profit_list);

    int max_profit_list_with_bound = knapsack_list_with_bound(number, capacity, profits, weights);
    printf("Maximum profit (list with bound): %d\n", max_profit_list_with_bound);

    return 0;
}