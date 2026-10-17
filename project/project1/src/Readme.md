  本目录下实现了时间基准测试和可视化功能,可以使用`make`命令进行编译和运行。
  >
  主要文件包括：
  - `testbench_time.c`：时间基准测试程序
  - `visualization.py`：可视化程序
  - `data_set.csv`：生成的测试数据文件
  - `visualizations/`：生成的可视化图片目录

  使用步骤：
  1. 编译并运行时间基准测试程序：
     ```bash
     make time
     ```
  2. 生成可视化图片：
     ```bash
     make time_pdf
     ```
  3. 生成树的可视化图片：
     ```bash
     make show
     ```
  4. 清理生成的文件：
     ```bash
     make clean
     ```

目前时间基准测试实现了三种输入场景：
- `ordered`：顺序插入，顺序删除
- `ordered_reverse`：顺序插入，逆序删除
- `random`：随机输入

其中测试数量$N$表示每次测试的节点数量，现在从1000开始，到50000结束，步长为1000。