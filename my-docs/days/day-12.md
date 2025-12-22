# Day 12：OpenCASCADE几何内核

**所属周次**：第2周 - GEO脚本进阶与API入门
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

掌握OCC的强大功能

---

## 学习文件

- `tutorials/t16.geo`（83行）
- `tutorials/t18.geo`（97行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:30 | 30min | 理解CAD内核的概念，对比内置vs OCC |
| 09:30-10:15 | 45min | 精读t16.geo，学习布尔运算 |
| 10:15-11:00 | 45min | 实验Union/Difference/Intersection |
| 11:00-12:00 | 1h | 用布尔运算创建复杂几何 |
| 14:00-14:45 | 45min | 精读t18.geo，学习Fillet/Chamfer |
| 14:45-15:15 | 30min | 实验圆角和倒角效果 |
| 15:15-16:00 | 45min | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 学习t16.geo（83行）：
  - SetFactory("OpenCASCADE")
  - CSG布尔运算
  - BooleanUnion, BooleanDifference, BooleanIntersection

---

## 下午任务（2小时）

- [ ] 学习t18.geo（97行）：
  - OCC高级几何创建
  - Fillet, Chamfer
  - 更复杂的布尔操作
- [ ] 对比内置内核和OCC内核的差异

---

## 练习作业

### 1. 【基础】带圆孔的立方体

使用布尔运算创建一个带圆孔的立方体

```geo
// cube_with_hole.geo - 布尔差集
SetFactory("OpenCASCADE");

// 创建立方体
Box(1) = {0, 0, 0, 2, 2, 2};

// 创建贯穿的圆柱（比立方体稍长，确保完全贯穿）
Cylinder(2) = {1, 1, -0.1, 0, 0, 2.2, 0.3};

// 布尔差集：立方体减去圆柱
BooleanDifference(3) = {Volume{1}; Delete;}{Volume{2}; Delete;};

// 生成3D网格
Mesh 3;
```

### 2. 【进阶】布尔并集与圆角

创建两个相交的球体，并为交界处添加圆角

```geo
// spheres_union.geo - 布尔并集 + 圆角
SetFactory("OpenCASCADE");

// 两个相交的球体
Sphere(1) = {0, 0, 0, 1};
Sphere(2) = {1.2, 0, 0, 1};  // 中心距1.2，有重叠

// 布尔并集
BooleanUnion(3) = {Volume{1}; Delete;}{Volume{2}; Delete;};

// 查找相交边界的边，添加圆角
// 使用GUI查看边的编号，然后：
// Fillet{3}{边编号列表}{圆角半径};

// 示例（边编号需要根据实际情况调整）：
// Fillet{3}{1}{0.1};

Mesh 3;
```

### 3. 【挑战】法兰盘

创建一个机械零件：带螺栓孔的法兰盘

```geo
// flange.geo - 法兰盘
SetFactory("OpenCASCADE");

// 主体：圆盘
Cylinder(1) = {0, 0, 0, 0, 0, 0.5, 2};  // 半径2，高0.5

// 中心孔
Cylinder(2) = {0, 0, -0.1, 0, 0, 0.7, 0.5};  // 半径0.5

// 减去中心孔
BooleanDifference(3) = {Volume{1}; Delete;}{Volume{2}; Delete;};

// 创建6个螺栓孔（圆周均布）
For i In {0:5}
    angle = i * Pi / 3;  // 60度间隔
    x = 1.5 * Cos(angle);
    y = 1.5 * Sin(angle);
    Cylinder(10+i) = {x, y, -0.1, 0, 0, 0.7, 0.2};  // 半径0.2
EndFor

// 减去所有螺栓孔
BooleanDifference(100) = {Volume{3}; Delete;}{Volume{10:15}; Delete;};

// 可选：为孔边缘添加倒角
// 获取边编号后使用：
// Chamfer{100}{边列表}{倒角尺寸};

Mesh.CharacteristicLengthMax = 0.15;
Mesh 3;
```

---

## 今日检查点

- [ ] 理解SetFactory("OpenCASCADE")的作用
- [ ] 掌握三种布尔运算的语法和用途
- [ ] 理解Delete选项的作用（删除原始几何）

---

## 核心概念

### 内置内核 vs OpenCASCADE

| 特性 | 内置内核 | OpenCASCADE |
|------|----------|-------------|
| 启用方式 | 默认 | SetFactory("OpenCASCADE") |
| 几何精度 | 一般 | 高精度 |
| 布尔运算 | 有限支持 | 完整支持 |
| STEP/IGES导入 | 不支持 | 支持 |
| 圆角/倒角 | 不支持 | 支持 |
| 复杂曲面 | 有限 | 完整支持 |

### SetFactory

```geo
SetFactory("OpenCASCADE");  // 启用OCC内核
SetFactory("Built-in");      // 使用内置内核（默认）
```

### 布尔运算

#### BooleanUnion（并集）

```geo
BooleanUnion(new_tag) = {Volume{v1}; Delete;}{Volume{v2}; Delete;};
// 结果：两个体的并集
```

#### BooleanDifference（差集）

```geo
BooleanDifference(new_tag) = {Volume{v1}; Delete;}{Volume{v2}; Delete;};
// 结果：v1 减去 v2
```

#### BooleanIntersection（交集）

```geo
BooleanIntersection(new_tag) = {Volume{v1}; Delete;}{Volume{v2}; Delete;};
// 结果：两个体的交集
```

#### BooleanFragments（分割）

```geo
BooleanFragments{Volume{v1}; Delete;}{Volume{v2}; Delete;};
// 结果：保留所有分割后的部分
```

### Delete 选项

- `Delete;`：操作后删除原始几何
- 不使用：保留原始几何（可能导致重叠）

### Fillet（圆角）

```geo
// 为边添加圆角
Fillet{volume_tag}{edge_tags}{radius};

// 示例
Fillet{1}{5, 6, 7}{0.1};  // 为边5,6,7添加半径0.1的圆角
```

### Chamfer（倒角）

```geo
// 为边添加倒角
Chamfer{volume_tag}{edge_tags}{distance1, distance2};

// 等距倒角
Chamfer{1}{5}{0.1};

// 不等距倒角
Chamfer{1}{5}{0.1, 0.2};
```

### OCC基本几何命令

```geo
SetFactory("OpenCASCADE");

// 2D
Rectangle(tag) = {x, y, z, dx, dy};
Disk(tag) = {x, y, z, rx, ry};  // 椭圆，rx=ry时为圆

// 3D
Box(tag) = {x, y, z, dx, dy, dz};
Sphere(tag) = {x, y, z, radius};
Cylinder(tag) = {x, y, z, ax, ay, az, radius};
Cone(tag) = {x, y, z, ax, ay, az, r1, r2};
Torus(tag) = {x, y, z, r1, r2};  // 大半径r1，小半径r2
```

---

## OCC高级功能

### 导入CAD文件

```geo
SetFactory("OpenCASCADE");
Merge "model.step";  // 导入STEP文件
Merge "model.iges";  // 导入IGES文件
Merge "model.brep";  // 导入BREP文件
```

### 几何修复

```geo
// OCC自动修复导入的几何
// 通常在Merge后自动进行

// 手动触发修复
Coherence;
```

### 几何变换

```geo
// 平移
Translate {dx, dy, dz} { Volume{v}; }

// 旋转
Rotate { {ax, ay, az}, {px, py, pz}, angle } { Volume{v}; }

// 缩放
Dilate { {cx, cy, cz}, {sx, sy, sz} } { Volume{v}; }

// 镜像
Symmetry { a, b, c, d } { Volume{v}; }
// 关于平面 ax + by + cz + d = 0 镜像
```

---

## 产出

- 能使用OCC创建复杂几何

---

## 导航

- **上一天**：[Day 11 - 边界层与网格优化](day-11.md)
- **下一天**：[Day 13 - C++ API入门](day-13.md)
- **返回目录**：[学习计划总览](../study.md)
