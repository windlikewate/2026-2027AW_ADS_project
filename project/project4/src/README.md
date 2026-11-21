# kplib 性能测试程序

源码为 `src/testbench.c`，调用四个算法，包括新增的 `w3`（前缀解、邻近性与 SMAWK 凹卷积）。
程序面向 Windows，使用 C11 与 Windows API，可用项目当前的 MinGW GCC 编译。
保留 `src/testbench_demo.c` 作为原来的小样例测试。

## 编译

在 **project4 根目录**的 PowerShell 中执行：

```powershell
gcc -std=c11 -O2 -Wall -Wextra src/testbench.c -o src/testbench.exe
```

如果 GCC 能启动但编译无输出且失败，确认编译器的 `bin` 目录在 `PATH` 中。
当前机器可先执行：

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
```

## 使用方法

测试某个 R 目录下的全部数据，并对比四个算法：

```powershell
.\src\testbench.exe --algorithm all --root kplib\00Uncorrelated\n00050\R01000 --output benchmark.csv
```

只运行指定算法，递归测试整个 kplib 中所有 `R` 加数字命名目录下的 `.kp` 文件：

```powershell
.\src\testbench.exe --algorithm list --root kplib --output benchmark_list.csv
```

`--root` 也可指定一种实例类型或某个 `nxxxxx` 目录。程序会递归发现其下所有
`Rxxxxx` 目录及其中的 `.kp` 文件，并按完整路径排序。可以直接把 `Rxxxxx`
或它的子目录作为根目录。普通说明文件及 R 目录之外的 `.kp` 文件不参与测试。
不遍历目录链接或 junction，避免循环。

算法名称：

| 参数 | 调用函数 |
| --- | --- |
| `origin` | `knapsack_origin` |
| `list` | `knapsack_list` |
| `bound` | `knapsack_list_with_bound` |
| `w3` | `knapsack_w3` |
| `all` | 按上述顺序运行四个算法，并检查结果是否一致 |

例如，仅测试前 10 个文件，每个算法独立运行 3 次：

```powershell
.\src\testbench.exe --algorithm all --root kplib\00Uncorrelated\n00050\R01000 --limit 10 --repeat 3 --output benchmark_repeat.csv
```

先扫描全部数据、核对读取和目录识别，不执行算法：

```powershell
.\src\testbench.exe --algorithm list --root kplib --dry-run --output inventory.csv
```

限制求解规模，并设置每次求解的资源上限：

```powershell
.\src\testbench.exe --algorithm all --root kplib --max-n 200 --max-capacity 100000 --timeout 30 --memory-mb 512 --output benchmark_small.csv
```

| 选项 | 默认值 | 含义 |
| --- | --- | --- |
| `--algorithm` | 必填 | `origin` / `list` / `bound` / `w3` / `all` |
| `--root` | `kplib` | 相对于启动目录的数据目录，也支持绝对路径 |
| `--output` | `benchmark.csv` | CSV 文件路径，父目录须已存在；已有文件会被覆盖 |
| `--repeat` | `1` | 每个文件、每个算法的独立测试次数，每次一行 |
| `--timeout` | `60` | 单个工作进程的超时秒数，包含启动、读取和算法执行 |
| `--memory-mb` | `1024` | 单个工作进程的提交内存限制，单位 MiB；`0` 表示不限制 |
| `--max-n` | 不限制 | 超过该物品数的文件记录为跳过 |
| `--max-capacity` | 不限制 | 超过该容量的文件记录为跳过 |
| `--max-weight` | 不限制 | 实际最大物品重量超过上限时记为 `skipped_weight`，对所有所选算法生效 |
| `--limit` | 全部 | 按路径排序后选取的前 N 个文件，包含之后被跳过的文件 |
| `--dry-run` | 关闭 | 仅读取、检查数据，不调用算法 |
| `--help` | — | 显示命令帮助 |

默认不限制数据规模：不传 `--limit`、`--max-n`、`--max-capacity` 时，
会逐一尝试所选目录中的全部实例。库中现有 20,800 个数据文件，大实例可能运行较久。

## CSV 内容与计时口径

每个数据文件、算法、重复次数对应一行，包含：

| 字段 | 含义 |
| --- | --- |
| `dataset` | 数据文件绝对路径，可区分类型、n、R 和种子编号 |
| `algorithm` | 算法名称 |
| `n` | 从文件读取的物品数 |
| `capacity` | 从文件读取的背包容量 |
| `max_weight` | 实际最大物品重量 |
| `trial` | 从 1 开始的重复序号 |
| `profit` | 算法返回的最大价值；未完成时为空 |
| `time_seconds` | 算法函数执行耗时，单位秒；未完成时为空 |
| `status` | 执行状态，见下表 |
| `exit_code` | 工作进程的 Windows 退出码（十进制）；未执行或超时时记为 0 |
| `agreement` | 同文件、同轮中成功返回的算法之间是否一致 |

计时使用 `QueryPerformanceCounter`，仅包含算法函数调用，
包括算法内部的排序、分配和释放；不包含目录扫描、输入读取、进程启动或 CSV 写入。
为隔离崩溃，每次算法调用均使用新进程。默认每次调用有 60 秒超时和 1 GiB 内存限制。
超时的算法耗时留空，不能将其当作测得了 60 秒的完整运行时间。

| `status` | 含义 |
| --- | --- |
| `ok` | 函数正常返回，记录结果和时间；不代表已验证最优性 |
| `timeout` | 超时，终止该工作进程并继续下一个测试 |
| `worker_failed` | 子进程异常或返回非零退出码，可能为算法崩溃或内存不足 |
| `process_error` | 进程、Job 资源限制或结果文件处理失败 |
| `invalid_input` | 文件内容不符合 kplib 格式或缺少数据 |
| `unsupported_input` | 数据超出当前 int 算法的支持范围，或重量非正、价值为负 |
| `input_open_error` | 输入文件无法打开 |
| `allocation_error` | 测试程序读取数据时分配内存失败 |
| `skipped_n` / `skipped_capacity` / `skipped_weight` | 超出指定规模上限 |
| `dry_run` | 数据有效，仅扫描未求解 |

`agreement=match` 表示至少两个成功算法的结果一致，`mismatch` 表示有差异，
`not_checked` 表示不足两个成功结果。它不是与 kplib 最优解的比较；本库文件不附带最优值。
输入损坏时，尚未读取的规模信息可能显示为 0。

程序退出码：`0` 表示运行完成且无失败或结果差异（允许主动跳过与 dry-run）；
`1` 表示 CSV 已生成但存在失败或结果差异；`2` 表示参数、扫描或输出错误。
每个数据文件的每轮结束后立即刷新 CSV，因此已经完成的记录会保留。

现有算法使用 `int`，程序会检查输入容量、重量和总价值范围；测试程序不会修复
算法内部的逻辑错误。某个实例失败会被隔离，并在 CSV 中留下记录。

# CSV 绘图

在 project4 根目录执行。脚本为 `src/plot_benchmark.py`，需要 Python 3 和 matplotlib：

```powershell
python -m pip install matplotlib
```

## 两种模式

以物品数量 n 为横轴，运行时间（秒）为纵轴，每种算法一条曲线：

```powershell
python src/plot_benchmark.py benchmark.csv --mode n --output plots/time_vs_n.png
```

以背包容量 c 为横轴：

```powershell
python src/plot_benchmark.py benchmark.csv --mode c --output plots/time_vs_c.png
```

支持 PNG、PDF、SVG。脚本同时输出同名 `_summary.csv`，包含图中各点的
算法、横坐标、聚合时间、实例数和有效测试次数。图中文字使用英文以避免中文字体依赖。
当前 `benchmark.csv` 的所有实例均为 n=50，因此 n 模式只有每种算法一个点，
不能展示随 n 增长的趋势；需用包含多种 n 的测试结果绘制趋势。

## 筛选和聚合

只画指定算法、实例类型与 R 范围，使用对数纵轴：

```powershell
python src/plot_benchmark.py benchmark.csv --mode n --algorithms origin list bound w3 --category 00Uncorrelated --r R01000 --log-y
```

按容量绘图时，可固定 n，减小其他规模因素的影响：

```powershell
python src/plot_benchmark.py benchmark.csv --mode c --n 50 --category 00Uncorrelated --r R01000
```

合并多个 CSV，使用中位数聚合，排除答案不一致的记录：

```powershell
python src/plot_benchmark.py result1.csv result2.csv --mode n --aggregate median --exclude-mismatch --output plots/comparison.pdf
```

参数：`--mode n|c|w` 必填；`--algorithms` 选择算法；`--n` 固定物品数量；
`--capacity` 固定容量；`--max-weight` 按实际最大重量精确筛选；`--category` 与 `--r` 按数据文件路径中的目录名筛选；
`--log-x` / `--log-y` 使用对数坐标；`--title` 自定义标题。
默认输出为 `plots/time_vs_n.png` 或 `plots/time_vs_c.png`。

只有 `status=ok`、时间合法且非负的行参与绘图。超时、失败、dry-run、主动跳过的行
不参与平均，排除数量会打印到终端。对数坐标不显示对应轴上的零值。
`--exclude-mismatch` 可进一步排除 `agreement=mismatch` 的行；默认保留正常返回的计时。
若筛选后没有有效记录，程序报错退出，不生成空图。

聚合分两步：先对同一数据文件、同一算法的有效重复测试时间取平均；再对同一横坐标、
同一算法的各个实例均值取平均（默认）或中位数（`--aggregate median`）。
这样每个实例在聚合中权重相同，不会因重复次数不同获得更大权重。
多个输入 CSV 中内容完全相同的有效行会去重。

不同类型、R 范围或不同 n/容量的实例可能混在同一横坐标聚合中。
比较单个因素时，请利用筛选参数选择可比较的数据。容量模式按实际容量精确聚合，
不会自动分箱；当各实例容量不同，每个点代表该容量的有效实例。
曲线只是按横坐标连接观测值，不是拟合结果，也不表示复杂度证明。



# Bonus：O(n + W^3) 精确算法

实现文件为 `Knapsack_w3.h`，接口与原有算法一致：

```c
int knapsack_w3(int number, int capacity, int *profits, int *weights);
```

依据 ADS 笔记附录“进一步改进——O(n+W^3)”和本地论文
`doc/n+w^3时间的背包问题.pdf`（Polak、Rohwedder、Węgrzycki，
*Knapsack and Subset Sum with Small Items*，ICALP 2021）的 Lemma 2.1、
Lemma 2.2 与 Theorem 1.1 实现，仅返回最优价值。代码各阶段均有注释。

1. 先排除超出容量或价值为零的物品，使用 BFPRT（五元组中位数选择）按价值密度
   寻找最大可行前缀，不对全部 n 个物品排序；密度用 64 位整数交叉相乘比较，
   相同密度按原始下标打破平局。前缀在第一件放不下的物品处停止。
2. 记前缀价值为 Vp、剩余容量为 delta。邻近性引理保证存在最优解与前缀最多相差
   2W 件物品，所以删除总重量至多 L=2W²。分别计算删除 DP D[k]（删除价值取负）
   和增加 DP A[t]，增加数组范围为 0..L+delta，delta<W。
3. 按重量 h 分组，各组只保留最多 min(2W, floor(limit/h)) 个最佳物品，
   再对保留部分排序。增加侧取价值最高的物品，删除侧取原价值最低的物品。
   每组的价值前缀和为凹序列。按重量模 h 拆分转移，使用 SMAWK 求隐式
   inverse-Monge 矩阵的行最大值，每个重量组共 O(W²) 时间。
4. 对 A 取前缀最大值得到 A_le，返回
   `Vp + max(D[k] + A_le[delta+k])`，其中 **0 <= k <= L**。
   这里补齐笔记公式中的 k=0 和边界，覆盖只增加物品、不删除物品的最优修正。

BFPRT 与分组共 O(n)，保留物品排序 O(W² log W)，两侧 DP 各 O(W³)，
合并 O(W²)，所以总时间为 **O(n+W³)**、额外空间为 **O(n+W²)**。
内部不分配容量 c 大小的表。这里 W 是预处理后剩余物品的最大重量，不大于输入的
最大重量；CSV 的 `max_weight` 仍记录原始输入的实际最大重量。

SMAWK 只保留可达旧状态列，并通过凹序列的带外延拓处理无效位置，
避免把所有不可达项直接当作相同负无穷时破坏行最优位置的单调性。
延拓比较采用“距离、价值”的字典序，不实际计算巨大惩罚，避免溢出。

输入约定为重量正整数、价值非负整数、容量非负、总价值不超过 INT_MAX。
函数不修改输入数组，空实例返回 0；非法输入、尺寸溢出或分配失败返回 -1 并设置 errno。
benchmark 会将负返回值视为工作进程失败，不会记录成成功的最优值。

## 小重量实验与绘图

W=1000 或 10000 时，立方项可能很大，建议先使用小重量生成实例。
下面在外层设置纯 `R<数字>` 目录，使现有扫描器可以识别生成器带参数后缀的内部目录：

```powershell
python src/knapsack_generator.py --n 50 100 200 500 1000 --range 16 --instances 10 --output generated_bonus/R00016
.\src\testbench.exe --algorithm all --root generated_bonus --repeat 3 --output bonus.csv
python src/plot_benchmark.py bonus.csv --mode n --algorithms origin list bound w3 --exclude-mismatch --output plots/bonus_vs_n.png
```

仅运行新算法，或限制输入最大重量：

```powershell
.\src\testbench.exe --algorithm w3 --root generated_bonus --max-weight 32 --output bonus_w3.csv
```

`--algorithm all` 现在按 origin、list、bound、w3 顺序运行四个算法，
均保留进程隔离、超时、内存限制及同轮答案比较。历史 CSV 不会自动补出 w3 计时，
需要重新运行或在绘图时同时传入新旧 CSV。

新增 `--mode w` 按 CSV 的**实际最大重量**绘图，区别于生成范围参数 R。
可先分别用 `--range 8/16/32` 生成数据并统一测试，再运行：

```powershell
python src/plot_benchmark.py bonus.csv --mode w --n 200 --exclude-mismatch --output plots/bonus_vs_w.png
python src/plot_benchmark.py bonus.csv --mode n --max-weight 16 --output plots/bonus_fixed_w.png
```

n/c 模式仍可读取不含 `max_weight` 字段的旧 CSV；w 模式和重量筛选需要该字段。
聚合、重复次数去重、失败记录排除与原先相同。

报告绘图脚本 `visualization.py` 也支持 w3：读取既有 `all_n*_R10000.csv` 和
`all_nvariable_C1000.csv` 中实际存在的 w3 记录，显示耗时、加速比与箱线图；
只有旧三算法数据时继续绘制三算法。可通过以下参数选择数据与输出目录：

```powershell
python src/visualization.py --data-dir data --output-dir src/visualizations
```

## 正确性回归

`test_w3.c` 使用固定种子，与独立的一维容量 DP、微型实例穷举以及朴素卷积对拍，
包含稀疏不可达状态、负增益、相同密度、同重量、零价值、空实例、零容量、
全可装入、大价值及输入数组不变性检查：

```powershell
gcc -std=c11 -O2 -Wall -Wextra -Werror src/test_w3.c -o src/test_w3.exe
.\src\test_w3.exe
```


## 已运行的 Bonus 优势区间实验

`experiment_bonus.py` 会生成两组共同随机数的受控实验，再调用现有四算法 benchmark：
固定 W=16 扫描 n=50..3200，以及固定 n=500、c=1000 扫描 W=4..256。
每组 3 个固定种子，每个算法默认重复 3 次，价值范围始终为 1..1000。
不修改算法实现，保留所有输入、原始 CSV、四算法答案一致性检查与实例配对加速比。

```powershell
python src/experiment_bonus.py
python src/experiment_bonus.py --plot-only
```

需要 matplotlib 和 numpy。脚本采用“实例内重复中位数，再取实例间中位数”聚合，
图中阴影为实例间四分位范围，超时或答案不一致会使脚本停止并要求检查原始数据。

- 图像：[bonus_advantage.png](../plots/bonus_advantage.png)，另存同名 PDF。
- 实测结果与解释：[实验报告](../data/bonus_advantage_report.md)。
- 原始计时：`data/bonus_advantage_n.csv` 和 `data/bonus_advantage_w.csv`。
- 汇总：`plots/bonus_advantage_summary.csv` 和 `plots/bonus_advantage_speedups.csv`。

小重量优势与大重量退化会同时展示；这是一组适用区间实验，并不保证 w3 对所有实例更快。
