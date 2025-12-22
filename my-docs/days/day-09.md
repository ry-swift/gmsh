# Day 9：网格尺寸场系统

**所属周次**：第2周 - GEO脚本进阶与API入门
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

深入理解Field系统

---

## 学习文件

- `tutorials/t10.geo`（117行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:30 | 30min | 学习Distance场的概念和用法 |
| 09:30-10:00 | 30min | 学习Threshold场的阈值控制 |
| 10:00-10:30 | 30min | 学习MathEval场的数学表达式 |
| 10:30-11:00 | 30min | 学习Box场的区域控制 |
| 11:00-12:00 | 1h | 精读t10.geo完整代码 |
| 14:00-14:30 | 30min | 学习Min/Max场组合 |
| 14:30-15:00 | 30min | 学习Restrict场的范围限制 |
| 15:00-16:00 | 1h | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 学习各种Field类型：
  - Distance - 距离场
  - Threshold - 阈值场
  - MathEval - 数学表达式场
  - Box - 盒形区域场

---

## 下午任务（2小时）

- [ ] 学习场的组合：
  - Min/Max - 取最小/最大值
  - Restrict - 限制范围
- [ ] 练习：创建局部加密的网格

---

## 练习作业

### 1. 【基础】距离场控制网格密度

创建一个正方形，在中心点附近使用Distance场加密网格

```geo
// distance_field.geo - 距离场控制网格密度
SetFactory("OpenCASCADE");
Rectangle(1) = {0, 0, 0, 2, 2};

// 在中心创建一个点用于距离计算
Point(100) = {1, 1, 0};

// 创建距离场
Field[1] = Distance;
Field[1].PointsList = {100};

// 创建阈值场：距离近时网格细，距离远时网格粗
Field[2] = Threshold;
Field[2].InField = 1;
Field[2].SizeMin = 0.02;   // 最小网格尺寸
Field[2].SizeMax = 0.2;    // 最大网格尺寸
Field[2].DistMin = 0.1;    // 开始过渡距离
Field[2].DistMax = 0.5;    // 结束过渡距离

Background Field = 2;
Mesh 2;
```

### 2. 【进阶】数学表达式场

使用MathEval创建一个网格密度随Y坐标变化的场

```geo
// matheval_field.geo - 数学表达式场
SetFactory("OpenCASCADE");
Rectangle(1) = {0, 0, 0, 1, 1};

// 网格尺寸随Y线性变化：底部细(0.01)，顶部粗(0.1)
Field[1] = MathEval;
Field[1].F = "0.01 + 0.09 * y";  // y从0到1

Background Field = 1;
Mesh 2;
// 观察网格在Y方向的渐变效果
```

### 3. 【挑战】多场组合

组合多个Field：在矩形的左下角和右上角同时加密

```geo
// multi_field.geo - 多场组合
SetFactory("OpenCASCADE");
Rectangle(1) = {0, 0, 0, 2, 2};

// 左下角加密点
Point(100) = {0, 0, 0};
// 右上角加密点
Point(101) = {2, 2, 0};

// 为左下角创建距离场和阈值场
Field[1] = Distance;
Field[1].PointsList = {100};

Field[2] = Threshold;
Field[2].InField = 1;
Field[2].SizeMin = 0.02;
Field[2].SizeMax = 0.15;
Field[2].DistMin = 0.1;
Field[2].DistMax = 0.5;

// 为右上角创建距离场和阈值场
Field[3] = Distance;
Field[3].PointsList = {101};

Field[4] = Threshold;
Field[4].InField = 3;
Field[4].SizeMin = 0.02;
Field[4].SizeMax = 0.15;
Field[4].DistMin = 0.1;
Field[4].DistMax = 0.5;

// 使用Min场组合：取两个场的最小值
Field[5] = Min;
Field[5].FieldsList = {2, 4};

Background Field = 5;
Mesh 2;
```

---

## 今日检查点

- [ ] 能解释Distance、Threshold、MathEval、Box四种场的作用
- [ ] 理解Min/Max场如何组合多个场
- [ ] 能根据需求选择合适的场类型

---

## 核心概念

### Field系统概述

Field是Gmsh中控制网格尺寸的强大工具。每个Field计算空间中每一点的目标网格尺寸。

### Distance 距离场

计算到指定几何实体的距离：

```geo
Field[n] = Distance;
Field[n].PointsList = {p1, p2, ...};   // 点列表
Field[n].CurvesList = {c1, c2, ...};   // 曲线列表
Field[n].SurfacesList = {s1, s2, ...}; // 曲面列表
```

### Threshold 阈值场

基于另一个场的值，进行线性插值确定网格尺寸：

```geo
Field[n] = Threshold;
Field[n].InField = m;        // 输入场编号
Field[n].SizeMin = 0.01;     // 最小尺寸
Field[n].SizeMax = 0.1;      // 最大尺寸
Field[n].DistMin = 0.1;      // 开始过渡的距离
Field[n].DistMax = 1.0;      // 结束过渡的距离
```

### MathEval 数学表达式场

使用数学表达式定义尺寸：

```geo
Field[n] = MathEval;
Field[n].F = "0.1 + 0.5*x + Sin(y)";  // 表达式
// 可用变量：x, y, z
// 可用函数：Sin, Cos, Sqrt, Exp, Log, Abs, Min, Max...
```

### Box 盒形区域场

在指定区域内外使用不同尺寸：

```geo
Field[n] = Box;
Field[n].VIn = 0.01;         // 盒内尺寸
Field[n].VOut = 0.1;         // 盒外尺寸
Field[n].XMin = 0.0; Field[n].XMax = 1.0;
Field[n].YMin = 0.0; Field[n].YMax = 1.0;
Field[n].ZMin = 0.0; Field[n].ZMax = 1.0;
Field[n].Thickness = 0.1;    // 过渡厚度（可选）
```

### Min/Max 场组合

取多个场的最小或最大值：

```geo
Field[n] = Min;  // 或 Max
Field[n].FieldsList = {f1, f2, f3};
```

### Restrict 限制场

将场限制在特定区域：

```geo
Field[n] = Restrict;
Field[n].InField = m;
Field[n].SurfacesList = {s1, s2};  // 仅在这些面上有效
```

### 设置背景场

```geo
Background Field = n;
```

---

## 常用Field类型速查

| Field类型 | 用途 |
|-----------|------|
| Distance | 计算到几何的距离 |
| Threshold | 基于距离的线性插值 |
| MathEval | 数学表达式 |
| Box | 盒形区域 |
| Cylinder | 圆柱区域 |
| Ball | 球形区域 |
| Min | 取最小值 |
| Max | 取最大值 |
| Restrict | 限制到特定区域 |
| Attractor | 吸引子（类似Distance） |
| BoundaryLayer | 边界层 |

---

## 产出

- 能使用Field控制复杂的网格分布

---

## 导航

- **上一天**：[Day 8 - 缝合、分割与复合操作](day-08.md)
- **下一天**：[Day 10 - 网格细化与各向异性网格](day-10.md)
- **返回目录**：[学习计划总览](../study.md)
