#ifndef FRACTION_KNAPSACK_H
#define FRACTION_KNAPSACK_H
#include<stdio.h>
#include<stdlib.h>

/*
Represents an item with its index and ratio (value/weight).
 */
typedef struct {
    int index;
    double ratio;
} ItemRatio;

/* 
 Sorts the items by their ratio in descending order using selection sort.
 ratio The array of item ratios
 number The number of items
 The sorted array of item ratios
*/
ItemRatio* selection_sort(ItemRatio *ratio, int number){
    for(int i = 0;i < number - 1;i++){
        int max_index = i;
        for(int j = i+1;j < number;j++){
            if(ratio[j].ratio > ratio[max_index].ratio){
                max_index = j;
            }
        }
        ItemRatio temp = ratio[i];
        ratio[i] = ratio[max_index];
        ratio[max_index] = temp;
    }
    return ratio;
}

/*
The function to solve the fractional knapsack problem
 *
 * number The number of items
 * capacity The capacity of the knapsack
 * weights The array of weights of each item
 * values The array of values of each item
 * The maximum value achievable
 */
int fraction_knapsack(int number, int capacity, int weights[], int values[]){ // Here the weights and values have been sorted by their ratio in descending order
    // Calculate the maximum value achievable
    int total_value = 0;
    int remaining_capacity = capacity;
    for(int i = 0;i < number;i++){
        if(remaining_capacity >= weights[i]){
            total_value += values[i];
            remaining_capacity -= weights[i];
        }else{
            total_value += (int)((values[i] * (double)remaining_capacity) / weights[i]);
            break;
        }
    }
    return total_value;
}
#endif // FRACTION_KNAPSACK_H