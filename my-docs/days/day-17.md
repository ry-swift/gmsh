# Day 17：空间索引结构

**所属周次**：第3周 - 通用工具模块与核心数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

掌握八叉树和哈希表

---

## 学习文件

- `src/common/Octree.h`
- `src/common/Hash.h`

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 学习八叉树的基本概念 |
| 09:45-10:30 | 45min | 阅读Octree.h头文件 |
| 10:30-11:15 | 45min | 理解八叉树的构建过程 |
| 11:15-12:00 | 45min | 理解空间查询操作 |
| 14:00-14:45 | 45min | 阅读Hash.h |
| 14:45-15:30 | 45min | 理解MEdgeHash和MFaceHash |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 八叉树基本概念

八叉树（Octree）是一种空间分割数据结构，将3D空间递归地分成8个子空间。

```
     ┌────────┬────────┐
    /        /        /│
   /   4    /   5    / │
  ├────────┼────────┤  │
  │        │        │ 7│
  │   0    │   1    │ /│
  ├────────┼────────┤/ │
 /│       /│       /│  │
/6│      /7│      / │ 3│
├─┼──────┼─┼──────┤ │ /
│ │   2  │ │   3  │ │/
│ └──────┼─┴──────┤/
│        │        │
│   2    │   3    │
└────────┴────────┘
```

### 阅读 Octree.h

**文件路径**：`src/common/Octree.h`

关注内容：
- [ ] Octree类的构造和析构
- [ ] `insert()` - 插入元素
- [ ] `search()` - 空间查询
- [ ] 边界盒（Bounding Box）的使用

```cpp
// Octree核心接口（概念）
class Octree {
public:
  Octree(double *bbox);  // 根据边界盒构建
  void insert(void *element, double *bbox);
  void search(double *query_bbox, std::vector<void*> &result);
  void searchAll(double *point, std::vector<void*> &result);
};
```

---

## 下午任务（2小时）

### 阅读 Hash.h

**文件路径**：`src/common/Hash.h`

关注内容：
- [ ] 哈希函数设计
- [ ] MEdgeHash - 网格边的哈希
- [ ] MFaceHash - 网格面的哈希

```cpp
// 网格边哈希（用于边的去重和查询）
class MEdge {
  MVertex *v0, *v1;  // 两个端点
public:
  bool operator==(const MEdge &other) const;
};

// 哈希函数
struct MEdgeHash {
  size_t operator()(const MEdge &e) const {
    // 基于顶点指针计算哈希值
    return hash(e.v0) ^ hash(e.v1);
  }
};

// 使用示例
std::unordered_set<MEdge, MEdgeHash> uniqueEdges;
```

### 哈希表在Gmsh中的应用

1. **边去重**：网格生成时避免重复创建边
2. **拓扑查询**：快速查找共享某条边的单元
3. **面匹配**：3D网格中识别相邻单元

---

## 练习作业

### 1. 【基础】理解八叉树的使用场景

搜索八叉树在Gmsh中的使用位置：

```bash
grep -rn "Octree" src/ | head -20
```

记录至少3个八叉树被使用的场景。

### 2. 【进阶】分析哈希函数设计

阅读MEdgeHash的实现：

```bash
grep -A 10 "struct MEdgeHash" src/
# 或
grep -A 10 "operator()(const MEdge" src/
```

思考问题：
- 为什么哈希值与顶点顺序无关？（即MEdge(v0,v1)和MEdge(v1,v0)应该相同）
- 如何处理哈希冲突？

### 3. 【挑战】空间查询算法分析

假设要查找包含某个点的所有网格单元：

```cpp
// 问题：如何高效实现？
std::vector<MElement*> findElementsContainingPoint(double x, double y, double z);
```

分析步骤：
1. 使用八叉树缩小搜索范围
2. 对候选单元进行精确判断
3. 估算时间复杂度

---

## 今日检查点

- [ ] 理解八叉树的空间分割原理
- [ ] 理解Octree类的主要接口
- [ ] 理解哈希表在网格处理中的作用
- [ ] 能解释MEdge和MFace哈希的设计原因

---

## 核心概念

### 八叉树（Octree）

**用途**：
- 空间点的快速查询
- 碰撞检测
- 最近邻搜索
- 网格局部化操作

**复杂度**：
- 插入：O(log N)
- 查询：O(log N + k)，k为结果数量
- 空间：O(N)

### 哈希表（Hash Table）

**用途**：
- 网格边/面的去重
- 快速拓扑查询
- 共享实体识别

**MEdge哈希设计要点**：

```cpp
// 边由两个顶点定义，但顺序不重要
// 即 Edge(A,B) == Edge(B,A)

// 方法1：排序后哈希
size_t hash(const MEdge &e) {
  MVertex *v0 = std::min(e.v0, e.v1);
  MVertex *v1 = std::max(e.v0, e.v1);
  return hash(v0) * 31 + hash(v1);
}

// 方法2：交换不变哈希（XOR）
size_t hash(const MEdge &e) {
  return hash(e.v0) ^ hash(e.v1);  // XOR交换不变
}
```

### 空间查询流程

```
┌──────────────────────────────────────────┐
│           查询请求                        │
│    point(x, y, z) 或 bbox               │
└────────────────────┬─────────────────────┘
                     │
┌────────────────────▼─────────────────────┐
│        八叉树空间索引                     │
│   快速缩小搜索范围到候选集合              │
│   复杂度: O(log N)                       │
└────────────────────┬─────────────────────┘
                     │
┌────────────────────▼─────────────────────┐
│         精确几何判断                      │
│   对候选集进行精确的包含/相交测试         │
│   复杂度: O(k)                           │
└────────────────────┬─────────────────────┘
                     │
┌────────────────────▼─────────────────────┐
│           返回结果                        │
└──────────────────────────────────────────┘
```

---

## 数据结构对比

| 结构 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| 八叉树 | 空间局部性好 | 构建开销 | 空间查询 |
| 哈希表 | O(1)查询 | 无序 | 去重、查找 |
| KD树 | 最近邻高效 | 高维退化 | 点云处理 |
| R树 | 矩形查询好 | 实现复杂 | GIS应用 |

---

## 源码导航

### 关键文件

```
src/common/
├── Octree.h          # 八叉树头文件
├── Octree.cpp        # 八叉树实现
├── Hash.h            # 哈希函数
└── SBoundingBox3d.h  # 3D边界盒

src/geo/
├── MEdge.h           # 网格边定义
├── MFace.h           # 网格面定义
└── MElement.h        # 网格单元定义
```

### 搜索命令

```bash
# 查找八叉树使用
grep -rn "new Octree" src/
grep -rn "Octree::search" src/

# 查找哈希使用
grep -rn "MEdgeHash" src/
grep -rn "MFaceHash" src/
grep -rn "unordered_set.*MEdge" src/
```

---

## 产出

- 理解关键数据结构
- 记录八叉树和哈希表的使用场景

---

## 导航

- **上一天**：[Day 16 - 选项系统与上下文](day-16.md)
- **下一天**：[Day 18 - 几何实体基类](day-18.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
