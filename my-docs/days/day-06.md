# Day 6：曲面与高阶几何

**所属周次**：第1周 - 环境搭建与初识Gmsh
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

掌握曲线、曲面和高阶网格

---

## 学习文件

- `tutorials/t7.geo`（105行）
- `tutorials/t8.geo`（135行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 学习t7.geo的背景网格概念 |
| 09:45-10:30 | 45min | 学习Spline曲线的创建 |
| 10:30-11:15 | 45min | 学习BSpline和Bezier曲线 |
| 11:15-12:00 | 45min | 学习t8.geo的高阶网格 |
| 14:00-14:45 | 45min | 学习使用外部标量场控制网格 |
| 14:45-15:30 | 45min | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 学习曲线创建：
  - Spline() - 样条曲线
  - BSpline() - B样条曲线
  - Bezier() - 贝塞尔曲线
- [ ] 学习背景网格和标量场

---

## 下午任务（2小时）

- [ ] 学习t8.geo的高阶网格
- [ ] 理解Post-processing视图的概念
- [ ] 实验不同阶数的网格元素

---

## 练习作业

### 1. 【基础】创建样条曲线

使用Spline创建一条光滑曲线

```geo
// spline_curve.geo - 样条曲线
lc = 0.1;

// 定义控制点
Point(1) = {0, 0, 0, lc};
Point(2) = {1, 1, 0, lc};
Point(3) = {2, 0.5, 0, lc};
Point(4) = {3, 1.5, 0, lc};
Point(5) = {4, 0, 0, lc};

// 创建样条曲线（经过所有点）
Spline(1) = {1, 2, 3, 4, 5};

// 添加两端的直线形成封闭区域
Point(6) = {0, -0.5, 0, lc};
Point(7) = {4, -0.5, 0, lc};

Line(2) = {5, 7};
Line(3) = {7, 6};
Line(4) = {6, 1};

Curve Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};

Mesh 2;
```

### 2. 【进阶】BSpline和Bezier对比

创建相同控制点的BSpline和Bezier曲线，对比差异

```geo
// bspline_vs_bezier.geo - 曲线类型对比
lc = 0.1;

// 控制点 (BSpline)
Point(1) = {0, 0, 0, lc};
Point(2) = {1, 2, 0, lc};
Point(3) = {2, 2, 0, lc};
Point(4) = {3, 0, 0, lc};

// 控制点 (Bezier，向下偏移用于对比)
Point(11) = {0, -1, 0, lc};
Point(12) = {1, 1, 0, lc};
Point(13) = {2, 1, 0, lc};
Point(14) = {3, -1, 0, lc};

// BSpline曲线
BSpline(1) = {1, 2, 3, 4};

// Bezier曲线
Bezier(2) = {11, 12, 13, 14};

// 观察：
// - Spline经过所有点
// - BSpline接近但不一定经过控制点
// - Bezier仅经过首尾点，中间点为控制点
```

### 3. 【挑战】高阶网格

创建二阶网格元素（每条边有中点节点）

```geo
// high_order.geo - 高阶网格
lc = 0.5;

// 创建简单几何
Point(1) = {0, 0, 0, lc};
Point(2) = {1, 0, 0, lc};
Point(3) = {0.5, 0.866, 0, lc};  // 等边三角形

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 1};

Curve Loop(1) = {1, 2, 3};
Plane Surface(1) = {1};

// 设置高阶网格
Mesh.ElementOrder = 2;          // 2阶元素
Mesh.SecondOrderLinear = 0;     // 曲边（0）或直边（1）
Mesh.HighOrderOptimize = 2;     // 高阶优化级别

Mesh 2;

// 生成后，每条边中间会有一个额外节点
// 三角形变为6节点三角形
```

---

## 今日检查点

- [ ] 理解Spline/BSpline/Bezier的区别
- [ ] 能解释高阶网格的意义
- [ ] 理解背景网格场的概念

---

## 核心概念

### 曲线类型对比

| 类型 | 特点 | 是否经过控制点 |
|------|------|----------------|
| Spline | 插值样条 | 是，经过所有点 |
| BSpline | B样条 | 否，仅近似 |
| Bezier | 贝塞尔 | 仅首尾点 |

### Spline 语法

```geo
// 插值样条，曲线经过所有点
Spline(tag) = {point1, point2, point3, ...};
```

### BSpline 语法

```geo
// B样条，曲线近似控制点
BSpline(tag) = {point1, point2, point3, ...};
```

### Bezier 语法

```geo
// 贝塞尔曲线
Bezier(tag) = {start, control1, control2, ..., end};
```

### 高阶网格设置

```geo
Mesh.ElementOrder = n;  // n阶元素（1=线性，2=二次，3=三次...）
// 1阶：三角形3节点，四边形4节点
// 2阶：三角形6节点，四边形9节点
// 3阶：三角形10节点，四边形16节点
```

### 背景网格

使用后处理视图控制网格尺寸：

```geo
// 从外部文件加载网格尺寸场
Merge "size_field.pos";
Background Mesh View[0];
```

或使用MathEval场：

```geo
Field[1] = MathEval;
Field[1].F = "0.1 + 0.5*x";  // 尺寸随x增加
Background Field = 1;
```

---

## 产出

- 能创建各种类型的曲线
- 理解高阶网格的概念和用途

---

## 导航

- **上一天**：[Day 5 - 结构化网格](day-05.md)
- **下一天**：[Day 7 - 第一周总结与项目](day-07.md)
- **返回目录**：[学习计划总览](../study.md)
