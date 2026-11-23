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

/* Sentinel for unreachable DP states, chosen far from INT64_MIN to avoid overflow. */
#define KW3_NEG (INT64_MIN / 4)

/* Item record storing profit, weight, and original 0-based index for tie-breaking. */
typedef struct { int p, w, id; } KW3Item;

/* Return -1 when a precedes b. Cross products are exact for int inputs.
 * Unique IDs make density ties deterministic, including zero profits.
 * density != 0 compares p/w; density == 0 compares raw profit p. */
static int kw3_compare(KW3Item a, KW3Item b, int density) {
    /* Compute 64-bit cross products when comparing profit-to-weight ratios. */
    int64_t x = density ? (int64_t)a.p * b.w : a.p;
    int64_t y = density ? (int64_t)b.p * a.w : b.p;
    /* Larger ratio or larger profit comes first in descending order. */
    if (x != y) return x > y ? -1 : 1;
    /* Break equal ratios or equal profits deterministically by original index. */
    return (a.id > b.id) - (a.id < b.id);
}

/* Swap two item records in place using a temporary copy. */
static void kw3_swap(KW3Item *a, KW3Item *b) {
    KW3Item t = *a; *a = *b; *b = t;
}

/* Insertion sort for small groups of at most 5 elements inside BFPRT. */
static void kw3_small_sort(KW3Item *a, size_t n, int density) {
    /* Iterate over each prefix boundary from 1 up to n - 1. */
    for (size_t i = 1; i < n; ++i)
        /* Shift element a[j] left until its sorted position is reached. */
        for (size_t j = i; j && kw3_compare(a[j], a[j-1], density) < 0; --j)
            kw3_swap(a+j, a+j-1);
}

/* Three-way partition, with the equal interval [left,right).
 * Elements before left precede pivot; elements from right onward follow pivot. */
static void kw3_partition(KW3Item *a, size_t n, KW3Item pivot, int density,
                          size_t *left, size_t *right) {
    size_t lo = 0, i = 0, hi = n;
    /* Scan unprocessed slice [i, hi) and place items into <, ==, or > regions. */
    while (i < hi) {
        int c = kw3_compare(a[i], pivot, density);
        if (c < 0) kw3_swap(a+i++, a+lo++);
        else if (c > 0) kw3_swap(a+i, a+--hi);
        else ++i;
    }
    /* Output boundaries of the middle region equal to pivot. */
    *left = lo; *right = hi;
}

/* BFPRT median-of-five selection: worst-case linear, rather than a sorted
 * density list or expected-linear quickselect. Also used for group trimming. */
static KW3Item kw3_select(KW3Item *a, size_t n, size_t k, int density) {
    for (;;) {
        /* Base case: sort at most 5 elements directly and return rank k. */
        if (n <= 5) { kw3_small_sort(a, n, density); return a[k]; }
        size_t medians = 0;
        /* Find the median of each 5-element block and move it to the front. */
        for (size_t i = 0; i < n; i += 5) {
            size_t len = n-i < 5 ? n-i : 5;
            kw3_small_sort(a+i, len, density);
            kw3_swap(a+medians++, a+i+len/2);
        }
        /* Recursively select the median of block medians as partition pivot. */
        KW3Item pivot = kw3_select(a, medians, medians/2, density);
        size_t lo, hi;
        kw3_partition(a, n, pivot, density, &lo, &hi);
        /* Narrow the active slice to whichever side contains index k. */
        if (k < lo) n = lo;
        else if (k < hi) return a[k];
        else { a += hi; n -= hi; k -= hi; }
    }
}

/* Comparator wrapper for qsort to sort retained equal-weight items by profit. */
static int kw3_profit_order(const void *a, const void *b) {
    return kw3_compare(*(const KW3Item *)a, *(const KW3Item *)b, 0);
}

/* Implicit matrix parameters for one weight group h and residue class r mod h. */
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
    /* Compute out-of-band distance when column j lies outside [i-count, i]. */
    if (j > i) { *distance = j-i; q = 0; }
    else if (i-j > m->count) { *distance = i-j-m->count; q = m->count; }
    else { *distance = 0; q = i-j; }
    *base = m->old[m->residue+j*m->h] + m->gain[q];
}

/* Compare two candidate columns a and b in row: smaller distance wins first,
 * and equal distance is broken by larger base value. */
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
    /* Base case: nothing to solve when the row set is empty. */
    if (!nr) return 1;
    /* Allocate reduced-column stack and odd-row index array. */
    size_t *reduced = malloc(nr * sizeof(*reduced));
    size_t *odd = malloc((nr/2 + 1) * sizeof(*odd));
    if (!reduced || !odd) { free(reduced); free(odd); return 0; }
    /* Step 1 (Reduce): prune dominated columns until at most nr columns remain. */
    size_t used = 0;
    for (size_t c = 0; c < nc; ++c) {
        /* Pop previous candidate columns beaten on their corresponding test row. */
        while (used && kw3_better(m, rows[used-1], cols[c], reduced[used-1])) --used;
        if (used < nr) reduced[used++] = cols[c];
    }
    /* Step 2 (Recurse): collect odd-indexed rows and solve them recursively. */
    for (size_t i = 1; i < nr; i += 2) odd[i/2] = rows[i];
    if (!kw3_smawk(m, odd, nr/2, reduced, used, answer)) {
        free(reduced); free(odd); return 0;
    }
    /* Step 3 (Interpolate): scan between adjacent odd-row maxima for even rows. */
    size_t start = 0;
    for (size_t i = 0; i < nr; i += 2) {
        size_t end = start;
        /* Upper search bound comes from the next odd row's optimal column. */
        if (i+1 < nr) {
            while (reduced[end] != answer[rows[i+1]]) ++end;
        } else end = used-1;
        /* Pick the leftmost maximum column inside [start, end]. */
        size_t best = start;
        for (size_t c = start+1; c <= end; ++c)
            if (kw3_better(m, rows[i], reduced[c], reduced[best])) best = c;
        answer[rows[i]] = reduced[best];
        start = end;
    }
    /* Free temporary buffers for this recursion level. */
    free(reduced); free(odd); return 1;
}

/* Compute exact-weight DP for additions (sign=1) or negative-profit removals
 * (sign=-1). Equal-weight profits sorted descending have concave prefix sums.
 * BFPRT keeps only min(2W, limit/h) best items before sorting. Across groups
 * the retained count is O(W^2); sorting costs O(W^2 log W) <= O(W^3). */
static int64_t *kw3_dp(const KW3Item *items, size_t n, size_t W,
                       size_t limit, int sign) {
    size_t max_h = W < limit ? W : limit;
    /* Allocate ping-pong DP arrays, group prefix-sum buffer, and bucket arrays. */
    int64_t *old = malloc((limit+1) * sizeof(*old));
    int64_t *next = malloc((limit+1) * sizeof(*next));
    int64_t *gain = malloc((2*W + 2) * sizeof(*gain));
    size_t *counts = calloc(max_h+1, sizeof(*counts));
    size_t *offsets = calloc(max_h+2, sizeof(*offsets));
    /* Allocate row/column index buffers and output array for SMAWK calls. */
    size_t *rows = malloc((limit+1) * sizeof(*rows));
    size_t *cols = malloc((limit+1) * sizeof(*cols));
    size_t *ans = malloc((limit+1) * sizeof(*ans));
    KW3Item *grouped = malloc((n ? n : 1) * sizeof(*grouped));
    /* Release any partial allocations if out of memory. */
    if (!old || !next || !gain || !counts || !offsets || !rows || !cols || !ans || !grouped)
        goto fail;

    /* Initialize exact-weight DP: weight 0 has profit 0, others unreachable. */
    old[0] = 0;
    for (size_t t = 1; t <= limit; ++t) old[t] = KW3_NEG;

    /* Count items per weight h <= max_h and compute bucket start offsets. */
    for (size_t i = 0; i < n; ++i)
        if ((size_t)items[i].w <= max_h) ++counts[items[i].w];
    /* Convert per-weight counts into prefix-sum bucket boundaries. */
    for (size_t h = 1; h <= max_h; ++h) offsets[h+1] = offsets[h] + counts[h];
    /* Scatter items into contiguous weight buckets with signed profits. */
    for (size_t i = 0; i < n; ++i) if ((size_t)items[i].w <= max_h) {
        size_t pos = offsets[items[i].w]++;
        grouped[pos] = (KW3Item){sign * items[i].p, items[i].w, items[i].id};
    }
    /* Restore starting offset of each weight bucket after scatter pass. */
    for (size_t h = max_h; h >= 1; --h) offsets[h] = offsets[h-1];
    offsets[0] = 0;

    /* Process each weight group h using concave prefix sums and SMAWK. */
    for (size_t h = 1; h <= max_h; ++h) {
        size_t c = counts[h];
        if (!c) continue;
        /* Keep at most min(2W, limit/h) best items in weight group h. */
        size_t keep = 2*W < limit/h ? 2*W : limit/h;
        if (keep > c) keep = c;
        if (!keep) continue;
        /* Trim group h via BFPRT when needed, then sort retained items by profit. */
        KW3Item *g = grouped + offsets[h];
        if (keep < c) kw3_select(g, c, keep-1, 0);
        qsort(g, keep, sizeof(*g), kw3_profit_order);
        /* Build concave prefix-sum array gain[0..keep] of sorted group profits. */
        gain[0] = 0;
        for (size_t k = 0; k < keep; ++k) gain[k+1] = gain[k] + g[k].p;

        /* Update each residue class r mod h independently via SMAWK. */
        for (size_t r = 0; r < h && r <= limit; ++r) {
            size_t nr = (limit-r)/h + 1, nc = 0;
            /* Collect all row indices and retain only reachable old columns. */
            for (size_t i = 0; i < nr; ++i) {
                rows[i] = i;
                if (old[r+i*h] != KW3_NEG) cols[nc++] = i;
            }
            /* Skip SMAWK if no state in this residue class is reachable yet. */
            if (!nc) {
                for (size_t i = 0; i < nr; ++i) next[r+i*h] = KW3_NEG;
                continue;
            }
            /* Solve row maxima on the implicit matrix for weight h, residue r. */
            KW3Matrix m = {old, gain, h, r, keep};
            if (!kw3_smawk(&m, rows, nr, cols, nc, ans)) goto fail;
            /* Write back valid in-band maxima; mark out-of-band rows KW3_NEG. */
            for (size_t i = 0; i < nr; ++i) {
                size_t d; int64_t value;
                kw3_entry(&m, i, ans[i], &d, &value);
                next[r+i*h] = d ? KW3_NEG : value;
            }
        }
        /* Swap old and next DP buffers for the next weight group. */
        int64_t *tmp = old; old = next; next = tmp;
    }
    /* Free temporary buffers and return the final exact-weight DP table. */
    free(next); free(gain); free(counts); free(offsets);
    free(rows); free(cols); free(ans); free(grouped);
    return old;
fail:
    /* Clean up all heap allocations on error and propagate NULL to caller. */
    free(old); free(next); free(gain); free(counts); free(offsets);
    free(rows); free(cols); free(ans); free(grouped); return NULL;
}

/* Same ABI as the other project solvers; inputs are not modified.
 * Positive int weights, nonnegative int profits, total profit <= INT_MAX.
 * Return -1 with errno on invalid input, allocation failure or size overflow.
 * Working space O(n+W^2); deterministic time O(n+W^3). */
static int knapsack_w3(int number, int capacity, int *profits, int *weights) {
    /* Reject negative sizes or null input pointers when number > 0. */
    if (number < 0 || capacity < 0 || (number && (!profits || !weights))) {
        errno = EINVAL; return -1;
    }
    KW3Item *a = NULL; int64_t *add = NULL, *del = NULL;
    size_t n = 0, W = 0;
    int64_t total_profit = 0, total_weight = 0;
    /* Guard item buffer allocation against size_t multiplication overflow. */
    if ((size_t)number > SIZE_MAX/sizeof(*a)) { errno = EOVERFLOW; return -1; }
    a = malloc((number ? (size_t)number : 1)*sizeof(*a));
    if (!a) { errno = ENOMEM; return -1; }
    /* Check input bounds and filter out overweight or zero-profit items. */
    for (int i = 0; i < number; ++i) {
        /* Weights must be positive and profits must be non-negative. */
        if (weights[i] <= 0 || profits[i] < 0) { errno = EINVAL; goto fail; }
        /* Ensure total profit fits inside a signed 32-bit int return value. */
        total_profit += profits[i];
        if (total_profit > INT_MAX) { errno = EOVERFLOW; goto fail; }
        /* Overweight and zero-profit items cannot improve the optimum. */
        if (weights[i] > capacity || !profits[i]) continue;
        /* Append usable item and update total usable weight and max weight W. */
        a[n++] = (KW3Item){profits[i], weights[i], i};
        total_weight += weights[i];
        if ((size_t)weights[i] > W) W = weights[i];
    }
    /* Return early if no usable items remain or if all usable items fit. */
    if (!n) { free(a); return 0; }
    if (total_weight <= capacity) {
        /* Sum all usable item profits when capacity covers every item. */
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
        /* Select the median-density pivot inside the remaining slice [lo, hi). */
        KW3Item pivot = kw3_select(a+lo, hi-lo, (hi-lo)/2, 1);
        size_t left, right;
        kw3_partition(a+lo, hi-lo, pivot, 1, &left, &right);
        /* Sum weight and profit of all items strictly denser than pivot. */
        int64_t weight = 0, profit = 0;
        for (size_t j = lo; j < lo+left; ++j) { weight += a[j].w; profit += a[j].p; }
        /* If the denser partition exceeds residual capacity, search inside it. */
        if (weight > delta) { hi = lo+left; continue; }
        /* Otherwise take the denser partition and test the pivot item at a[lo]. */
        lo += left; delta -= weight; prefix_profit += profit;
        /* Stop at the first item that cannot fit into residual capacity delta. */
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
    /* Run removal DP on prefix [0,lo) and addition DP on remaining [lo,n). */
    del = kw3_dp(a, lo, W, L, -1);
    add = kw3_dp(a+lo, n-lo, W, limit, 1);
    if (!del || !add) { errno = ENOMEM; goto fail; }
    /* Convert exact additions A[t] to at-most additions A_le[t]. */
    for (size_t t = 1; t <= limit; ++t) if (add[t-1] > add[t]) add[t] = add[t-1];
    int64_t best = prefix_profit;
    /* Combine removed weight k from prefix with added weight <= k + delta. */
    for (size_t k = 0; k <= L; ++k) if (del[k] != KW3_NEG) {
        int64_t value = prefix_profit+del[k]+add[k+(size_t)delta];
        if (value > best) best = value;
    }
    free(a); free(add); free(del); return (int)best;
fail:
    /* Free any allocated buffers and return -1 with errno preserved. */
    free(a); free(add); free(del); return -1;
}
#endif
