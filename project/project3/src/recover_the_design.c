#include "recover_the_design.h"
bool backtrack(Graph* g,int index){
    // Every selected edge contributes two to the total degree.
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
        }// in this case, all edges have been selected, check if the degree of each vertex is equal to the degree of the vertex
        return true;
    }
    // Try to select the edge
    g->edges[index].is_selected = true;
    g->current_degree[FlattenIndex(g->edges[index].from.x, g->edges[index].from.y, g->columns)]++;
    g->current_degree[FlattenIndex(g->edges[index].to.x, g->edges[index].to.y, g->columns)]++;
    if(check_edge(g,index)){
        if(backtrack(g,index+1)){
            return true;
        }
    }
    // Try to not select the edge
    g->edges[index].is_selected = false;
    g->current_degree[FlattenIndex(g->edges[index].from.x, g->edges[index].from.y, g->columns)]--;
    g->current_degree[FlattenIndex(g->edges[index].to.x, g->edges[index].to.y, g->columns)]--;
    /* Reject this branch immediately if either endpoint can no longer
    reach its required degree, rather than waiting for the leaf.*/
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
    return false;
}


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
