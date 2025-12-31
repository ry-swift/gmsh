# 计算几何及应用：理论总结与Gmsh/OpenCASCADE实践

## 目录

1. [计算几何核心内容概览](#1-计算几何核心内容概览)
2. [基础几何运算](#2-基础几何运算)
3. [凸包算法](#3-凸包算法)
4. [Voronoi图与Delaunay三角剖分](#4-voronoi图与delaunay三角剖分)
5. [空间索引结构](#5-空间索引结构)
6. [几何查询与定位](#6-几何查询与定位)
7. [布尔运算与集合操作](#7-布尔运算与集合操作)
8. [Gmsh中的计算几何实现](#8-gmsh中的计算几何实现)
9. [OpenCASCADE中的计算几何实现](#9-opencascade中的计算几何实现)
10. [总结与对照表](#10-总结与对照表)

---

## 1. 计算几何核心内容概览

《计算几何及应用》通常涵盖以下核心主题：

### 1.1 基础主题

| 主题 | 描述 | 复杂度 |
|------|------|--------|
| 点、线、面的基本运算 | 距离、交点、方位判断 | O(1) |
| 凸包 | 点集的最小凸多边形 | O(n log n) |
| 线段相交 | 检测和计算交点 | O((n+k) log n) |
| 多边形三角剖分 | 将多边形分割为三角形 | O(n log n) |
| Voronoi图 | 基于最近邻的空间划分 | O(n log n) |
| Delaunay三角剖分 | Voronoi的对偶结构 | O(n log n) |

### 1.2 高级主题

| 主题 | 描述 | 应用场景 |
|------|------|----------|
| 点定位 | 确定点所在区域 | 网格查询、碰撞检测 |
| 范围搜索 | 查询空间区域内的对象 | 可视化、数据库 |
| 最近邻搜索 | 查找最近的点/对象 | 网格生成、路径规划 |
| 布尔运算 | 并、交、差集合操作 | CAD建模 |
| 曲线曲面求交 | 参数曲线/曲面交点 | CAD/CAM |

---

## 2. 基础几何运算

### 2.1 向量运算

```cpp
// 叉积判断方位（左/右/共线）
double cross(Point O, Point A, Point B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}
// > 0: B在OA左侧
// < 0: B在OA右侧
// = 0: O, A, B共线
```

### 2.2 Gmsh中的向量实现

**文件**: `src/geo/SVector3.h`

```cpp
class SVector3 {
    double _v[3];
public:
    // 点积
    double dot(const SVector3 &other) const;
    // 叉积
    SVector3 cross(const SVector3 &other) const;
    // 归一化
    double normalize();
    // 模长
    double norm() const;
};
```

### 2.3 OpenCASCADE中的向量实现

```cpp
#include <gp_Vec.hxx>
#include <gp_Pnt.hxx>

gp_Vec v1(1.0, 0.0, 0.0);
gp_Vec v2(0.0, 1.0, 0.0);
gp_Vec cross = v1.Crossed(v2);  // 叉积
Standard_Real dot = v1.Dot(v2);  // 点积
```

---

## 3. 凸包算法

### 3.1 理论概述

**定义**: 包含点集的最小凸多边形/多面体

**主要算法**:

| 算法 | 时间复杂度 | 适用场景 |
|------|------------|----------|
| Graham扫描 | O(n log n) | 2D |
| 分治法 | O(n log n) | 2D/3D |
| 快包算法 | O(n log n) 平均 | 2D |
| 增量法 | O(n²) | 3D |

### 3.2 Gmsh中的凸包实现

**文件**: `src/mesh/DivideAndConquer.h`

```cpp
class DocRecord {
private:
    int _hullSize;           // 凸包顶点数
    PointNumero *_hull;      // 凸包顶点索引

    void ConvexHull();       // 构建凸包
    int CountPointsOnHull(); // 计算凸包顶点数

public:
    int hullSize() { return _hullSize; }
    int onHull(PointNumero i) {
        return std::binary_search(_hull, _hull + _hullSize, i);
    }
};
```

**应用场景**:
- Delaunay三角剖分的初始化
- 判断点是否在计算域边界

---

## 4. Voronoi图与Delaunay三角剖分

### 4.1 理论关系

```
Voronoi图 ←──对偶──→ Delaunay三角剖分
    ↓                        ↓
每个生成点对应一个区域    每个三角形外接圆内无其他点
```

### 4.2 Delaunay性质

1. **空圆性质**: 每个三角形的外接圆内不包含其他点
2. **最大化最小角**: 在所有可能的三角剖分中，Delaunay使最小角最大化
3. **局部性**: 插入/删除点只影响局部区域

### 4.3 Gmsh中的Delaunay实现

**文件**: `src/mesh/DivideAndConquer.h`, `src/mesh/delaunay3d.cpp`

```cpp
class DocRecord {
private:
    // 分治算法核心
    DT RecurTrig(PointNumero left, PointNumero right);

    // 合并左右子Delaunay
    int Merge(DT vl, DT vr);

    // 空圆测试（判断点是否在外接圆内）
    int Qtest(PointNumero h, PointNumero i, PointNumero j, PointNumero k);

    // 公共切线（合并时使用）
    Segment LowerCommonTangent(DT vl, DT vr);
    Segment UpperCommonTangent(DT vl, DT vr);

public:
    int numTriangles;      // 三角形数量
    Triangle *triangles;   // 三角形结果

    void MakeMeshWithPoints();  // 生成Delaunay网格
    void Voronoi();             // 生成Voronoi图
    void voronoiCell(PointNumero pt, std::vector<SPoint2> &pts) const;
};
```

**Qtest空圆测试原理**:
```
给定四点 h, i, j, k:
若 k 在三角形 hij 的外接圆内，则需要翻转边
使用行列式判断：
| hx-kx  hy-ky  (hx-kx)²+(hy-ky)² |
| ix-kx  iy-ky  (ix-kx)²+(iy-ky)² | > 0 表示k在圆内
| jx-kx  jy-ky  (jx-kx)²+(jy-ky)² |
```

### 4.4 3D Delaunay实现

**文件**: `src/mesh/delaunay3d.cpp`

```cpp
// 3D Delaunay使用增量插入算法
// 核心步骤：
// 1. 创建超级四面体包含所有点
// 2. 逐个插入点
// 3. 对每个插入点，找到包含它的四面体
// 4. 删除违反空球性质的四面体
// 5. 重新连接形成新的四面体
```

### 4.5 Voronoi应用

**文件**: `src/plugin/VoroMetal.cpp`

```cpp
// VoroMetal插件：使用Voronoi进行多晶材料建模
// 每个Voronoi单元代表一个晶粒
```

---

## 5. 空间索引结构

### 5.1 理论比较

| 数据结构 | 维度 | 查询复杂度 | 特点 |
|----------|------|------------|------|
| kd-tree | 任意 | O(log n) 平均 | 点数据高效 |
| R-tree | 任意 | O(log n) | 矩形包围盒 |
| Octree | 3D | O(log n) | 均匀空间划分 |
| Quadtree | 2D | O(log n) | 2D空间划分 |
| BSP树 | 任意 | O(n) 最差 | 任意划分超平面 |

### 5.2 Gmsh中的R-Tree实现

**文件**: `src/common/rtree.h`, `src/geo/MVertexRTree.h`

```cpp
// R-Tree模板类
template<class DATATYPE, class ELEMTYPE, int NUMDIMS,
         class ELEMTYPEREAL = ELEMTYPE,
         int TMAXNODES = 8, int TMINNODES = TMAXNODES / 2>
class RTree {
public:
    void Insert(const ELEMTYPE a_min[NUMDIMS],
                const ELEMTYPE a_max[NUMDIMS],
                const DATATYPE& a_dataId);

    bool Search(const ELEMTYPE a_min[NUMDIMS],
                const ELEMTYPE a_max[NUMDIMS],
                bool __cdecl a_resultCallback(DATATYPE, void*),
                void* a_context) const;

    void Remove(const ELEMTYPE a_min[NUMDIMS],
                const ELEMTYPE a_max[NUMDIMS],
                const DATATYPE& a_dataId);
};
```

**MVertexRTree封装**:

```cpp
// 用于网格顶点的快速查找和去重
class MVertexRTree {
private:
    RTree<MVertex *, double, 3, double> *_rtree;
    double _tol;  // 容差

public:
    MVertexRTree(double tolerance = 1.e-8);

    // 插入顶点（返回已存在的重复顶点）
    MVertex *insert(MVertex *v, bool warnIfExists = false);

    // 按坐标查找顶点
    MVertex *find(double x, double y, double z);

    std::size_t size();
};
```

**应用场景**:
- 网格顶点去重
- 快速空间查询
- 碰撞检测

### 5.3 Gmsh中的Octree实现

**文件**: `src/common/Octree.h`, `src/common/OctreeInternals.h`

```cpp
// Octree API
Octree *Octree_Create(
    int maxElements,           // 每个八叉树节点最大元素数
    double *origin,            // 包围盒原点
    double *size,              // 包围盒尺寸
    void (*BB)(void *, double *, double *),      // 获取元素包围盒
    void (*Centroid)(void *, double *),          // 获取元素质心
    int (*InEle)(void *, double *)               // 点是否在元素内
);

void Octree_Insert(void *element, Octree *tree);
void Octree_Arrange(Octree *tree);  // 重新平衡
void *Octree_Search(double *point, Octree *tree);
void Octree_SearchAll(double *point, Octree *tree, std::vector<void *> *results);
```

**MElementOctree封装**:

**文件**: `src/geo/MElementOctree.h`

```cpp
// 用于网格元素的空间查询
class MElementOctree {
    Octree *_octree;
public:
    MElementOctree(GModel *m);
    MElementOctree(std::vector<MElement *> &elements);

    MElement *find(double x, double y, double z, int dim = -1);
    std::vector<MElement *> findAll(double x, double y, double z, int dim = -1);
};
```

### 5.4 OpenCASCADE中的空间索引

```cpp
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

// 计算形状的轴对齐包围盒
TopoDS_Shape shape = ...;
Bnd_Box box;
BRepBndLib::Add(shape, box);

Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

// 包围盒操作
box.Enlarge(tolerance);    // 扩展
box.IsOut(point);          // 点是否在外
box.IsOut(otherBox);       // 盒子是否不相交
```

---

## 6. 几何查询与定位

### 6.1 包围盒

#### Gmsh实现

**文件**: `src/geo/SBoundingBox3d.h`

```cpp
class SBoundingBox3d {
private:
    SPoint3 MinPt, MaxPt;

public:
    SBoundingBox3d();

    // 添加点，自动扩展包围盒
    void operator+=(const SPoint3 &pt);
    void operator+=(const SBoundingBox3d &box);

    // 缩放
    void operator*=(double scale);
    void scale(double sx, double sy, double sz);

    // 查询
    SPoint3 min() const;
    SPoint3 max() const;
    SPoint3 center() const;
    double diag() const;  // 对角线长度

    // 包含测试
    bool contains(const SBoundingBox3d &bound);
    bool contains(const SPoint3 &p);
    bool contains(double x, double y, double z);

    // 转换为立方体
    void makeCube();
    void thicken(double factor);  // 扩展
};
```

#### 有向包围盒 (OBB)

**文件**: `src/geo/SOrientedBoundingBox.h`

```cpp
class SOrientedBoundingBox {
private:
    SVector3 center;  // 中心
    SVector3 size;    // 各轴尺寸
    SVector3 axisX, axisY, axisZ;  // 局部坐标轴

public:
    // 从点集构建OBB（使用PCA主成分分析）
    static SOrientedBoundingBox *buildOBB(std::vector<SPoint3> &vertices);

    // 相交测试
    bool intersects(SOrientedBoundingBox &obb) const;

    // 比较两个OBB的相似度
    static double compare(SOrientedBoundingBox &obb1, SOrientedBoundingBox &obb2);

    double getMaxSize() const;
    double getMinSize() const;
    SVector3 getAxis(int axis) const;
};
```

### 6.2 点定位

#### OpenCASCADE点分类

```cpp
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepClass_FaceClassifier.hxx>

// 判断点是否在实体内
TopoDS_Solid solid = ...;
gp_Pnt point(x, y, z);
BRepClass3d_SolidClassifier classifier(solid);
classifier.Perform(point, tolerance);

TopAbs_State state = classifier.State();
// TopAbs_IN:  点在内部
// TopAbs_ON:  点在边界上
// TopAbs_OUT: 点在外部

// 判断点是否在面上
TopoDS_Face face = ...;
BRepClass_FaceClassifier faceClassifier;
faceClassifier.Perform(face, gp_Pnt2d(u, v), tolerance);
```

**Gmsh封装**（文件: `src/geo/OCCRegion.cpp`）:

```cpp
GRegion::containsPoint(double x, double y, double z) {
    BRepClass3d_SolidClassifier solidClassifier(_s);
    solidClassifier.Perform(gp_Pnt(x, y, z), tolerance);
    return solidClassifier.State() == TopAbs_IN;
}
```

---

## 7. 布尔运算与集合操作

### 7.1 理论基础

**布尔运算类型**:
- **并集 (Union)**: A ∪ B
- **交集 (Intersection)**: A ∩ B
- **差集 (Difference)**: A - B
- **对称差 (XOR)**: (A - B) ∪ (B - A)

**实现挑战**:
1. 曲面求交的数值稳定性
2. 拓扑的正确性维护
3. 退化情况处理（相切、共面）

### 7.2 OpenCASCADE布尔运算

```cpp
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>

TopoDS_Shape shape1 = ...;
TopoDS_Shape shape2 = ...;

// 并集
BRepAlgoAPI_Fuse fuse(shape1, shape2);
TopoDS_Shape result = fuse.Shape();

// 差集
BRepAlgoAPI_Cut cut(shape1, shape2);
TopoDS_Shape difference = cut.Shape();

// 交集
BRepAlgoAPI_Common common(shape1, shape2);
TopoDS_Shape intersection = common.Shape();
```

### 7.3 Gmsh API封装

**文件**: `src/geo/GModelIO_OCC.cpp`

```cpp
// Gmsh提供统一的API调用OCC布尔运算
gmsh::model::occ::fuse(objectDimTags, toolDimTags, outDimTags, outDimTagsMap);
gmsh::model::occ::cut(objectDimTags, toolDimTags, outDimTags, outDimTagsMap);
gmsh::model::occ::intersect(objectDimTags, toolDimTags, outDimTags, outDimTagsMap);
gmsh::model::occ::fragment(objectDimTags, toolDimTags, outDimTags, outDimTagsMap);
```

---

## 8. Gmsh中的计算几何实现

### 8.1 核心模块分布

```
src/
├── common/
│   ├── Octree.cpp/h           # 八叉树实现
│   ├── OctreeInternals.cpp/h  # 八叉树内部实现
│   └── rtree.h                # R-tree模板
├── geo/
│   ├── SBoundingBox3d.h       # 轴对齐包围盒
│   ├── SOrientedBoundingBox.* # 有向包围盒
│   ├── MVertexRTree.h         # 顶点R-tree
│   ├── MElementOctree.*       # 元素八叉树
│   ├── SPoint3.h              # 3D点
│   ├── SVector3.h             # 3D向量
│   └── OCC*.cpp/h             # OpenCASCADE封装
├── mesh/
│   ├── DivideAndConquer.*     # 分治Delaunay
│   ├── delaunay3d.cpp         # 3D Delaunay
│   ├── meshTriangulation.*    # 三角剖分
│   └── meshGFaceDelaunayInsertion.cpp  # 面网格Delaunay插入
└── plugin/
    ├── VoroMetal.*            # Voronoi应用
    └── CVTRemesh.cpp          # 质心Voronoi镶嵌重网格
```

### 8.2 网格生成中的计算几何

**Delaunay网格生成流程**:

```
1. 边界离散化 → 获取边界点
      ↓
2. 构建初始Delaunay三角剖分
      ↓
3. 边界恢复（Boundary Recovery）
      ↓
4. 尺寸场驱动的点插入
      ↓
5. 质量优化（边翻转、点平滑）
```

**关键文件**:

| 文件 | 功能 |
|------|------|
| `meshGFace.cpp` | 面网格生成主流程 |
| `meshGFaceDelaunayInsertion.cpp` | Delaunay点插入 |
| `meshGRegion.cpp` | 体网格生成 |
| `meshGRegionDelaunayInsertion.cpp` | 3D Delaunay插入 |
| `BackgroundMesh.cpp` | 背景网格尺寸控制 |

### 8.3 使用示例

```python
import gmsh
gmsh.initialize()

# 创建几何
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

# 设置网格尺寸
gmsh.option.setNumber("Mesh.CharacteristicLengthMax", 0.1)

# 使用Delaunay算法生成网格
gmsh.option.setNumber("Mesh.Algorithm", 5)  # Delaunay
gmsh.option.setNumber("Mesh.Algorithm3D", 1)  # Delaunay 3D

gmsh.model.mesh.generate(3)
gmsh.finalize()
```

---

## 9. OpenCASCADE中的计算几何实现

### 9.1 核心模块

```
OpenCASCADE Architecture:
├── FoundationClasses/
│   ├── gp_*          # 基本几何原语（点、向量、变换）
│   └── Bnd_*         # 包围盒
├── ModelingData/
│   ├── Geom_*        # 几何曲线曲面
│   └── TopoDS_*      # 拓扑结构
├── ModelingAlgorithms/
│   ├── BRepAlgoAPI_* # 布尔运算
│   ├── BRepBndLib    # 包围盒计算
│   ├── BRepClass*    # 点分类
│   └── GeomAPI_*     # 几何算法
└── Visualization/
    └── AIS_*         # 交互显示
```

### 9.2 常用计算几何类

| 类名 | 功能 |
|------|------|
| `Bnd_Box` | 轴对齐包围盒 |
| `BRepBndLib` | 从Shape计算包围盒 |
| `BRepClass3d_SolidClassifier` | 点与实体位置关系 |
| `BRepClass_FaceClassifier` | 点与面位置关系 |
| `GeomAPI_ProjectPointOnCurve` | 点到曲线投影 |
| `GeomAPI_ProjectPointOnSurf` | 点到曲面投影 |
| `GeomAPI_IntCS` | 曲线曲面求交 |
| `GeomAPI_IntSS` | 曲面曲面求交 |
| `BRepExtrema_DistShapeShape` | 两形状最短距离 |

### 9.3 Gmsh对OCC的封装

**文件**: `src/geo/GModelIO_OCC.cpp`

主要封装：

```cpp
// OCC形状到Gmsh几何实体的转换
class OCC_Internals {
    // 形状管理
    std::map<int, TopoDS_Vertex> _tagVertex;
    std::map<int, TopoDS_Edge> _tagEdge;
    std::map<int, TopoDS_Face> _tagFace;
    std::map<int, TopoDS_Solid> _tagSolid;

    // 创建基本形状
    bool addBox(int &tag, double x, double y, double z,
                double dx, double dy, double dz);
    bool addSphere(int &tag, double xc, double yc, double zc,
                   double radius, double angle1, double angle2, double angle3);
    bool addCylinder(int &tag, double x, double y, double z,
                     double dx, double dy, double dz, double r, double angle);

    // 布尔运算
    bool booleanOperator(int tag, BooleanOperator op,
                         const std::vector<std::pair<int, int>> &objectDimTags,
                         const std::vector<std::pair<int, int>> &toolDimTags,
                         std::vector<std::pair<int, int>> &outDimTags,
                         ...);

    // 同步到GModel
    void synchronize(GModel *model);
};
```

---

## 10. 总结与对照表

### 10.1 计算几何概念与实现对照

| 计算几何概念 | Gmsh实现 | OpenCASCADE实现 |
|-------------|----------|-----------------|
| **点/向量** | `SPoint3`, `SVector3` | `gp_Pnt`, `gp_Vec` |
| **包围盒(AABB)** | `SBoundingBox3d` | `Bnd_Box` |
| **有向包围盒(OBB)** | `SOrientedBoundingBox` | 需自行实现 |
| **R-Tree** | `RTree<>`, `MVertexRTree` | 无内置 |
| **八叉树** | `Octree`, `MElementOctree` | 无内置 |
| **Delaunay三角剖分** | `DocRecord`, `delaunay3d` | 无内置 |
| **Voronoi图** | `DocRecord::Voronoi()` | 无内置 |
| **凸包** | `DocRecord::ConvexHull()` | `BRepBuilderAPI_MakePolygon` |
| **点定位** | `Octree_Search` | `BRepClass3d_SolidClassifier` |
| **布尔运算** | 委托OCC | `BRepAlgoAPI_*` |
| **曲线求交** | 委托OCC | `GeomAPI_IntCC` |
| **曲面求交** | 委托OCC | `GeomAPI_IntSS` |
| **最短距离** | 委托OCC | `BRepExtrema_DistShapeShape` |

### 10.2 设计哲学对比

| 特性 | Gmsh | OpenCASCADE |
|------|------|-------------|
| **定位** | 网格生成工具 | CAD内核 |
| **几何表示** | 离散+参数化 | NURBS/B-Rep |
| **主要关注** | 网格质量与效率 | 几何精度与完整性 |
| **空间索引** | 内置多种实现 | 需外部实现 |
| **三角剖分** | 核心功能 | 表面离散化辅助 |
| **布尔运算** | 依赖OCC | 核心功能 |

### 10.3 学习路线建议

```
Week 1-2: 基础几何运算
├── 向量运算: SVector3, gp_Vec
├── 包围盒: SBoundingBox3d, Bnd_Box
└── 点线面关系判断

Week 3-4: 空间索引
├── R-Tree: rtree.h, MVertexRTree
├── Octree: Octree.h
└── 应用: 快速查询、碰撞检测

Week 5-6: 三角剖分
├── Delaunay: DivideAndConquer
├── 约束Delaunay
└── 网格质量优化

Week 7-8: CAD几何
├── 布尔运算: BRepAlgoAPI_*
├── 曲面求交: GeomAPI_IntSS
└── 点分类: BRepClass3d_*
```

---

## 附录：关键源码位置

### Gmsh

| 功能 | 文件路径 |
|------|----------|
| 向量/点 | `src/geo/SPoint3.h`, `src/geo/SVector3.h` |
| 包围盒 | `src/geo/SBoundingBox3d.h` |
| OBB | `src/geo/SOrientedBoundingBox.cpp` |
| R-Tree | `src/common/rtree.h` |
| Octree | `src/common/Octree.cpp` |
| Delaunay 2D | `src/mesh/DivideAndConquer.cpp` |
| Delaunay 3D | `src/mesh/delaunay3d.cpp` |
| Voronoi | `src/mesh/DivideAndConquer.cpp` |
| 面网格 | `src/mesh/meshGFace.cpp` |
| 体网格 | `src/mesh/meshGRegion.cpp` |
| OCC封装 | `src/geo/GModelIO_OCC.cpp` |

### OpenCASCADE (在Gmsh中的使用)

| 功能 | 头文件 |
|------|--------|
| 包围盒 | `Bnd_Box.hxx`, `BRepBndLib.hxx` |
| 点分类 | `BRepClass3d_SolidClassifier.hxx` |
| 布尔运算 | `BRepAlgoAPI_Fuse.hxx`, `BRepAlgoAPI_Cut.hxx` |
| 曲线曲面 | `Geom_Curve.hxx`, `Geom_Surface.hxx` |
| 拓扑 | `TopoDS_Shape.hxx` |

---

*文档创建日期: 2025-12-26*
*作者: Claude Code Study Notes*
