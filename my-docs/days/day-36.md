# Day 36：网格生成器入口

**所属周次**：第6周 - 网格模块基础
**所属阶段**：第三阶段 - 深入核心算法

---

## 学习目标

理解Gmsh网格生成的入口机制和整体流程

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读Generator.h/cpp，理解网格生成入口 |
| 09:45-10:30 | 45min | 分析mesh()函数的调用流程 |
| 10:30-11:15 | 45min | 研究网格生成的参数控制机制 |
| 11:15-12:00 | 45min | 梳理各维度网格生成的关系 |
| 14:00-15:00 | 60min | 完成练习作业 |
| 15:00-16:00 | 60min | 调试跟踪网格生成流程 |

---

## 上午任务（2小时）

### 1. Generator模块概述

```
网格生成入口架构：
┌─────────────────────────────────────────────────────────────────┐
│                      API调用入口                                 │
│   gmsh::model::mesh::generate(dim)                              │
│   GModel::mesh(dim)                                             │
└─────────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Generator                                   │
│   GenerateMesh()  - 主入口函数                                   │
│   Mesh0D()        - 0D网格（顶点）                               │
│   Mesh1D()        - 1D网格（边）                                 │
│   Mesh2D()        - 2D网格（面）                                 │
│   Mesh3D()        - 3D网格（体）                                 │
└─────────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                   各维度网格生成器                               │
│   meshGVertex  →  meshGEdge  →  meshGFace  →  meshGRegion      │
└─────────────────────────────────────────────────────────────────┘
```

### 2. 关键源文件

| 文件 | 位置 | 功能 |
|------|------|------|
| Generator.h/cpp | src/mesh/ | 网格生成主入口 |
| meshGVertex.cpp | src/mesh/ | 顶点网格 |
| meshGEdge.cpp | src/mesh/ | 边网格 |
| meshGFace.cpp | src/mesh/ | 面网格 |
| meshGRegion.cpp | src/mesh/ | 体网格 |

### 3. GenerateMesh函数分析

```cpp
// Generator.cpp 核心流程
void GenerateMesh(GModel *m, int ask)
{
  // 阶段1：初始化
  SetOrder1(m);  // 设置一阶元素

  // 阶段2：按维度生成网格
  if(ask == 0) {
    Mesh0D(m);  // 生成顶点网格
  }
  if(ask == 1) {
    Mesh0D(m);
    Mesh1D(m);  // 生成边网格
  }
  if(ask == 2) {
    Mesh0D(m);
    Mesh1D(m);
    Mesh2D(m);  // 生成面网格
  }
  if(ask == 3) {
    Mesh0D(m);
    Mesh1D(m);
    Mesh2D(m);
    Mesh3D(m);  // 生成体网格
  }

  // 阶段3：后处理
  if(CTX::instance()->mesh.order > 1) {
    SetOrderN(m, CTX::instance()->mesh.order);  // 高阶元素
  }

  // 阶段4：优化
  OptimizeMesh(m);
}
```

### 4. 各维度网格生成入口

```cpp
// 0D网格 - meshGVertex.cpp
void meshGVertex(GVertex *gv)
{
  // 简单地在几何顶点位置创建一个网格顶点
  MVertex *v = new MVertex(gv->x(), gv->y(), gv->z(), gv);
  gv->mesh_vertices.push_back(v);
}

// 1D网格 - meshGEdge.cpp
void meshGEdge::operator()(GEdge *ge)
{
  // 根据尺寸场在边上分布节点
  // 处理周期边界
  // 调用具体的1D网格算法
}

// 2D网格 - meshGFace.cpp
void meshGFace::operator()(GFace *gf)
{
  // 恢复边界网格
  // 选择2D网格算法（Delaunay、Frontal等）
  // 生成面网格
}

// 3D网格 - meshGRegion.cpp
void meshGRegion::operator()(GRegion *gr)
{
  // 恢复边界网格
  // 选择3D网格算法（Delaunay、HXT等）
  // 生成体网格
}
```

### 5. 网格生成选项

```cpp
// Context.h 中的网格选项
struct contextMeshOptions {
  int algo2d;        // 2D算法：1=MeshAdapt, 5=Delaunay, 6=Frontal
  int algo3d;        // 3D算法：1=Delaunay, 4=Frontal, 7=MMG3D, 10=HXT
  int order;         // 网格阶数：1=线性, 2=二次, ...
  int optimize;      // 优化选项
  int smoothRatio;   // 平滑比例
  int recombine;     // 四边形重组
  double lcMin;      // 最小网格尺寸
  double lcMax;      // 最大网格尺寸
  double lcFactor;   // 尺寸因子
  // ...
};
```

---

## 下午任务（2小时）

### 练习作业

#### 练习1：【基础】追踪网格生成流程

使用调试器或日志追踪一个简单几何的网格生成流程：

```python
import gmsh

gmsh.initialize()
gmsh.model.add("trace")

# 启用详细输出
gmsh.option.setNumber("General.Verbosity", 99)

# 创建简单几何
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

# 分步生成网格
print("=== Generating 0D mesh ===")
# gmsh.model.mesh.generate(0)  # 0D网格

print("=== Generating 1D mesh ===")
gmsh.model.mesh.generate(1)

print("=== Generating 2D mesh ===")
gmsh.model.mesh.generate(2)

print("=== Generating 3D mesh ===")
gmsh.model.mesh.generate(3)

# 统计网格信息
nodes = gmsh.model.mesh.getNodes()
print(f"Total nodes: {len(nodes[0])}")

for dim in [1, 2, 3]:
    types, tags, nodesList = gmsh.model.mesh.getElements(dim)
    count = sum(len(t) for t in tags)
    print(f"Dim {dim} elements: {count}")

gmsh.finalize()
```

#### 练习2：【进阶】对比不同算法

```python
import gmsh

def mesh_with_algorithm(algo2d, algo3d, name):
    """使用指定算法生成网格"""
    gmsh.initialize()
    gmsh.model.add(name)

    # 创建测试几何
    gmsh.model.occ.addSphere(0, 0, 0, 1)
    gmsh.model.occ.synchronize()

    # 设置算法
    gmsh.option.setNumber("Mesh.Algorithm", algo2d)
    gmsh.option.setNumber("Mesh.Algorithm3D", algo3d)
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.3)

    # 生成网格
    import time
    start = time.time()
    gmsh.model.mesh.generate(3)
    elapsed = time.time() - start

    # 统计
    nodes = gmsh.model.mesh.getNodes()
    _, tets, _ = gmsh.model.mesh.getElements(3)

    print(f"{name}: {len(nodes[0])} nodes, {len(tets[0])} tets, {elapsed:.3f}s")

    gmsh.write(f"{name}.msh")
    gmsh.finalize()

# 对比不同算法
algorithms = [
    (6, 1, "frontal2d_delaunay3d"),
    (5, 1, "delaunay2d_delaunay3d"),
    (6, 4, "frontal2d_frontal3d"),
    (6, 10, "frontal2d_hxt3d"),
]

for algo2d, algo3d, name in algorithms:
    mesh_with_algorithm(algo2d, algo3d, name)
```

#### 练习3：【挑战】自定义网格生成回调

```python
import gmsh

gmsh.initialize()
gmsh.model.add("callback")

# 创建几何
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

# 定义尺寸回调函数
def mesh_size_callback(dim, tag, x, y, z, lc):
    """根据位置返回网格尺寸"""
    import math
    # 距离中心越近，网格越细
    dist = math.sqrt((x-0.5)**2 + (y-0.5)**2 + (z-0.5)**2)
    return 0.05 + 0.2 * dist

# 设置回调
gmsh.model.mesh.setSizeCallback(mesh_size_callback)

# 生成网格
gmsh.model.mesh.generate(3)

# 统计
nodes = gmsh.model.mesh.getNodes()
print(f"Nodes: {len(nodes[0])}")

gmsh.fltk.run()
gmsh.finalize()
```

---

## 核心概念

### 网格生成流程图

```
┌──────────────────────────────────────────────────────────────────┐
│                     用户调用 generate(3)                          │
└──────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────────┐
│  GenerateMesh(model, 3)                                          │
│    ├── 初始化网格数据结构                                         │
│    ├── 计算背景网格尺寸场                                         │
│    └── 按维度递增生成网格                                         │
└──────────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        ▼                     ▼                     ▼
┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│   Mesh0D     │    │   Mesh1D     │    │   Mesh2D     │
│  遍历GVertex │    │  遍历GEdge   │    │  遍历GFace   │
│  创建MVertex │    │  离散化边   │    │  三角化面    │
└──────────────┘    └──────────────┘    └──────────────┘
                              │
                              ▼
                    ┌──────────────┐
                    │   Mesh3D     │
                    │  遍历GRegion │
                    │  四面体化体  │
                    └──────────────┘
                              │
                              ▼
                    ┌──────────────┐
                    │   后处理     │
                    │  高阶元素    │
                    │  网格优化    │
                    └──────────────┘
```

### 算法选择指南

| 维度 | 算法ID | 名称 | 特点 |
|------|--------|------|------|
| 2D | 1 | MeshAdapt | 自适应 |
| 2D | 5 | Delaunay | 快速、稳定 |
| 2D | 6 | Frontal-Delaunay | 高质量 |
| 2D | 8 | Delaunay For Quads | 四边形 |
| 3D | 1 | Delaunay | 标准 |
| 3D | 4 | Frontal | 高质量 |
| 3D | 7 | MMG3D | 自适应 |
| 3D | 10 | HXT | 高性能并行 |

---

## 今日检查点

- [ ] 理解GenerateMesh的整体流程
- [ ] 能解释各维度网格生成的顺序和依赖
- [ ] 了解常用网格算法选项
- [ ] 能追踪代码中的网格生成路径

---

## 源码导航

关键代码位置：
- `src/mesh/Generator.cpp:GenerateMesh()` - 主入口函数
- `src/mesh/Generator.cpp:Mesh0D/1D/2D/3D()` - 各维度入口
- `src/mesh/meshGVertex.cpp` - 顶点网格
- `src/mesh/meshGEdge.cpp` - 边网格
- `src/mesh/meshGFace.cpp` - 面网格
- `src/mesh/meshGRegion.cpp` - 体网格

---

## 扩展阅读

- 阅读 `src/mesh/Generator.h` 中的函数声明
- 查看 `CTX::instance()->mesh` 中的所有网格选项
- 研究不同算法的实现文件

---

## 导航

- **上一天**：[Day 35 - 第五周复习](day-35.md)
- **下一天**：[Day 37 - 1D网格生成](day-37.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
