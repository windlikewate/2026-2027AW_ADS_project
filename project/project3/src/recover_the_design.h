#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// 对于这个恢复栅栏结构问题，基本的想法是转化为一个图的遍历问题。读入
// 所有节点之后首先连接所有可能的边，随后使用回溯算法逐步选择边，直到所有的边都被选择或者没有可选的边为止。每次选择一条边之后，检查当前的图是否满足条件，如果不满足条件则回溯。
#define horizontal 0
#define vertical 1

typedef struct{
    int degree;
    int x;
    int y;
}Vertex; //Store the degree and coordinates of each vertex

typedef struct{
    Vertex from;
    Vertex to;
    int type;
    bool is_selected; //Store whether the edge has been selected or not
}Edge; //Store the start point, end point, and type of each edge

typedef struct{
    int rows;
    int columns;
    int edge_num;
    int vertex_num;
    int *current_degree; //Store the current degree of each vertex
    Vertex* vertices;
    Edge* edges;
}Graph; //Store the number of vertices and edges, and the arrays of vertices and edges

typedef struct{
    int x;
    int y;
    bool left;
    bool right;
    bool up;
    bool down;
}result; //Store the coordinates and directions of each edge in the result


int FlattenIndex(int x, int y, int columns) {
    return x * columns + y;
} // Convert 2D coordinates to 1D index

void init_Graph(Graph* g, int rows,int columns){
    g->rows = rows;
    g->columns = columns;
    g->edge_num = 0;
    g->vertex_num = 0;
    g->current_degree = (int*)malloc(rows * columns * sizeof(int)); //allocate enough space for current degrees
    for(int i = 0; i < rows * columns; i++){
        g->current_degree[i] = 0;
    }
    g->vertices = (Vertex*)malloc(rows * columns * sizeof(Vertex)); //allocate enough space for vertices, assuming each cell can have a vertex
    g->edges = (Edge*)malloc(rows * columns *4 * sizeof(Edge)); //allocate enough space for edges, assuming each vertex can have up to 4 edges
    for(int i = 0 ; i < g->rows ; i++){
        for(int j = 0 ; j < g->columns ; j++){
            int degree;
            scanf("%d",&degree);
            if(degree > 0){
                g->vertex_num++;
                Vertex v = {degree, i, j};
                g->vertices[FlattenIndex(i, j, g->columns)] = v; // flatten the 2D array into 1D array
            }
            else{
                Vertex v = {0, i, j};
                g->vertices[FlattenIndex(i, j, g->columns)] = v; 
            }
        }
    }
    for(int k = 0;k < rows*columns;k++){
        Vertex now_vertex = g->vertices[k];
        if(now_vertex.degree > 0){
            int x = now_vertex.x;
            int y = now_vertex.y;
            // deal with horizontal edges,only need to find the nearest vertex on the right
            int right_y = y + 1;
            while(right_y < g->columns && g->vertices[FlattenIndex(x, right_y, g->columns)].degree == 0){
                right_y++;
            }
            if(right_y < g->columns){ // if there is a vertex on the right, add an edge
                Vertex right_vertex = g->vertices[FlattenIndex(x, right_y, g->columns)];
                Edge e = {now_vertex, right_vertex, horizontal,false};
                g->edges[g->edge_num] = e; // add the edge to the edges array
                g->edge_num++;
            }
            // deal with vertical edges, only need to find the nearest vertex below
            int down_x = x + 1;
            while(down_x < g->rows && g->vertices[FlattenIndex(down_x, y, g->columns)].degree == 0){
                down_x++;
            }
            if(down_x < g->rows){ // if there is a vertex below, add an edge
                Vertex down_vertex = g->vertices[FlattenIndex(down_x, y, g->columns)];
                Edge e = {now_vertex, down_vertex, vertical,false};
                g->edges[g->edge_num] = e; // add the edge to the edges array
                g->edge_num++;
            }
        }
    }
}

bool is_crossing(Edge a, Edge b) {// judge whether two edges cross each other
    if (a.type == b.type) {
        return false;
    }

    Edge h = (a.type == horizontal) ? a : b;
    Edge v = (a.type == vertical) ? a : b;

    int hx = h.from.x;
    int hy1 = h.from.y;
    int hy2 = h.to.y;

    int vy = v.from.y;
    int vx1 = v.from.x;
    int vx2 = v.to.x;

    return (hy1 < vy && vy < hy2 &&
            vx1 < hx && hx < vx2);
}

int count_remaining_edges(Graph* g, Vertex v, int index) {// Count the number of edges connected to vertex v that have not been selected yet
    int count = 0;

    for (int i = index + 1; i < g->edge_num; i++) {
        if ((g->edges[i].from.x == v.x &&
             g->edges[i].from.y == v.y) ||
            (g->edges[i].to.x == v.x &&
             g->edges[i].to.y == v.y)) {
            count++;
        }
    }

    return count;
}

// if we add the edge, we need to check whether the degree of the vertex is less than 0 or the number of edges connected to the vertex is less than the degree of the vertex, and whether the picked edge crosses with any other picked edge. If any of these conditions are met, return false.
// if we don't add the edge, the check will be done in the .c,only case2 needs to be checked
bool check_edge(Graph* g,int index){
    // Check if the picked edges are valid
    if(g->current_degree[FlattenIndex(g->edges[index].from.x, g->edges[index].from.y, g->columns)] > g->edges[index].from.degree || g->current_degree[FlattenIndex(g->edges[index].to.x, g->edges[index].to.y, g->columns)] > g->edges[index].to.degree){
        return false;
    }// if add the edge will make the degree of the vertex less than 0, return false
    if(g->current_degree[FlattenIndex(g->edges[index].from.x, g->edges[index].from.y, g->columns)]+count_remaining_edges(g, g->edges[index].from, index) < g->edges[index].from.degree || g->current_degree[FlattenIndex(g->edges[index].to.x, g->edges[index].to.y, g->columns)]+count_remaining_edges(g, g->edges[index].to, index) < g->edges[index].to.degree){
        return false;
    }// if the number of edges connected to the vertex is less than the degree of the vertex, return false
    for(int i = 0;i < g->edge_num;i++){
        if(g->edges[i].is_selected && is_crossing(g->edges[i],g->edges[index])){
            return false;
        }// if the picked edge crosses with any other picked edge, return false
    }
    return true;
}

void show_result(Graph* g){
    result* res = (result*)calloc(g->rows* g->columns,sizeof(result)); //allocate enough space for the result
    int res_index = 0;

    for(int i = 0 ; i < g-> rows;i++){
        for(int j = 0 ; j < g->columns;j++){
            res_index = FlattenIndex(i, j, g->columns);
            res[res_index].x = i;
            res[res_index].y = j;
        }
    }// initialize the coordinates of the result array
    for(int i = 0;i < g->edge_num;i++){
        if(g->edges[i].is_selected){
            int from_index = FlattenIndex(g->edges[i].from.x, g->edges[i].from.y, g->columns);
            int to_index = FlattenIndex(g->edges[i].to.x, g->edges[i].to.y, g->columns);
            if(g->edges[i].type == horizontal){
                res[from_index].right = true;
                res[to_index].left = true;
            }
            else{
                res[from_index].down = true;
                res[to_index].up = true;
            }
        }
    }

    for(int i = 0 ; i < g-> rows;i++){
        for(int j = 0 ; j < g->columns;j++){
            res_index = FlattenIndex(i, j, g->columns);
            if(g->vertices[res_index].degree == 0){
                continue;
            }// if the vertex is not a connector, skip it
            printf("%d %d",res[res_index].x+1,res[res_index].y+1);
            printf(" %d %d %d %d",res[res_index].up,res[res_index].down,res[res_index].left,res[res_index].right);
            printf("\n"); //else print the coordinates and directions of the edges connected to the vertex
        }
    }

}