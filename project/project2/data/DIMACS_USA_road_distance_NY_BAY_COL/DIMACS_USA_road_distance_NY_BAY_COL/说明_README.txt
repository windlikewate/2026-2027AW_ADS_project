DIMACS 美国道路网络数据集：NY、BAY、COL
整理日期：2026-10-23（Asia/Shanghai）

一、数据选择与地区标签
本包包含三个地区的距离权重版本（USA-road-d），不是行驶时间版本（USA-road-t）。
NY_纽约/USA-road-d.NY.gr：264,346 个节点，733,846 条有向边。
BAY_旧金山湾区/USA-road-d.BAY.gr：321,270 个节点，800,172 条有向边。
COL_科罗拉多/USA-road-d.COL.gr：435,666 个节点，1,057,066 条有向边。
各 .gr 文件保持下载内容不变；中文标签仅用于目录和本说明。

二、来源与验证范围
原始数据集：9th DIMACS Implementation Challenge — Shortest Paths。
官方目录：http://www.dis.uniroma1.it/challenge9/download.shtml
实际下载来源：第三方研究项目 https://github.com/lbwang95/QHL4CSP
项目标题：QHL: A Fast Algorithm for Exact Constrained Shortest Path Search on Road Networks。
该项目 README 将这些道路网络数据的说明指向 DIMACS 网站。
仓库固定提交：96ad08e7e514bbe78531972784f354147733ab9c

已完整读取三个文件，检查 DIMACS 图格式、实际边数与文件声明、节点编号范围、
边权非负性，并校验文件大小和 Git blob SHA-1 与该仓库对应版本一致。
SHA256SUMS.txt 提供本包三个数据文件的 SHA-256；manifest.json 提供详细记录。
这些检查确认下载完整且与所列仓库版本一致；官网无法访问，尚未与官方原件
或官方校验值比对，不能据此证明第三方副本从未被修改。

三、文件格式与使用
文件为未压缩的 ASCII 文本 .gr，可直接读取，无需再解压 .gz。
c 开头：注释。
p sp n m：节点数 n、有向边数 m。
a u v w：从节点 u 到节点 v 的有向边，权重为 w。
节点编号为 1 到 n。应按文件中的有向边读取，不要擅自添加反向边。
Dijkstra 计算只需图文件，不依赖坐标文件。

四、课程实验说明
本包只含图数据，没有附带 1000 对查询。
请按题目要求准备至少 1000 对起点、终点查询，并让各堆实现使用相同查询。
提交报告时列出实际采用的数据下载链接，不要将本包或大型图文件一并上传。
下面列出固定版本下载链接；普通直链若不可达，可用 GitHub 文件页面下载，
或通过 GitHub API 读取对应 blob（本次新增下载使用了 API 方式）。

NY（纽约）
下载链接：https://raw.githubusercontent.com/lbwang95/QHL4CSP/96ad08e7e514bbe78531972784f354147733ab9c/data/NY/USA-road-d.NY.gr
文件页面：https://github.com/lbwang95/QHL4CSP/blob/96ad08e7e514bbe78531972784f354147733ab9c/data/NY/USA-road-d.NY.gr

BAY（旧金山湾区）
下载链接：https://raw.githubusercontent.com/lbwang95/QHL4CSP/96ad08e7e514bbe78531972784f354147733ab9c/data/BAY/USA-road-d.BAY.gr
文件页面：https://github.com/lbwang95/QHL4CSP/blob/96ad08e7e514bbe78531972784f354147733ab9c/data/BAY/USA-road-d.BAY.gr

COL（科罗拉多）
下载链接：https://raw.githubusercontent.com/lbwang95/QHL4CSP/96ad08e7e514bbe78531972784f354147733ab9c/data/COL/USA-road-d.COL.gr
文件页面：https://github.com/lbwang95/QHL4CSP/blob/96ad08e7e514bbe78531972784f354147733ab9c/data/COL/USA-road-d.COL.gr

