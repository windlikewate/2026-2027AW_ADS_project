import pandas as pd
import matplotlib.pyplot as plt
import os
# 读取 CSV 文件
df = pd.read_csv("data_set.csv")

# 设置绘图风格与中文字体
plt.rcParams["font.sans-serif"] = ["Microsoft YaHei"]
plt.rcParams["axes.unicode_minus"] = False

scenarios = {
    "ordered": "Ordered Insert, Ordered Delete",
    "ordered_reverse": "Ordered Insert, Reverse Delete",
    "random": "Random Input"
}

operations = {
    "insert_seconds": "Insertion Time",
    "delete_seconds": "Deletion Time"
}

# 分别绘制顺序删除、逆序删除和随机删除三种情形
for scenario, scenario_title in scenarios.items():
    data = df[df["scenario"] == scenario]

    for column, operation_title in operations.items():
        plt.figure(figsize=(9, 6))

        # 每种树绘制一条曲线
        for tree in data["tree"].unique():
            tree_data = data[data["tree"] == tree].sort_values("n")

            plt.plot(
                tree_data["n"],
                tree_data[column],
                marker="o",
                label=tree
            )

        plt.xlabel("Number of Nodes (N)")
        plt.ylabel("Time (seconds)")
        plt.title(f"{operation_title} - {scenario_title}")
        plt.legend()
        plt.grid(True, linestyle="--", alpha=0.5)
        plt.tight_layout()

        # 保存图片
        filename = f"{scenario}_{column}.png"
        os.makedirs("visualizations", exist_ok=True)
        plt.savefig(f"visualizations/{filename}", dpi=300)
        plt.show()
