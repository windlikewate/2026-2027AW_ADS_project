/*
 * File: recover_the_design.c
 * Description: Depth-first search over the candidate fences built in
 *              recover_the_design.h. Each edge is taken or skipped.
 *              The first feasible layout is kept. Inclusion is tried
 *              before exclusion, which fixes which drawing is printed.
 */

#include "recover_the_design.h"
/*
 * Decide edges[index .. edge_num - 1].
 * The taken branch is tried before the skipped branch.
 * A skipped edge is explored only when both endpoints can still
 * meet their required degree with the edges that remain.
 */
bool backtrack(Graph* g,int index){
    /* Every selected fence has two endpoints, so the degree sum is even. */
    /* An odd sum is rejected here, before any edge is decided. */
    if(index == 0){
        int degree_sum = 0;
        for(int i = 0; i < g->rows * g->columns; i++){
            degree_sum += g->vertices[i].degree;
        }
        if(degree_sum % 2 != 0){
            return false;
        }
    }
    if(index == g->edge_num){
        for(int i = 0;i < g->rows* g->columns;i++){
            if(g->current_degree[FlattenIndex(g->vertices[i].x, g->vertices[i].y, g->columns)] != g->vertices[i].degree){
                return false;
            }
        } /* Leaf: every cell must meet its required degree exactly. */
        return true;
    }
    /* Branch 1: take edges[index] and test it in check_edge. */
    g->edges[index].is_selected = true;
    g->current_degree[FlattenIndex(g->edges[index].from.x, g->edges[index].from.y, g->columns)]++;
    g->current_degree[FlattenIndex(g->edges[index].to.x, g->edges[index].to.y, g->columns)]++;
    if(check_edge(g,index)){
        if(backtrack(g,index+1)){
            return true; /* A layout was found under this choice. */
        }
    }
    /* Branch 2: put the edge back and restore both endpoint degrees. */
    g->edges[index].is_selected = false;
    g->current_degree[FlattenIndex(g->edges[index].from.x, g->edges[index].from.y, g->columns)]--;
    g->current_degree[FlattenIndex(g->edges[index].to.x, g->edges[index].to.y, g->columns)]--;
    /* Skip this branch when either endpoint is already short of edges. */
    /* count_remaining_edges counts only candidates after this index. */
    Vertex from = g->edges[index].from;
    Vertex to = g->edges[index].to;
    if(g->current_degree[FlattenIndex(from.x, from.y, g->columns)]
           + count_remaining_edges(g, from, index) >= from.degree &&
       g->current_degree[FlattenIndex(to.x, to.y, g->columns)]
           + count_remaining_edges(g, to, index) >= to.degree){
        if(backtrack(g,index+1)){
            return true;
        }
    }
    return false; /* Neither branch produced a feasible layout. */
}



/*
 * Read n and m, build the candidate graph, and print one layout.
 * The drawing, or the line "No Solution", follows "The answer is:".
 */
int main(void){
    Graph g;
    int rows,columns;
    scanf("%d %d",&rows,&columns);
    init_Graph(&g,rows,columns);
    printf("The answer is:\n");
    if(backtrack(&g,0)){
        show_result(&g);
    }
    else{
        printf("No Solution\n");
    }
}
