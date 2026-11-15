#ifndef KNAPSACK_LIST_WITH_BOUND_H
#define KNAPSACK_LIST_WITH_BOUND_H
#include"Fraction_Knapsack.h"
#include"Knapsack_List.h"

StateList Mergelist_with_bound(
    StateList *source1, // The first list of states to be merged
    StateList *source2, // The second list of states to be merged
    int left_number, // The number of remaining items
    int capacity, // The whole capacity of the knapsack
    int *sorted_weights, // The weights of the remaining items, sorted by ratio
    int *sorted_values, // The values of the remaining items, sorted by ratio
    int* best_profit){ // Pointer to the best profit found so far
    add_sentinel(source1);
    add_sentinel(source2);
    StateList list1 = *source1;
    StateList list2 = *source2;
    StateList Merged;
    Merged.data = (State*) malloc((list1.size + list2.size+1) * sizeof(State));
    Merged.size = 1;
    Merged.allocated = list1.size + list2.size+1;
    Merged.data[0] = (State){INT_MAX, 0};
    int flag_list1 = 0;
    int flag_list2 = 0;
    int flag_merged = 0;
    do{
        State small_state = (list1.data[flag_list1].weight < list2.data[flag_list2].weight) ? list1.data[flag_list1++] : list2.data[flag_list2++];
        int potential_profit = small_state.profit+fraction_knapsack(left_number, capacity-small_state.weight, sorted_weights, sorted_values);
        if(potential_profit <= *best_profit)
            continue;
        *best_profit = (small_state.profit > *best_profit) ? small_state.profit : *best_profit;
        if(small_state.profit > Merged.data[flag_merged].profit||small_state.weight==0&&small_state.profit==0){
            if(small_state.weight <= Merged.data[flag_merged].weight){
                Merged.data[flag_merged] = small_state;
            }
            else{
                Merged.data[++flag_merged] = small_state;
                Merged.size++;
            }
        }
    }while(list1.data[flag_list1].weight != INT_MAX || list2.data[flag_list2].weight != INT_MAX);
    return Merged;
}

int knapsack_list_with_bound(int number, int capacity, int profits[], int weights[]){
    // Sort the items by their ratio in descending order
    ItemRatio *ratio = (ItemRatio *)malloc(number * sizeof(ItemRatio));
    for(int i = 0;i < number;i++){
        ratio[i].index = i;
        ratio[i].ratio = (double)profits[i]/weights[i];
    }
    ratio = selection_sort(ratio, number);
    int* sorted_weights = (int*)malloc(number * sizeof(int));
    int* sorted_values = (int*)malloc(number * sizeof(int));
    for(int i = 0;i < number;i++){
        sorted_weights[i] = weights[ratio[i].index];
        sorted_values[i] = profits[ratio[i].index];
    }

    StateList *lists = (StateList*) malloc((number+1) * sizeof(StateList)); // Allocate memory for the list of lists
    lists[0].data = (State*) malloc((capacity+1)* sizeof(State)); // Allocate memory for the first list
    lists[0].size = 1;
    lists[0].allocated = capacity+1;
    lists[0].data[0] = (State){0, 0};

    // the main loop
    int best_profit = 0;
    for(int j = 1; j <= number; j++){
        StateList *prev = &lists[j-1];

        StateList L_jprime;
        L_jprime.data = (State*) malloc(prev->size * sizeof(State));
        L_jprime.size = 0;
        L_jprime.allocated = prev->size;
        for(int k = 0; k < prev->size; k++){
            State temp;
            temp.weight = prev->data[k].weight + sorted_weights[j-1];
            temp.profit = prev->data[k].profit + sorted_values[j-1];
            if(temp.weight > capacity){
                break;
            }
            L_jprime.data[L_jprime.size++] = temp;
        }
        lists[j] = Mergelist_with_bound(&lists[j-1], &L_jprime, number-j,capacity, sorted_weights+j, sorted_values+j, &best_profit);
        free(L_jprime.data);
    }
    // Free the allocated memory
    for(int i = 0; i <= number; i++){
        free(lists[i].data);
    }
    free(lists);
    free(ratio);
    free(sorted_weights);
    free(sorted_values);

    // Return the maximum profit
    return best_profit;
}

#endif // KNAPSACK_LIST_WITH_BOUND_H
