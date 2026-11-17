/*
 * File: Knapsack_List_with_bound.h
 * Description: Hybrid Dynamic Programming with Lists and LP-Relaxation Upper
 *              Bounds (DP-with-Upper-Bounds, Kellerer et al. Section 3.5).
 *              Sorts items by non-increasing efficiency p_i / w_i, tracks a
 *              running incumbent `best_profit`, and fathoms any state (w, p)
 *              whose fractional-knapsack upper bound U_j(w, p) <= best_profit.
 */

#ifndef KNAPSACK_LIST_WITH_BOUND_H
#define KNAPSACK_LIST_WITH_BOUND_H

#include"Fraction_Knapsack.h"
#include"Knapsack_List.h"

/**
 * @brief Merges L_{j-1} and L'_j while applying both upper-bound fathoming
 *        and Pareto dominance pruning.
 *
 * @param source1        Pointer to unshifted list L_{j-1}.
 * @param source2        Pointer to shifted list L'_j = L_{j-1} (+) (w_j, p_j).
 * @param left_number    Number of remaining undecided items (n - j).
 * @param capacity       Total knapsack capacity c.
 * @param sorted_weights Remaining weights w_{j+1..n} sorted by decreasing p/w.
 * @param sorted_values  Remaining profits p_{j+1..n} sorted by decreasing p/w.
 * @param best_profit    In/out pointer to global incumbent feasible profit P*.
 * @return Pruned state list L_j containing only promising undominated states.
 */
StateList Mergelist_with_bound(
    StateList *source1,
    StateList *source2,
    int left_number,
    int capacity,
    int *sorted_weights,
    int *sorted_values,
    int* best_profit){
    /* Append sentinels (INT_MAX, 0) to simplify two-pointer ordered scanning. */
    add_sentinel(source1);
    add_sentinel(source2);
    StateList list1 = *source1;
    StateList list2 = *source2;

    /* Allocate result list L_j and seed index 0 with placeholder sentinel. */
    StateList Merged;
    Merged.data = (State*) malloc((list1.size + list2.size+1) * sizeof(State));
    Merged.size = 1;
    Merged.allocated = list1.size + list2.size+1;
    Merged.data[0] = (State){INT_MAX, 0};

    int flag_list1 = 0;  /* Cursor into L_{j-1}. */
    int flag_list2 = 0;  /* Cursor into L'_j. */
    int flag_merged = 0; /* Tail index of Merged. */
    do{
        /* Extract next candidate state in non-decreasing order of weight. */
        State small_state = (list1.data[flag_list1].weight < list2.data[flag_list2].weight) ? list1.data[flag_list1++] : list2.data[flag_list2++];
        /* Evaluate LP-relaxation upper bound U_j(w, p) on residual capacity c - w. */
        int potential_profit = small_state.profit+fraction_knapsack(left_number, capacity-small_state.weight, sorted_weights, sorted_values);
        /* Branch-and-bound fathoming: discard state if it cannot beat incumbent P*. */
        if(potential_profit <= *best_profit)
            continue;
        /* Every surviving state is already a valid integer solution (setting x_{>j}=0). */
        *best_profit = (small_state.profit > *best_profit) ? small_state.profit : *best_profit;
        /* Apply standard Section 3.4 Pareto dominance filter against Merged tail. */
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
 * @brief Solves the 0/1 knapsack problem via DP-with-Lists + Fractional Upper Bounds.
 *
 * @param number   Total number of items n.
 * @param capacity Knapsack capacity c.
 * @param profits  Input array of item profits p_1, ..., p_n.
 * @param weights  Input array of positive item weights w_1, ..., w_n.
 * @return Optimal integer profit P* tracked in `best_profit`.
 */
int knapsack_list_with_bound(int number, int capacity, int profits[], int weights[]){
    /* Build (index, p_i / w_i) pairs and sort items in descending efficiency order. */
    ItemRatio *ratio = (ItemRatio *)malloc(number * sizeof(ItemRatio));
    for(int i = 0;i < number;i++){
        ratio[i].index = i;
        ratio[i].ratio = (double)profits[i]/weights[i];
    }
    ratio = selection_sort(ratio, number);

    /* Permute weights and profits so suffixes [j..n-1] can be passed to Dantzig greedy. */
    int* sorted_weights = (int*)malloc(number * sizeof(int));
    int* sorted_values = (int*)malloc(number * sizeof(int));
    for(int i = 0;i < number;i++){
        sorted_weights[i] = weights[ratio[i].index];
        sorted_values[i] = profits[ratio[i].index];
    }

    /* Initialize list array and root state list L_0 = <(0, 0)>. */
    StateList *lists = (StateList*) malloc((number+1) * sizeof(StateList));
    lists[0].data = (State*) malloc((capacity+1)* sizeof(State));
    lists[0].size = 1;
    lists[0].allocated = capacity+1;
    lists[0].data[0] = (State){0, 0};

    /* Main DP loop with incumbent tracking: P* starts at 0. */
    int best_profit = 0;
    for(int j = 1; j <= number; j++){
        StateList *prev = &lists[j-1];

        /* Generate feasible shifted states L'_j using the j-th highest-ratio item. */
        StateList L_jprime;
        L_jprime.data = (State*) malloc(prev->size * sizeof(State));
        L_jprime.size = 0;
        L_jprime.allocated = prev->size;
        for(int k = 0; k < prev->size; k++){
            State temp;
            temp.weight = prev->data[k].weight + sorted_weights[j-1];
            temp.profit = prev->data[k].profit + sorted_values[j-1];
            /* Break early as soon as cumulative weight exceeds knapsack capacity. */
            if(temp.weight > capacity){
                break;
            }
            L_jprime.data[L_jprime.size++] = temp;
        }
        /* Merge L_{j-1} and L'_j while fathoming states with U_j(w, p) <= best_profit. */
        lists[j] = Mergelist_with_bound(&lists[j-1], &L_jprime, number-j,capacity, sorted_weights+j, sorted_values+j, &best_profit);
        free(L_jprime.data);
    }

    /* Deallocate all dynamic state arrays and sorting buffers. */
    for(int i = 0; i <= number; i++){
        free(lists[i].data);
    }
    free(lists);
    free(ratio);
    free(sorted_weights);
    free(sorted_values);

    /* Note: at j = n all remaining states satisfy U_n(w, p) = p <= P*, so return P*. */
    return best_profit;
}

#endif // KNAPSACK_LIST_WITH_BOUND_H
