#ifndef KNAPSACK_LIST_H
#define KNAPSACK_LIST_H
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
// The implementation of the DP-with-Lists algorithm for the knapsack problem (DP-2)
/**
 * @brief Solves the 0/1 knapsack problem using the DP-with-Lists algorithm (DP-2).
 *
 * @param number The number of items.
 * @param capacity The capacity of the knapsack.
 * @param profits An array of item profits.
 * @param weights An array of item weights.
 * @return The maximum profit that can be obtained without exceeding the knapsack capacity.
 */
typedef struct {
    int weight;
    int profit;
} State;

State sentinel = {INT_MAX, 0};
/**
 * @brief Represents a list of states for the DP-with-Lists algorithm.
 *
 * @details The list stores states in increasing order of weight and profit.
 *          The `size` field indicates the number of valid states, while the
 *          `allocated` field indicates the total allocated space for states.
 */
typedef struct {
    State *data;  // the array of states
    int size;     // the number of valid states
    int allocated; // the total allocated space for states
} StateList;



void add_sentinel(StateList *list) {
    if (list->allocated <= list->size) {
        State *temp = (State*) realloc(
            list->data,
            ((size_t)list->size + 1) * sizeof(State)
        );

        if (temp == NULL) {
            exit(EXIT_FAILURE);
        }

        list->data = temp;
        list->allocated = list->size + 1;
    }

    list->data[list->size] = (State){INT_MAX, 0};
}
/**
 * @brief Adds a sentinel state to the end of the list.
 *
 * @details The sentinel state has weight INT_MAX and profit 0.
 */

StateList Mergelist(StateList *source1, StateList *source2){
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
/**
 * @brief Merges two state lists into a single list.
 *
 * @details The merged list contains all states from both input lists,
 *          maintaining the order by weight and removing dominated states.
*/


int knapsack_list(int number, int capacity, int *profits, int *weights){
    StateList *lists = (StateList*) malloc((number+1) * sizeof(StateList)); // Allocate memory for the list of lists
    lists[0].data = (State*) malloc((capacity+1)* sizeof(State)); // Allocate memory for the first list
    lists[0].size = 1;
    lists[0].allocated = capacity+1;
    lists[0].data[0] = (State){0, 0};

    // the main loop
    for(int j = 1; j <= number; j++){
        StateList *prev = &lists[j-1];

        StateList L_jprime;
        L_jprime.data = (State*) malloc(prev->size * sizeof(State));
        L_jprime.size = 0;
        L_jprime.allocated = prev->size;
        for(int k = 0; k < prev->size; k++){
            State temp;
            temp.weight = prev->data[k].weight + weights[j-1];
            temp.profit = prev->data[k].profit + profits[j-1];
            if(temp.weight > capacity){
                break;
            }
            L_jprime.data[L_jprime.size++] = temp;
        }
        lists[j] = Mergelist(&lists[j-1], &L_jprime);
        free(L_jprime.data);
    }
    int ans = lists[number].data[lists[number].size - 1].profit;
    // Free the allocated memory
    for(int i = 0; i <= number; i++){
        free(lists[i].data);
    }
    free(lists);

    // Return the maximum profit
    return ans;
}


#endif // KNAPSACK_LIST_H
