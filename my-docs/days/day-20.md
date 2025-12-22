# Day 20：面实体

**所属周次**：第3周 - 通用工具模块与核心数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

深入理解2维实体

---

## 学习文件

- `src/geo/GFace.h`（约500行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读GFace.h前半部分 |
| 09:45-10:30 | 45min | 理解边界边列表和方向 |
| 10:30-11:15 | 45min | 理解参数空间(u,v)概念 |
| 11:15-12:00 | 45min | 理解法向量计算 |
| 14:00-14:45 | 45min | 理解edgeLoops边界环 |
| 14:45-15:30 | 45min | 理解边界层数据 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 阅读 GFace.h

**文件路径**：`src/geo/GFace.h`

GFace表示2维几何实体（面/曲面）。

```cpp
class GFace : public GEntity {
protected:
  // 边界边列表
  std::vector<GEdge*> l_edges;
  // 边界边的方向（1=正向，-1=反向）
  std::vector<int> l_dirs;

  // 边界环（一个面可能有多个边界环，如带孔的面）
  std::vector<GEdgeLoop> edgeLoops;

  // 相邻的体
  GRegion *r1, *r2;

  // 嵌入的实体
  std::vector<GVertex*> embedded_vertices;
  std::vector<GEdge*> embedded_edges;

public:
  // 维度固定为2
  virtual int dim() const { return 2; }

  // 参数范围
  virtual Range<double> parBounds(int i) const = 0;

  // 参数到点的映射：(u,v) → (x,y,z)
  virtual GPoint point(double u, double v) const = 0;

  // 法向量
  virtual SVector3 normal(const SPoint2 &param) const;

  // 一阶偏导数
  virtual Pair<SVector3, SVector3> firstDer(const SPoint2 &param) const = 0;

  // 曲率
  virtual double curvatureMax(const SPoint2 &param) const;

  // 边界边
  std::vector<GEdge*> &edges() { return l_edges; }
  std::vector<int> &edgeDirs() { return l_dirs; }
};
```

### 参数空间概念

```
物理空间 (x, y, z)          参数空间 (u, v)
      ╭─────╮                  ┌─────┐
     ╱       ╲                 │     │
    │         │      ←→        │     │
     ╲       ╱                 │     │
      ╰─────╯                  └─────┘
   曲面在3D中的形状         矩形参数域

映射关系：S(u,v) = (x(u,v), y(u,v), z(u,v))
```

---

## 下午任务（2小时）

### 边界环（Edge Loops）

一个面可以有多个边界环：
- 外边界环（必须有）
- 内边界环（孔）

```cpp
// 带孔的正方形面
//
//    ┌─────────────┐  外边界（逆时针）
//    │             │
//    │   ┌─────┐   │  内边界/孔（顺时针）
//    │   │     │   │
//    │   └─────┘   │
//    │             │
//    └─────────────┘

class GEdgeLoop {
  std::vector<GEdge*> edges;
  std::vector<int> dirs;  // 方向
};

// edgeLoops[0] = 外边界
// edgeLoops[1] = 第一个孔
// edgeLoops[2] = 第二个孔
// ...
```

### 边界层数据

```cpp
// 用于CFD边界层网格
struct BoundaryLayerColumns {
  // 边界层列数据
  std::vector<MVertex*> columns;
  // ...
};

class GFace {
  // 边界层相关
  BoundaryLayerColumns *_bl_columns;
  bool addBoundaryLayerColumn(MVertex *v, ...);
};
```

---

## 练习作业

### 1. 【基础】理解边界方向

分析边界边的方向含义：

```bash
# 查找边方向的使用
grep -rn "l_dirs" src/geo/GFace.cpp | head -10
```

思考：
- 为什么需要记录边的方向？
- 正向和反向有什么区别？

### 2. 【进阶】理解法向量计算

分析法向量的计算方法：

```bash
# 查找法向量计算
grep -A 20 "SVector3.*normal" src/geo/GFace.cpp
```

法向量的数学定义：
```
∂S/∂u = (∂x/∂u, ∂y/∂u, ∂z/∂u)
∂S/∂v = (∂x/∂v, ∂y/∂v, ∂z/∂v)

法向量 N = (∂S/∂u) × (∂S/∂v)
单位法向量 n = N / |N|
```

### 3. 【挑战】追踪面的网格生成

找到面的网格生成入口：

```bash
# 查找面网格生成
grep -rn "meshGFace" src/mesh/
```

理解面网格生成的基本流程：
1. 获取参数范围
2. 在参数空间生成网格
3. 映射回物理空间

---

## 今日检查点

- [ ] 理解GFace的边界边列表和方向
- [ ] 理解参数空间(u,v)的概念
- [ ] 理解法向量的计算方法
- [ ] 理解边界环在带孔面中的作用

---

## 核心概念

### 面（GFace）

**数据**：
- 边界边列表 l_edges
- 边界边方向 l_dirs
- 边界环 edgeLoops
- 相邻体 r1, r2
- 嵌入实体（点和边）

**关键方法**：

| 方法 | 说明 |
|------|------|
| `point(u,v)` | 参数(u,v)对应的空间点 |
| `normal(param)` | 参数点处的法向量 |
| `firstDer(param)` | 一阶偏导数（切向量） |
| `curvatureMax(param)` | 最大曲率 |
| `parBounds(i)` | 第i个参数的范围 |

### 参数曲面的几何属性

```
S(u,v) = (x(u,v), y(u,v), z(u,v))

一阶偏导数（切向量）：
∂S/∂u = Su
∂S/∂v = Sv

法向量：
N = Su × Sv

第一基本形式（度量）：
E = Su · Su
F = Su · Sv
G = Sv · Sv

第二基本形式（曲率）：
L = Suu · n
M = Suv · n
N = Svv · n

主曲率：
κ1, κ2 = 解方程 (EG-F²)κ² - (EN-2FM+GL)κ + (LN-M²) = 0

高斯曲率：K = κ1 * κ2 = (LN-M²)/(EG-F²)
平均曲率：H = (κ1+κ2)/2 = (EN-2FM+GL)/(2(EG-F²))
```

### 边界方向的重要性

```
外边界：逆时针（从法向量方向看）
    ────→
   ↗      ↘
  │    n    │
   ↖      ↙
    ←────

内边界（孔）：顺时针
    ←────
   ↙      ↖
  │        │
   ↘      ↗
    ────→

这保证了：
- 面的"内部"始终在边界的左侧
- 便于后续的网格生成和积分
```

---

## 继承关系

### GFace的派生类

```
GFace
├── gmshFace         # 内置几何面
│   ├── gmshPlane    # 平面
│   ├── gmshSurface  # 参数曲面
│   └── ...
├── OCCFace          # OpenCASCADE面
└── discreteFace     # 离散面（从网格创建）
```

---

## 源码导航

### 关键文件

```
src/geo/
├── GFace.h/cpp          # 面基类
├── gmshFace.h/cpp       # 内置面
├── OCCFace.h/cpp        # OCC面
├── discreteFace.h/cpp   # 离散面
└── GEdgeLoop.h          # 边界环

src/mesh/
├── meshGFace.h/cpp      # 面网格生成
└── meshGFaceBDS.cpp     # BDS面网格
```

### 搜索命令

```bash
# 查看面的参数化
grep -n "point.*double u.*double v" src/geo/GFace.cpp

# 查看法向量计算
grep -n "normal" src/geo/GFace.cpp

# 查看边界环
grep -n "edgeLoop" src/geo/GFace.h
```

---

## 产出

- 掌握面实体设计
- 理解参数化曲面的数学基础

---

## 导航

- **上一天**：[Day 19 - 顶点与边实体](day-19.md)
- **下一天**：[Day 21 - 第三周复习](day-21.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
