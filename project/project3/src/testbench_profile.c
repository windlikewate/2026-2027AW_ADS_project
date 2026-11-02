/*
 * File: testbench_profile.c
 * Description: Instrumented replay of the solver in recover_the_design.c.
 *              Reads one instance from stdin, runs the same pruned search
 *              while counting recursive calls, and prints one summary line:
 *              rows cols |V| |E| |E*| calls time_us solved
 * Build:       gcc -O2 -o testbench_profile testbench_profile.c
 * Ablation:    add -DNO_LOOKAHEAD to drop the lower-bound test on the
 *              exclusion branch; the search gives up after CALL_LIMIT calls.
 */

#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include "recover_the_design.h"

#define CALL_LIMIT 20000000LL     /* Abort budget, reported as solved = -1 */

static long long call_count = 0; /* Number of search-tree nodes visited */

/*
 * Identical to backtrack() in recover_the_design.c except for the counter.
 */
bool backtrack_counted(Graph* g, int index) {
    call_count++;

    /* Root parity check (handshaking lemma) */
    if (index == 0) {
        int degree_sum = 0;
        for (int i = 0; i < g->rows * g->columns; i++) {
            degree_sum += g->vertices[i].degree;
        }
        if (degree_sum % 2 != 0) {
            return false;
        }
    }

    /* Leaf: every connector must meet its degree exactly */
    if (index == g->edge_num) {
        for (int i = 0; i < g->rows * g->columns; i++) {
            if (g->current_degree[i] != g->vertices[i].degree) {
                return false;
            }
        }
        return true;
    }

    int from_idx = FlattenIndex(
        g->edges[index].from.x, g->edges[index].from.y, g->columns);
    int to_idx = FlattenIndex(
        g->edges[index].to.x, g->edges[index].to.y, g->columns);

    /* Inclusion branch */
    g->edges[index].is_selected = true;
    g->current_degree[from_idx]++;
    g->current_degree[to_idx]++;
    if (check_edge(g, index) && call_count < CALL_LIMIT &&
        backtrack_counted(g, index + 1)) {
        return true;
    }

    /* Exclusion branch with lookahead lower bound */
    g->edges[index].is_selected = false;
    g->current_degree[from_idx]--;
    g->current_degree[to_idx]--;
    Vertex from = g->edges[index].from;
    Vertex to = g->edges[index].to;
#ifdef NO_LOOKAHEAD
    /* Ablation: always explore the exclusion branch */
    bool lookahead_ok = true;
#else
    bool lookahead_ok =
        g->current_degree[from_idx] +
            count_remaining_edges(g, from, index) >= from.degree &&
        g->current_degree[to_idx] +
            count_remaining_edges(g, to, index) >= to.degree;
#endif
    if (lookahead_ok && call_count < CALL_LIMIT &&
        backtrack_counted(g, index + 1)) {
        return true;
    }
    return false;
}

int main(void) {
    Graph g;
    int rows, columns;
    if (scanf("%d %d", &rows, &columns) != 2) {
        return 1;
    }
    init_Graph(&g, rows, columns);

    /* Time only the search, not input parsing */
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    bool ok = backtrack_counted(&g, 0);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double us = (t1.tv_sec - t0.tv_sec) * 1e6 + (t1.tv_nsec - t0.tv_nsec) / 1e3;

    /* Count fences in the returned drawing */
    int selected = 0;
    for (int i = 0; i < g.edge_num; i++) {
        selected += g.edges[i].is_selected;
    }
    int status = ok ? 1 : (call_count >= CALL_LIMIT ? -1 : 0);
    printf("%d %d %d %d %d %lld %.3f %d\n", rows, columns, g.vertex_num,
           g.edge_num, ok ? selected : 0, call_count, us, status);
    return 0;
}
