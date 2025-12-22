# Day 41：网格优化基础

**所属周次**：第6周 - 网格模块基础
**所属阶段**：第三阶段 - 深入核心算法

---

## 学习目标

理解Gmsh中的网格优化技术，包括平滑、边交换和优化算法

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 了解网格优化的目标和方法 |
| 09:45-10:30 | 45min | 研究Laplacian平滑算法 |
| 10:30-11:15 | 45min | 研究边交换(Edge Swapping)算法 |
| 11:15-12:00 | 45min | 了解其他优化技术(Netgen, Gmsh自带) |
| 14:00-15:00 | 60min | 完成练习作业 |
| 15:00-16:00 | 60min | 实验优化参数效果 |

---

## 上午任务（2小时）

### 1. 网格优化概述

```
网格优化目标：
┌─────────────────────────────────────────────────────────────────┐
│  1. 提高网格质量                                                 │
│     - 最大化最小角                                               │
│     - 最小化最大角                                               │
│     - 减少扭曲元素                                               │
│                                                                  │
│  2. 改善单元形状                                                 │
│     - 接近正三角形/正四面体                                      │
│     - 保持边长均匀                                               │
│                                                                  │
│  3. 保持几何特征                                                 │
│     - 边界形状不变                                               │
│     - 特征边/点固定                                              │
└─────────────────────────────────────────────────────────────────┘

主要优化方法：
┌─────────────────┬─────────────────┬─────────────────┐
│   节点移动      │    拓扑修改     │    组合优化     │
│ (Smoothing)     │ (Connectivity)  │  (Combined)     │
├─────────────────┼─────────────────┼─────────────────┤
│ Laplacian平滑   │ 边交换         │ Netgen优化      │
│ 优化平滑        │ 面交换(3D)     │ HighOrder优化   │
│ 约束平滑        │ 边折叠         │                 │
│                 │ 边分裂         │                 │
└─────────────────┴─────────────────┴─────────────────┘
```

### 2. Laplacian平滑

```cpp
// 基本Laplacian平滑
void laplacianSmooth(GFace *gf, int iterations)
{
  for(int iter = 0; iter < iterations; iter++) {
    for(auto v : gf->mesh_vertices) {
      // 跳过边界点
      if(v->onWhat()->dim() < 2) continue;

      // 收集邻接点
      std::vector<MVertex*> neighbors;
      getNeighbors(v, neighbors);

      if(neighbors.empty()) continue;

      // 计算邻居质心
      double x = 0, y = 0, z = 0;
      for(auto n : neighbors) {
        x += n->x();
        y += n->y();
        z += n->z();
      }
      x /= neighbors.size();
      y /= neighbors.size();
      z /= neighbors.size();

      // 移动到质心
      v->setXYZ(x, y, z);
    }
  }
}
```

```
Laplacian平滑示意：
       B                      B
      ╱ ╲                    ╱ ╲
     ╱   ╲                  ╱   ╲
    A─────C    →     A─────┼─────C
     ╲   ╱                  ╲   ╱
      ╲ ╱                    ╲ ╱
       D                      D

    V原位置     →    V移到邻居质心
```

### 3. 优化平滑(Optimization-based Smoothing)

```cpp
// 基于优化的平滑
void optimizedSmooth(GFace *gf)
{
  for(auto v : gf->mesh_vertices) {
    if(v->onWhat()->dim() < 2) continue;

    // 获取周围三角形
    std::vector<MTriangle*> triangles;
    getSurroundingTriangles(v, triangles);

    // 定义目标函数：最小化周围元素的质量度量之和的负值
    auto objective = [&](double x, double y, double z) {
      v->setXYZ(x, y, z);
      double quality = 0;
      for(auto t : triangles) {
        quality += t->gammaShapeQuality();  // 越大越好
      }
      return -quality;  // 最小化
    };

    // 使用梯度下降或其他优化方法
    double x = v->x(), y = v->y(), z = v->z();
    optimize(objective, x, y, z);

    v->setXYZ(x, y, z);
  }
}
```

### 4. 边交换(Edge Swapping/Flipping)

```cpp
// 边交换优化
void edgeSwapping(GFace *gf)
{
  bool improved = true;

  while(improved) {
    improved = false;

    // 遍历所有内部边
    for(auto edge : getInternalEdges(gf)) {
      // 获取共享该边的两个三角形
      MTriangle *t1, *t2;
      getAdjacentTriangles(edge, t1, t2);

      if(!t1 || !t2) continue;  // 边界边

      // 当前配置质量
      double q_before = std::min(t1->quality(), t2->quality());

      // 尝试交换
      MTriangle *t1_new, *t2_new;
      if(trySwap(edge, t1, t2, t1_new, t2_new)) {
        double q_after = std::min(t1_new->quality(), t2_new->quality());

        if(q_after > q_before) {
          // 接受交换
          applySwap(edge, t1_new, t2_new);
          improved = true;
        }
      }
    }
  }
}
```

```
边交换示意：
    A               A
   ╱|╲             ╱ ╲
  ╱ | ╲           ╱   ╲
 D  |  B   →    D─────B
  ╲ | ╱           ╲   ╱
   ╲|╱             ╲ ╱
    C               C

 边AC交换为边DB（如果能提高质量）
```

### 5. 边折叠与边分裂

```cpp
// 边折叠：移除短边
bool edgeCollapse(MEdge *edge)
{
  MVertex *v1 = edge->getVertex(0);
  MVertex *v2 = edge->getVertex(1);

  // 检查折叠是否有效
  // - 不会产生翻转元素
  // - 不会破坏拓扑
  if(!isCollapseValid(edge)) return false;

  // 计算新顶点位置
  MVertex *newV = new MVertex(
    (v1->x() + v2->x()) / 2,
    (v1->y() + v2->y()) / 2,
    (v1->z() + v2->z()) / 2
  );

  // 更新拓扑
  replaceVertex(v1, newV);
  replaceVertex(v2, newV);

  return true;
}

// 边分裂：细化长边
bool edgeSplit(MEdge *edge)
{
  // 在边中点插入新顶点
  MVertex *mid = new MVertex(
    (edge->v1()->x() + edge->v2()->x()) / 2,
    (edge->v1()->y() + edge->v2()->y()) / 2,
    (edge->v1()->z() + edge->v2()->z()) / 2
  );

  // 分裂相邻三角形
  splitAdjacentTriangles(edge, mid);

  return true;
}
```

### 6. Gmsh优化选项

```cpp
// Gmsh网格优化API
gmsh::model::mesh::optimize(const std::string &method)

// 可用方法：
// "Laplacian"     - Laplacian平滑
// "Relocate2D"    - 2D节点重定位
// "Relocate3D"    - 3D节点重定位
// "Netgen"        - Netgen优化器
// "HighOrder"     - 高阶元素优化
// "HighOrderElastic" - 弹性高阶优化
// "HighOrderFastCurving" - 快速高阶曲线优化
// "UntangleMeshGeometry" - 解缠网格

// 选项控制
gmsh.option.setNumber("Mesh.Optimize", 1);     // 启用优化
gmsh.option.setNumber("Mesh.OptimizeNetgen", 1); // 使用Netgen
gmsh.option.setNumber("Mesh.Smoothing", 2);    // 平滑次数
```

---

## 下午任务（2小时）

### 练习作业

#### 练习1：【基础】网格优化效果对比

```python
import gmsh

def create_test_mesh():
    """创建测试网格"""
    gmsh.initialize()
    gmsh.model.add("optimize_test")

    # 创建L型区域（容易产生差质量网格）
    gmsh.model.occ.addRectangle(0, 0, 0, 2, 1)
    gmsh.model.occ.addRectangle(0, 1, 0, 1, 1)
    gmsh.model.occ.fuse([(2, 1)], [(2, 2)])
    gmsh.model.occ.synchronize()

    gmsh.option.setNumber("Mesh.Algorithm", 5)  # Delaunay
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.15)
    gmsh.option.setNumber("Mesh.Optimize", 0)   # 关闭自动优化

    gmsh.model.mesh.generate(2)

def compute_quality_stats():
    """计算网格质量统计"""
    import math

    nodes, coords, _ = gmsh.model.mesh.getNodes(2)
    coords = coords.reshape(-1, 3)
    node_dict = {tag: coords[i] for i, tag in enumerate(nodes)}

    types, tags, node_tags = gmsh.model.mesh.getElements(2)

    qualities = []

    for t, nd in zip(types, node_tags):
        if t != 2:
            continue

        nd = nd.reshape(-1, 3)
        for tri in nd:
            v1 = node_dict[tri[0]]
            v2 = node_dict[tri[1]]
            v3 = node_dict[tri[2]]

            a = math.sqrt(sum((v2[i]-v1[i])**2 for i in range(3)))
            b = math.sqrt(sum((v3[i]-v2[i])**2 for i in range(3)))
            c = math.sqrt(sum((v1[i]-v3[i])**2 for i in range(3)))

            s = (a + b + c) / 2
            area = math.sqrt(max(0, s*(s-a)*(s-b)*(s-c)))

            if a*b*c > 0:
                q = 4 * math.sqrt(3) * area / (a*a + b*b + c*c)
            else:
                q = 0

            qualities.append(q)

    return {
        'min': min(qualities),
        'max': max(qualities),
        'avg': sum(qualities) / len(qualities),
        'count': len(qualities)
    }

# 测试
create_test_mesh()

print("Before optimization:")
stats = compute_quality_stats()
print(f"  Min quality: {stats['min']:.4f}")
print(f"  Avg quality: {stats['avg']:.4f}")
print(f"  Elements: {stats['count']}")

# 应用优化
print("\nApplying Laplacian smoothing...")
gmsh.model.mesh.optimize("Laplacian")

print("After Laplacian:")
stats = compute_quality_stats()
print(f"  Min quality: {stats['min']:.4f}")
print(f"  Avg quality: {stats['avg']:.4f}")

# 再次优化
print("\nApplying Relocate2D...")
gmsh.model.mesh.optimize("Relocate2D")

print("After Relocate2D:")
stats = compute_quality_stats()
print(f"  Min quality: {stats['min']:.4f}")
print(f"  Avg quality: {stats['avg']:.4f}")

gmsh.fltk.run()
gmsh.finalize()
```

#### 练习2：【进阶】自定义平滑算法

```python
import gmsh
import math

def custom_laplacian_smooth(iterations=3):
    """自定义Laplacian平滑实现"""

    # 获取网格数据
    nodes, coords, _ = gmsh.model.mesh.getNodes(2)
    coords = coords.reshape(-1, 3)

    # 构建节点索引
    node_dict = {tag: i for i, tag in enumerate(nodes)}

    # 获取边界节点
    boundary_nodes = set()
    for dim in [0, 1]:
        entities = gmsh.model.getEntities(dim)
        for d, t in entities:
            bnodes, _, _ = gmsh.model.mesh.getNodes(d, t)
            boundary_nodes.update(bnodes)

    # 获取元素连接关系
    types, tags, node_tags = gmsh.model.mesh.getElements(2)

    # 构建邻接关系
    adjacency = {tag: set() for tag in nodes}

    for t, nd in zip(types, node_tags):
        if t != 2:
            continue
        nd = nd.reshape(-1, 3)
        for tri in nd:
            for i in range(3):
                for j in range(3):
                    if i != j:
                        adjacency[tri[i]].add(tri[j])

    # 迭代平滑
    for iter in range(iterations):
        new_coords = coords.copy()

        for tag, neighbors in adjacency.items():
            # 跳过边界节点
            if tag in boundary_nodes:
                continue

            if not neighbors:
                continue

            # 计算邻居质心
            i = node_dict[tag]
            centroid = [0, 0, 0]

            for n in neighbors:
                j = node_dict[n]
                centroid[0] += coords[j, 0]
                centroid[1] += coords[j, 1]
                centroid[2] += coords[j, 2]

            centroid = [c / len(neighbors) for c in centroid]

            # 更新坐标
            new_coords[i] = centroid

        coords = new_coords
        print(f"  Iteration {iter+1} completed")

    # 应用新坐标
    for tag in nodes:
        i = node_dict[tag]
        gmsh.model.mesh.setNode(tag, list(coords[i]), [])

# 测试
gmsh.initialize()
gmsh.model.add("custom_smooth")

gmsh.model.occ.addDisk(0, 0, 0, 1, 1)
gmsh.model.occ.synchronize()

gmsh.option.setNumber("Mesh.MeshSizeMax", 0.15)
gmsh.option.setNumber("Mesh.Optimize", 0)
gmsh.model.mesh.generate(2)

print("Applying custom Laplacian smoothing...")
custom_laplacian_smooth(5)

gmsh.fltk.run()
gmsh.finalize()
```

#### 练习3：【挑战】实现边交换优化

```python
import gmsh
import math

def triangle_quality(v1, v2, v3):
    """计算三角形质量"""
    a = math.sqrt(sum((v2[i]-v1[i])**2 for i in range(3)))
    b = math.sqrt(sum((v3[i]-v2[i])**2 for i in range(3)))
    c = math.sqrt(sum((v1[i]-v3[i])**2 for i in range(3)))

    s = (a + b + c) / 2
    area = math.sqrt(max(0, s*(s-a)*(s-b)*(s-c)))

    if a*b*c > 0:
        return 4 * math.sqrt(3) * area / (a*a + b*b + c*c)
    return 0

def find_edge_swaps(elements, coords):
    """找出可以改善质量的边交换"""
    # 构建边到三角形的映射
    edge_to_tris = {}

    for idx, tri in enumerate(elements):
        for i in range(3):
            edge = tuple(sorted([tri[i], tri[(i+1)%3]]))
            if edge not in edge_to_tris:
                edge_to_tris[edge] = []
            edge_to_tris[edge].append(idx)

    # 找可交换的边
    swaps = []

    for edge, tri_indices in edge_to_tris.items():
        if len(tri_indices) != 2:
            continue  # 边界边或其他

        t1_idx, t2_idx = tri_indices
        t1 = elements[t1_idx]
        t2 = elements[t2_idx]

        # 找对顶点
        v1, v2 = edge
        v3 = [v for v in t1 if v not in edge][0]
        v4 = [v for v in t2 if v not in edge][0]

        # 当前质量
        q_before = min(
            triangle_quality(coords[v1], coords[v2], coords[v3]),
            triangle_quality(coords[v1], coords[v2], coords[v4])
        )

        # 交换后质量
        q_after = min(
            triangle_quality(coords[v1], coords[v3], coords[v4]),
            triangle_quality(coords[v2], coords[v3], coords[v4])
        )

        if q_after > q_before + 0.01:  # 有改善
            swaps.append({
                'edge': edge,
                'improvement': q_after - q_before,
                'v3': v3,
                'v4': v4,
                't1': t1_idx,
                't2': t2_idx
            })

    return swaps

# 这是一个概念演示，实际修改Gmsh网格需要使用低级API
print("Edge swap optimization concept:")
print("1. Build edge-to-triangle mapping")
print("2. For each internal edge:")
print("   - Compute current triangle quality")
print("   - Compute quality after swap")
print("   - If improved, mark for swap")
print("3. Apply beneficial swaps")
print("4. Repeat until no improvement")
```

---

## 核心概念

### 优化方法总结

| 方法 | 原理 | 优点 | 缺点 |
|------|------|------|------|
| Laplacian平滑 | 移动到邻居质心 | 简单快速 | 可能压缩网格 |
| 优化平滑 | 最大化质量函数 | 效果好 | 计算量大 |
| 边交换 | 改变连接关系 | 不增加节点 | 可能不够 |
| 边折叠 | 移除短边 | 简化网格 | 可能改变几何 |
| 边分裂 | 细化长边 | 增加质量 | 增加节点 |

### Gmsh优化API

```python
# 启用自动优化
gmsh.option.setNumber("Mesh.Optimize", 1)

# 手动优化
gmsh.model.mesh.optimize("Laplacian")
gmsh.model.mesh.optimize("Relocate2D")
gmsh.model.mesh.optimize("Netgen")

# 高阶元素优化
gmsh.model.mesh.optimize("HighOrder")
```

---

## 今日检查点

- [ ] 理解网格优化的目标
- [ ] 能解释Laplacian平滑的原理
- [ ] 理解边交换如何改善质量
- [ ] 能使用Gmsh优化API

---

## 源码导航

关键代码位置：
- `src/mesh/meshOptimize.cpp` - 优化主文件
- `src/mesh/meshRelocate.cpp` - 节点重定位
- `src/mesh/meshGFaceOptimize.cpp` - 2D优化
- `src/mesh/meshGRegionOptimize.cpp` - 3D优化
- `src/contrib/Netgen/` - Netgen优化器

---

## 导航

- **上一天**：[Day 40 - 前沿推进法基础](day-40.md)
- **下一天**：[Day 42 - 第六周复习](day-42.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
