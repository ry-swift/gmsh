# Day 38：2D网格算法概述

**所属周次**：第6周 - 网格模块基础
**所属阶段**：第三阶段 - 深入核心算法

---

## 学习目标

了解Gmsh支持的各种2D网格生成算法及其特点

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读meshGFace.cpp，理解面网格生成框架 |
| 09:45-10:30 | 45min | 研究Delaunay三角化算法概述 |
| 10:30-11:15 | 45min | 研究Frontal算法概述 |
| 11:15-12:00 | 45min | 了解其他2D算法（Packing、BDS等） |
| 14:00-15:00 | 60min | 完成练习作业 |
| 15:00-16:00 | 60min | 对比不同算法的效果 |

---

## 上午任务（2小时）

### 1. 2D网格生成框架

```
meshGFace流程：
┌─────────────────────────────────────────────────────────────────┐
│  输入: GFace (几何面)                                            │
│    - 边界: 多个GEdge组成的闭合曲线                               │
│    - 参数域: (u,v) 空间                                          │
│    - 曲面方程: point(u,v), normal(u,v)                          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  步骤1: 恢复边界网格                                             │
│    - 收集边界上的MVertex                                         │
│    - 建立边界多边形                                              │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  步骤2: 选择算法                                                 │
│    根据 Mesh.Algorithm 选项:                                     │
│    1 = MeshAdapt (自适应)                                        │
│    5 = Delaunay (标准)                                           │
│    6 = Frontal-Delaunay (默认，高质量)                           │
│    7 = BAMG (各向异性)                                           │
│    8 = Frontal-Delaunay for Quads                               │
│    9 = Packing of Parallelograms                                │
│   11 = Quasi-Structured Quad                                    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  步骤3: 执行网格生成                                             │
│    - 在参数空间或物理空间进行                                    │
│    - 生成三角形或四边形                                          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  步骤4: 后处理                                                   │
│    - 网格优化（平滑、交换）                                      │
│    - 四边形重组（可选）                                          │
└─────────────────────────────────────────────────────────────────┘
```

### 2. 主要2D算法对比

| 算法 | ID | 特点 | 适用场景 |
|------|-----|------|----------|
| MeshAdapt | 1 | 自适应细化 | 需要局部加密 |
| Delaunay | 5 | 快速稳定 | 通用场景 |
| Frontal-Delaunay | 6 | 高质量 | 默认选择 |
| BAMG | 7 | 各向异性 | 边界层 |
| Frontal for Quads | 8 | 四边形 | CFD前处理 |
| Packing | 9 | 圆堆积 | 结构化趋势 |
| Quasi-Structured | 11 | 准结构化四边形 | 规则区域 |

### 3. Delaunay三角化

```cpp
// meshGFaceDelaunay.cpp 核心思想
class DelaunayTriangulation {
  // 核心数据结构
  std::vector<MTriangle*> triangles;
  std::vector<MVertex*> vertices;

  void triangulate(GFace *gf) {
    // 1. 初始化超级三角形
    createSuperTriangle();

    // 2. 逐点插入
    for(auto v : interior_vertices) {
      // 找到包含点的三角形
      MTriangle *t = locatePoint(v);

      // 插入点，更新三角化
      insertPoint(v, t);

      // 翻转非Delaunay边
      flipEdges(v);
    }

    // 3. 恢复约束边界
    recoverBoundary();

    // 4. 移除外部三角形
    removeExterior();
  }
};
```

### 4. Frontal算法

```cpp
// meshGFaceFrontal.cpp 核心思想
class FrontalTriangulation {
  // 前沿（活动边列表）
  std::list<MEdge> front;

  void triangulate(GFace *gf) {
    // 1. 初始化：边界作为初始前沿
    initializeFront(gf->edges());

    // 2. 主循环：推进前沿直到为空
    while(!front.empty()) {
      // 选择最佳边
      MEdge edge = selectBestEdge();

      // 计算理想的新点位置
      SPoint2 idealPoint = computeIdealPoint(edge);

      // 检查是否可以连接到现有点
      MVertex *v = tryConnectExisting(edge, idealPoint);

      if(!v) {
        // 创建新点
        v = createNewVertex(idealPoint);
      }

      // 创建新三角形
      createTriangle(edge, v);

      // 更新前沿
      updateFront(edge, v);
    }
  }
};
```

### 5. 四边形网格生成

```cpp
// 方法1：三角形重组
void recombineTriangles(GFace *gf) {
  // Blossom算法找最优配对
  // 将成对的三角形合并为四边形
  for(auto pair : findOptimalPairs()) {
    MTriangle *t1 = pair.first;
    MTriangle *t2 = pair.second;

    // 合并为四边形
    MQuadrangle *q = mergeToQuad(t1, t2);
  }
}

// 方法2：直接四边形生成
void generateQuadsMesh(GFace *gf) {
  // Frontal for Quads算法
  // 或 Packing of Parallelograms
}
```

### 6. 算法选择代码

```cpp
// meshGFace.cpp
void meshGFace::operator()(GFace *gf)
{
  int algo = CTX::instance()->mesh.algo2d;

  switch(algo) {
    case ALGO_2D_MESHADAPT:
      meshGFaceMeshAdapt(gf);
      break;
    case ALGO_2D_DELAUNAY:
      meshGFaceDelaunay(gf);
      break;
    case ALGO_2D_FRONTAL:
      meshGFaceFrontal(gf);
      break;
    case ALGO_2D_BAMG:
      meshGFaceBAMG(gf);
      break;
    case ALGO_2D_FRONTAL_QUAD:
      meshGFaceFrontalQuad(gf);
      break;
    case ALGO_2D_PACKING:
      meshGFacePacking(gf);
      break;
    default:
      meshGFaceFrontal(gf);
  }

  // 四边形重组（如果需要）
  if(CTX::instance()->mesh.recombineAll) {
    recombineTriangles(gf);
  }
}
```

---

## 下午任务（2小时）

### 练习作业

#### 练习1：【基础】对比不同2D算法

```python
import gmsh

algorithms = {
    1: "MeshAdapt",
    5: "Delaunay",
    6: "Frontal-Delaunay",
    8: "Frontal for Quads",
}

for algo_id, algo_name in algorithms.items():
    gmsh.initialize()
    gmsh.model.add(f"algo_{algo_id}")

    # 创建测试几何
    gmsh.model.occ.addDisk(0, 0, 0, 1, 1)
    gmsh.model.occ.synchronize()

    # 设置算法
    gmsh.option.setNumber("Mesh.Algorithm", algo_id)
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)

    # 生成网格
    import time
    start = time.time()
    gmsh.model.mesh.generate(2)
    elapsed = time.time() - start

    # 统计
    nodes = gmsh.model.mesh.getNodes(2)
    types, tags, _ = gmsh.model.mesh.getElements(2)

    tri_count = 0
    quad_count = 0
    for t, tg in zip(types, tags):
        if t == 2:  # Triangle
            tri_count += len(tg)
        elif t == 3:  # Quad
            quad_count += len(tg)

    print(f"{algo_name}: {len(nodes[0])} nodes, "
          f"{tri_count} tris, {quad_count} quads, {elapsed:.3f}s")

    gmsh.write(f"algo_{algo_id}.msh")
    gmsh.finalize()
```

#### 练习2：【进阶】四边形网格生成

```python
import gmsh

gmsh.initialize()
gmsh.model.add("quads")

# 创建L型区域
gmsh.model.occ.addRectangle(0, 0, 0, 2, 1)
gmsh.model.occ.addRectangle(0, 1, 0, 1, 1)
gmsh.model.occ.fuse([(2, 1)], [(2, 2)])
gmsh.model.occ.synchronize()

# 方法1：三角形重组
gmsh.option.setNumber("Mesh.Algorithm", 6)  # Frontal
gmsh.option.setNumber("Mesh.RecombineAll", 1)  # 重组为四边形
gmsh.option.setNumber("Mesh.RecombinationAlgorithm", 1)  # Blossom

gmsh.option.setNumber("Mesh.MeshSizeMax", 0.15)
gmsh.model.mesh.generate(2)

# 统计四边形比例
types, tags, _ = gmsh.model.mesh.getElements(2)
tri_count = sum(len(t) for t, ty in zip(tags, types) if ty == 2)
quad_count = sum(len(t) for t, ty in zip(tags, types) if ty == 3)

print(f"Triangles: {tri_count}")
print(f"Quadrangles: {quad_count}")
print(f"Quad ratio: {quad_count / (tri_count + quad_count) * 100:.1f}%")

gmsh.fltk.run()
gmsh.finalize()
```

#### 练习3：【挑战】Transfinite面网格

```python
import gmsh

gmsh.initialize()
gmsh.model.add("transfinite")

# 创建四边形区域
lc = 0.1
p1 = gmsh.model.geo.addPoint(0, 0, 0, lc)
p2 = gmsh.model.geo.addPoint(1, 0, 0, lc)
p3 = gmsh.model.geo.addPoint(1, 1, 0, lc)
p4 = gmsh.model.geo.addPoint(0, 1, 0, lc)

l1 = gmsh.model.geo.addLine(p1, p2)
l2 = gmsh.model.geo.addLine(p2, p3)
l3 = gmsh.model.geo.addLine(p3, p4)
l4 = gmsh.model.geo.addLine(p4, p1)

cl = gmsh.model.geo.addCurveLoop([l1, l2, l3, l4])
s = gmsh.model.geo.addPlaneSurface([cl])

gmsh.model.geo.synchronize()

# Transfinite设置
N = 10
gmsh.model.mesh.setTransfiniteCurve(l1, N)
gmsh.model.mesh.setTransfiniteCurve(l2, N)
gmsh.model.mesh.setTransfiniteCurve(l3, N)
gmsh.model.mesh.setTransfiniteCurve(l4, N)

# 设置面为Transfinite
gmsh.model.mesh.setTransfiniteSurface(s)

# 四边形网格
gmsh.model.mesh.setRecombine(2, s)

gmsh.model.mesh.generate(2)

# 应该得到完全结构化的四边形网格
types, tags, _ = gmsh.model.mesh.getElements(2)
print(f"Elements: {len(tags[0])}")

gmsh.fltk.run()
gmsh.finalize()
```

---

## 核心概念

### 算法选择指南

```
需要快速网格？
    → Delaunay (algo=5)

需要高质量三角形？
    → Frontal-Delaunay (algo=6, 默认)

需要四边形？
    → Frontal for Quads (algo=8)
    → 或 Frontal + RecombineAll

需要各向异性网格？
    → BAMG (algo=7)

需要结构化网格？
    → Transfinite + Recombine

复杂几何失败？
    → 尝试 MeshAdapt (algo=1)
```

### 网格质量对比

```
                 质量      速度      四边形    备注
Delaunay         中        快        否       稳定
Frontal          高        中        否       默认
MeshAdapt        中        慢        否       自适应
Frontal Quad     高        中        是       直接四边形
Recombine        中        中        是       后处理
Transfinite      高        快        是       需要手动设置
```

---

## 今日检查点

- [ ] 理解meshGFace的整体流程
- [ ] 能解释Delaunay和Frontal算法的基本原理
- [ ] 能使用不同算法生成2D网格
- [ ] 能生成四边形网格

---

## 源码导航

关键代码位置：
- `src/mesh/meshGFace.cpp:meshGFace::operator()` - 面网格主入口
- `src/mesh/meshGFaceDelaunay.cpp` - Delaunay算法
- `src/mesh/meshGFaceFrontal.cpp` - Frontal算法
- `src/mesh/meshGFaceQuadrilateralize.cpp` - 四边形重组
- `src/mesh/meshGFaceTransfinite.cpp` - Transfinite面网格

---

## 导航

- **上一天**：[Day 37 - 1D网格生成](day-37.md)
- **下一天**：[Day 39 - Delaunay三角化基础](day-39.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
