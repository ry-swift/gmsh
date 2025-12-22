# Day 19：顶点与边实体

**所属周次**：第3周 - 通用工具模块与核心数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解0维和1维实体

---

## 学习文件

- `src/geo/GVertex.h`（约300行）
- `src/geo/GEdge.h`（约400行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读GVertex.h |
| 09:45-10:30 | 45min | 理解顶点的关联边列表 |
| 10:30-11:15 | 45min | 理解坐标获取和网格尺寸 |
| 11:15-12:00 | 45min | 阅读GEdge.h前半部分 |
| 14:00-14:45 | 45min | 理解边的起点和终点 |
| 14:45-15:30 | 45min | 理解参数化方程和弧长 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 阅读 GVertex.h

**文件路径**：`src/geo/GVertex.h`

GVertex表示0维几何实体（点/顶点）。

```cpp
class GVertex : public GEntity {
protected:
  // 关联的边列表（以此顶点为端点的边）
  std::vector<GEdge*> l_edges;

  // 预设的网格尺寸
  double meshSize;

public:
  // 维度固定为0
  virtual int dim() const { return 0; }

  // 获取坐标
  virtual double x() const = 0;
  virtual double y() const = 0;
  virtual double z() const = 0;

  // 返回坐标点
  virtual GPoint point() const { return GPoint(x(), y(), z()); }

  // 关联边操作
  void addEdge(GEdge *e);
  void delEdge(GEdge *e);
  std::vector<GEdge*> &edges() { return l_edges; }

  // 网格尺寸
  void setMeshSize(double s) { meshSize = s; }
  double getMeshSize() const { return meshSize; }

  // 边界层
  bool onBoundaryLayer() const;
};
```

### 顶点与边的关联

```
         e1
    v1 ←──── v2
    │↘      ↗│
  e4│  e5  e6│
    │↙      ↘│
    v4 ────→ v3
         e3

顶点v1的关联边：l_edges = {e1, e4, e5}
顶点v2的关联边：l_edges = {e1, e2, e6}
```

---

## 下午任务（2小时）

### 阅读 GEdge.h

**文件路径**：`src/geo/GEdge.h`

GEdge表示1维几何实体（边/曲线）。

```cpp
class GEdge : public GEntity {
protected:
  // 起点和终点
  GVertex *v0, *v1;

  // 关联的面列表（以此边为边界的面）
  std::vector<GFace*> l_faces;

  // 边界层数据
  BoundaryLayerData *bl_data;

public:
  // 维度固定为1
  virtual int dim() const { return 1; }

  // 获取端点
  GVertex *getBeginVertex() const { return v0; }
  GVertex *getEndVertex() const { return v1; }

  // 参数化：t ∈ [parBounds(0).low(), parBounds(0).high()]
  virtual Range<double> parBounds(int i) const = 0;

  // 参数到点的映射
  virtual GPoint point(double t) const = 0;

  // 切向量
  virtual SVector3 firstDer(double t) const = 0;

  // 曲率
  virtual double curvature(double t) const;

  // 曲线长度
  virtual double length() const;

  // 弧长参数化：s ∈ [0, length()]
  virtual double parFromPoint(const SPoint3 &pt) const;

  // 关联面
  std::vector<GFace*> &faces() { return l_faces; }
  void addFace(GFace *f);
};
```

### 参数化的数学意义

```
曲线参数化：C(t) = (x(t), y(t), z(t))

例如：直线段从P0到P1
C(t) = P0 + t * (P1 - P0), t ∈ [0, 1]

圆弧：
C(t) = center + r * (cos(t), sin(t), 0), t ∈ [θ0, θ1]
```

---

## 练习作业

### 1. 【基础】统计顶点关联边

分析顶点如何维护关联边列表：

```bash
# 查找addEdge的调用位置
grep -rn "addEdge" src/geo/ | head -20

# 查找delEdge的调用位置
grep -rn "delEdge" src/geo/ | head -10
```

思考：什么时候需要更新关联边列表？

### 2. 【进阶】理解参数化范围

找到不同边类型的参数范围实现：

```bash
# 直线的参数范围
grep -A 5 "parBounds" src/geo/gmshEdge.cpp

# 圆弧的参数范围
grep -A 5 "parBounds" src/geo/OCCEdge.cpp
```

对比不同边类型的参数范围设计。

### 3. 【挑战】追踪point()函数

分析从参数到点的映射过程：

```bash
# 查找point函数实现
grep -n "GPoint.*point.*double" src/geo/GEdge.cpp
grep -n "GPoint.*point.*double" src/geo/OCCEdge.cpp
```

思考：
- 直线的point(t)如何实现？
- 圆弧的point(t)如何实现？
- OCC边的point(t)如何调用底层库？

---

## 今日检查点

- [ ] 理解GVertex的l_edges关联边列表
- [ ] 理解GEdge的v0, v1端点概念
- [ ] 理解参数化曲线的数学表示
- [ ] 能解释point(t)、firstDer(t)、curvature(t)的含义

---

## 核心概念

### 顶点（GVertex）

**数据**：
- 坐标 (x, y, z)
- 关联边列表 l_edges
- 网格尺寸 meshSize

**职责**：
- 作为边的端点
- 定义网格节点位置
- 控制局部网格密度

### 边（GEdge）

**数据**：
- 端点 v0, v1
- 关联面列表 l_faces
- 参数范围 [t_min, t_max]

**关键方法**：

| 方法 | 说明 |
|------|------|
| `point(t)` | 参数t对应的空间点 |
| `firstDer(t)` | t处的切向量（一阶导数） |
| `curvature(t)` | t处的曲率 |
| `length()` | 曲线总长度 |
| `parFromPoint(pt)` | 点到参数的逆映射 |

### 曲线的几何属性

```
C(t) = (x(t), y(t), z(t))

切向量（一阶导数）：
C'(t) = (x'(t), y'(t), z'(t))

单位切向量：
T(t) = C'(t) / |C'(t)|

曲率：
κ(t) = |C'(t) × C''(t)| / |C'(t)|³

弧长：
s = ∫|C'(t)| dt
```

---

## 继承关系

### GVertex的派生类

```
GVertex
├── gmshVertex      # 内置几何顶点
├── OCCVertex       # OpenCASCADE顶点
└── discreteVertex  # 离散顶点（从网格创建）
```

### GEdge的派生类

```
GEdge
├── gmshEdge        # 内置几何边
│   ├── gmshLine    # 直线
│   ├── gmshCircle  # 圆弧
│   ├── gmshSpline  # 样条
│   └── ...
├── OCCEdge         # OpenCASCADE边
└── discreteEdge    # 离散边（从网格创建）
```

---

## 源码导航

### 关键文件

```
src/geo/
├── GVertex.h/cpp        # 顶点基类
├── GEdge.h/cpp          # 边基类
├── gmshVertex.h/cpp     # 内置顶点
├── gmshEdge.h/cpp       # 内置边
├── OCCVertex.h/cpp      # OCC顶点
├── OCCEdge.h/cpp        # OCC边
├── discreteVertex.h/cpp # 离散顶点
└── discreteEdge.h/cpp   # 离散边
```

### 搜索命令

```bash
# 查看边的参数化实现
grep -n "point.*double t" src/geo/gmshEdge.cpp

# 查看边长计算
grep -n "length()" src/geo/GEdge.cpp

# 查看曲率计算
grep -n "curvature" src/geo/GEdge.cpp
```

---

## 产出

- 理解低维实体设计
- 记录顶点和边的关键属性

---

## 导航

- **上一天**：[Day 18 - 几何实体基类](day-18.md)
- **下一天**：[Day 20 - 面实体](day-20.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
