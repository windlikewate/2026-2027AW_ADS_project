#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * Recover the Design.
 * The board is stored as a flat array of cells. init_Graph links every
 * connector to its nearest visible neighbor on the right and below, so
 * each possible fence is one undirected edge. backtrack, in the .c file,
 * then chooses a subset of those edges. A subset is kept only when every
 * connector has its required degree and no two chosen fences cross in an
 * empty cell. The search tries taking an edge before skipping it, and it
 * drops a branch as soon as a degree can no longer be met.
 */
#define horizontal 0
#define vertical 1

typedef struct{
    int degree;
    int x;
    int y;
}Vertex; /* degree is the required number of fences; (x, y) is 0-based. */


typedef struct{
    Vertex from;
    Vertex to;
    int type;
    bool is_selected; /* True when this fence is taken on the current path. */
}Edge; /* from is the left or upper end; type is horizontal or vertical. */


typedef struct{
    int rows;
    int columns;
    int edge_num;
    int vertex_num;
    int *current_degree; /* Fences taken so far at each flattened cell. */
    Vertex* vertices;
    Edge* edges;
}Graph; /* vertices[] covers every cell; edges[] holds only candidates. */


typedef struct{
    int x;
    int y;
    bool left;
    bool right;
    bool up;
    bool down;
}result; /* One output record: row, column, and the four direction flags. */



int FlattenIndex(int x, int y, int columns) {
    return x * columns + y;
} /* Row-major index x * columns + y. */


/*
 * Read the degree matrix and build the candidate list.
 * A connector emits at most one edge to the right and one edge downward,
 * each ending at the nearest connector in that direction.
 */
void init_Graph(Graph* g, int rows,int columns){
    g->rows = rows;
    g->columns = columns;
    g->edge_num = 0;
    g->vertex_num = 0;
    g->current_degree = (int*)malloc(rows * columns * sizeof(int)); /* One counter per cell. */
    for(int i = 0; i < rows * columns; i++){
        g->current_degree[i] = 0;
    }
    g->vertices = (Vertex*)malloc(rows * columns * sizeof(Vertex)); /* One slot per cell, empty or not. */
    g->edges = (Edge*)malloc(rows * columns *4 * sizeof(Edge)); /* Upper bound: four stubs per cell. */
    /* Read the n by m degree matrix. A positive entry is a connector. */
    for(int i = 0 ; i < g->rows ; i++){
        for(int j = 0 ; j < g->columns ; j++){
            int degree;
            scanf("%d",&degree);
            if(degree > 0){
                g->vertex_num++;
                Vertex v = {degree, i, j};
                g->vertices[FlattenIndex(i, j, g->columns)] = v; /* Store the connector in row-major order. */
            }
            else{
                Vertex v = {0, i, j};
                g->vertices[FlattenIndex(i, j, g->columns)] = v; 
            }
        }
    }
    /* Second pass: emit the rightward edge, then the downward edge. */
    /* Empty cells emit nothing. Each candidate is stored once. */
    for(int k = 0;k < rows*columns;k++){
        Vertex now_vertex = g->vertices[k];
        if(now_vertex.degree > 0){
            int x = now_vertex.x;
            int y = now_vertex.y;
            /* Horizontal candidate: nearest connector strictly to the right. */
            int right_y = y + 1;
            while(right_y < g->columns && g->vertices[FlattenIndex(x, right_y, g->columns)].degree == 0){
                right_y++;
            }
            if(right_y < g->columns){ /* A connector was found before the row ended. */
                Vertex right_vertex = g->vertices[FlattenIndex(x, right_y, g->columns)];
                Edge e = {now_vertex, right_vertex, horizontal,false};
                g->edges[g->edge_num] = e; /* Append; from is the left endpoint. */
                g->edge_num++;
            }
            /* Vertical candidate: nearest connector strictly below. */
            int down_x = x + 1;
            while(down_x < g->rows && g->vertices[FlattenIndex(down_x, y, g->columns)].degree == 0){
                down_x++;
            }
            if(down_x < g->rows){ /* A connector was found before the column ended. */
                Vertex down_vertex = g->vertices[FlattenIndex(down_x, y, g->columns)];
                Edge e = {now_vertex, down_vertex, vertical,false};
                g->edges[g->edge_num] = e; /* Append; from is the upper endpoint. */
                g->edge_num++;
            }
        }
    }
}

/*
 * Two fences cross only when one is horizontal and one is vertical and
 * their open interiors share a cell. Sharing an endpoint is not a cross.
 */
bool is_crossing(Edge a, Edge b) {
    /* Parallel fences share a row or a column, so they do not cross. */
    if (a.type == b.type) {
        return false;
    }

    /* Name the horizontal edge h and the vertical edge v. */
    Edge h = (a.type == horizontal) ? a : b;
    Edge v = (a.type == vertical) ? a : b;

    /* h runs along row hx from column hy1 to hy2. */
    /* v runs along column vy from row vx1 to vx2. */

    int hx = h.from.x;
    int hy1 = h.from.y;
    int hy2 = h.to.y;

    int vy = v.from.y;
    int vx1 = v.from.x;
    int vx2 = v.to.x;

    /* Open interval on both axes: the shared point is an empty cell. */
    return (hy1 < vy && vy < hy2 &&
            vx1 < hx && hx < vx2);
}

/*
 * How many candidate edges after index are incident to v.
 * Those are the only edges that can still raise v's current degree.
 */
int count_remaining_edges(Graph* g, Vertex v, int index) {
    int count = 0;

    /* Edges at index or earlier have already been decided. */
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

/*
 * Validate taking edges[index]. Three tests, in order:
 * the edge must not push either endpoint past its required degree,
 * each endpoint must still be able to reach that degree, and
 * the edge must not cross a fence already taken.
 * The skip branch is tested in backtrack, not here.
 */
bool check_edge(Graph* g,int index){
    if(g->current_degree[FlattenIndex(g->edges[index].from.x, g->edges[index].from.y, g->columns)] > g->edges[index].from.degree || g->current_degree[FlattenIndex(g->edges[index].to.x, g->edges[index].to.y, g->columns)] > g->edges[index].to.degree){
        return false;
    } /* Taken fences at an endpoint must not exceed its required degree. */
    if(g->current_degree[FlattenIndex(g->edges[index].from.x, g->edges[index].from.y, g->columns)]+count_remaining_edges(g, g->edges[index].from, index) < g->edges[index].from.degree || g->current_degree[FlattenIndex(g->edges[index].to.x, g->edges[index].to.y, g->columns)]+count_remaining_edges(g, g->edges[index].to, index) < g->edges[index].to.degree){
        return false;
    } /* Current fences plus later candidates must still cover the degree. */
    /* Compare the new edge with every edge already marked selected. */
    for(int i = 0;i < g->edge_num;i++){
        if(g->edges[i].is_selected && is_crossing(g->edges[i],g->edges[index])){
            return false;
        } /* A crossing with an already taken fence rejects this edge. */
    }
    return true; /* All three tests passed, so the edge may be taken. */
}

/*
 * Turn taken edges into up/down/left/right flags and print one line per
 * connector, in row-major order, with coordinates starting at 1.
 */
void show_result(Graph* g){
    result* res = (result*)calloc(g->rows* g->columns,sizeof(result)); /* Flags start cleared. */
    int res_index = 0;

    /* Fill coordinates first; direction flags stay false until a fence sets them. */
    for(int i = 0 ; i < g-> rows;i++){
        for(int j = 0 ; j < g->columns;j++){
            res_index = FlattenIndex(i, j, g->columns);
            res[res_index].x = i;
            res[res_index].y = j;
        }
    } /* Record the 0-based coordinates of every cell. */
    /* A horizontal fence sets right and left; a vertical fence sets down and up. */
    for(int i = 0;i < g->edge_num;i++){
        if(g->edges[i].is_selected){
            int from_index = FlattenIndex(g->edges[i].from.x, g->edges[i].from.y, g->columns);
            int to_index = FlattenIndex(g->edges[i].to.x, g->edges[i].to.y, g->columns);
            /* Horizontal: left end points right, right end points left. */
            if(g->edges[i].type == horizontal){
                res[from_index].right = true;
                res[to_index].left = true;
            }
            else{ /* Vertical: upper end points down, lower end points up. */
                res[from_index].down = true;
                res[to_index].up = true;
            }
        }
    }

    /* Print connectors only, scanning the board from top to bottom. */
    for(int i = 0 ; i < g-> rows;i++){
        for(int j = 0 ; j < g->columns;j++){
            res_index = FlattenIndex(i, j, g->columns);
            if(g->vertices[res_index].degree == 0){
                continue;
            } /* Empty cells are not printed. */
            /* Coordinates in the output start at 1, not 0. */
            printf("%d %d",res[res_index].x+1,res[res_index].y+1);
            printf(" %d %d %d %d",res[res_index].up,res[res_index].down,res[res_index].left,res[res_index].right);
            printf("\n"); /* row col up down left right, one connector per line. */
        }
    }

}