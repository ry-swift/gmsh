# Day 40：前沿推进法基础

**所属周次**：第6周 - 网格模块基础
**所属阶段**：第三阶段 - 深入核心算法

---

## 学习目标

理解前沿推进法(Advancing Front Method)的原理和实现

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 学习前沿推进法的基本原理 |
| 09:45-10:30 | 45min | 阅读meshGFaceFrontal.cpp |
| 10:30-11:15 | 45min | 研究理想点计算和前沿更新 |
| 11:15-12:00 | 45min | 对比Frontal与Delaunay的优缺点 |
| 14:00-15:00 | 60min | 完成练习作业 |
| 15:00-16:00 | 60min | 实验不同参数效果 |

---

## 上午任务（2小时）

### 1. 前沿推进法原理

```
前沿推进法流程：
┌─────────────────────────────────────────────────────────────────┐
│  初始状态: 边界作为初始前沿                                      │
│                                                                  │
│     ╭──────────────────────────╮                                │
│     │         前沿             │                                │
│     │    ╱              ╲      │                                │
│     │  ╱                  ╲    │                                │
│     ╰────────────────────────╯                                  │
│                                                                  │
│  迭代过程: 选边 → 创建点 → 生成三角形 → 更新前沿                │
│                                                                  │
│     ╭──────────────────────────╮                                │
│     │     ╱▲            ╲      │                                │
│     │   ╱  │              ╲    │                                │
│     │ ╱────│────────────────╲  │  新点向内推进                  │
│     ╰────────────────────────╯                                  │
│                                                                  │
│  结束: 前沿闭合，区域完全填充                                    │
│                                                                  │
│     ╭──────────────────────────╮                                │
│     │╲    ╱╲    ╱╲    ╱╲    ╱ │                                │
│     │  ╲╱    ╲╱    ╲╱    ╲╱   │ 完全三角化                     │
│     │  ╱╲    ╱╲    ╱╲    ╱╲   │                                │
│     ╰────────────────────────╯                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 2. 算法核心步骤

```cpp
// meshGFaceFrontal.cpp 核心流程
class AdvancingFront {
  // 前沿：活动边的列表
  std::list<HalfEdge*> front;

  // 已生成的三角形
  std::vector<Triangle*> triangles;

  void generate() {
    // 1. 初始化前沿为边界
    initializeFront();

    // 2. 主循环
    while(!front.empty()) {
      // 2.1 选择最佳边
      HalfEdge* edge = selectBestEdge();

      // 2.2 计算理想的新点位置
      Point idealPoint = computeIdealPoint(edge);

      // 2.3 尝试连接到现有前沿点
      Point* existingPoint = findExistingPoint(edge, idealPoint);

      // 2.4 创建三角形
      if(existingPoint) {
        createTriangle(edge, existingPoint);
      } else {
        // 创建新点
        Point* newPoint = createPoint(idealPoint);
        createTriangle(edge, newPoint);
      }

      // 2.5 更新前沿
      updateFront(edge);
    }
  }
};
```

### 3. 理想点计算

```cpp
// 计算理想的新点位置
Point computeIdealPoint(HalfEdge* edge)
{
  Point p1 = edge->from;
  Point p2 = edge->to;

  // 边的中点
  Point midpoint = (p1 + p2) / 2;

  // 边的法向量（指向内部）
  Vector normal = edge->normal();

  // 边长
  double edgeLength = edge->length();

  // 获取期望的网格尺寸
  double h = meshSizeAt(midpoint);

  // 理想高度（等边三角形）
  double idealHeight = h * sqrt(3) / 2;

  // 理想点 = 中点 + 高度 * 法向
  return midpoint + normal * idealHeight;
}
```

### 4. 现有点搜索

```cpp
// 在前沿上搜索可用的现有点
Point* findExistingPoint(HalfEdge* edge, Point idealPoint)
{
  double radius = edge->length() * 1.5;  // 搜索半径

  Point* bestPoint = nullptr;
  double bestScore = -1e30;

  // 遍历前沿上的点
  for(auto* frontEdge : front) {
    Point* p = frontEdge->from;

    // 检查距离
    if(distance(p, idealPoint) > radius) continue;

    // 检查是否形成有效三角形
    if(!isValidTriangle(edge, p)) continue;

    // 计算质量得分
    double score = triangleQuality(edge->from, edge->to, p);

    if(score > bestScore) {
      bestScore = score;
      bestPoint = p;
    }
  }

  // 如果找到足够好的点，使用它
  if(bestScore > 0.5) {
    return bestPoint;
  }

  return nullptr;  // 需要创建新点
}
```

### 5. 前沿更新

```cpp
// 更新前沿
void updateFront(HalfEdge* edge, Point* newPoint)
{
  // 从前沿移除当前边
  front.remove(edge);

  // 检查新边是否与前沿边重合
  HalfEdge* newEdge1 = new HalfEdge(edge->from, newPoint);
  HalfEdge* newEdge2 = new HalfEdge(newPoint, edge->to);

  // 如果新边与现有前沿边重合，两者都移除
  HalfEdge* matching1 = findMatchingEdge(newEdge1);
  if(matching1) {
    front.remove(matching1);
    delete newEdge1;
  } else {
    front.push_back(newEdge1);
  }

  HalfEdge* matching2 = findMatchingEdge(newEdge2);
  if(matching2) {
    front.remove(matching2);
    delete newEdge2;
  } else {
    front.push_back(newEdge2);
  }
}
```

### 6. 边选择策略

```cpp
// 选择最佳边进行处理
HalfEdge* selectBestEdge()
{
  // 策略1：最短边优先
  // 可以产生更均匀的网格
  HalfEdge* shortest = nullptr;
  double minLen = 1e30;

  for(auto* e : front) {
    if(e->length() < minLen) {
      minLen = e->length();
      shortest = e;
    }
  }
  return shortest;

  // 策略2：最长边优先
  // 可以快速填充大区域

  // 策略3：FIFO
  // 简单，但可能产生扭曲
}
```

### 7. Frontal-Delaunay混合算法

```cpp
// Gmsh默认使用Frontal-Delaunay混合
void meshGFaceFrontalDelaunay(GFace *gf)
{
  // 1. 使用前沿推进生成点
  std::vector<Point> points;
  generatePointsUsingFrontal(gf, points);

  // 2. 对生成的点进行Delaunay三角化
  delaunayTriangulation(points, gf);

  // 优点：
  // - 前沿推进产生均匀分布的点
  // - Delaunay保证三角形质量
}
```

---

## 下午任务（2小时）

### 练习作业

#### 练习1：【基础】对比Frontal和Delaunay

```python
import gmsh

def mesh_with_algorithm(algo, name):
    gmsh.initialize()
    gmsh.model.add(name)

    # 创建L型区域
    gmsh.model.occ.addRectangle(0, 0, 0, 2, 1)
    gmsh.model.occ.addRectangle(0, 1, 0, 1, 1)
    gmsh.model.occ.fuse([(2, 1)], [(2, 2)])
    gmsh.model.occ.synchronize()

    gmsh.option.setNumber("Mesh.Algorithm", algo)
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)

    import time
    start = time.time()
    gmsh.model.mesh.generate(2)
    elapsed = time.time() - start

    # 计算网格质量
    quality = compute_mesh_quality()

    print(f"{name}: time={elapsed:.3f}s, min_quality={quality['min']:.3f}, "
          f"avg_quality={quality['avg']:.3f}")

    gmsh.write(f"{name}.msh")
    gmsh.finalize()

    return quality

def compute_mesh_quality():
    """计算三角形质量"""
    import math

    nodes, coords, _ = gmsh.model.mesh.getNodes(2)
    coords = coords.reshape(-1, 3)
    node_dict = {tag: coords[i] for i, tag in enumerate(nodes)}

    types, tags, node_tags = gmsh.model.mesh.getElements(2)

    qualities = []

    for t, nd in zip(types, node_tags):
        if t != 2:  # 只处理三角形
            continue

        nd = nd.reshape(-1, 3)
        for tri in nd:
            v1 = node_dict[tri[0]]
            v2 = node_dict[tri[1]]
            v3 = node_dict[tri[2]]

            # 计算SICN质量（归一化形状因子）
            a = math.sqrt(sum((v2[i]-v1[i])**2 for i in range(3)))
            b = math.sqrt(sum((v3[i]-v2[i])**2 for i in range(3)))
            c = math.sqrt(sum((v1[i]-v3[i])**2 for i in range(3)))

            s = (a + b + c) / 2
            area = math.sqrt(max(0, s*(s-a)*(s-b)*(s-c)))

            if a*b*c > 0:
                quality = 4 * math.sqrt(3) * area / (a*a + b*b + c*c)
            else:
                quality = 0

            qualities.append(quality)

    return {
        'min': min(qualities) if qualities else 0,
        'max': max(qualities) if qualities else 0,
        'avg': sum(qualities)/len(qualities) if qualities else 0
    }

# 对比
mesh_with_algorithm(5, "delaunay")
mesh_with_algorithm(6, "frontal_delaunay")
```

#### 练习2：【进阶】可视化前沿推进过程

```python
import gmsh

gmsh.initialize()
gmsh.model.add("frontal_viz")

# 创建简单几何
gmsh.model.occ.addDisk(0, 0, 0, 1, 1)
gmsh.model.occ.synchronize()

# 使用Frontal算法
gmsh.option.setNumber("Mesh.Algorithm", 6)
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.2)

# 启用网格生成步骤可视化
gmsh.option.setNumber("Mesh.SaveAll", 1)

# 生成网格
gmsh.model.mesh.generate(2)

# 获取网格信息
nodes, coords, _ = gmsh.model.mesh.getNodes(2)
types, tags, node_tags = gmsh.model.mesh.getElements(2)

print(f"Nodes: {len(nodes)}")
print(f"Elements: {sum(len(t) for t in tags)}")

# 分析三角形分布
coords = coords.reshape(-1, 3)
centroid_radii = []

for t, nd in zip(types, node_tags):
    if t != 2:
        continue
    nd = nd.reshape(-1, 3)
    for tri in nd:
        # 计算质心
        cx = sum(coords[int(n)-1][0] for n in tri) / 3
        cy = sum(coords[int(n)-1][1] for n in tri) / 3
        r = (cx**2 + cy**2)**0.5
        centroid_radii.append(r)

# 分析：理想情况下，三角形应该从边界向中心填充
import statistics
print(f"Mean centroid radius: {statistics.mean(centroid_radii):.3f}")

gmsh.fltk.run()
gmsh.finalize()
```

#### 练习3：【挑战】模拟前沿推进算法

```python
import math
import random

class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def distance(self, other):
        return math.sqrt((self.x - other.x)**2 + (self.y - other.y)**2)

class Edge:
    def __init__(self, p1, p2):
        self.p1 = p1
        self.p2 = p2

    def midpoint(self):
        return Point((self.p1.x + self.p2.x) / 2,
                    (self.p1.y + self.p2.y) / 2)

    def length(self):
        return self.p1.distance(self.p2)

    def normal(self):
        """返回指向内部的单位法向量"""
        dx = self.p2.x - self.p1.x
        dy = self.p2.y - self.p1.y
        length = math.sqrt(dx*dx + dy*dy)
        # 法向量（逆时针90度）
        return (-dy / length, dx / length)

    def matches(self, other):
        """检查是否是同一条边（忽略方向）"""
        return (self.p1 == other.p2 and self.p2 == other.p1) or \
               (self.p1 == other.p1 and self.p2 == other.p2)

class Triangle:
    def __init__(self, p1, p2, p3):
        self.vertices = [p1, p2, p3]

class AdvancingFront:
    def __init__(self, boundary_points, mesh_size=0.2):
        self.mesh_size = mesh_size
        self.triangles = []
        self.points = list(boundary_points)

        # 初始化前沿
        self.front = []
        for i in range(len(boundary_points)):
            p1 = boundary_points[i]
            p2 = boundary_points[(i + 1) % len(boundary_points)]
            self.front.append(Edge(p1, p2))

    def generate(self, max_iterations=1000):
        """主生成循环"""
        iteration = 0

        while self.front and iteration < max_iterations:
            iteration += 1

            # 选择最短边
            edge = min(self.front, key=lambda e: e.length())

            # 计算理想点
            ideal = self.compute_ideal_point(edge)

            # 查找现有点或创建新点
            point = self.find_or_create_point(edge, ideal)

            if point is None:
                # 无法创建有效三角形，跳过
                self.front.remove(edge)
                continue

            # 创建三角形
            tri = Triangle(edge.p1, edge.p2, point)
            self.triangles.append(tri)

            # 更新前沿
            self.update_front(edge, point)

        return self.triangles

    def compute_ideal_point(self, edge):
        """计算理想的新点位置"""
        mid = edge.midpoint()
        nx, ny = edge.normal()
        h = self.mesh_size * math.sqrt(3) / 2
        return Point(mid.x + nx * h, mid.y + ny * h)

    def find_or_create_point(self, edge, ideal):
        """查找现有点或创建新点"""
        search_radius = edge.length() * 1.5

        best_point = None
        best_score = -1e30

        # 搜索前沿上的点
        front_points = set()
        for e in self.front:
            front_points.add(e.p1)
            front_points.add(e.p2)

        for p in front_points:
            if p == edge.p1 or p == edge.p2:
                continue

            if p.distance(ideal) > search_radius:
                continue

            # 简单质量评估
            score = self.triangle_quality(edge.p1, edge.p2, p)

            if score > best_score and score > 0.3:
                best_score = score
                best_point = p

        if best_point:
            return best_point

        # 创建新点
        self.points.append(ideal)
        return ideal

    def triangle_quality(self, p1, p2, p3):
        """计算三角形质量（归一化形状因子）"""
        a = p1.distance(p2)
        b = p2.distance(p3)
        c = p3.distance(p1)

        s = (a + b + c) / 2
        area = math.sqrt(max(0, s*(s-a)*(s-b)*(s-c)))

        if a*b*c > 0:
            return 4 * math.sqrt(3) * area / (a*a + b*b + c*c)
        return 0

    def update_front(self, edge, point):
        """更新前沿"""
        self.front.remove(edge)

        new_edge1 = Edge(edge.p1, point)
        new_edge2 = Edge(point, edge.p2)

        # 检查是否与现有边匹配
        for new_edge in [new_edge1, new_edge2]:
            matching = None
            for e in self.front:
                if e.matches(new_edge):
                    matching = e
                    break

            if matching:
                self.front.remove(matching)
            else:
                self.front.append(new_edge)

# 测试：正方形边界
boundary = [
    Point(0, 0), Point(1, 0), Point(1, 1), Point(0, 1)
]

af = AdvancingFront(boundary, mesh_size=0.25)
triangles = af.generate()

print(f"Generated {len(triangles)} triangles")
print(f"Total points: {len(af.points)}")
print(f"Remaining front edges: {len(af.front)}")
```

---

## 核心概念

### Frontal vs Delaunay对比

| 特性 | Frontal | Delaunay |
|------|---------|----------|
| 点生成 | 主动创建 | 插入给定点 |
| 质量控制 | 直接控制 | 后验优化 |
| 边界处理 | 自然保持 | 需要恢复 |
| 速度 | 较慢 | 较快 |
| 均匀性 | 好 | 一般 |
| 各向异性 | 支持 | 困难 |

### Frontal-Delaunay混合

```
优点：
1. Frontal生成均匀点分布
2. Delaunay保证满足空圆性质
3. 结合两者优势

Gmsh默认算法(algo=6)就是这种混合方法
```

---

## 今日检查点

- [ ] 理解前沿推进法的基本思想
- [ ] 能解释前沿更新的过程
- [ ] 理解理想点的计算方法
- [ ] 能对比Frontal和Delaunay的优缺点

---

## 源码导航

关键代码位置：
- `src/mesh/meshGFaceFrontal.cpp` - 前沿推进实现
- `src/mesh/meshGFace.cpp:meshGFaceFrontal()` - 调用入口
- `src/mesh/BDS.h/cpp` - 基础数据结构

---

## 导航

- **上一天**：[Day 39 - Delaunay三角化基础](day-39.md)
- **下一天**：[Day 41 - 网格优化基础](day-41.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
