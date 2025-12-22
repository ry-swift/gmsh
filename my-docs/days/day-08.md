# Day 8：缝合、分割与复合操作

**所属周次**：第2周 - GEO脚本进阶与API入门
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

掌握高级几何操作

---

## 学习文件

- `tutorials/t9.geo`（65行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 精读t9.geo，理解Coherence命令 |
| 09:45-10:30 | 45min | 学习Split Curve曲线分割操作 |
| 10:30-11:15 | 45min | 学习Compound复合实体概念 |
| 11:15-12:00 | 45min | 手动实验各种缝合场景 |
| 14:00-14:45 | 45min | 实验复合曲面网格生成 |
| 14:45-15:30 | 45min | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 学习t9.geo（65行）：
  - Coherence - 几何缝合
  - Split Curve - 曲线分割
  - Compound - 复合实体

---

## 下午任务（2小时）

- [ ] 实验复合曲面和复合体的网格生成
- [ ] 理解这些操作对网格质量的影响

---

## 练习作业

### 1. 【基础】缝合相邻正方形

创建两个相邻的正方形，使用Coherence命令缝合共享边界

```geo
// coherence_squares.geo - 缝合相邻正方形
lc = 0.1;

// 第一个正方形
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

// 第二个正方形（与第一个共享右边）
Point(5) = {2, 0, 0, lc};
Point(6) = {2, 1, 0, lc};
// 注意：Point(2)和Point(3)应该被共享

// 重新定义共享边界的点
Point(12) = {1, 0, 0, lc};  // 与Point(2)重复
Point(13) = {1, 1, 0, lc};  // 与Point(3)重复

Line(5) = {12, 5};
Line(6) = {5, 6};
Line(7) = {6, 13};
Line(8) = {13, 12};

Curve Loop(2) = {5, 6, 7, 8};
Plane Surface(2) = {2};

// 缝合重复的几何实体
Coherence;

// 缝合后，重复的点和线会被合并
Mesh 2;
```

### 2. 【进阶】曲线分割

使用Split Curve在曲线中间插入一个点，观察对网格的影响

```geo
// split_curve.geo - 曲线分割
lc_coarse = 0.2;
lc_fine = 0.05;

// 创建一条长线
Point(1) = {0, 0, 0, lc_coarse};
Point(2) = {4, 0, 0, lc_coarse};
Line(1) = {1, 2};

// 创建分割点（在中点位置，使用更细的网格）
Point(10) = {2, 0, 0, lc_fine};

// 分割曲线
Split Curve {1} Point {10};
// 分割后：原来的Line(1)被删除
// 新生成的曲线编号会自动分配

// 创建矩形的其余部分
Point(3) = {4, 1, 0, lc_coarse};
Point(4) = {0, 1, 0, lc_coarse};

// 注意：需要根据分割后的实际曲线编号来创建
// 可以使用Printf查看当前几何信息

// 查看分割后的曲线
Printf("After split, check the curve numbers in GUI");

// 创建完整几何后生成网格
// Mesh 2;
```

### 3. 【挑战】L形复合曲面

创建一个L形区域，使用Compound Surface将其作为单一曲面进行网格划分

```geo
// compound_surface.geo - L形复合曲面
lc = 0.1;

// L形由两个矩形组成
// 矩形1：水平部分
Point(1) = {0, 0, 0, lc};
Point(2) = {3, 0, 0, lc};
Point(3) = {3, 1, 0, lc};
Point(4) = {0, 1, 0, lc};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};

Curve Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};

// 矩形2：垂直部分
Point(5) = {0, 2, 0, lc};
Point(6) = {1, 2, 0, lc};
Point(7) = {1, 1, 0, lc};

Line(5) = {4, 5};
Line(6) = {5, 6};
Line(7) = {6, 7};
Line(8) = {7, 4};  // 注意：Point(4)是共享点

// 内部边界需要处理...
// 这里简化：先创建两个面，再合并

Curve Loop(2) = {5, 6, 7, 8};
Plane Surface(2) = {2};

// 创建复合曲面
Compound Surface {1, 2};

// 复合曲面会被当作一个整体进行网格划分
// 内部边界处的网格会更加平滑

Mesh 2;
```

---

## 今日检查点

- [ ] 理解Coherence何时自动执行、何时需要手动调用
- [ ] 理解Split Curve对网格节点分布的影响
- [ ] 理解Compound Surface的优缺点

---

## 核心概念

### Coherence（几何缝合）

自动合并重复的几何实体：

```geo
Coherence;  // 缝合所有重复实体

// 或者针对特定实体
Coherence Point {p1, p2};
```

**作用**：
- 合并位置相同的点
- 合并端点相同的曲线
- 确保几何的拓扑一致性

### Split Curve（曲线分割）

在曲线上指定位置插入点：

```geo
Split Curve {line_tag} Point {point_tag};
// 或者
Split Curve {line_tag} Point {p1, p2, p3};  // 多点分割
```

**用途**：
- 在曲线中间增加网格控制点
- 为局部加密创建参考点

### Compound（复合实体）

将多个实体组合为单一实体进行网格划分：

```geo
Compound Curve {curve_tags};    // 复合曲线
Compound Surface {surface_tags}; // 复合曲面
Compound Volume {volume_tags};   // 复合体
```

**优点**：
- 跨越内部边界的平滑网格
- 避免内部边界的网格约束

**缺点**：
- 丢失内部边界信息
- 可能导致某些操作失败

---

## 产出

- 掌握几何修复和组合技术

---

## 导航

- **上一天**：[Day 7 - 第一周总结与项目](day-07.md)
- **下一天**：[Day 9 - 网格尺寸场系统](day-09.md)
- **返回目录**：[学习计划总览](../study.md)
