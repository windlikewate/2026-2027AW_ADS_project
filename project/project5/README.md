
# 8-1 Shopping With Coupons


## Problem Description

There are `N` items on your shopping list, and you have `N` kinds of coupons that can save you some money when buying these items. Meanwhile, you have `D` dollars in your pocket.

Your goal is to buy as many items as possible.

The following rules apply:

1. You may use any coupon multiple times.
2. You may buy any item multiple times.
3. Each coupon can be used at most once for purchasing a particular item.
4. Multiple different coupons can be applied to the same item,and for each item you can use only one coupon at a time.
5. A coupon can be used on different items independently.
For example, you may use coupon `A1` to buy item `B1`, and then use coupon `A2` to buy item `B1`. Meanwhile, coupon `A1` can also be used to buy item `B2`. However, you cannot use coupon `A1` to buy item `B1` again.

### Example

Suppose there are 4 items with prices of `$10`, `$12`, `$15` and `$20`, and 4 kinds of coupons with values of `$6`, `$7`, `$8` and `$9`. If you have `$30`, the optimal purchasing strategy is:

- Buy the `$10` item 4 times, using all four coupons:
  \[
  10 \times 4 - 6 - 7 - 8 - 9 = 10
  \]

- Buy the `$12` item 3 times, using coupons worth `$7`, `$8` and `$9`:
  \[
  12 \times 3 - 7 - 8 - 9 = 12
  \]

- Buy the `$15` item once, using the `$9` coupon:
  \[
  15 - 9 = 6
  \]

The total cost is:
\[
10 + 12 + 6 = 28
\]

Therefore, you can buy 8 items and have `$2` left.

## Input Specification

Each input file contains one test case.

- The first line contains two positive integers `N` (≤ 10^5), the number of items and coupons, and `D` (≤ 10^6), the amount of money available.
- The second line contains `N` positive integers representing the prices of the items.
- The third line contains `N` positive integers representing the values of the coupons.

It is guaranteed that:

- The highest coupon value is strictly less than the lowest item price.
- All input numbers are bounded above by 10^9.
- Numbers in each line are separated by spaces.

## Output Specification

Print two integers in one line, separated by a space:

- The maximum number of items you can buy.
- The maximum amount of money left after buying that maximum number of items.

## Sample Input

```text
4 30
12 20 15 10
9 6 8 7
```

## Sample Output

```text
8 2
```

## Grading Policy

| Category | Requirements | Points |
|---|---|---:|
| Programming | Implement the program with sufficient comments. | 10 |
| Testing | Provide test cases for the test report, including analysis and comments. | 6 |
| Documentation | Chapter 1 | 1 |
| Documentation | Chapter 2 | 2 |
| Documentation | Overall documentation style | 1 |
| **Total** | | **20** |


## 解决思路
基本的思路是使用贪心算法，优先购买物品价格减去优惠券后的实际价格最低的商品，具体可以分为一下几步

1. 首先对优惠券进行排序，确保每次使用优惠券时都能选择当前最优的优惠券。
2. 使用最小堆来维护每个商品在使用不同优惠券后的实际价格，确保每次都能快速找到当前最便宜的商品。
3. 每次从堆中取出最便宜的商品，如果当前剩余的钱不足以购买该商品，则停止购买。
4. 如果当前使用优惠券后的最优价格不低于所有物品中不使用优惠券的最低价格，则尽可能多地购买该商品，直到剩余的钱不足以购买更多。
5. 更新商品使用的优惠券索引，并将更新后的价格重新插入堆中。
6. 重复上述步骤，直到剩余的钱不足以购买任何商品。
7. 输出购买的商品数量和剩余的钱。