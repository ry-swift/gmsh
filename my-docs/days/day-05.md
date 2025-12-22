# Day 5：结构化网格

**所属周次**：第1周 - 环境搭建与初识Gmsh
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

掌握Transfinite（结构化）网格

---

## 学习文件

- `tutorials/t6.geo`（95行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 学习Transfinite Curve的概念和语法 |
| 09:45-10:30 | 45min | 学习Transfinite Surface的设置 |
| 10:30-11:15 | 45min | 学习Transfinite Volume的设置 |
| 11:15-12:00 | 45min | 学习Recombine生成四边形网格 |
| 14:00-14:45 | 45min | 精读t6.geo完整代码 |
| 14:45-15:30 | 45min | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 学习Transfinite概念：
  - Transfinite Curve - 曲线节点分布
  - Transfinite Surface - 结构化曲面网格
  - Transfinite Volume - 结构化体网格
- [ ] 学习Recombine生成四边形/六面体网格

---

## 下午任务（2小时）

- [ ] 精读t6.geo，理解完整的结构化网格流程
- [ ] 实验不同的节点分布模式
- [ ] 练习：创建自己的结构化网格

---

## 练习作业

### 1. 【基础】正方形结构化网格

创建一个10x10的结构化正方形网格

```geo
// structured_square.geo - 结构化正方形网格
lc = 0.1;
n = 10;  // 每边节点数

Point(1) = {0, 0, 0, lc};
Point(2) = {1, 0, 0, lc};
Point(3) = {1, 1, 0, lc};
Point(4) = {0, 1, 0, lc};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};

Curve Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};

// 设置结构化网格
Transfinite Curve {1, 2, 3, 4} = n Using Progression 1;
Transfinite Surface {1};

// 生成四边形网格（可选）
Recombine Surface {1};

Mesh 2;
```

### 2. 【进阶】非均匀节点分布

创建一个边界层效应的网格，靠近一端节点密集

```geo
// graded_mesh.geo - 渐变网格
lc = 0.1;
n = 20;
ratio = 1.2;  // 增长比

Point(1) = {0, 0, 0, lc};
Point(2) = {1, 0, 0, lc};
Point(3) = {1, 1, 0, lc};
Point(4) = {0, 1, 0, lc};

Line(1) = {1, 2};  // 底边
Line(2) = {2, 3};  // 右边
Line(3) = {3, 4};  // 顶边
Line(4) = {4, 1};  // 左边

Curve Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};

// 左右边使用渐变分布（从下到上变稀疏）
Transfinite Curve {2} = n Using Progression ratio;
Transfinite Curve {4} = n Using Progression 1/ratio;

// 上下边均匀分布
Transfinite Curve {1, 3} = n Using Progression 1;

Transfinite Surface {1} = {1, 2, 3, 4};
Recombine Surface {1};

Mesh 2;
```

### 3. 【挑战】六面体结构化体网格

创建一个立方体的全六面体网格

```geo
// hex_cube.geo - 六面体立方体网格
lc = 0.1;
n = 5;  // 每边节点数

// 底面四个点
Point(1) = {0, 0, 0, lc};
Point(2) = {1, 0, 0, lc};
Point(3) = {1, 1, 0, lc};
Point(4) = {0, 1, 0, lc};

// 顶面四个点
Point(5) = {0, 0, 1, lc};
Point(6) = {1, 0, 1, lc};
Point(7) = {1, 1, 1, lc};
Point(8) = {0, 1, 1, lc};

// 底面线
Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};

// 顶面线
Line(5) = {5, 6};
Line(6) = {6, 7};
Line(7) = {7, 8};
Line(8) = {8, 5};

// 竖直线
Line(9) = {1, 5};
Line(10) = {2, 6};
Line(11) = {3, 7};
Line(12) = {4, 8};

// 各个面
Curve Loop(1) = {1, 2, 3, 4};      // 底面
Curve Loop(2) = {5, 6, 7, 8};      // 顶面
Curve Loop(3) = {1, 10, -5, -9};   // 前面
Curve Loop(4) = {2, 11, -6, -10};  // 右面
Curve Loop(5) = {3, 12, -7, -11};  // 后面
Curve Loop(6) = {4, 9, -8, -12};   // 左面

Plane Surface(1) = {1};
Plane Surface(2) = {2};
Plane Surface(3) = {3};
Plane Surface(4) = {4};
Plane Surface(5) = {5};
Plane Surface(6) = {6};

// 创建体积
Surface Loop(1) = {1, 2, 3, 4, 5, 6};
Volume(1) = {1};

// 设置结构化网格
Transfinite Curve {:} = n;
Transfinite Surface {:};
Recombine Surface {:};
Transfinite Volume {1};

Mesh 3;
```

---

## 今日检查点

- [ ] 理解Transfinite和非结构化网格的区别
- [ ] 能设置Progression参数控制节点分布
- [ ] 理解Recombine的作用（三角形→四边形）

---

## 核心概念

### Transfinite Curve

控制曲线上的节点数量和分布：

```geo
// 均匀分布
Transfinite Curve {line_tags} = n_nodes;

// 渐变分布（等比数列）
Transfinite Curve {line_tags} = n_nodes Using Progression ratio;
// ratio > 1: 从起点到终点变稀疏
// ratio < 1: 从起点到终点变密集

// 双向渐变（中间密两端疏，或相反）
Transfinite Curve {line_tags} = n_nodes Using Bump coef;
```

### Transfinite Surface

创建结构化曲面网格：

```geo
// 自动确定角点（仅对四边形曲面有效）
Transfinite Surface {surface_tag};

// 手动指定角点顺序
Transfinite Surface {surface_tag} = {p1, p2, p3, p4};
// 角点必须按逆时针或顺时针排列
```

### Recombine

将三角形网格合并为四边形：

```geo
Recombine Surface {surface_tags};
Recombine Surface {:};  // 所有面
```

### Transfinite Volume

创建结构化体网格（仅六面体）：

```geo
Transfinite Volume {volume_tag};
Transfinite Volume {volume_tag} = {corner_points};
```

### 结构化网格的限制

1. Transfinite Surface 仅支持3边或4边的面
2. Transfinite Volume 仅支持棱柱或六面体形状
3. 对边的节点数必须相同

---

## 产出

- 能创建结构化的2D和3D网格

---

## 导航

- **上一天**：[Day 4 - 循环与控制结构](day-04.md)
- **下一天**：[Day 6 - 曲面与高阶几何](day-06.md)
- **返回目录**：[学习计划总览](../study.md)
