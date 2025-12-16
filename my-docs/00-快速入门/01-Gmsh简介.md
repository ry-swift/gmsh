# Gmsh 简介

## 什么是 Gmsh？

Gmsh 是一款功能强大的开源三维有限元网格生成器，由比利时列日大学和洛林大学联合开发，广泛应用于科学研究和工程仿真领域。

### 核心特点

- **开源免费**: 基于GNU GPL许可证，可免费用于学术和商业项目
- **跨平台**: 支持 Windows、macOS、Linux
- **多语言API**: 提供 C++、C、Python、Julia、Fortran 接口
- **集成化**: 几何建模、网格生成、后处理一体化解决方案
- **高性能**: 支持并行网格生成，处理大规模模型

## 四大核心模块

Gmsh 围绕四个核心模块构建，覆盖从几何建模到结果分析的完整工作流：

```
┌─────────────────────────────────────────────────────────┐
│                        Gmsh                             │
├──────────────┬──────────────┬──────────────┬───────────┤
│   Geometry   │     Mesh     │    Solver    │   Post    │
│   几何模块   │   网格模块   │   求解模块   │ 后处理模块│
├──────────────┼──────────────┼──────────────┼───────────┤
│ • 点/线/面/体│ • 1D/2D/3D   │ • 线性系统   │ • 数据视图│
│ • CAD导入    │ • 结构化网格 │ • 有限元框架 │ • 等值线  │
│ • 布尔运算   │ • 非结构化   │ • PETSc集成  │ • 动画    │
│ • 参数化建模 │ • 高阶元素   │ • MUMPS集成  │ • 图像导出│
└──────────────┴──────────────┴──────────────┴───────────┘
```

### 1. 几何模块 (Geometry)

几何模块负责创建和管理CAD模型，提供两个几何内核：

| 内核 | 命名空间 | 特点 | 适用场景 |
|-----|---------|------|---------|
| **内置内核** | `gmsh.model.geo` | 轻量级、学习简单 | 简单几何、教学 |
| **OpenCASCADE** | `gmsh.model.occ` | 功能完整、支持布尔运算 | 复杂CAD、工程应用 |

**主要功能**：
- 基本实体创建（点、线、曲线、曲面、体积）
- 几何变换（平移、旋转、缩放、镜像）
- 布尔运算（并集、交集、差集、分片）
- CAD文件导入（STEP、IGES、BREP、STL）

### 2. 网格模块 (Mesh)

网格模块是Gmsh的核心，提供多种网格生成算法：

**支持的网格类型**：
- **1D网格**: 曲线离散化
- **2D网格**: 三角形、四边形、混合网格
- **3D网格**: 四面体、六面体、棱柱、金字塔

**网格算法**：
```
2D算法:
├── MeshAdapt (1)       - 自适应算法
├── Automatic (2)       - 自动选择（默认）
├── Delaunay (5)        - Delaunay三角化
├── Frontal-Delaunay (6)- 前沿Delaunay
└── BAMG (7)            - 各向异性网格

3D算法:
├── Delaunay (1)        - Delaunay四面体化（默认）
├── Frontal (4)         - 前沿算法
├── MMG3D (7)           - MMG3D算法
├── R-tree (9)          - R-tree优化
└── HXT (10)            - 高性能并行算法
```

### 3. 求解模块 (Solver)

求解模块提供有限元分析框架（注：大多数用户使用外部求解器）：

- 线性系统求解器接口
- PETSc、MUMPS、Eigen集成
- 弹性力学、热传导等物理模块

### 4. 后处理模块 (Post-processing)

后处理模块用于可视化和分析仿真结果：

- **PView**: 数据视图管理
- **标量/矢量/张量场**可视化
- **等值线/等值面**提取
- **动画**制作和**图像导出**
- **插件系统**扩展功能

## 应用场景

### 计算流体力学 (CFD)
- 外部空气动力学（汽车、飞机）
- 内部流动（管道、阀门）
- 边界层网格生成

### 结构力学 / 有限元分析
- 结构应力分析
- 模态分析
- 多物理场耦合

### 电磁仿真
- 天线设计
- 波导结构
- 电机电磁场分析

### 半导体与微纳器件
- 器件结构网格
- 周期性结构
- 多材料域

### 其他领域
- 地球物理（地形网格）
- 生物医学（器官建模）
- 3D打印（STL处理）

## 使用方式

Gmsh 提供多种使用方式，满足不同用户需求：

### 1. 图形用户界面 (GUI)

启动Gmsh应用程序，通过菜单和工具栏进行交互操作。

```bash
# 命令行启动GUI
gmsh

# 打开文件并显示GUI
gmsh model.geo
```

### 2. 命令行批处理

无需GUI，直接在命令行执行网格生成。

```bash
# 生成2D网格
gmsh model.geo -2

# 生成3D网格并保存
gmsh model.geo -3 -o output.msh

# 设置网格尺寸
gmsh model.geo -3 -clscale 0.5
```

### 3. GEO脚本语言

使用Gmsh专用脚本语言编写 `.geo` 文件。

```geo
// 示例：创建矩形并生成网格
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
```

### 4. 编程API（推荐）

通过Python、C++等语言调用Gmsh API，实现自动化和参数化。

```python
import gmsh

gmsh.initialize()
gmsh.model.add("example")

# 创建几何
lc = 0.1
gmsh.model.geo.addPoint(0, 0, 0, lc, 1)
gmsh.model.geo.addPoint(1, 0, 0, lc, 2)
gmsh.model.geo.addPoint(1, 1, 0, lc, 3)
gmsh.model.geo.addPoint(0, 1, 0, lc, 4)

gmsh.model.geo.addLine(1, 2, 1)
gmsh.model.geo.addLine(2, 3, 2)
gmsh.model.geo.addLine(3, 4, 3)
gmsh.model.geo.addLine(4, 1, 4)

gmsh.model.geo.addCurveLoop([1, 2, 3, 4], 1)
gmsh.model.geo.addPlaneSurface([1], 1)

gmsh.model.geo.synchronize()
gmsh.model.mesh.generate(2)
gmsh.write("example.msh")
gmsh.finalize()
```

## 文件格式支持

### 输入格式
| 格式 | 扩展名 | 描述 |
|-----|-------|------|
| GEO | .geo | Gmsh脚本文件 |
| STEP | .step, .stp | ISO标准CAD格式 |
| IGES | .iges, .igs | 通用CAD交换格式 |
| BREP | .brep | OpenCASCADE原生格式 |
| STL | .stl | 三角面片格式 |

### 输出格式
| 格式 | 扩展名 | 描述 |
|-----|-------|------|
| MSH | .msh | Gmsh原生网格格式 |
| VTK | .vtk | ParaView兼容格式 |
| UNV | .unv | IDEAS通用格式 |
| CGNS | .cgns | CFD通用格式 |
| MED | .med | Salome/Code_Aster格式 |

## 版本信息

- **当前版本**: 4.14.0
- **API版本**: 4.14.0
- **开发者**: Christophe Geuzaine, Jean-François Remacle 等
- **官方网站**: https://gmsh.info/

## 下一步

现在您已经了解了Gmsh的基本概念，接下来请阅读：
- [02-安装与配置](./02-安装与配置.md) - 在您的系统上安装Gmsh
