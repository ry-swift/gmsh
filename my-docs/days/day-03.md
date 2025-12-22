# Day 3：GEO脚本语言基础（二）

**所属周次**：第1周 - 环境搭建与初识Gmsh
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

掌握变换操作和3D几何

---

## 学习文件

- `tutorials/t2.geo`（122行）
- `tutorials/t3.geo`（99行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习t2.geo的变换操作 |
| 10:00-11:00 | 1h | 学习Extrude拉伸创建3D几何 |
| 11:00-12:00 | 1h | 学习t3.geo的ONELAB参数化 |
| 14:00-15:00 | 1h | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 学习变换操作：
  - Translate() - 平移
  - Rotate() - 旋转
  - Extrude() - 拉伸
- [ ] 理解Volume()的创建方式

---

## 下午任务（2小时）

- [ ] 学习`tutorials/t3.geo`（99行）
- [ ] 理解ONELAB参数化概念
- [ ] 实验挤出网格的不同模式
- [ ] 练习：创建一个带孔的3D方块

---

## 练习作业

### 1. 【基础】拉伸长方体

将Day2的正方形拉伸成高度为0.5的长方体

```geo
// cube.geo - 拉伸创建长方体
lc = 0.1;

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

// 拉伸创建3D几何
Extrude {0, 0, 0.5} { Surface{1}; }
```

### 2. 【进阶】旋转体

创建一个通过旋转2D轮廓生成的简单圆柱

```geo
// cylinder_by_rotate.geo - 旋转创建圆柱
lc = 0.1;

// 创建矩形轮廓（将绕Y轴旋转）
Point(1) = {0.5, 0, 0, lc};
Point(2) = {1, 0, 0, lc};
Point(3) = {1, 2, 0, lc};
Point(4) = {0.5, 2, 0, lc};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};

Curve Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};

// 绕Y轴旋转90度（Pi/2弧度）
// Extrude { {轴向量}, {轴上一点}, 角度 } { Surface{1}; }
Extrude { {0, 1, 0}, {0, 0, 0}, Pi/2 } { Surface{1}; }
```

### 3. 【挑战】L型支架

创建一个L型3D支架（两个相连的长方体）

```geo
// l_bracket.geo - L型支架
lc = 0.1;

// 底部水平部分
Point(1) = {0, 0, 0, lc};
Point(2) = {3, 0, 0, lc};
Point(3) = {3, 1, 0, lc};
Point(4) = {1, 1, 0, lc};
Point(5) = {1, 2, 0, lc};
Point(6) = {0, 2, 0, lc};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 5};
Line(5) = {5, 6};
Line(6) = {6, 1};

Curve Loop(1) = {1, 2, 3, 4, 5, 6};
Plane Surface(1) = {1};

// 拉伸成3D
Extrude {0, 0, 0.5} { Surface{1}; }
```

---

## 今日检查点

- [ ] 理解Extrude的返回值结构
- [ ] 能区分平移拉伸和旋转拉伸
- [ ] 理解ONELAB参数如何在GUI中显示

---

## 核心概念

### Extrude 平移拉伸

```geo
// 沿向量拉伸
out[] = Extrude {dx, dy, dz} { Surface{tag}; };
// out[0] = 顶部新生成的面
// out[1] = 新生成的体积
```

### Extrude 旋转拉伸

```geo
// 绕轴旋转拉伸
Extrude { {ax, ay, az}, {px, py, pz}, angle } { Surface{tag}; }
// ax, ay, az: 旋转轴方向
// px, py, pz: 轴上一点
// angle: 旋转角度（弧度）
```

### Translate 平移

```geo
Translate {dx, dy, dz} { Point{1}; Line{2}; Surface{3}; }
```

### Rotate 旋转

```geo
// 绕轴旋转
Rotate { {ax, ay, az}, {px, py, pz}, angle } { entities; }
```

### ONELAB 参数化

```geo
// 定义可调参数
DefineConstant[
  length = {1.0, Min 0.1, Max 5.0, Step 0.1, Name "Parameters/Length"}
];

// 使用参数
Point(1) = {0, 0, 0, 0.1};
Point(2) = {length, 0, 0, 0.1};
```

在GUI中，这些参数会显示为可调的滑块或输入框。

---

## 产出

- 能创建简单的3D几何
- 理解参数化脚本的基本概念

---

## 导航

- **上一天**：[Day 2 - GEO脚本语言基础（一）](day-02.md)
- **下一天**：[Day 4 - 循环与控制结构](day-04.md)
- **返回目录**：[学习计划总览](../study.md)
