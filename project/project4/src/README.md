# kplib 性能测试程序

源码为 `src/testbench.c`，直接调用现有三个算法，不修改算法实现。
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

测试某个 R 目录下的全部数据，并对比三个算法：

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
| `all` | 按上述顺序运行三个算法，并检查结果是否一致 |

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
| `--algorithm` | 必填 | `origin` / `list` / `bound` / `all` |
| `--root` | `kplib` | 相对于启动目录的数据目录，也支持绝对路径 |
| `--output` | `benchmark.csv` | CSV 文件路径，父目录须已存在；已有文件会被覆盖 |
| `--repeat` | `1` | 每个文件、每个算法的独立测试次数，每次一行 |
| `--timeout` | `60` | 单个工作进程的超时秒数，包含启动、读取和算法执行 |
| `--memory-mb` | `1024` | 单个工作进程的提交内存限制，单位 MiB；`0` 表示不限制 |
| `--max-n` | 不限制 | 超过该物品数的文件记录为跳过 |
| `--max-capacity` | 不限制 | 超过该容量的文件记录为跳过 |
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
| `skipped_n` / `skipped_capacity` | 超出指定规模上限 |
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
python src/plot_benchmark.py benchmark.csv --mode n --algorithms origin list bound --category 00Uncorrelated --r R01000 --log-y
```

按容量绘图时，可固定 n，减小其他规模因素的影响：

```powershell
python src/plot_benchmark.py benchmark.csv --mode c --n 50 --category 00Uncorrelated --r R01000
```

合并多个 CSV，使用中位数聚合，排除答案不一致的记录：

```powershell
python src/plot_benchmark.py result1.csv result2.csv --mode n --aggregate median --exclude-mismatch --output plots/comparison.pdf
```

参数：`--mode n|c` 必填；`--algorithms` 选择算法；`--n` 固定物品数量；
`--capacity` 固定容量；`--category` 与 `--r` 按数据文件路径中的目录名筛选；
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

