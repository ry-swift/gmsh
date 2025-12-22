# Day 24：MVertex网格顶点

**所属周次**：第4周 - 网格数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解网格顶点的表示和分类

---

## 学习文件

- `src/geo/MVertex.h`（约300行）
- `src/geo/MVertex.cpp`（约400行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读MVertex基类定义 |
| 09:45-10:30 | 45min | 理解坐标存储和访问 |
| 10:30-11:15 | 45min | 理解顶点分类（内部/边界） |
| 11:15-12:00 | 45min | 阅读各种派生类 |
| 14:00-14:45 | 45min | 理解顶点编号系统 |
| 14:45-15:30 | 45min | 理解参数坐标存储 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 阅读 MVertex.h

**文件路径**：`src/geo/MVertex.h`

MVertex表示网格中的顶点（节点）。

```cpp
class MVertex {
protected:
  // 顶点编号
  std::size_t _num;

  // 顶点坐标
  double _x, _y, _z;

  // 所属几何实体
  GEntity *_ge;

  // 可见性标志
  char _visible;

public:
  MVertex(double x, double y, double z = 0.0, GEntity *ge = nullptr,
          std::size_t num = 0);

  virtual ~MVertex() {}

  // 获取/设置编号
  std::size_t getNum() const { return _num; }
  void setNum(std::size_t num) { _num = num; }

  // 获取坐标
  double x() const { return _x; }
  double y() const { return _y; }
  double z() const { return _z; }

  // 设置坐标
  void setXYZ(double x, double y, double z) {
    _x = x; _y = y; _z = z;
  }

  // 获取坐标点
  SPoint3 point() const { return SPoint3(_x, _y, _z); }

  // 获取所属几何实体
  GEntity *onWhat() const { return _ge; }
  void setEntity(GEntity *ge) { _ge = ge; }
};
```

### 顶点分类

根据所属几何实体的维度，顶点可以分为：

```
顶点分类（按几何位置）：
┌──────────────────────────────────────────────┐
│  GVertex (0D)  - 顶点在几何点上              │
│  GEdge (1D)    - 顶点在几何边上              │
│  GFace (2D)    - 顶点在几何面的内部          │
│  GRegion (3D)  - 顶点在几何体的内部          │
└──────────────────────────────────────────────┘

代码判断：
int dim = vertex->onWhat()->dim();
switch(dim) {
  case 0: // 在几何顶点上
  case 1: // 在边上
  case 2: // 在面内部
  case 3: // 在体内部
}
```

### MVertex派生类

```cpp
// 在几何边上的顶点（带参数坐标）
class MEdgeVertex : public MVertex {
protected:
  double _u;  // 边上的参数坐标 t

public:
  MEdgeVertex(double x, double y, double z, GEntity *ge,
              double u, std::size_t num = 0);

  double getParameter(int i) const { return _u; }
  void setParameter(int i, double u) { _u = u; }
};

// 在几何面上的顶点（带参数坐标）
class MFaceVertex : public MVertex {
protected:
  double _u, _v;  // 面上的参数坐标 (u, v)

public:
  MFaceVertex(double x, double y, double z, GEntity *ge,
              double u, double v, std::size_t num = 0);

  double getParameter(int i) const {
    return (i == 0) ? _u : _v;
  }
  void setParameter(int i, double p) {
    if(i == 0) _u = p; else _v = p;
  }
};
```

---

## 下午任务（2小时）

### 顶点编号系统

```cpp
// 顶点编号的管理
class MVertexRTree {
  // 空间索引，用于快速查找顶点
};

// 全局顶点编号
// 编号从1开始（MSH文件格式要求）
// 0表示未分配编号

// 编号重新分配
void GModel::renumberMeshVertices() {
  std::size_t num = 0;
  // 先编号几何顶点上的网格顶点
  for(auto v : vertices) {
    for(auto mv : v->mesh_vertices)
      mv->setNum(++num);
  }
  // 再编号边上的
  for(auto e : edges) {
    for(auto mv : e->mesh_vertices)
      mv->setNum(++num);
  }
  // 再编号面上的
  // ...
}
```

### 参数坐标的意义

```
物理坐标 (x, y, z)           参数坐标
─────────────────────────────────────────
边上顶点:                     t ∈ [t0, t1]
  (x, y, z) = C(t)

面上顶点:                     (u, v) ∈ D
  (x, y, z) = S(u, v)

体内顶点:                     无参数坐标
  直接存储 (x, y, z)

为什么需要参数坐标？
1. 几何操作：移动顶点时保持在曲面上
2. 网格优化：在参数空间进行平滑
3. 高阶元素：插值计算需要参数坐标
```

### 顶点-元素关联

```cpp
// 顶点不直接存储关联元素
// 需要时动态构建

// 构建顶点到元素的映射
std::map<MVertex*, std::vector<MElement*>> vertex2elements;

for(auto e : elements) {
  for(int i = 0; i < e->getNumVertices(); i++) {
    MVertex *v = e->getVertex(i);
    vertex2elements[v].push_back(e);
  }
}

// 使用场景：
// - 顶点平滑（需要知道周围元素）
// - 拓扑查询（找相邻元素）
// - 质量检查（局部区域分析）
```

---

## 练习作业

### 1. 【基础】理解顶点存储

查找顶点在GEntity中的存储：

```bash
# 查找mesh_vertices成员
grep -rn "mesh_vertices" src/geo/GEntity.h

# 查找顶点添加函数
grep -rn "addMeshVertex" src/geo/GEntity.cpp
```

理解顶点存储结构：
```cpp
// GEntity中的顶点存储
std::vector<MVertex*> mesh_vertices;

// 顶点属于哪个几何实体由onWhat()决定
// mesh_vertices只存储"属于"该实体的顶点
// 而不是"位于"该实体上的所有顶点
```

### 2. 【进阶】分析参数坐标使用

查找参数坐标的使用场景：

```bash
# 查找getParameter调用
grep -rn "getParameter" src/mesh/

# 查找setParameter调用
grep -rn "setParameter" src/mesh/
```

思考：
- 在哪些网格操作中需要用到参数坐标？
- 参数坐标如何帮助保持网格在曲面上？

### 3. 【挑战】实现顶点邻接查询

写出查找顶点相邻顶点的算法：

```cpp
// 伪代码
std::set<MVertex*> getAdjacentVertices(MVertex *v,
                                        const std::vector<MElement*> &elements) {
  std::set<MVertex*> adjacent;

  // 1. 找到包含v的所有元素
  for(auto e : elements) {
    bool containsV = false;
    for(int i = 0; i < e->getNumVertices(); i++) {
      if(e->getVertex(i) == v) {
        containsV = true;
        break;
      }
    }

    // 2. 收集这些元素的其他顶点
    if(containsV) {
      for(int i = 0; i < e->getNumVertices(); i++) {
        MVertex *vv = e->getVertex(i);
        if(vv != v) adjacent.insert(vv);
      }
    }
  }

  return adjacent;
}
```

---

## 今日检查点

- [ ] 理解MVertex的基本结构（坐标、编号、所属实体）
- [ ] 理解MEdgeVertex和MFaceVertex的参数坐标
- [ ] 理解顶点在GEntity中的存储方式
- [ ] 能解释参数坐标的作用

---

## 核心概念

### MVertex继承体系

```
MVertex（基类）
├── MEdgeVertex      # 边上顶点（带参数t）
├── MFaceVertex      # 面上顶点（带参数u,v）
└── (基类直接使用)   # 几何顶点或体内顶点
```

### 顶点的关键属性

| 属性 | 方法 | 说明 |
|------|------|------|
| 编号 | getNum()/setNum() | 全局唯一标识 |
| 坐标 | x()/y()/z() | 物理空间位置 |
| 几何实体 | onWhat() | 所属的几何实体 |
| 参数坐标 | getParameter() | 在曲线/曲面上的参数 |

### 顶点与几何实体的关系

```
GModel
├── GVertex[1] ──→ mesh_vertices: [MVertex_a]
├── GVertex[2] ──→ mesh_vertices: [MVertex_b]
├── GEdge[1]   ──→ mesh_vertices: [MVertex_c, MVertex_d, ...]
├── GEdge[2]   ──→ mesh_vertices: [...]
├── GFace[1]   ──→ mesh_vertices: [MVertex_e, MVertex_f, ...]
└── GRegion[1] ──→ mesh_vertices: [MVertex_g, ...]

每个MVertex的onWhat()指向其所属的GEntity
顶点按所属实体分组存储，便于管理和遍历
```

### 顶点遍历方式

```cpp
// 方式1：遍历所有实体的顶点
for(auto gv : model->getVertices()) {
  for(auto mv : gv->mesh_vertices) { /* 几何顶点上的网格顶点 */ }
}
for(auto ge : model->getEdges()) {
  for(auto mv : ge->mesh_vertices) { /* 边上的网格顶点 */ }
}
for(auto gf : model->getFaces()) {
  for(auto mv : gf->mesh_vertices) { /* 面内的网格顶点 */ }
}
for(auto gr : model->getRegions()) {
  for(auto mv : gr->mesh_vertices) { /* 体内的网格顶点 */ }
}

// 方式2：从元素获取顶点
for(auto elem : elements) {
  for(int i = 0; i < elem->getNumVertices(); i++) {
    MVertex *v = elem->getVertex(i);
  }
}
```

---

## 源码导航

### 关键文件

```
src/geo/
├── MVertex.h/cpp        # 顶点基类和派生类
├── MVertexRTree.h/cpp   # 顶点空间索引
└── MVertexBoundaryLayerData.h # 边界层顶点数据

src/mesh/
├── meshGFace.cpp        # 面网格（顶点创建）
├── meshGRegion.cpp      # 体网格（顶点创建）
└── meshGFaceOptimize.cpp # 顶点平滑
```

### 搜索命令

```bash
# 查找MVertex的创建
grep -rn "new MVertex\|new MEdgeVertex\|new MFaceVertex" src/mesh/

# 查找顶点平滑
grep -rn "smooth.*Vertex\|vertex.*smooth" src/mesh/

# 查找参数坐标更新
grep -rn "setParameter\|reparametrize" src/mesh/
```

---

## 产出

- 理解MVertex数据结构
- 理解顶点分类和参数坐标

---

## 导航

- **上一天**：[Day 23 - 各类型网格元素](day-23.md)
- **下一天**：[Day 25 - 网格拓扑关系](day-25.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
