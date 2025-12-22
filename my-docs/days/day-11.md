# Day 11：边界层与网格优化

**所属周次**：第2周 - GEO脚本进阶与API入门
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

学习工程实用的边界层网格

---

## 学习文件

- `tutorials/t13.geo`（65行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 理解边界层网格的物理意义（CFD/传热） |
| 09:45-10:30 | 45min | 精读t13.geo，学习BoundaryLayer Field |
| 10:30-11:15 | 45min | 学习增长比、层数、厚度参数 |
| 11:15-12:00 | 45min | 实验边界层与自由网格的过渡 |
| 14:00-14:30 | 30min | 学习Mesh.Algorithm选项 |
| 14:30-15:00 | 30min | 学习Mesh.Optimize优化级别 |
| 15:00-15:30 | 30min | 学习Mesh.SmoothNormals |
| 15:30-16:00 | 30min | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 学习边界层概念：
  - BoundaryLayer Field的参数
  - 增长比、层数、厚度控制
  - 边界层与自由网格的过渡

---

## 下午任务（2小时）

- [ ] 学习网格优化选项：
  - Mesh.Algorithm - 算法选择
  - Mesh.Optimize - 优化级别
  - Mesh.SmoothNormals - 法向平滑
- [ ] 实验不同优化参数的效果

---

## 练习作业

### 1. 【基础】圆柱边界层

为一个圆柱体的外表面创建边界层网格

```geo
// cylinder_boundary_layer.geo - 圆柱边界层
SetFactory("OpenCASCADE");

// 创建圆柱
Cylinder(1) = {0, 0, 0, 0, 0, 2, 0.5};  // 半径0.5，高度2

// 获取圆柱侧面的tag（通常是2）
// 使用GUI的Tools -> Visibility查看

// 定义边界层场
Field[1] = BoundaryLayer;
Field[1].CurvesList = {};      // 边界曲线列表
Field[1].PointsList = {};       // 边界点列表
Field[1].SurfacesList = {2};    // 边界面列表（圆柱侧面）
Field[1].Thickness = 0.05;      // 边界层总厚度
Field[1].Ratio = 1.2;           // 增长比
Field[1].NbLayers = 5;          // 层数
Field[1].AnisoMax = 1.0;        // 最大各向异性比
Field[1].Quads = 1;             // 使用四边形（1）还是三角形（0）

BoundaryLayer Field = 1;
Mesh 3;
```

### 2. 【进阶】网格算法对比

对比不同网格算法的效果

```geo
// algorithm_comparison.geo - 网格算法对比
SetFactory("OpenCASCADE");
Rectangle(1) = {0, 0, 0, 1, 1};

// 算法选项：
// 1 = MeshAdapt
// 2 = Automatic
// 5 = Delaunay
// 6 = Frontal-Delaunay
// 7 = BAMG
// 8 = Frontal-Delaunay for Quads
// 9 = Packing of Parallelograms

Mesh.Algorithm = 5;  // 尝试修改这个值
Mesh.CharacteristicLengthMax = 0.1;
Mesh 2;

// 生成网格后，在GUI中查看"Tools -> Statistics"
// 对比不同算法的：单元数量、质量分布、生成时间
```

### 3. 【挑战】简化翼型边界层

为NACA翼型截面创建带边界层的网格（简化版）

```geo
// airfoil_boundary_layer.geo - 简化翼型边界层
SetFactory("OpenCASCADE");

// 简化翼型：使用椭圆近似
Disk(1) = {0, 0, 0, 1, 0.12};  // 长轴1，短轴0.12

// 外部流场区域
Rectangle(2) = {-2, -1.5, 0, 5, 3};

// 布尔差集：流场减去翼型
BooleanDifference(3) = {Surface{2}; Delete;}{Surface{1}; Delete;};

// 为翼型边界创建边界层
// 首先需要找到翼型边界的曲线编号
// 使用 Coherence 确保几何一致性
Coherence;

// 设置边界层（需要根据实际曲线编号调整）
Field[1] = BoundaryLayer;
Field[1].CurvesList = {5};  // 翼型边界曲线（编号可能不同）
Field[1].Thickness = 0.02;
Field[1].Ratio = 1.15;
Field[1].NbLayers = 8;
Field[1].Quads = 1;

BoundaryLayer Field = 1;

Mesh.CharacteristicLengthMax = 0.2;
Mesh 2;
```

---

## 今日检查点

- [ ] 理解边界层在CFD中的重要性（壁面附近速度梯度大）
- [ ] 能设置合理的增长比（通常1.1-1.3）
- [ ] 理解不同网格算法的适用场景

---

## 核心概念

### BoundaryLayer Field

创建边界层网格的专用场：

```geo
Field[n] = BoundaryLayer;

// 指定边界
Field[n].CurvesList = {c1, c2, ...};    // 2D边界曲线
Field[n].SurfacesList = {s1, s2, ...};  // 3D边界曲面

// 边界层参数
Field[n].Thickness = 0.1;     // 总厚度
Field[n].Ratio = 1.2;         // 增长比（层间厚度比）
Field[n].NbLayers = 10;       // 层数

// 可选参数
Field[n].AnisoMax = 100;      // 最大各向异性比
Field[n].Quads = 1;           // 使用四边形（1）或三角形（0）
Field[n].hwall_n = 0.001;     // 第一层厚度（替代Thickness/NbLayers）
Field[n].hfar = 0.1;          // 远场尺寸

// 激活边界层场
BoundaryLayer Field = n;
```

### 增长比的选择

| 应用场景 | 推荐增长比 | 原因 |
|----------|-----------|------|
| 层流CFD | 1.1-1.15 | 需要较细的分辨率 |
| 湍流CFD | 1.2-1.3 | 可以接受较大梯度 |
| 传热分析 | 1.15-1.2 | 热边界层较薄 |
| 结构分析 | 1.3-1.5 | 应力梯度相对较小 |

### 第一层厚度的估算

CFD中，第一层厚度通常根据y+确定：

```
y+ = (y * u_tau) / nu

其中：
y = 第一层厚度
u_tau = 壁面摩擦速度
nu = 运动粘度
```

### 网格优化选项

```geo
// 优化开关
Mesh.Optimize = 1;              // 启用优化
Mesh.OptimizeNetgen = 1;        // 使用Netgen优化（3D）

// 优化参数
Mesh.OptimizeThreshold = 0.3;   // 优化阈值
Mesh.Smoothing = 5;             // 平滑迭代次数

// 高阶网格优化
Mesh.HighOrderOptimize = 2;     // 高阶优化级别
```

### 网格质量评估

在GUI中：Tools -> Statistics

关键指标：
- **质量分布直方图**：应集中在高质量区域
- **最小质量**：不应低于0.1
- **平均质量**：应大于0.7
- **单元数量**：检查是否在合理范围

---

## 边界层网格的物理背景

### 为什么需要边界层网格？

在流体力学中，靠近壁面的区域（边界层）存在：
1. 大的速度梯度
2. 强烈的剪切应力
3. 湍流结构的发展

准确捕捉这些现象需要：
- 壁面法向的细网格
- 壁面切向可以较粗
- 平滑的网格过渡

### y+的工程意义

y+是无量纲壁面距离，用于判断第一层网格是否足够细：

| y+ 范围 | 模型类型 | 说明 |
|---------|----------|------|
| < 1 | 低Re模型 | 直接解析粘性底层 |
| 30-300 | 壁面函数 | 使用壁面函数近似 |
| 1-5 | 增强壁面处理 | 混合方法 |

---

## 产出

- 能生成带边界层的高质量网格

---

## 导航

- **上一天**：[Day 10 - 网格细化与各向异性网格](day-10.md)
- **下一天**：[Day 12 - OpenCASCADE几何内核](day-12.md)
- **返回目录**：[学习计划总览](../study.md)
