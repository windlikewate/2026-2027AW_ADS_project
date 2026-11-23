#include "shopping_with_cunpons.h"

int main() {
    int N, D;
    scanf("%d %d", &N, &D);
    int prices[N], coupons[N];
    for (int i = 0; i < N; i++) {
        scanf("%d", &prices[i]);
    }
    for (int i = 0; i < N; i++) {
        scanf("%d", &coupons[i]);
    }
    solveShopping(N, D, prices, coupons);
    return 0;
}