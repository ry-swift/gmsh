# Gmsh完整学习计划：从零到精通源码

## 计划概述

| 项目 | 说明 |
|------|------|
| **目标** | 从零基础到完全掌握Gmsh使用和源码理解 |
| **总周期** | 16周（约4个月） |
| **每日学习** | 4小时 |
| **主要语言** | C++（与源码一致，便于后期开发） |
| **难度设置** | 渐进式（每天2-3个练习，从基础到进阶） |

---

## 学习进度导航

### 第一阶段：基础入门（第1-2周）

#### 第1周：环境搭建与初识Gmsh

| Day | 主题 | 学习文件 | 状态 |
|-----|------|----------|------|
| [Day 1](days/day-01.md) | 环境准备与第一次运行 | README, CLAUDE.md | ☐ |
| [Day 2](days/day-02.md) | GEO脚本语言基础（一） | t1.geo | ☐ |
| [Day 3](days/day-03.md) | GEO脚本语言基础（二） | t2.geo, t3.geo | ☐ |
| [Day 4](days/day-04.md) | 循环与控制结构 | t4.geo, t5.geo | ☐ |
| [Day 5](days/day-05.md) | 结构化网格 | t6.geo | ☐ |
| [Day 6](days/day-06.md) | 曲面与高阶几何 | t7.geo, t8.geo | ☐ |
| [Day 7](days/day-07.md) | 第一周总结与综合项目 | 综合练习 | ☐ |

**里程碑**：能独立创建参数化2D几何并生成网格

---

#### 第2周：GEO脚本进阶与API入门

| Day | 主题 | 学习文件 | 状态 |
|-----|------|----------|------|
| [Day 8](days/day-08.md) | 缝合、分割与复合操作 | t9.geo | ☐ |
| [Day 9](days/day-09.md) | 网格尺寸场系统 | t10.geo | ☐ |
| [Day 10](days/day-10.md) | 网格细化与各向异性网格 | t11.geo, t12.geo | ☐ |
| [Day 11](days/day-11.md) | 边界层与网格优化 | t13.geo | ☐ |
| [Day 12](days/day-12.md) | OpenCASCADE几何内核 | t16.geo, t18.geo | ☐ |
| [Day 13](days/day-13.md) | C++ API入门 | c++/t1.cpp | ☐ |
| [Day 14](days/day-14.md) | 第二周复习与API练习 | c++/t2.cpp, t3.cpp | ☐ |

**里程碑**：能用C++ API完成完整网格生成流程

---

### 第二阶段：源码阅读基础（第3-6周）

#### 第3周：通用工具模块与核心数据结构

| Day | 主题 | 学习文件 | 状态 |
|-----|------|----------|------|
| [Day 15](days/day-15.md) | 项目架构全览 | src/目录结构 | ☐ |
| [Day 16](days/day-16.md) | 选项系统与上下文 | Options.h, Context.h | ☐ |
| [Day 17](days/day-17.md) | 空间索引结构 | Octree.h, Hash.h | ☐ |
| [Day 18](days/day-18.md) | 几何实体基类 | GEntity.h | ☐ |
| [Day 19](days/day-19.md) | 顶点与边实体 | GVertex.h, GEdge.h | ☐ |
| [Day 20](days/day-20.md) | 面实体 | GFace.h | ☐ |
| [Day 21](days/day-21.md) | 第三周复习 | 综合练习 | ☐ |

**里程碑**：理解GEntity继承体系和几何参数化概念

---

#### 第4周：网格数据结构

| Day | 主题 | 学习文件 | 状态 |
|-----|------|----------|------|
| [Day 22](days/day-22.md) | MElement网格元素基类 | MElement.h | ☐ |
| [Day 23](days/day-23.md) | 各类型网格元素 | MTriangle.h, MTetrahedron.h | ☐ |
| [Day 24](days/day-24.md) | MVertex网格顶点 | MVertex.h | ☐ |
| [Day 25](days/day-25.md) | 网格拓扑关系 | MEdge.h, MFace.h | ☐ |
| [Day 26](days/day-26.md) | 网格质量度量 | qualityMeasures.h | ☐ |
| [Day 27](days/day-27.md) | 网格数据IO | GModelIO_MSH.cpp | ☐ |
| [Day 28](days/day-28.md) | 第四周复习 | 综合练习 | ☐ |

**里程碑**：理解MElement继承体系和网格拓扑查询

---

#### 第5周：几何模块深入

| Day | 主题 | 学习文件 | 状态 |
|-----|------|----------|------|
| [Day 29](days/day-29.md) | GModel几何模型类 | GModel.h | ☐ |
| [Day 30](days/day-30.md) | 内置几何引擎 | gmshVertex.h, gmshEdge.h | ☐ |
| [Day 31](days/day-31.md) | OpenCASCADE集成 | OCC_Internals.h, OCCVertex.h | ☐ |
| [Day 32](days/day-32.md) | 几何操作 | 布尔运算、变换 | ☐ |
| [Day 33](days/day-33.md) | CAD文件导入流程 | GModelIO_OCC.cpp | ☐ |
| [Day 34](days/day-34.md) | 几何修复与简化 | healShapes | ☐ |
| [Day 35](days/day-35.md) | 第五周复习 | 综合练习 | ☐ |

**里程碑**：能追踪CAD导入的完整代码路径

---

#### 第6周：网格模块基础

| Day | 主题 | 学习文件 | 状态 |
|-----|------|----------|------|
| [Day 36](days/day-36.md) | 网格生成器入口 | Generator.h/cpp | ☐ |
| [Day 37](days/day-37.md) | 1D网格生成 | meshGEdge.cpp | ☐ |
| [Day 38](days/day-38.md) | 2D网格算法概述 | meshGFace.cpp | ☐ |
| [Day 39](days/day-39.md) | Delaunay三角化基础 | meshGFaceDelaunay.cpp | ☐ |
| [Day 40](days/day-40.md) | 前沿推进法基础 | meshGFaceFrontal.cpp | ☐ |
| [Day 41](days/day-41.md) | 网格优化基础 | meshOptimize.cpp | ☐ |
| [Day 42](days/day-42.md) | 第六周复习 | 综合练习 | ☐ |

**阶段里程碑**：理解网格生成的整体流程和核心算法

---

### 第三阶段：深入核心算法（第7-10周）

#### 第7周：网格算法深度剖析

| Day | 主题 | 学习文件 | 状态 |
|-----|------|----------|------|
| [Day 43](days/day-43.md) | Delaunay算法理论基础 | 空圆性质、Voronoi对偶 | ☐ |
| [Day 44](days/day-44.md) | 3D Delaunay实现深度分析 | delaunay3d.cpp | ☐ |
| [Day 45](days/day-45.md) | 几何谓词与数值稳定性 | robustPredicates.cpp | ☐ |
| [Day 46](days/day-46.md) | 网格质量度量 | qualityMeasures.h/cpp | ☐ |
| [Day 47](days/day-47.md) | 网格优化算法 | meshGFaceOptimize.cpp | ☐ |
| [Day 48](days/day-48.md) | Netgen集成分析 | meshGRegionNetgen.cpp | ☐ |
| [Day 49](days/day-49.md) | 第七周复习 | HXT、算法对比 | ☐ |

**里程碑**：能阅读并理解Delaunay三角化实现

---

#### 第8-10周预览

| 周次 | 主题 | Days | 状态 |
|------|------|------|------|
| 第8周 | 数值计算模块 | 50-56 | ☐ |
| 第9周 | 后处理与可视化 | 57-63 | ☐ |
| 第10周 | 求解器与数值计算 | 64-70 | ☐ |

---

### 第四阶段：实战与开发（第11-16周）

| 周次 | 主题 | Days | 状态 |
|------|------|------|------|
| 第11周 | API高级应用 | 71-77 | ☐ |
| 第12周 | 实战项目：参数化网格生成器 | 78-84 | ☐ |
| 第13周 | 源码级开发入门 | 85-91 | ☐ |
| 第14周 | 源码级开发实践 | 92-98 | ☐ |
| 第15周 | 性能优化与最佳实践 | 99-105 | ☐ |
| 第16周 | 综合项目与总结 | 106-112 | ☐ |

**里程碑**：能开发自定义Field或插件

---

## 快速链接

### 详细文档

- [完整学习计划（含附录）](study.md)

### 附录资源

| 附录 | 内容 |
|------|------|
| [附录A](study.md#附录acookbook学习指南) | Cookbook学习指南 |
| [附录B](study.md#附录b扩展教程x1-x7详解) | 扩展教程(x1-x7)详解 |
| [附录C](study.md#附录cc-api示例精选) | C++ API示例精选 |
| [附录D](study.md#附录d学习检查点) | 学习检查点 |
| [附录E](study.md#附录e常见问题解答) | 常见问题解答 |
| [附录F](study.md#附录f学习笔记模板) | 学习笔记模板 |
| [附录G](study.md#附录ggeo命令速查) | GEO命令速查 |

---

## 目录结构

```
my-docs/
├── STUDY-INDEX.md     # 本文件（学习计划索引）
├── study.md           # 详细学习计划（完整版，含附录）
├── README.md          # 另一个文档系统索引
└── days/              # 每日学习内容
    │
    │── 第1周：环境搭建与初识Gmsh
    ├── day-01.md      # Day 1: 环境准备
    ├── day-02.md      # Day 2: GEO基础(一)
    ├── day-03.md      # Day 3: GEO基础(二)
    ├── day-04.md      # Day 4: 循环与控制
    ├── day-05.md      # Day 5: 结构化网格
    ├── day-06.md      # Day 6: 曲面与高阶
    ├── day-07.md      # Day 7: 第一周总结
    │
    │── 第2周：GEO脚本进阶与API入门
    ├── day-08.md      # Day 8: 缝合与复合
    ├── day-09.md      # Day 9: 尺寸场
    ├── day-10.md      # Day 10: 网格细化
    ├── day-11.md      # Day 11: 边界层
    ├── day-12.md      # Day 12: OpenCASCADE
    ├── day-13.md      # Day 13: C++ API入门
    ├── day-14.md      # Day 14: 第二周总结
    │
    │── 第3周：通用工具模块与核心数据结构
    ├── day-15.md      # Day 15: 项目架构全览
    ├── day-16.md      # Day 16: 选项系统与上下文
    ├── day-17.md      # Day 17: 空间索引结构
    ├── day-18.md      # Day 18: 几何实体基类
    ├── day-19.md      # Day 19: 顶点与边实体
    ├── day-20.md      # Day 20: 面实体
    ├── day-21.md      # Day 21: 第三周复习
    │
    │── 第4周：网格数据结构
    ├── day-22.md      # Day 22: MElement基类
    ├── day-23.md      # Day 23: 各类型网格元素
    ├── day-24.md      # Day 24: MVertex网格顶点
    ├── day-25.md      # Day 25: 网格拓扑关系
    ├── day-26.md      # Day 26: 网格质量度量
    ├── day-27.md      # Day 27: 网格数据IO
    ├── day-28.md      # Day 28: 第四周复习
    │
    │── 第5周：几何模块深入
    ├── day-29.md      # Day 29: GModel几何模型类
    ├── day-30.md      # Day 30: 内置几何引擎
    ├── day-31.md      # Day 31: OpenCASCADE集成
    ├── day-32.md      # Day 32: 几何操作
    ├── day-33.md      # Day 33: CAD文件导入流程
    ├── day-34.md      # Day 34: 几何修复与简化
    ├── day-35.md      # Day 35: 第五周复习
    │
    │── 第6周：网格模块基础
    ├── day-36.md      # Day 36: 网格生成器入口
    ├── day-37.md      # Day 37: 1D网格生成
    ├── day-38.md      # Day 38: 2D网格算法概述
    ├── day-39.md      # Day 39: Delaunay三角化基础
    ├── day-40.md      # Day 40: 前沿推进法基础
    ├── day-41.md      # Day 41: 网格优化基础
    ├── day-42.md      # Day 42: 第六周复习
    │
    │── 第7周：网格算法深度剖析
    ├── day-43.md      # Day 43: Delaunay算法理论基础
    ├── day-44.md      # Day 44: 3D Delaunay实现
    ├── day-45.md      # Day 45: 几何谓词与数值稳定性
    ├── day-46.md      # Day 46: 网格质量度量
    ├── day-47.md      # Day 47: 网格优化算法
    ├── day-48.md      # Day 48: Netgen集成分析
    └── day-49.md      # Day 49: 第七周复习
```

---

## 使用指南

### 每日学习流程

1. 打开当天的 `days/day-XX.md` 文件
2. 按照时间分配表安排学习
3. 完成练习作业（基础→进阶→挑战）
4. 核对检查点确认掌握程度
5. 更新本文件的状态列（☐ → ☑）

### 进度标记

- `☐` 未开始
- `⬜` 进行中
- `☑` 已完成

---

## 外部资源

- [Gmsh官方文档](https://gmsh.info/doc/texinfo/gmsh.html)
- [Gmsh教程](https://gmsh.info/#Documentation)
- [问题报告](https://gitlab.onelab.info/gmsh/gmsh/issues)

---

*最后更新：2025-12-18*
