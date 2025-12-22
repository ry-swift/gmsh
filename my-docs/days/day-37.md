# Day 37：1D网格生成

**所属周次**：第6周 - 网格模块基础
**所属阶段**：第三阶段 - 深入核心算法

---

## 学习目标

深入理解Gmsh的1D网格生成算法和边离散化过程

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读meshGEdge.cpp，理解边离散化流程 |
| 09:45-10:30 | 45min | 研究尺寸场在1D网格中的应用 |
| 10:30-11:15 | 45min | 分析周期边和复合边的处理 |
| 11:15-12:00 | 45min | 研究Transfinite线网格 |
| 14:00-15:00 | 60min | 完成练习作业 |
| 15:00-16:00 | 60min | 实践不同1D网格参数的效果 |

---

## 上午任务（2小时）

### 1. 1D网格生成概述

```
边离散化流程：
┌─────────────────────────────────────────────────────────────────┐
│  GEdge (几何边)                                                  │
│    - 起点 GVertex                                                │
│    - 终点 GVertex                                                │
│    - 参数范围 [t0, t1]                                           │
│    - 曲线方程 point(t)                                           │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  meshGEdge                                                       │
│    1. 计算边上的尺寸场                                           │
│    2. 根据尺寸确定节点数量                                       │
│    3. 分布节点（等参数/等弧长/渐变）                             │
│    4. 创建MLine元素连接相邻节点                                  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  结果: mesh_vertices + lines                                     │
│    V0 ─────────── V1 ─────────── V2 ─────────── V3              │
│        MLine[0]       MLine[1]       MLine[2]                   │
└─────────────────────────────────────────────────────────────────┘
```

### 2. meshGEdge核心代码

```cpp
// meshGEdge.cpp
void meshGEdge::operator()(GEdge *ge)
{
  // 跳过已有网格的边
  if(ge->meshStatistics.status == GEdge::DONE) return;

  // 获取边的参数范围
  Range<double> bounds = ge->parBounds(0);
  double t0 = bounds.low();
  double t1 = bounds.high();

  // 获取边长度
  double length = ge->length(t0, t1);

  // 计算需要的网格尺寸
  double lc = computeEdgeLc(ge);

  // 计算节点数量
  int N = std::max(1, (int)(length / lc + 0.5));

  // 根据网格类型分布节点
  if(ge->meshAttributes.method == MESH_TRANSFINITE) {
    meshTransfiniteEdge(ge, N);
  }
  else {
    meshEdge(ge, N);  // 标准边网格
  }

  ge->meshStatistics.status = GEdge::DONE;
}
```

### 3. 节点分布策略

```cpp
// 标准边网格
void meshEdge(GEdge *ge, int N)
{
  Range<double> bounds = ge->parBounds(0);
  double t0 = bounds.low();
  double t1 = bounds.high();

  // 方法1：等参数分布
  for(int i = 1; i < N; i++) {
    double t = t0 + (t1 - t0) * i / N;
    GPoint gp = ge->point(t);
    MVertex *v = new MEdgeVertex(gp.x(), gp.y(), gp.z(), ge, t);
    ge->mesh_vertices.push_back(v);
  }

  // 方法2：等弧长分布（更常用）
  // 使用数值积分计算弧长
  // 根据尺寸场调整分布

  // 创建MLine元素
  createLines(ge);
}

// Transfinite边网格
void meshTransfiniteEdge(GEdge *ge, int N)
{
  // 使用指定的节点数和分布类型
  // coef > 0: 渐变比例
  // type: 0=Progression, 1=Bump
  double coef = ge->meshAttributes.coeffTransfinite;
  int type = ge->meshAttributes.typeTransfinite;

  std::vector<double> params;
  computeTransfiniteParams(t0, t1, N, coef, type, params);

  for(int i = 1; i < N; i++) {
    GPoint gp = ge->point(params[i]);
    MVertex *v = new MEdgeVertex(gp.x(), gp.y(), gp.z(), ge, params[i]);
    ge->mesh_vertices.push_back(v);
  }
}
```

### 4. 尺寸场计算

```cpp
// 计算边上某点的网格尺寸
double computeLc(GEdge *ge, double t)
{
  // 来源1：几何点上的尺寸
  double lc1 = ge->getBeginVertex()->prescribedMeshSizeAtVertex();
  double lc2 = ge->getEndVertex()->prescribedMeshSizeAtVertex();

  // 来源2：尺寸场
  GPoint gp = ge->point(t);
  double lc_field = BGM_MeshSize(ge->model(), 1, ge->tag(),
                                  gp.x(), gp.y(), gp.z());

  // 来源3：全局设置
  double lc_min = CTX::instance()->mesh.lcMin;
  double lc_max = CTX::instance()->mesh.lcMax;
  double lc_factor = CTX::instance()->mesh.lcFactor;

  // 综合计算
  double lc = std::min({lc1, lc2, lc_field});
  lc = std::max(lc_min, std::min(lc_max, lc * lc_factor));

  return lc;
}
```

### 5. 特殊边处理

```cpp
// 周期边处理
void meshPeriodicEdge(GEdge *ge)
{
  // 获取周期主边
  GEdge *master = dynamic_cast<GEdge*>(ge->getMeshMaster());

  // 复制主边的网格结构
  // 应用周期变换
  for(auto v : master->mesh_vertices) {
    // 变换坐标
    SPoint3 p = v->point();
    SPoint3 transformed = ge->getMeshMasterTransform() * p;

    MVertex *newV = new MEdgeVertex(...);
    ge->mesh_vertices.push_back(newV);
  }
}

// 复合边处理
void meshCompoundEdge(std::vector<GEdge*> &compound)
{
  // 将多条边视为一条边进行网格划分
  // 保证节点在边界处连续
}
```

---

## 下午任务（2小时）

### 练习作业

#### 练习1：【基础】控制边网格密度

```python
import gmsh

gmsh.initialize()
gmsh.model.add("edge_mesh")

# 创建一个正方形
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

# 设置不同边的网格数量
gmsh.model.mesh.setTransfiniteCurve(l1, 20)  # 下边20个节点
gmsh.model.mesh.setTransfiniteCurve(l2, 10)  # 右边10个节点
gmsh.model.mesh.setTransfiniteCurve(l3, 20)  # 上边20个节点
gmsh.model.mesh.setTransfiniteCurve(l4, 10)  # 左边10个节点

# 生成1D网格
gmsh.model.mesh.generate(1)

# 查看每条边的节点
for line in [l1, l2, l3, l4]:
    nodes = gmsh.model.mesh.getNodes(1, line)
    print(f"Line {line}: {len(nodes[0])} nodes")

gmsh.fltk.run()
gmsh.finalize()
```

#### 练习2：【进阶】渐变网格

```python
import gmsh

gmsh.initialize()
gmsh.model.add("gradient")

# 创建一条长线段
p1 = gmsh.model.geo.addPoint(0, 0, 0)
p2 = gmsh.model.geo.addPoint(10, 0, 0)
l1 = gmsh.model.geo.addLine(p1, p2)

gmsh.model.geo.synchronize()

# 渐变网格：从密到疏
# 参数: tag, numNodes, meshType, coef
# meshType: "Progression" 或 "Bump"
# coef: 比例系数

# Progression: 相邻单元比例为coef
gmsh.model.mesh.setTransfiniteCurve(l1, 20, "Progression", 1.2)

gmsh.model.mesh.generate(1)

# 获取节点坐标
nodes = gmsh.model.mesh.getNodes(1, l1)
coords = nodes[1].reshape(-1, 3)
print("Node X coordinates:")
for i, c in enumerate(sorted(coords[:, 0])):
    if i > 0:
        prev = sorted(coords[:, 0])[i-1]
        print(f"  x={c:.4f}, gap={c-prev:.4f}")
    else:
        print(f"  x={c:.4f}")

gmsh.fltk.run()
gmsh.finalize()
```

#### 练习3：【挑战】自适应1D网格

```python
import gmsh
import math

gmsh.initialize()
gmsh.model.add("adaptive_1d")

# 创建一条正弦曲线
gmsh.model.occ.addSpline([
    gmsh.model.occ.addPoint(x, math.sin(x), 0)
    for x in [i * 0.5 for i in range(13)]  # 0到6
])
gmsh.model.occ.synchronize()

# 方法1：使用尺寸场
# 曲率越大的地方网格越细
gmsh.model.mesh.field.add("Curvature", 1)
gmsh.model.mesh.field.setNumber(1, "EdgeTag", 1)
gmsh.model.mesh.field.setNumber(1, "NPoints", 100)

# 将曲率转换为尺寸
gmsh.model.mesh.field.add("MathEval", 2)
gmsh.model.mesh.field.setString(2, "F", "0.1 + 0.5 * (1 / (1 + F1^2))")

gmsh.model.mesh.field.setAsBackgroundMesh(2)

# 生成网格
gmsh.option.setNumber("Mesh.MeshSizeFromCurvature", 20)
gmsh.option.setNumber("Mesh.MeshSizeMin", 0.05)
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.5)

gmsh.model.mesh.generate(1)

# 统计
nodes = gmsh.model.mesh.getNodes(1, 1)
print(f"Generated {len(nodes[0])} nodes on the curve")

gmsh.fltk.run()
gmsh.finalize()
```

---

## 核心概念

### 1D网格参数

| 参数 | 说明 | 设置方法 |
|------|------|----------|
| 节点数 | 边上的节点数量 | setTransfiniteCurve(tag, N) |
| 分布类型 | Progression/Bump | setTransfiniteCurve(tag, N, type, coef) |
| 尺寸场 | 位置相关的尺寸 | field.setAsBackgroundMesh |
| 点尺寸 | 端点处的尺寸 | addPoint(x,y,z,lc) |

### 分布类型说明

```
Progression (比例渐变):
  单元1  单元2   单元3    单元4
  |─────|───────|─────────|────────────|
  长度1  长度2=长度1*r  长度3=长度2*r  ...

  r > 1: 从左到右变稀疏
  r < 1: 从左到右变密集
  r = 1: 均匀分布

Bump (中间密集/稀疏):
  |────|───|──|──|───|────|
  两端大，中间小（或相反）
```

---

## 今日检查点

- [ ] 理解边离散化的基本算法
- [ ] 能使用setTransfiniteCurve控制边网格
- [ ] 理解渐变网格的参数含义
- [ ] 能根据曲率自适应调整1D网格

---

## 源码导航

关键代码位置：
- `src/mesh/meshGEdge.cpp:meshGEdge::operator()` - 边网格主函数
- `src/mesh/meshGEdge.cpp:meshEdge()` - 标准边网格
- `src/mesh/meshGEdge.cpp:meshTransfiniteEdge()` - Transfinite边网格
- `src/geo/MEdge.h` - MEdge数据结构
- `src/geo/MLine.h` - MLine元素

---

## 导航

- **上一天**：[Day 36 - 网格生成器入口](day-36.md)
- **下一天**：[Day 38 - 2D网格算法概述](day-38.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
