/*
 * Exact O(n + W^3) 0/1 knapsack, W = maximum usable item weight.
 * Polak, Rohwedder, Wegrzycki, ICALP 2021, Lemmas 2.1/2.2, Theorem 1.1.
 * A linear-time density prefix and proximity reduce both correction DPs to
 * O(W^2) weights. Concave weight-group transitions use linear-time SMAWK.
 * No capacity-sized table or full n-item comparison sort is used.
 */
#ifndef KNAPSACK_W3_H
#define KNAPSACK_W3_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#define KW3_NEG (INT64_MIN / 4)
typedef struct { int p, w, id; } KW3Item;

/* Return -1 when a precedes b. Cross products are exact for int inputs.
 * Unique IDs make density ties deterministic, including zero profits. */
static int kw3_compare(KW3Item a, KW3Item b, int density) {
    int64_t x = density ? (int64_t)a.p * b.w : a.p;
    int64_t y = density ? (int64_t)b.p * a.w : b.p;
    if (x != y) return x > y ? -1 : 1;
    return (a.id > b.id) - (a.id < b.id);
}
static void kw3_swap(KW3Item *a, KW3Item *b) {
    KW3Item t = *a; *a = *b; *b = t;
}
static void kw3_small_sort(KW3Item *a, size_t n, int density) {
    for (size_t i = 1; i < n; ++i)
        for (size_t j = i; j && kw3_compare(a[j], a[j-1], density) < 0; --j)
            kw3_swap(a+j, a+j-1);
}
/* Three-way partition, with the equal interval [left,right). */
static void kw3_partition(KW3Item *a, size_t n, KW3Item pivot, int density,
                          size_t *left, size_t *right) {
    size_t lo = 0, i = 0, hi = n;
    while (i < hi) {
        int c = kw3_compare(a[i], pivot, density);
        if (c < 0) kw3_swap(a+i++, a+lo++);
        else if (c > 0) kw3_swap(a+i, a+--hi);
        else ++i;
    }
    *left = lo; *right = hi;
}
/* BFPRT median-of-five selection: worst-case linear, rather than a sorted
 * density list or expected-linear quickselect. Also used for group trimming. */
static KW3Item kw3_select(KW3Item *a, size_t n, size_t k, int density) {
    for (;;) {
        if (n <= 5) { kw3_small_sort(a, n, density); return a[k]; }
        size_t medians = 0;
        for (size_t i = 0; i < n; i += 5) {
            size_t len = n-i < 5 ? n-i : 5;
            kw3_small_sort(a+i, len, density);
            kw3_swap(a+medians++, a+i+len/2);
        }
        KW3Item pivot = kw3_select(a, medians, medians/2, density);
        size_t lo, hi;
        kw3_partition(a, n, pivot, density, &lo, &hi);
        if (k < lo) n = lo;
        else if (k < hi) return a[k];
        else { a += hi; n -= hi; k -= hi; }
    }
}
static int kw3_profit_order(const void *a, const void *b) {
    return kw3_compare(*(const KW3Item *)a, *(const KW3Item *)b, 0);
}

typedef struct {
    const int64_t *old, *gain;
    size_t h, residue, count;
} KW3Matrix;

/* M[i,j] = old[r+j*h] + gain[i-j], for 0 <= i-j <= count.
 * Keep ONLY reachable old columns. To preserve total monotonicity even in
 * unreachable rows, extend the concave gain by huge positive/negative slopes
 * outside its support. Represent -M*distance + base lexicographically; this
 * avoids overflow and the incorrect all-minus-infinity tie handling common
 * in naive banded SMAWK implementations. */
static void kw3_entry(const KW3Matrix *m, size_t i, size_t j,
                      size_t *distance, int64_t *base) {
    size_t q;
    if (j > i) { *distance = j-i; q = 0; }
    else if (i-j > m->count) { *distance = i-j-m->count; q = m->count; }
    else { *distance = 0; q = i-j; }
    *base = m->old[m->residue+j*m->h] + m->gain[q];
}
static int kw3_better(const KW3Matrix *m, size_t row, size_t a, size_t b) {
    size_t da, db; int64_t va, vb;
    kw3_entry(m, row, a, &da, &va);
    kw3_entry(m, row, b, &db, &vb);
    return da != db ? da < db : va > vb;
}
/* Find leftmost maxima of an implicit totally monotone matrix in O(rows+cols).
 * Reduce columns, recurse on odd rows, interpolate even rows between their
 * neighboring maxima. Recursive workspace has geometric total size. */
static int kw3_smawk(const KW3Matrix *m, const size_t *rows, size_t nr,
                     const size_t *cols, size_t nc, size_t *answer) {
    if (!nr) return 1;
    size_t *reduced = malloc(nr * sizeof(*reduced));
    size_t *odd = malloc((nr/2 + 1) * sizeof(*odd));
    if (!reduced || !odd) { free(reduced); free(odd); return 0; }
    size_t used = 0;
    for (size_t c = 0; c < nc; ++c) {
        while (used && kw3_better(m, rows[used-1], cols[c], reduced[used-1])) --used;
        if (used < nr) reduced[used++] = cols[c];
    }
    for (size_t i = 1; i < nr; i += 2) odd[i/2] = rows[i];
    if (!kw3_smawk(m, odd, nr/2, reduced, used, answer)) {
        free(reduced); free(odd); return 0;
    }
    size_t start = 0;
    for (size_t i = 0; i < nr; i += 2) {
        size_t end = start;
        if (i+1 < nr) {
            while (reduced[end] != answer[rows[i+1]]) ++end;
        } else end = used-1;
        size_t best = start;
        for (size_t c = start+1; c <= end; ++c)
            if (kw3_better(m, rows[i], reduced[c], reduced[best])) best = c;
        answer[rows[i]] = reduced[best];
        start = end;
    }
    free(reduced); free(odd); return 1;
}

/* Compute exact-weight DP for additions (sign=1) or negative-profit removals
 * (sign=-1). Equal-weight profits sorted descending have concave prefix sums.
 * BFPRT keeps only min(2W, limit/h) best items before sorting. Across groups
 * the retained count is O(W^2); sorting costs O(W^2 log W) <= O(W^3). */
static int64_t *kw3_dp(const KW3Item *items, size_t n, size_t W,
                       size_t limit, int sign) {
    size_t len = limit+1;
    int64_t *old = NULL, *next = NULL, *gain = NULL;
    size_t *offset = NULL, *cursor = NULL, *rows = NULL, *cols = NULL, *ans = NULL;
    KW3Item *grouped = NULL;
    old = malloc(len*sizeof(*old)); next = malloc(len*sizeof(*next));
    gain = malloc((2*W+1)*sizeof(*gain));
    offset = calloc(W+2, sizeof(*offset)); cursor = malloc((W+1)*sizeof(*cursor));
    rows = malloc(len*sizeof(*rows)); cols = malloc(len*sizeof(*cols));
    ans = malloc(len*sizeof(*ans)); grouped = malloc((n ? n : 1)*sizeof(*grouped));
    if (!old || !next || !gain || !offset || !cursor || !rows || !cols || !ans || !grouped)
        goto fail;
    for (size_t i = 0; i < n; ++i) ++offset[(size_t)items[i].w+1];
    for (size_t h = 1; h <= W+1; ++h) offset[h] += offset[h-1];
    for (size_t h = 1; h <= W; ++h) cursor[h] = offset[h];
    for (size_t i = 0; i < n; ++i) {
        KW3Item x = items[i]; x.p *= sign;
        grouped[cursor[x.w]++] = x;
    }
    for (size_t t = 0; t < len; ++t) old[t] = KW3_NEG;
    old[0] = 0;
    for (size_t h = 1; h <= W && h <= limit; ++h) {
        size_t count = offset[h+1]-offset[h], keep = limit/h;
        if (keep > 2*W) keep = 2*W;
        if (keep > count) keep = count;
        if (!keep) continue;
        KW3Item *g = grouped+offset[h];
        if (keep < count) kw3_select(g, count, keep-1, 0);
        qsort(g, keep, sizeof(*g), kw3_profit_order);
        gain[0] = 0;
        for (size_t q = 1; q <= keep; ++q) gain[q] = gain[q-1]+g[q-1].p;
        for (size_t r = 0; r < h && r < len; ++r) {
            size_t nr = (limit-r)/h+1, nc = 0;
            for (size_t i = 0; i < nr; ++i) {
                rows[i] = i;
                if (old[r+i*h] != KW3_NEG) cols[nc++] = i;
            }
            if (!nc) {
                for (size_t i = 0; i < nr; ++i) next[r+i*h] = KW3_NEG;
                continue;
            }
            KW3Matrix m = {old, gain, h, r, keep};
            if (!kw3_smawk(&m, rows, nr, cols, nc, ans)) goto fail;
            for (size_t i = 0; i < nr; ++i) {
                size_t d; int64_t value;
                kw3_entry(&m, i, ans[i], &d, &value);
                next[r+i*h] = d ? KW3_NEG : value;
            }
        }
        int64_t *tmp = old; old = next; next = tmp;
    }
    free(next); free(gain); free(offset); free(cursor);
    free(rows); free(cols); free(ans); free(grouped); return old;
fail:
    free(old); free(next); free(gain); free(offset); free(cursor);
    free(rows); free(cols); free(ans); free(grouped); return NULL;
}

/* Same ABI as the other project solvers; inputs are not modified.
 * Positive int weights, nonnegative int profits, total profit <= INT_MAX.
 * Return -1 with errno on invalid input, allocation failure or size overflow.
 * Working space O(n+W^2); deterministic time O(n+W^3) in the word-RAM model. */
static int knapsack_w3(int number, int capacity, int *profits, int *weights) {
    if (number < 0 || capacity < 0 || (number && (!profits || !weights))) {
        errno = EINVAL; return -1;
    }
    KW3Item *a = NULL;
    int64_t *add = NULL, *del = NULL, total_profit = 0, total_weight = 0;
    size_t n = 0, W = 0;
    if ((size_t)number > SIZE_MAX/sizeof(*a)) { errno = EOVERFLOW; return -1; }
    a = malloc((number ? (size_t)number : 1)*sizeof(*a));
    if (!a) { errno = ENOMEM; return -1; }
    for (int i = 0; i < number; ++i) {
        if (weights[i] <= 0 || profits[i] < 0) { errno = EINVAL; goto fail; }
        total_profit += profits[i];
        if (total_profit > INT_MAX) { errno = EOVERFLOW; goto fail; }
        /* Overweight and zero-profit items cannot improve the optimum. */
        if (weights[i] > capacity || !profits[i]) continue;
        a[n++] = (KW3Item){profits[i], weights[i], i};
        total_weight += weights[i];
        if ((size_t)weights[i] > W) W = weights[i];
    }
    if (!n) { free(a); return 0; }
    if (total_weight <= capacity) {
        int value = 0;
        for (size_t i = 0; i < n; ++i) value += a[i].p;
        free(a); return value;
    }
    /* Weighted selection constructs a maximal density prefix in linear time.
     * Selected items occupy [0,lo); when the pivot does not fit, STOP, rather
     * than skipping it as in a different greedy heuristic. */
    size_t lo = 0, hi = n;
    int64_t delta = capacity, prefix_profit = 0;
    while (lo < hi) {
        KW3Item pivot = kw3_select(a+lo, hi-lo, (hi-lo)/2, 1);
        size_t left, right;
        kw3_partition(a+lo, hi-lo, pivot, 1, &left, &right);
        int64_t weight = 0, profit = 0;
        for (size_t j = lo; j < lo+left; ++j) { weight += a[j].w; profit += a[j].p; }
        if (weight > delta) { hi = lo+left; continue; }
        lo += left; delta -= weight; prefix_profit += profit;
        if (a[lo].w > delta) break;
        delta -= a[lo].w; prefix_profit += a[lo].p; ++lo;
    }
    /* Proximity: <=2W changed items, hence <=2W^2 removed weight. Additions
     * need at most that plus delta. Include k=0 (addition-only corrections). */
    if (W > (SIZE_MAX-(size_t)delta-1)/2/W) { errno = EOVERFLOW; goto fail; }
    size_t L = 2*W*W, limit = L+(size_t)delta;
    if (limit+1 > SIZE_MAX/sizeof(int64_t) || limit+1 > SIZE_MAX/sizeof(size_t)) {
        errno = EOVERFLOW; goto fail;
    }
    del = kw3_dp(a, lo, W, L, -1);
    add = kw3_dp(a+lo, n-lo, W, limit, 1);
    if (!del || !add) { errno = ENOMEM; goto fail; }
    /* Convert exact additions A[t] to at-most additions A_le[t]. */
    for (size_t t = 1; t <= limit; ++t) if (add[t-1] > add[t]) add[t] = add[t-1];
    int64_t best = prefix_profit;
    for (size_t k = 0; k <= L; ++k) if (del[k] != KW3_NEG) {
        int64_t value = prefix_profit+del[k]+add[k+(size_t)delta];
        if (value > best) best = value;
    }
    free(a); free(add); free(del); return (int)best;
fail:
    free(a); free(add); free(del); return -1;
}
#endif
