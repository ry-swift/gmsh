# Day 10：网格细化与各向异性网格

**所属周次**：第2周 - GEO脚本进阶与API入门
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

掌握自适应网格技术

---

## 学习文件

- `tutorials/t11.geo`（69行）
- `tutorials/t12.geo`（64行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 精读t11.geo，理解网格细化原理 |
| 09:45-10:30 | 45min | 学习RefineByField命令 |
| 10:30-11:15 | 45min | 实验不同细化策略的效果 |
| 11:15-12:00 | 45min | 记录笔记，总结细化方法 |
| 14:00-14:45 | 45min | 精读t12.geo，学习各向异性网格 |
| 14:45-15:30 | 45min | 理解混合单元网格概念 |
| 15:30-16:00 | 30min | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 学习t11.geo（69行）：
  - 不规则网格细化
  - RefineByField命令

---

## 下午任务（2小时）

- [ ] 学习t12.geo（64行）：
  - 各向异性网格概念
  - 混合单元网格
- [ ] 理解各向异性对数值模拟的意义

---

## 练习作业

### 1. 【基础】整体网格细化

创建一个粗网格，然后使用RefineMesh命令整体细化

```geo
// refine_mesh.geo - 整体网格细化
SetFactory("OpenCASCADE");
Rectangle(1) = {0, 0, 0, 1, 1};

// 先生成粗网格
Mesh.CharacteristicLengthMax = 0.3;
Mesh 2;

// 整体细化（每个三角形分成4个）
RefineMesh;

// 再次细化
RefineMesh;

// 观察细化前后的单元数量变化
// 使用GUI的 Tools -> Statistics 查看
```

### 2. 【进阶】基于场的局部细化

使用RefineByField根据场条件局部细化

```geo
// refine_by_field.geo - 基于场的局部细化
SetFactory("OpenCASCADE");
Rectangle(1) = {0, 0, 0, 2, 2};

// 创建一个Box场，定义细化区域（中心1x1区域）
Field[1] = Box;
Field[1].VIn = 0.05;    // 盒内尺寸
Field[1].VOut = 0.2;    // 盒外尺寸
Field[1].XMin = 0.5;
Field[1].XMax = 1.5;
Field[1].YMin = 0.5;
Field[1].YMax = 1.5;

Background Field = 1;
Mesh 2;

// 可选：进一步细化
// RefineByField[1];
```

### 3. 【挑战】各向异性网格

创建一个细长矩形，使用各向异性网格使单元沿长边拉伸

```geo
// anisotropic_mesh.geo - 各向异性网格
SetFactory("OpenCASCADE");

// 创建10:1的细长矩形
Rectangle(1) = {0, 0, 0, 10, 1};

// 设置各向异性选项
Mesh.CharacteristicLengthExtendFromBoundary = 1;
Mesh.CharacteristicLengthMin = 0.1;
Mesh.CharacteristicLengthMax = 1.0;

// 使用BAMG算法支持各向异性
Mesh.Algorithm = 7;  // BAMG

Mesh 2;

// 观察单元形状是否沿长边拉伸
// 使用 Tools -> Statistics 查看网格质量分布
```

---

## 今日检查点

- [ ] 理解RefineMesh和RefineByField的区别
- [ ] 理解各向异性网格在CFD边界层中的应用
- [ ] 能根据物理问题选择合适的网格类型

---

## 核心概念

### 网格细化方法

#### RefineMesh - 整体细化

```geo
RefineMesh;  // 所有单元细化一次
```

每个三角形被分成4个更小的三角形。

#### RefineByField - 基于场的细化

```geo
RefineByField[field_id];
```

仅在场值较小的区域进行细化。

### 各向异性网格

**定义**：单元在不同方向有不同的尺寸比例

**应用场景**：
- CFD边界层：壁面法向需要细网格，切向可以粗网格
- 细长结构：沿长度方向可以使用较大尺寸
- 梯度变化大的方向：需要更细的网格

**控制方法**：

```geo
// 使用BAMG算法（支持各向异性）
Mesh.Algorithm = 7;

// 或使用MeshAdapt
Mesh.Algorithm = 1;

// 各向异性网格的质量参数
Mesh.AnisoMax = 10;  // 最大各向异性比
```

### 网格算法选择

| 算法 | 编号 | 特点 |
|------|------|------|
| MeshAdapt | 1 | 自适应，支持各向异性 |
| Automatic | 2 | 自动选择 |
| Delaunay | 5 | 经典Delaunay，稳定 |
| Frontal-Delaunay | 6 | 适合复杂几何 |
| BAMG | 7 | 各向异性，高质量 |
| Frontal-Quad | 8 | 四边形网格 |
| Packing | 9 | 并行四边形 |

### 网格质量度量

```geo
// 优化网格
Mesh.Optimize = 1;           // 一般优化
Mesh.OptimizeNetgen = 1;     // Netgen优化（3D）
Mesh.OptimizeThreshold = 0.3; // 优化阈值

// 质量评估
// 在GUI中：Tools -> Statistics
// 显示：单元质量直方图、最差单元、平均质量
```

---

## 各向异性网格的物理意义

在CFD（计算流体动力学）中，边界层的特点：
1. 法向速度梯度大 → 需要细网格
2. 切向变化小 → 可以用粗网格

各向异性网格可以：
- 减少总单元数
- 保持关键区域的精度
- 提高计算效率

示意图：
```
边界层各向异性网格：
壁面 ═══════════════════════════
      ▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔ (细)
      ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀ (逐渐变粗)
      █████████████████████████
      █████████████████████████ (粗)
流场  █████████████████████████
```

---

## 产出

- 掌握自适应和各向异性网格生成

---

## 导航

- **上一天**：[Day 9 - 网格尺寸场系统](day-09.md)
- **下一天**：[Day 11 - 边界层与网格优化](day-11.md)
- **返回目录**：[学习计划总览](../study.md)
