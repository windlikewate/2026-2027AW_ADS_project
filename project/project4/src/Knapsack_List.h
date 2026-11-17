/*
 * File: Knapsack_List.h
 * Description: Dynamic Programming with Lists (DP-with-Lists, Kellerer et al.
 *              Section 3.4) for the 0/1 Knapsack Problem. Replaces dense table
 *              rows with Pareto-undominated state lists L_j = <(w, p)> ordered
 *              strictly by increasing weight and profit.
 */

#ifndef KNAPSACK_LIST_H
#define KNAPSACK_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/**
 * @brief Represents one feasible dynamic programming state (w, p).
 *
 * @details Encodes a subset of the first j items having total weight `weight`
 *          and maximum achievable profit `profit` at that weight.
 */
typedef struct {
    int weight; /* Cumulative weight w in [0, capacity] (INT_MAX for sentinel). */
    int profit; /* Maximum profit p achieved at cumulative weight w. */
} State;

/* Global sentinel state (infinity, 0) marking the end of a sorted StateList. */
State sentinel = {INT_MAX, 0};

/**
 * @brief Contiguous dynamic array storing undominated states in increasing order.
 *
 * @details By Corollary 3.4.2, undominated states satisfy both w_1 < w_2 < ...
 *          and p_1 < p_2 < ..., so the final element always holds maximum profit.
 */
typedef struct {
    State *data;   /* Heap-allocated buffer of State records. */
    int size;      /* Number of active undominated states (excluding sentinel). */
    int allocated; /* Total capacity of `data` in number of State elements. */
} StateList;

/**
 * @brief Appends a sentinel state (INT_MAX, 0) right past the last valid state.
 *
 * @details Ensures two-way list merging terminates cleanly without explicit
 *          index-bounds checks on every iteration; reallocates if needed.
 */
void add_sentinel(StateList *list) {
    /* Grow buffer by 1 slot if no room remains for the trailing sentinel. */
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

    /* Place sentinel immediately after `size` valid elements. */
    list->data[list->size] = (State){INT_MAX, 0};
}

/**
 * @brief Merges two weight-sorted state lists and prunes all dominated states.
 *
 * @details Implements Procedure Merge-Lists (Kellerer et al., Fig. 3.6).
 *          Repeatedly extracts the smaller-weight state `small_state` from
 *          `source1` (L_{j-1}) or `source2` (L'_{j-1} = L_{j-1} (+) (w_j, p_j)):
 *            - If profit <= last merged profit, `small_state` is dominated: skip.
 *            - If profit > last merged profit and weight == last weight, replace
 *              the dominated equal-weight tail state in place.
 *            - If profit > last merged profit and weight > last weight, append.
 */
StateList Mergelist(StateList *source1, StateList *source2){
    /* Append sentinels so exhausted lists present weight = INT_MAX. */
    add_sentinel(source1);
    add_sentinel(source2);
    StateList list1 = *source1;
    StateList list2 = *source2;

    /* Allocate upper-bound capacity for the merged Pareto frontier L_j. */
    StateList Merged;
    Merged.data = (State*) malloc((list1.size + list2.size+1) * sizeof(State));
    Merged.size = 1;
    Merged.allocated = list1.size + list2.size+1;
    /* Seed slot 0 with (INT_MAX, 0) so the first valid state overwrites slot 0. */
    Merged.data[0] = (State){INT_MAX, 0};

    int flag_list1 = 0;  /* Read cursor in L_{j-1}. */
    int flag_list2 = 0;  /* Read cursor in shifted list L'_{j-1}. */
    int flag_merged = 0; /* Index of the largest undominated state in Merged. */
    do{
        /* Pop candidate state with minimum weight across the two input lists. */
        State small_state = (list1.data[flag_list1].weight < list2.data[flag_list2].weight) ? list1.data[flag_list1++] : list2.data[flag_list2++];
        /* Retain (0, 0) at initialization or any state strictly improving profit. */
        if(small_state.profit > Merged.data[flag_merged].profit||small_state.weight==0&&small_state.profit==0){
            /* Same weight (or initial INT_MAX seed) with higher profit: overwrite tail. */
            if(small_state.weight <= Merged.data[flag_merged].weight){
                Merged.data[flag_merged] = small_state;
            }
            /* Strictly larger weight and strictly larger profit: append new Pareto point. */
            else{
                Merged.data[++flag_merged] = small_state;
                Merged.size++;
            }
        }
    }while(list1.data[flag_list1].weight != INT_MAX || list2.data[flag_list2].weight != INT_MAX);
    return Merged;
}

/**
 * @brief Solves the 0/1 knapsack problem via DP-with-Lists and dominance pruning.
 *
 * @param number   Total number of items n.
 * @param capacity Knapsack capacity c.
 * @param profits  Array of item profits p_1, ..., p_n.
 * @param weights  Array of positive item weights w_1, ..., w_n.
 * @return Optimal total profit max_{(w, p) in L_n} p.
 */
int knapsack_list(int number, int capacity, int *profits, int *weights){
    /* Allocate state-list headers L_0, L_1, ..., L_n and initialize L_0 = <(0, 0)>. */
    StateList *lists = (StateList*) malloc((number+1) * sizeof(StateList));
    lists[0].data = (State*) malloc((capacity+1)* sizeof(State));
    lists[0].size = 1;
    lists[0].allocated = capacity+1;
    lists[0].data[0] = (State){0, 0};

    /* Process items j = 1..number by shifting L_{j-1} and merging with pruning. */
    for(int j = 1; j <= number; j++){
        StateList *prev = &lists[j-1];

        /* Build L'_j = { (w + w_j, p + p_j) | (w, p) in L_{j-1}, w + w_j <= c }. */
        StateList L_jprime;
        L_jprime.data = (State*) malloc(prev->size * sizeof(State));
        L_jprime.size = 0;
        L_jprime.allocated = prev->size;
        for(int k = 0; k < prev->size; k++){
            State temp;
            temp.weight = prev->data[k].weight + weights[j-1];
            temp.profit = prev->data[k].profit + profits[j-1];
            /* Since L_{j-1} is sorted by weight, all subsequent states also exceed c. */
            if(temp.weight > capacity){
                break;
            }
            L_jprime.data[L_jprime.size++] = temp;
        }
        /* Merge L_{j-1} and L'_j while discarding dominated states. */
        lists[j] = Mergelist(&lists[j-1], &L_jprime);
        free(L_jprime.data);
    }

    /* Because L_n is strictly ordered by profit, its last element is optimal. */
    int ans = lists[number].data[lists[number].size - 1].profit;

    /* Free all state buffers before returning. */
    for(int i = 0; i <= number; i++){
        free(lists[i].data);
    }
    free(lists);

    return ans;
}

#endif // KNAPSACK_LIST_H
