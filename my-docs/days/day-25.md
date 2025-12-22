# Day 25：网格拓扑关系

**所属周次**：第4周 - 网格数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解MEdge、MFace和网格拓扑查询

---

## 学习文件

- `src/geo/MEdge.h`（约150行）
- `src/geo/MFace.h`（约200行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读MEdge定义和实现 |
| 09:45-10:30 | 45min | 理解边的哈希和比较 |
| 10:30-11:15 | 45min | 阅读MFace定义和实现 |
| 11:15-12:00 | 45min | 理解面的哈希和比较 |
| 14:00-14:45 | 45min | 理解拓扑关系构建 |
| 14:45-15:30 | 45min | 分析相邻元素查询 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 阅读 MEdge.h

**文件路径**：`src/geo/MEdge.h`

MEdge表示网格边（由两个顶点定义）。

```cpp
class MEdge {
private:
  MVertex *_v[2];  // 两个端点（按地址排序存储）

public:
  MEdge() : _v{nullptr, nullptr} {}

  MEdge(MVertex *v0, MVertex *v1) {
    // 按顶点地址排序，确保 MEdge(a,b) == MEdge(b,a)
    if(v0 < v1) {
      _v[0] = v0;
      _v[1] = v1;
    } else {
      _v[0] = v1;
      _v[1] = v0;
    }
  }

  // 获取端点
  MVertex *getVertex(int i) const { return _v[i]; }

  // 获取最小/最大顶点（按地址）
  MVertex *getMinVertex() const { return _v[0]; }
  MVertex *getMaxVertex() const { return _v[1]; }

  // 计算边长
  double length() const;

  // 计算中点
  SPoint3 barycenter() const {
    return SPoint3(0.5 * (_v[0]->x() + _v[1]->x()),
                   0.5 * (_v[0]->y() + _v[1]->y()),
                   0.5 * (_v[0]->z() + _v[1]->z()));
  }

  // 比较运算符
  bool operator==(const MEdge &e) const {
    return _v[0] == e._v[0] && _v[1] == e._v[1];
  }

  bool operator<(const MEdge &e) const {
    if(_v[0] < e._v[0]) return true;
    if(_v[0] > e._v[0]) return false;
    return _v[1] < e._v[1];
  }
};

// 边的哈希函数
struct MEdgeHash {
  size_t operator()(const MEdge &e) const {
    // 使用两个顶点地址计算哈希值
    size_t h1 = std::hash<MVertex*>{}(e.getMinVertex());
    size_t h2 = std::hash<MVertex*>{}(e.getMaxVertex());
    return h1 ^ (h2 << 1);
  }
};

// 边的相等比较
struct MEdgeEqual {
  bool operator()(const MEdge &e1, const MEdge &e2) const {
    return e1 == e2;
  }
};
```

### 阅读 MFace.h

**文件路径**：`src/geo/MFace.h`

MFace表示网格面（由3或4个顶点定义）。

```cpp
class MFace {
private:
  std::vector<MVertex*> _v;  // 顶点列表（排序后存储）

public:
  MFace() {}

  // 三角形面
  MFace(MVertex *v0, MVertex *v1, MVertex *v2) {
    _v.resize(3);
    // 找到最小顶点，从它开始排列
    // 保证 MFace(a,b,c) == MFace(b,c,a) == MFace(c,a,b)
    sortVertices(v0, v1, v2);
  }

  // 四边形面
  MFace(MVertex *v0, MVertex *v1, MVertex *v2, MVertex *v3) {
    _v.resize(4);
    sortVertices(v0, v1, v2, v3);
  }

  // 获取顶点数量
  std::size_t getNumVertices() const { return _v.size(); }

  // 获取顶点
  MVertex *getVertex(int i) const { return _v[i]; }

  // 计算面积
  double approximateArea() const;

  // 计算法向量
  SVector3 normal() const;

  // 计算重心
  SPoint3 barycenter() const;

  // 比较运算符
  bool operator==(const MFace &f) const;
  bool operator<(const MFace &f) const;

private:
  void sortVertices(MVertex *v0, MVertex *v1, MVertex *v2);
  void sortVertices(MVertex *v0, MVertex *v1, MVertex *v2, MVertex *v3);
};

// 面的哈希函数
struct MFaceHash {
  size_t operator()(const MFace &f) const;
};
```

### 顶点排序规则

```
MEdge排序：
  按顶点地址排序，小的在前
  MEdge(A, B) 存储为 [min(A,B), max(A,B)]
  保证 MEdge(A,B) == MEdge(B,A)

MFace排序（三角形）：
  1. 找到地址最小的顶点
  2. 从该顶点开始，按原来的顺序存储

  例如: MFace(B, C, A) 其中 A < B < C
  存储为: [A, B, C]（保持逆时针顺序）

  MFace(A,B,C) == MFace(B,C,A) == MFace(C,A,B)
  但 MFace(A,B,C) != MFace(A,C,B)（顺序相反）
```

---

## 下午任务（2小时）

### 拓扑关系构建

```cpp
// 构建边到元素的映射
typedef std::unordered_map<MEdge, std::vector<MElement*>,
                           MEdgeHash, MEdgeEqual> Edge2Elements;

Edge2Elements buildEdge2Elements(const std::vector<MElement*> &elements) {
  Edge2Elements e2e;
  for(auto elem : elements) {
    for(int i = 0; i < elem->getNumEdges(); i++) {
      MEdge edge = elem->getEdge(i);
      e2e[edge].push_back(elem);
    }
  }
  return e2e;
}

// 构建面到元素的映射（用于3D网格）
typedef std::unordered_map<MFace, std::vector<MElement*>,
                           MFaceHash> Face2Elements;

Face2Elements buildFace2Elements(const std::vector<MElement*> &elements) {
  Face2Elements f2e;
  for(auto elem : elements) {
    for(int i = 0; i < elem->getNumFaces(); i++) {
      MFace face = elem->getFace(i);
      f2e[face].push_back(elem);
    }
  }
  return f2e;
}
```

### 相邻元素查询

```cpp
// 通过共享边查找相邻元素（2D网格）
std::vector<MElement*> getNeighborsByEdge(MElement *elem,
                                           const Edge2Elements &e2e) {
  std::vector<MElement*> neighbors;
  for(int i = 0; i < elem->getNumEdges(); i++) {
    MEdge edge = elem->getEdge(i);
    auto it = e2e.find(edge);
    if(it != e2e.end()) {
      for(auto e : it->second) {
        if(e != elem) {
          neighbors.push_back(e);
        }
      }
    }
  }
  return neighbors;
}

// 通过共享面查找相邻元素（3D网格）
std::vector<MElement*> getNeighborsByFace(MElement *elem,
                                           const Face2Elements &f2e) {
  std::vector<MElement*> neighbors;
  for(int i = 0; i < elem->getNumFaces(); i++) {
    MFace face = elem->getFace(i);
    auto it = f2e.find(face);
    if(it != f2e.end()) {
      for(auto e : it->second) {
        if(e != elem) {
          neighbors.push_back(e);
        }
      }
    }
  }
  return neighbors;
}
```

### 边界识别

```cpp
// 识别边界边（只被一个元素使用的边）
std::vector<MEdge> getBoundaryEdges(const Edge2Elements &e2e) {
  std::vector<MEdge> boundary;
  for(auto &pair : e2e) {
    if(pair.second.size() == 1) {
      boundary.push_back(pair.first);
    }
  }
  return boundary;
}

// 识别边界面（只被一个元素使用的面）
std::vector<MFace> getBoundaryFaces(const Face2Elements &f2e) {
  std::vector<MFace> boundary;
  for(auto &pair : f2e) {
    if(pair.second.size() == 1) {
      boundary.push_back(pair.first);
    }
  }
  return boundary;
}

// 边界特征：
// - 内部边/面：被2个元素共享
// - 边界边/面：只被1个元素使用
// - 非流形边/面：被3个或更多元素共享（通常是错误）
```

---

## 练习作业

### 1. 【基础】验证边的唯一性

测试MEdge的排序和比较：

```bash
# 查找MEdge的构造函数
grep -A 15 "MEdge::MEdge" src/geo/MEdge.cpp

# 查找MEdge的比较运算符
grep -A 10 "operator==" src/geo/MEdge.h
```

手动验证：
```cpp
MVertex *a = ..., *b = ...;  // 假设 a < b（地址比较）
MEdge e1(a, b);
MEdge e2(b, a);
// e1 == e2 应该为 true
```

### 2. 【进阶】分析面的排序

理解MFace的顶点排序逻辑：

```bash
# 查找sortVertices实现
grep -A 30 "sortVertices" src/geo/MFace.cpp
```

验证三角形面的等价性：
```cpp
MFace f1(a, b, c);  // 假设 a < b < c
MFace f2(b, c, a);
MFace f3(c, a, b);
MFace f4(a, c, b);  // 反向

// f1 == f2 == f3 应该为 true
// f1 != f4 应该为 true（方向相反）
```

### 3. 【挑战】实现网格连通性检查

编写检查网格是否连通的算法：

```cpp
// 伪代码
bool isMeshConnected(const std::vector<MElement*> &elements) {
  if(elements.empty()) return true;

  // 构建边到元素的映射
  Edge2Elements e2e = buildEdge2Elements(elements);

  // BFS遍历
  std::set<MElement*> visited;
  std::queue<MElement*> queue;

  queue.push(elements[0]);
  visited.insert(elements[0]);

  while(!queue.empty()) {
    MElement *current = queue.front();
    queue.pop();

    // 获取相邻元素
    auto neighbors = getNeighborsByEdge(current, e2e);
    for(auto n : neighbors) {
      if(visited.find(n) == visited.end()) {
        visited.insert(n);
        queue.push(n);
      }
    }
  }

  // 如果访问了所有元素，则连通
  return visited.size() == elements.size();
}
```

---

## 今日检查点

- [ ] 理解MEdge的定义和顶点排序规则
- [ ] 理解MFace的定义和顶点排序规则
- [ ] 理解边/面哈希的实现原理
- [ ] 能实现简单的拓扑查询算法

---

## 核心概念

### 拓扑关系类型

```
顶点 (0D) ←→ 边 (1D) ←→ 面 (2D) ←→ 体 (3D)

关系类型：
┌─────────────────────────────────────────────────────┐
│ 边界关系：高维实体的边界是低维实体                  │
│   三角形的边界 = 3条边                              │
│   四面体的边界 = 4个三角形面                        │
│                                                     │
│ 邻接关系：通过共享低维实体相邻                      │
│   两个三角形通过共享边相邻                          │
│   两个四面体通过共享面相邻                          │
│                                                     │
│ 关联关系：顶点与包含它的元素的关系                  │
│   顶点的关联三角形 = 所有以该顶点为角点的三角形     │
└─────────────────────────────────────────────────────┘
```

### 网格数据结构对比

```
显式存储 vs 隐式存储：

显式存储（存储所有关系）：
  - 优点：查询快 O(1)
  - 缺点：内存大，更新复杂

隐式存储（只存储顶点，按需计算）：
  - 优点：内存小，更新简单
  - 缺点：查询需要遍历

Gmsh采用混合方式：
  - 元素直接存储顶点引用
  - 边和面按需计算（通过getEdge/getFace）
  - 拓扑关系按需构建（Edge2Elements等）
```

### 哈希表应用

```cpp
// 边去重
std::unordered_set<MEdge, MEdgeHash, MEdgeEqual> uniqueEdges;
for(auto elem : elements) {
  for(int i = 0; i < elem->getNumEdges(); i++) {
    uniqueEdges.insert(elem->getEdge(i));
  }
}
// uniqueEdges.size() = 网格中唯一边的数量

// 面去重
std::unordered_set<MFace, MFaceHash> uniqueFaces;
for(auto elem : elements) {
  for(int i = 0; i < elem->getNumFaces(); i++) {
    uniqueFaces.insert(elem->getFace(i));
  }
}
```

### Euler公式验证

对于流形网格：V - E + F = χ（欧拉示性数）

- 平面三角网格：χ = 1（有边界）或 χ = 2（封闭）
- 环面：χ = 0
- 球面：χ = 2

```cpp
// 验证2D网格的欧拉公式
int V = vertices.size();
int E = uniqueEdges.size();
int F = triangles.size();
int chi = V - E + F;  // 应该是1或2
```

---

## 源码导航

### 关键文件

```
src/geo/
├── MEdge.h/cpp          # 网格边
├── MFace.h/cpp          # 网格面
├── MElementOctree.h/cpp # 元素空间索引
└── MEdgeVertex.h        # 边上的顶点

src/mesh/
├── meshGFaceDelaunay.cpp  # Delaunay三角化（用边/面查询）
├── meshGRegion3D.cpp      # 3D网格（用面查询）
└── meshPartition.cpp      # 网格分区（用拓扑关系）
```

### 搜索命令

```bash
# 查找边的使用
grep -rn "MEdge " src/mesh/ | head -20

# 查找面的使用
grep -rn "MFace " src/mesh/ | head -20

# 查找拓扑构建
grep -rn "edge2elements\|face2elements" src/
```

---

## 产出

- 理解网格拓扑数据结构
- 掌握拓扑查询方法

---

## 导航

- **上一天**：[Day 24 - MVertex网格顶点](day-24.md)
- **下一天**：[Day 26 - 网格质量度量](day-26.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
