# 8-1 背包问题动态规划算法的改进（Improving Dynamic Programming for Knapsack）

性能测试程序位于 `src/testbench.c`，支持选择算法、递归识别 kplib 的 R 目录、
输出规模与耗时 CSV。编译、运行方式、字段说明与绘图方法见 [源码使用说明](src/README.md)。

**分数：** 20 分  
**作者：** Yuchen Mao  
**单位：** 浙江大学

## 一、项目背景（Project Description）

给定 \(n\) 个物品，每个物品具有对应的价值（Profit）和重量（Weight），分别记为：

- 物品价值：\(p_1,p_2,\ldots,p_n\)；
- 物品重量：\(w_1,w_2,\ldots,w_n\)。

现有一个容量为 \(c\) 的背包，要求选择若干物品放入背包，使得所选物品的总重量不超过背包容量 \(c\)，且总价值最大。

该问题可以通过标准动态规划算法在 \(\Theta(nc)\) 时间内求解。

定义状态：

\[
P[i][j]
\]

表示仅考虑前 \(i\) 个物品、背包容量为 \(j\) 时能够获得的最大价值，其中：

\[
0\leq i\leq n,\qquad 0\leq j\leq c.
\]

为简化状态转移，约定当 \(j<0\) 时：

\[
P[i][j]=-\infty.
\]

动态规划的初始条件为：

\[
P[0][j]=0,\qquad 0\leq j\leq c.
\]

状态转移方程为：

\[
P[i][j]=
\max\left\{
P[i-1][j],
P[i-1][j-w_i]+p_i
\right\},
\]

其中 \(i\geq 1\)，且 \(0\leq j\leq c\)。

该递推关系可以通过两层循环实现，时间复杂度为：

\[
\Theta(nc).
\]

然而，即使在最优情况下，该算法仍然需要 \(\Theta(nc)\) 的运行时间，这在实际应用中可能会造成不必要的计算开销。

参考教材第 **3.4 章** 提供了一种标准动态规划算法的替代实现方式。当结合剪枝（Pruning）思想时，该算法能够在最优情况下获得更好的运行时间。

此外，教材第 **3.5 章** 还提供了进一步改进动态规划算法的思路。可以尝试实现该算法，并将其与前两种算法进行比较。根据实现质量和实验效果，将给予相应的额外加分。

## 二、项目要求

需要完成以下任务：

1. 阅读参考教材《The Knapsack Problems》的第 3.4 章和第 3.5 章，并理解其中介绍的算法。
2. 实现教材中介绍的相关算法。
3. 设计并开展实验，将这些改进算法与课堂上介绍的原始动态规划算法进行性能比较。

**注意：** 为简化问题，本项目只要求计算背包问题的最优价值（Optimal Value），不要求输出对应的最优物品选择方案。

## 三、Bonus（额外加分）

此外，还存在一种时间复杂度为：

\[
O(n+w^3)
\]

的算法，其中 \(w\) 表示所有物品中的最大重量：

\[
w=\max_{1\leq j\leq n}w_j.
\]

可以尝试实现该算法，并通过实验测试其性能。

如果能够出色地回答 Bonus 部分的问题，还将获得额外的 1 分。

## 四、参考文献（Reference）

H. Kellerer, U. Pferschy, D. Pisinger. 2004.

*The Knapsack Problems*. Springer.

本项目所需的参考材料为教材第 3.4 章和第 3.5 章，具体见附件 **CH3.4&3.5.pdf**。

## 五、教材符号说明（Notation of the Book）

教材中使用的符号与课堂上介绍的符号略有不同，对应关系如下：

| 符号 | 含义 |
|---|---|
| \(w_j\) | 第 \(j\) 个物品的重量（Weight） |
| \(p_j\) | 第 \(j\) 个物品的价值（Profit / Value） |
| \(z_j(d)\) | 仅考虑前 \(j\) 个物品、背包容量为 \(d\) 时能够获得的最大价值 |
| DP-1 | 课堂上介绍的原始动态规划算法 |

其中：

\[
z_j(d)=P[j][d].
\]

## 六、评分标准（Grading Policy）

### 6.1 程序设计与测试（Programming and Testing）：8 分

| 评分项目 | 分值 | 要求 |
|---|---:|---|
| 核心算法实现 | 6 分 | 实现项目要求的主要算法 |
| 性能测试程序 | 1 分 | 编写用于比较不同算法运行性能的测试程序 |
| 测试数据生成 | 1 分 | 生成用于实验的背包问题测试数据 |

**代码要求：** 所有代码都必须包含充分的注释（Sufficiently Commented）。

### 6.2 实验报告（Report）：12 分

| 评分项目 | 分值 | 要求 |
|---|---:|---|
| 封面与引言 | 1 分 | 提供规范的报告封面和引言 |
| 问题描述 | 1 分 | 清晰描述背包问题及其数学定义 |
| 算法描述与分析 | 2 分 | 介绍各算法的基本思想，并分析其复杂度 |
| 实验结果与分析 | 7 分 | 展示实验结果，对不同算法的性能进行比较和分析 |
| 写作规范 | 1 分 | 报告语言、结构和格式规范 |

### 6.3 Bonus（额外加分）

实现并测试时间复杂度为：

\[
O(n+w^3)
\]

的算法。

此外，对于 Bonus 问题回答出色的同学，还将额外获得 1 分。

---

## 七、项目任务总结

本项目的核心目标是研究背包问题动态规划算法的优化方法，并通过实验分析不同算法的性能差异。

主要工作包括：

1. 实现课堂上的原始动态规划算法（DP-1）。
2. 阅读教材第 3.4 章，实现结合剪枝思想的改进动态规划算法。
3. 阅读教材第 3.5 章，实现进一步改进的动态规划算法。
4. 设计测试数据，编写性能测试程序。
5. 比较不同算法在不同规模、不同特征的输入数据下的运行时间。
6. 分析不同算法的时间复杂度、适用场景及实际性能。
7. （可选）实现时间复杂度为 \(O(n+w^3)\) 的 Bonus 算法。

**项目重点：** 不仅需要完成不同算法的实现，还需要通过充分的实验，分析改进算法在实际运行中相对于原始动态规划算法的性能优势。

---
# 项目实现

本项目库结构如下（kplib 数据文件与实验输出按代表性文件展示）：

```text
project4/
├── README.md                         # 项目要求与实现结构说明
├── src/                              # 算法实现、测试与绘图工具
│   ├── README.md                     # 编译、测试参数、CSV 字段及绘图说明
│   ├── Knapsack_origin.h             # 标准动态规划背包算法
│   ├── Knapsack_List.h               # 状态列表动态规划与主导性剪枝
│   ├── Knapsack_List_with_bound.h    # 结合分数背包上界的列表动态规划
│   ├── Knapsack_w3.h                 # O(n+W^3)：前缀邻近性与 SMAWK 凹卷积
│   ├── test_w3.c                     # 随机 DP、穷举与卷积正确性对拍
│   ├── Fraction_Knapsack.h           # 分数背包上界计算与排序工具
│   ├── testbench_demo.c              # 小规模样例测试
│   ├── testbench.c                   # kplib 批量测试程序，可选择算法并输出 CSV
│   ├── testbench.exe                 # 当前编译生成的测试程序
│   └── plot_benchmark.py             # 按物品数 n 或背包容量 c 绘制耗时图
├── kplib/                            # 0/1 背包测试数据，共 13 种实例类型
│   ├── 00Uncorrelated/               # 无相关实例（其他类型目录结构相同）
│   │   ├── n00050/                   # 物品数量 n=50
│   │   │   ├── R01000/               # 数据生成范围参数 R=1000
│   │   │   │   ├── s000.kp           # 一个实例：n、容量、逐行价值与重量
│   │   │   │   ├── ...
│   │   │   │   └── s099.kp
│   │   │   └── R10000/
│   │   ├── ...
│   │   └── n10000/
│   ├── 01WeaklyCorrelated/
│   ├── 02StronglyCorrelated/
│   ├── 03InverseStronglyCorrelated/
│   ├── 04AlmostStronglyCorrelated/
│   ├── 05SubsetSum/
│   ├── 06UncorrelatedWithSimilarWeights/
│   ├── 07SpannerUncorrelated/
│   ├── 08SpannerWeaklyCorrelated/
│   ├── 09SpannerStronglyCorrelated/
│   ├── 10MultipleStronglyCorrelated/
│   ├── 11ProfitCeiling/
│   └── 12Circle/
├── data/                             # 批量测试得到的原始 CSV 结果
│   ├── all_n50_R10000.csv
│   ├── all_n100_R10000.csv
│   ├── all_n200_R10000.csv
│   ├── all_n500_R10000.csv
│   └── all_n1000_R10000.csv
├── plots/                            # 绘图结果及对应的聚合数据
│   ├── time_vs_n.png
│   ├── time_vs_n_summary.csv
│   ├── time_vs_c_n50__R10000.png
│   ├── time_vs_c_n50__R10000_summary.csv
│   └── ...                           # 其他规模的容量耗时图与汇总 CSV
└── doc/                              # 参考文献
    ├── 改进背包问题的文献.pdf
    └── n+w^3时间的背包问题.pdf
```

实验流程为：使用 `src/testbench.c` 读取 `kplib/` 中的数据，将测试结果保存到
`data/`，再使用 `src/plot_benchmark.py` 将耗时图和聚合数据保存到 `plots/`。
`src/__pycache__/` 是 Python 自动生成的缓存，不属于算法源码。


## Bonus 算法实现

已新增 `src/Knapsack_w3.h`，依据 ADS 笔记附录和论文
*Knapsack and Subset Sum with Small Items* 实现确定性 O(n+W³) 精确算法，
其中 W 为最大物品重量。采用 BFPRT 线性时间构造密度前缀、2W 邻近性界、
按重量分组的凹卷积及 SMAWK，额外空间 O(n+W²)。实现包含关键步骤注释。

benchmark 新增 `--algorithm w3`；`all` 运行四算法并比较答案，
新增 `--max-weight` 规模限制。`plot_benchmark.py` 支持第四条曲线、
`--mode w` 实际最大重量横轴及 `--max-weight` 筛选；报告绘图脚本
`visualization.py` 同样识别新增算法，兼容历史三算法 CSV。

编译、复现实验、算法步骤和正确性回归命令见 [源码使用说明](src/README.md)。
建议从 W=8/16/32 的合成数据开始比较；大 W 时立方项开销很高。


已补充可复现的 Bonus 优势区间实验：`python src/experiment_bonus.py`。
图中同时比较固定小 W 时随 n 增长的加速效果，以及固定 n、c 时随 W 增长的耗时。
参见 [实验报告](data/bonus_advantage_report.md) 和 [实验图像](plots/bonus_advantage.png)。
