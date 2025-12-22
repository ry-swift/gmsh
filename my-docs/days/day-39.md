# Day 39：Delaunay三角化基础

**所属周次**：第6周 - 网格模块基础
**所属阶段**：第三阶段 - 深入核心算法

---

## 学习目标

深入理解Delaunay三角化算法的原理和Gmsh实现

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 学习Delaunay三角化的数学基础 |
| 09:45-10:30 | 45min | 阅读meshGFaceDelaunay.cpp实现 |
| 10:30-11:15 | 45min | 研究约束Delaunay三角化(CDT) |
| 11:15-12:00 | 45min | 分析边界恢复算法 |
| 14:00-15:00 | 60min | 完成练习作业 |
| 15:00-16:00 | 60min | 实验和调试 |

---

## 上午任务（2小时）

### 1. Delaunay三角化原理

```
Delaunay准则：
┌─────────────────────────────────────────────────────────────────┐
│  任意三角形的外接圆内部不包含其他点                              │
│                                                                  │
│       非Delaunay                         Delaunay               │
│           ○                                 ○                   │
│         ╱   ╲                            ╱   ╲                  │
│       ╱   P   ╲    P在圆内             ╱       ╲               │
│      A─────────B   需要翻转           A─────────B  P在圆外      │
│       ╲       ╱                        ╲       ╱                │
│         ╲   ╱                            ╲ P ╱                  │
│           ○                                 ○                   │
│           C                                 C                   │
└─────────────────────────────────────────────────────────────────┘

边翻转操作：
    A              A
   ╱|╲    翻转    ╱ ╲
  ╱ | ╲   ───→   ╱   ╲
 D  |  B        D─────B
  ╲ | ╱          ╲   ╱
   ╲|╱            ╲ ╱
    C              C
```

### 2. Bowyer-Watson算法

```cpp
// 增量式Delaunay三角化
class BowyerWatson {
  std::vector<Triangle> triangles;

  void insert(Point p) {
    // 1. 找到所有外接圆包含p的三角形
    std::vector<Triangle*> badTriangles;
    for(auto& t : triangles) {
      if(t.circumcircleContains(p)) {
        badTriangles.push_back(&t);
      }
    }

    // 2. 找到这些三角形的边界多边形
    std::vector<Edge> polygon;
    for(auto* t : badTriangles) {
      for(auto& e : t->edges()) {
        // 只被一个坏三角形共享的边
        if(isPolygonEdge(e, badTriangles)) {
          polygon.push_back(e);
        }
      }
    }

    // 3. 删除坏三角形
    for(auto* t : badTriangles) {
      removeTriangle(t);
    }

    // 4. 用新点和多边形边创建新三角形
    for(auto& e : polygon) {
      Triangle newT(e.p1, e.p2, p);
      triangles.push_back(newT);
    }
  }
};
```

### 3. Gmsh中的Delaunay实现

```cpp
// meshGFaceDelaunay.cpp
void meshGFaceDelaunay(GFace *gf)
{
  // 1. 获取边界点
  std::vector<MVertex*> boundaryVertices;
  collectBoundaryVertices(gf, boundaryVertices);

  // 2. 创建初始三角化（超级三角形）
  BDS_Mesh *m = new BDS_Mesh;
  createSuperTriangle(m, boundaryVertices);

  // 3. 插入边界点
  for(auto v : boundaryVertices) {
    m->add_point(v->x(), v->y(), v->z());
  }

  // 4. 恢复约束边（边界边）
  for(auto e : gf->edges()) {
    recoverEdge(m, e);
  }

  // 5. 在内部插入新点
  insertInteriorPoints(m, gf);

  // 6. 删除外部三角形
  removeExteriorTriangles(m, gf);

  // 7. 转换为Gmsh网格元素
  convertToMesh(m, gf);

  delete m;
}
```

### 4. 约束Delaunay三角化(CDT)

```cpp
// 边界恢复算法
void recoverEdge(BDS_Mesh *m, GEdge *ge)
{
  // 边界边必须出现在最终三角化中
  // 使用边翻转或边插入来恢复

  for(int i = 0; i < ge->mesh_vertices.size() - 1; i++) {
    MVertex *v1 = ge->mesh_vertices[i];
    MVertex *v2 = ge->mesh_vertices[i + 1];

    // 检查边是否已存在
    if(m->edgeExists(v1, v2)) continue;

    // 找到与约束边相交的三角形边
    std::vector<Edge*> crossingEdges;
    findCrossingEdges(m, v1, v2, crossingEdges);

    // 通过边翻转来恢复约束边
    for(auto* e : crossingEdges) {
      flipEdge(m, e);
    }
  }
}
```

### 5. 点定位算法

```cpp
// 快速定位包含点的三角形
Triangle* locatePoint(BDS_Mesh *m, Point p)
{
  // 方法1：遍历搜索（慢）
  // for(auto& t : m->triangles) {
  //   if(t.contains(p)) return &t;
  // }

  // 方法2：跳跃搜索（快）
  Triangle* current = &m->triangles[0];  // 从任意三角形开始

  while(true) {
    // 检查点在哪个邻居方向
    int orientation = current->orientationWrt(p);

    if(orientation == INSIDE) {
      return current;
    }

    // 跳到正确方向的邻居
    current = current->neighbor(orientation);

    if(current == nullptr) {
      return nullptr;  // 点在外部
    }
  }
}
```

### 6. 外接圆测试

```cpp
// 判断点是否在三角形外接圆内
bool inCircumcircle(Triangle* t, Point p)
{
  // 使用行列式计算（数值稳定）
  Point a = t->v1;
  Point b = t->v2;
  Point c = t->v3;

  double ax = a.x - p.x;
  double ay = a.y - p.y;
  double bx = b.x - p.x;
  double by = b.y - p.y;
  double cx = c.x - p.x;
  double cy = c.y - p.y;

  double det =
    (ax*ax + ay*ay) * (bx*cy - cx*by) -
    (bx*bx + by*by) * (ax*cy - cx*ay) +
    (cx*cx + cy*cy) * (ax*by - bx*ay);

  // det > 0: 点在圆内
  // det = 0: 点在圆上
  // det < 0: 点在圆外
  return det > 0;
}
```

---

## 下午任务（2小时）

### 练习作业

#### 练习1：【基础】可视化Delaunay性质

```python
import gmsh
import math

gmsh.initialize()
gmsh.model.add("delaunay_demo")

# 创建随机点集
import random
random.seed(42)

points = []
for i in range(20):
    x = random.uniform(0, 1)
    y = random.uniform(0, 1)
    p = gmsh.model.geo.addPoint(x, y, 0, 0.5)
    points.append(p)

# 创建包围盒
p1 = gmsh.model.geo.addPoint(-0.1, -0.1, 0, 0.5)
p2 = gmsh.model.geo.addPoint(1.1, -0.1, 0, 0.5)
p3 = gmsh.model.geo.addPoint(1.1, 1.1, 0, 0.5)
p4 = gmsh.model.geo.addPoint(-0.1, 1.1, 0, 0.5)

l1 = gmsh.model.geo.addLine(p1, p2)
l2 = gmsh.model.geo.addLine(p2, p3)
l3 = gmsh.model.geo.addLine(p3, p4)
l4 = gmsh.model.geo.addLine(p4, p1)

cl = gmsh.model.geo.addCurveLoop([l1, l2, l3, l4])
s = gmsh.model.geo.addPlaneSurface([cl])

# 将内部点嵌入面
for p in points:
    gmsh.model.geo.synchronize()
    coords = gmsh.model.getValue(0, p, [])
    gmsh.model.mesh.embed(0, [p], 2, s)

gmsh.model.geo.synchronize()

# 使用Delaunay算法
gmsh.option.setNumber("Mesh.Algorithm", 5)
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.5)  # 不插入新点

gmsh.model.mesh.generate(2)

gmsh.fltk.run()
gmsh.finalize()
```

#### 练习2：【进阶】检验Delaunay性质

```python
import gmsh
import math

def check_delaunay_property():
    """检查所有三角形是否满足Delaunay性质"""

    # 获取所有节点
    node_tags, coords, _ = gmsh.model.mesh.getNodes(2)
    coords = coords.reshape(-1, 3)
    node_dict = {tag: coords[i] for i, tag in enumerate(node_tags)}

    # 获取所有三角形
    types, tags, nodes = gmsh.model.mesh.getElements(2)

    violations = 0

    for t, tg, nd in zip(types, tags, nodes):
        if t != 2:  # 只处理三角形
            continue

        nd = nd.reshape(-1, 3)

        for tri_nodes in nd:
            # 三角形顶点
            v1 = node_dict[tri_nodes[0]]
            v2 = node_dict[tri_nodes[1]]
            v3 = node_dict[tri_nodes[2]]

            # 计算外接圆
            center, radius = circumcircle(v1, v2, v3)

            # 检查所有其他点
            for tag, coord in node_dict.items():
                if tag in tri_nodes:
                    continue

                dist = math.sqrt(sum((a-b)**2 for a, b in zip(coord, center)))

                if dist < radius - 1e-10:  # 考虑数值误差
                    violations += 1
                    break

    return violations

def circumcircle(v1, v2, v3):
    """计算三角形外接圆"""
    ax, ay = v1[0], v1[1]
    bx, by = v2[0], v2[1]
    cx, cy = v3[0], v3[1]

    d = 2 * (ax*(by-cy) + bx*(cy-ay) + cx*(ay-by))

    if abs(d) < 1e-10:
        return (0, 0, 0), float('inf')

    ux = ((ax*ax+ay*ay)*(by-cy) + (bx*bx+by*by)*(cy-ay) + (cx*cx+cy*cy)*(ay-by)) / d
    uy = ((ax*ax+ay*ay)*(cx-bx) + (bx*bx+by*by)*(ax-cx) + (cx*cx+cy*cy)*(bx-ax)) / d

    center = (ux, uy, 0)
    radius = math.sqrt((ax-ux)**2 + (ay-uy)**2)

    return center, radius

# 主程序
gmsh.initialize()
gmsh.model.add("check_delaunay")

gmsh.model.occ.addDisk(0, 0, 0, 1, 1)
gmsh.model.occ.synchronize()

gmsh.option.setNumber("Mesh.Algorithm", 5)  # Delaunay
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.2)

gmsh.model.mesh.generate(2)

violations = check_delaunay_property()
print(f"Delaunay violations: {violations}")

gmsh.fltk.run()
gmsh.finalize()
```

#### 练习3：【挑战】手动实现简单Delaunay

```python
import math

class Point:
    def __init__(self, x, y):
        self.x = x
        self.y = y

class Triangle:
    def __init__(self, p1, p2, p3):
        self.vertices = [p1, p2, p3]
        self.compute_circumcircle()

    def compute_circumcircle(self):
        """计算外接圆"""
        p1, p2, p3 = self.vertices
        ax, ay = p1.x, p1.y
        bx, by = p2.x, p2.y
        cx, cy = p3.x, p3.y

        d = 2 * (ax*(by-cy) + bx*(cy-ay) + cx*(ay-by))

        if abs(d) < 1e-10:
            self.center = Point(0, 0)
            self.radius = float('inf')
            return

        ux = ((ax*ax+ay*ay)*(by-cy) + (bx*bx+by*by)*(cy-ay) +
              (cx*cx+cy*cy)*(ay-by)) / d
        uy = ((ax*ax+ay*ay)*(cx-bx) + (bx*bx+by*by)*(ax-cx) +
              (cx*cx+cy*cy)*(bx-ax)) / d

        self.center = Point(ux, uy)
        self.radius = math.sqrt((ax-ux)**2 + (ay-uy)**2)

    def contains_point(self, p):
        """检查点是否在外接圆内"""
        dist = math.sqrt((p.x - self.center.x)**2 +
                        (p.y - self.center.y)**2)
        return dist < self.radius - 1e-10

def delaunay_triangulation(points):
    """简单的Bowyer-Watson实现"""

    # 创建超级三角形
    min_x = min(p.x for p in points)
    max_x = max(p.x for p in points)
    min_y = min(p.y for p in points)
    max_y = max(p.y for p in points)

    dx = max_x - min_x
    dy = max_y - min_y
    delta = max(dx, dy) * 2

    p1 = Point(min_x - delta, min_y - delta)
    p2 = Point(max_x + delta, min_y - delta)
    p3 = Point((min_x + max_x) / 2, max_y + delta)

    triangles = [Triangle(p1, p2, p3)]
    super_vertices = {p1, p2, p3}

    # 逐点插入
    for p in points:
        # 找坏三角形
        bad_triangles = [t for t in triangles if t.contains_point(p)]

        # 找边界多边形
        polygon = []
        for t in bad_triangles:
            for i in range(3):
                edge = (t.vertices[i], t.vertices[(i+1)%3])

                # 检查边是否只属于一个坏三角形
                shared = False
                for other in bad_triangles:
                    if other == t:
                        continue
                    if set(edge) <= set(other.vertices):
                        shared = True
                        break

                if not shared:
                    polygon.append(edge)

        # 删除坏三角形
        for t in bad_triangles:
            triangles.remove(t)

        # 创建新三角形
        for e in polygon:
            triangles.append(Triangle(e[0], e[1], p))

    # 删除包含超级三角形顶点的三角形
    triangles = [t for t in triangles
                 if not any(v in super_vertices for v in t.vertices)]

    return triangles

# 测试
import random
random.seed(42)

points = [Point(random.uniform(0, 1), random.uniform(0, 1))
          for _ in range(10)]

triangles = delaunay_triangulation(points)
print(f"Generated {len(triangles)} triangles")

# 打印结果
for i, t in enumerate(triangles):
    print(f"Triangle {i}: ({t.vertices[0].x:.2f}, {t.vertices[0].y:.2f}), "
          f"({t.vertices[1].x:.2f}, {t.vertices[1].y:.2f}), "
          f"({t.vertices[2].x:.2f}, {t.vertices[2].y:.2f})")
```

---

## 核心概念

### Delaunay三角化的性质

```
1. 空圆性质：任意三角形的外接圆内不包含其他点
2. 唯一性：对于一般位置的点集，Delaunay三角化唯一
3. 最大化最小角：所有三角化中，Delaunay三角化的最小角最大
4. 边的局部性：两点连边当且仅当存在一个不含其他点的圆经过它们
```

### 约束Delaunay三角化(CDT)

```
CDT = Delaunay + 强制边约束

应用场景：
- 边界必须是网格边
- 特征边（几何特征）保持
- 区域划分

恢复方法：
1. 边翻转
2. 边插入（添加Steiner点）
```

---

## 今日检查点

- [ ] 理解Delaunay准则（空圆性质）
- [ ] 能解释Bowyer-Watson算法步骤
- [ ] 理解约束Delaunay三角化的需求
- [ ] 能识别非Delaunay三角形

---

## 源码导航

关键代码位置：
- `src/mesh/meshGFaceDelaunay.cpp` - 面Delaunay网格
- `src/mesh/BDS.cpp` - BDS数据结构（用于2D Delaunay）
- `src/mesh/delaunay3d.cpp` - 3D Delaunay
- `src/numeric/predicates.cpp` - 几何谓词（精确计算）

---

## 导航

- **上一天**：[Day 38 - 2D网格算法概述](day-38.md)
- **下一天**：[Day 40 - 前沿推进法基础](day-40.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
