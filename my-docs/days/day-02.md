# Day 2：GEO脚本语言基础（一）

**所属周次**：第1周 - 环境搭建与初识Gmsh
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

掌握点、线、面的基本创建

---

## 学习文件

- `tutorials/t1.geo`（144行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 逐行阅读t1.geo，做注释笔记 |
| 10:00-11:00 | 1h | 学习Point、Line、Curve Loop、Plane Surface |
| 11:00-12:00 | 1h | 动手修改t1.geo，观察变化 |
| 14:00-15:00 | 1h | 完成练习作业并验证 |

---

## 上午任务（2小时）

- [ ] 逐行阅读t1.geo，理解每条命令的含义
- [ ] 学习关键概念：
  - Point() - 点的创建
  - Line() - 线的创建
  - Curve Loop() - 曲线环
  - Plane Surface() - 平面

---

## 下午任务（2小时）

- [ ] 动手练习：修改t1.geo创建不同形状的几何
- [ ] 学习Physical Group的概念和用途
- [ ] 实验网格生成命令：

```bash
gmsh t1.geo -2    # 2D网格
gmsh t1.geo -3    # 3D网格
```

---

## 练习作业

### 1. 【基础】创建正方形平面

创建一个边长为1.0的正方形平面，保存为`square.geo`

```geo
// 提示：需要4个点、4条线、1个曲线环、1个平面
Point(1) = {0, 0, 0, 0.1};
// ... 补充完整
```

### 2. 【进阶】创建等边三角形

创建一个等边三角形，顶点坐标自定义，保存为`triangle.geo`

**提示**：
- 等边三角形顶点坐标可以设为：
  - (0, 0, 0)
  - (1, 0, 0)
  - (0.5, √3/2, 0) ≈ (0.5, 0.866, 0)

### 3. 【挑战】带圆孔的正方形板

创建一个带圆形孔的正方形板

**提示**：使用Circle命令创建圆，然后用两个Curve Loop定义带孔的面

---

## 今日检查点

- [ ] 理解Point的4个参数含义（x, y, z, mesh_size）
- [ ] 能解释Curve Loop中数字正负号的意义
- [ ] 能独立创建简单的2D几何并生成网格

---

## 参考代码

### 正方形示例 (square.geo)

```geo
// square.geo - 正方形示例
lc = 0.1;  // 网格尺寸

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

// 生成网格：gmsh square.geo -2
```

### 等边三角形示例 (triangle.geo)

```geo
// triangle.geo - 等边三角形示例
lc = 0.1;

Point(1) = {0, 0, 0, lc};
Point(2) = {1, 0, 0, lc};
Point(3) = {0.5, 0.866, 0, lc};  // sqrt(3)/2 ≈ 0.866

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 1};

Curve Loop(1) = {1, 2, 3};
Plane Surface(1) = {1};
```

### 带圆孔的正方形板示例 (plate_with_hole.geo)

```geo
// plate_with_hole.geo - 带圆孔的正方形板
lc = 0.1;
lc_hole = 0.05;  // 孔边缘用更细的网格

// 外边界（正方形）
Point(1) = {0, 0, 0, lc};
Point(2) = {2, 0, 0, lc};
Point(3) = {2, 2, 0, lc};
Point(4) = {0, 2, 0, lc};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};

// 圆孔（中心在(1,1)，半径0.3）
Point(5) = {1, 1, 0, lc_hole};    // 圆心
Point(6) = {1.3, 1, 0, lc_hole};  // 右
Point(7) = {1, 1.3, 0, lc_hole};  // 上
Point(8) = {0.7, 1, 0, lc_hole};  // 左
Point(9) = {1, 0.7, 0, lc_hole};  // 下

Circle(5) = {6, 5, 7};  // 从右到上
Circle(6) = {7, 5, 8};  // 从上到左
Circle(7) = {8, 5, 9};  // 从左到下
Circle(8) = {9, 5, 6};  // 从下到右

// 曲线环
Curve Loop(1) = {1, 2, 3, 4};       // 外边界
Curve Loop(2) = {5, 6, 7, 8};       // 内边界（孔）

// 带孔的平面（第二个参数是要减去的孔）
Plane Surface(1) = {1, 2};
```

---

## 核心概念

### Point 语法

```geo
Point(tag) = {x, y, z, mesh_size};
```

- `tag`：点的唯一编号
- `x, y, z`：坐标
- `mesh_size`：该点附近的目标网格尺寸

### Line 语法

```geo
Line(tag) = {start_point, end_point};
```

### Curve Loop 语法

```geo
Curve Loop(tag) = {line1, line2, ...};
```

- 正数：沿线的正方向
- 负数：沿线的反方向
- 线必须首尾相连形成闭环

### Physical Group

用于定义边界条件和材料区域：

```geo
Physical Curve("boundary_name") = {line_tags};
Physical Surface("region_name") = {surface_tags};
```

---

## 产出

- 能独立编写简单的2D几何脚本

---

## 导航

- **上一天**：[Day 1 - 环境准备与第一次运行](day-01.md)
- **下一天**：[Day 3 - GEO脚本语言基础（二）](day-03.md)
- **返回目录**：[学习计划总览](../study.md)
