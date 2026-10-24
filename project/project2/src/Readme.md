# Dijkstra 查询与运行时间基准测试

## 编译与查询

```sh
gcc -std=c11 -O2 -Wall -Wextra -pedantic dijkstra.c dijkstra_main.c -o dijkstra.exe
./dijkstra.exe NY both 1 10000
```

数据集：`NY`、`BAY`、`COL`，或 DIMACS `.gr` 文件路径。  
堆：`binary`、`fib` 或 `both`。顶点 ID 从 1 开始。  
不可达目标会被明确报告。距离使用 64 位整数，并保留数据集原始的权重单位。

## 计时实验

```sh
gcc -std=c11 -O2 -Wall -Wextra -pedantic dijkstra.c testbench_dijkstra_time.c -o testbench_dijkstra_time.exe
./testbench_dijkstra_time.exe NY 1000 250 3 dijkstra_runtime_NY.csv
./testbench_dijkstra_time.exe BAY 1000 250 3 dijkstra_runtime_BAY.csv
./testbench_dijkstra_time.exe COL 1000 250 3 dijkstra_runtime_COL.csv
```

参数：`[数据集] [最大查询数] [步长] [重复次数] [输出.csv]`。  
默认值：`NY 1000 100 3 dijkstra_runtime.csv`。

例如，`1000 250 3` 会测试 250、500、750 和 1000 对查询；每个批次重复三次。最大规模必须至少为 1000，以满足项目 README 的要求。如果步长不能整除最大查询数，仍会包含最大查询数对应的测量结果。

每个图只加载一次，且不计入计时区域。确定性的 xorshift32 生成器（种子为 `0x6d2b79f5`）提供随机源点/目标点 ID；对于每种规模和两种堆结构，使用该序列的相同前缀。当图包含多个顶点时，源点和目标点不会相同。查询对可能重复；这是随机查询负载，而不是不重复查询对采样器。随机选择可能产生不可达的查询对，但这些查询仍计入总数。

基准测试测量批量墙钟时间（Windows 使用 `QueryPerformanceCounter`；其他平台使用 C11 `timespec_get`），覆盖完整的 `runDijkstra` 调用，包括距离/堆的分配与初始化、搜索和清理。文件解析、查询对生成、控制台输出和 CSV 写入均不计入。批量计时可避免 Windows 上单次查询计时被舍入到毫秒。交互式查询程序报告的 `elapsedSeconds` 计时范围更窄，不要直接混用这两种测量结果。

CSV 列：

| 列 | 含义 |
| --- | --- |
| dataset | 数据集别名或传入的路径 |
| query_pairs | 每次重复执行的查询数 |
| heap | `binary` 或 `fibonacci` |
| repetitions | 相同批次的重复次数 |
| avg_seconds | 所有重复批次的平均总墙钟时间 |
| total_seconds | 所有重复批次墙钟时间之和 |
| avg_ms_per_query | `avg_seconds * 1000 / query_pairs` |
| checksum | 用于粗略检查不同堆结果一致性的距离结果校验和 |

校验和不能替代最短路径正确性验证。大量随机查询可能需要几分钟；每次堆/规模测量完成后都会打印进度，并立即刷新 CSV 行。请依次运行基准测试，并停止其他高负载任务。两种堆都是项目现有实现：二叉堆使用延迟重复条目，斐波那契堆使用 decrease-key。性能结论应包含这些实现差异，而不仅仅是理论堆复杂度。

## 绘制曲线



```sh
python visualization.py dijkstra_runtime_NY.csv
python visualization.py dijkstra_runtime_BAY.csv
python visualization.py dijkstra_runtime_COL.csv
```

每个数据集都会在 `visualizations/` 中生成 PNG 和 PDF 文件。横轴为查询对数量，纵轴为批次平均秒数。

## Make 目标

```sh
make all
make benchmark DATASET=NY MAX_QUERIES=1000 STEP=250 REPETITIONS=3
make plot DATASET=NY
make test
```

