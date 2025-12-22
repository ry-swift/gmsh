# Day 31：OpenCASCADE集成

**所属周次**：第5周 - 几何模块深入
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解OpenCASCADE几何引擎的集成方式

---

## 学习文件

- `src/geo/OCCVertex.h`
- `src/geo/OCCEdge.h`
- `src/geo/OCCFace.h`
- `src/geo/OCCRegion.h`
- `src/geo/OCC_Internals.h`

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 了解OpenCASCADE基础概念 |
| 09:45-10:30 | 45min | 阅读OCC_Internals结构 |
| 10:30-11:15 | 45min | 阅读OCCVertex和OCCEdge |
| 11:15-12:00 | 45min | 理解TopoDS与GEntity的映射 |
| 14:00-14:45 | 45min | 阅读OCCFace和OCCRegion |
| 14:45-15:30 | 45min | 理解OCC参数化适配 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### OpenCASCADE基础

OpenCASCADE (OCC)是一个开源的CAD内核，提供：

```
OCC核心概念：
┌─────────────────────────────────────────────────────────────┐
│ TopoDS - 拓扑数据结构                                       │
│   TopoDS_Vertex  - 顶点                                     │
│   TopoDS_Edge    - 边                                       │
│   TopoDS_Wire    - 线环（边的有序集合）                      │
│   TopoDS_Face    - 面                                       │
│   TopoDS_Shell   - 壳（面的集合）                           │
│   TopoDS_Solid   - 实体                                     │
│   TopoDS_Shape   - 通用形状基类                             │
├─────────────────────────────────────────────────────────────┤
│ Geom - 几何对象                                             │
│   Geom_Point     - 点                                       │
│   Geom_Curve     - 曲线（直线、圆、样条等）                  │
│   Geom_Surface   - 曲面（平面、圆柱面、NURBS等）             │
├─────────────────────────────────────────────────────────────┤
│ BRep - 边界表示                                             │
│   BRep_Tool      - 从TopoDS提取几何信息                     │
│   BRep_Builder   - 构建拓扑结构                             │
└─────────────────────────────────────────────────────────────┘
```

### OCC_Internals - Gmsh的OCC封装

**文件路径**：`src/geo/OCC_Internals.h`

```cpp
class OCC_Internals {
private:
  // OCC形状到Gmsh标签的映射
  std::map<TopoDS_Vertex, int> _vertexTag;
  std::map<TopoDS_Edge, int> _edgeTag;
  std::map<TopoDS_Face, int> _faceTag;
  std::map<TopoDS_Solid, int> _solidTag;

  // 存储所有OCC形状
  TopoDS_Compound _compound;

public:
  // 添加基本形状
  bool addVertex(int &tag, double x, double y, double z);
  bool addLine(int &tag, int startTag, int endTag);
  bool addCircleArc(int &tag, int startTag, int centerTag, int endTag);
  bool addSpline(int &tag, const std::vector<int> &pointTags);

  // 添加曲面
  bool addPlaneSurface(int &tag, const std::vector<int> &wireTags);
  bool addSurfaceFilling(int &tag, int wireTag);

  // 添加实体
  bool addVolume(int &tag, const std::vector<int> &shellTags);
  bool addBox(int &tag, double x, double y, double z,
              double dx, double dy, double dz);
  bool addSphere(int &tag, double xc, double yc, double zc, double r);
  bool addCylinder(int &tag, double x, double y, double z,
                   double dx, double dy, double dz, double r);

  // 布尔运算
  bool booleanUnion(int tag, const std::vector<std::pair<int,int>> &objectDimTags,
                    const std::vector<std::pair<int,int>> &toolDimTags, ...);
  bool booleanIntersection(...);
  bool booleanDifference(...);
  bool booleanFragments(...);

  // 同步到GModel
  void synchronize(GModel *model);
};
```

### OCCVertex - OCC顶点

**文件路径**：`src/geo/OCCVertex.h`

```cpp
class OCCVertex : public GVertex {
protected:
  TopoDS_Vertex _v;  // OCC顶点
  double _x, _y, _z; // 缓存坐标

public:
  OCCVertex(GModel *model, TopoDS_Vertex v, int num);

  // 实现GVertex接口
  virtual double x() const { return _x; }
  virtual double y() const { return _y; }
  virtual double z() const { return _z; }

  // 获取OCC对象
  TopoDS_Vertex getShape() const { return _v; }
};

// 构造函数中提取坐标
OCCVertex::OCCVertex(GModel *model, TopoDS_Vertex v, int num)
  : GVertex(model, num), _v(v) {
  gp_Pnt pnt = BRep_Tool::Pnt(v);
  _x = pnt.X();
  _y = pnt.Y();
  _z = pnt.Z();
}
```

---

## 下午任务（2小时）

### OCCEdge - OCC边

**文件路径**：`src/geo/OCCEdge.h`

```cpp
class OCCEdge : public GEdge {
protected:
  TopoDS_Edge _e;         // OCC边
  Handle(Geom_Curve) _c;  // 几何曲线
  double _s0, _s1;        // 参数范围

public:
  OCCEdge(GModel *model, TopoDS_Edge e, GVertex *v0, GVertex *v1, int num);

  // 参数范围
  virtual Range<double> parBounds(int i) const {
    return Range<double>(_s0, _s1);
  }

  // 参数到点
  virtual GPoint point(double p) const {
    gp_Pnt pnt = _c->Value(p);
    return GPoint(pnt.X(), pnt.Y(), pnt.Z(), this, p);
  }

  // 切向量
  virtual SVector3 firstDer(double par) const {
    gp_Vec d1;
    gp_Pnt pnt;
    _c->D1(par, pnt, d1);
    return SVector3(d1.X(), d1.Y(), d1.Z());
  }

  // 曲率
  virtual double curvature(double par) const;

  // 获取OCC对象
  TopoDS_Edge getShape() const { return _e; }
};

// 构造函数中提取几何信息
OCCEdge::OCCEdge(GModel *model, TopoDS_Edge e, GVertex *v0, GVertex *v1, int num)
  : GEdge(model, num, v0, v1), _e(e) {
  // 从TopoDS_Edge提取Geom_Curve
  _c = BRep_Tool::Curve(e, _s0, _s1);
}
```

### OCCFace - OCC面

**文件路径**：`src/geo/OCCFace.h`

```cpp
class OCCFace : public GFace {
protected:
  TopoDS_Face _f;            // OCC面
  Handle(Geom_Surface) _s;   // 几何曲面
  bool _periodic[2];         // 周期性

public:
  OCCFace(GModel *model, TopoDS_Face f, int num);

  // 参数范围
  virtual Range<double> parBounds(int i) const;

  // 参数到点
  virtual GPoint point(double u, double v) const {
    gp_Pnt pnt = _s->Value(u, v);
    return GPoint(pnt.X(), pnt.Y(), pnt.Z(), this, u, v);
  }

  // 法向量
  virtual SVector3 normal(const SPoint2 &param) const;

  // 一阶偏导数
  virtual Pair<SVector3, SVector3> firstDer(const SPoint2 &param) const {
    gp_Pnt pnt;
    gp_Vec du, dv;
    _s->D1(param.x(), param.y(), pnt, du, dv);
    return Pair<SVector3, SVector3>(
      SVector3(du.X(), du.Y(), du.Z()),
      SVector3(dv.X(), dv.Y(), dv.Z())
    );
  }

  // 获取OCC对象
  TopoDS_Face getShape() const { return _f; }
};
```

### OCCRegion - OCC体

**文件路径**：`src/geo/OCCRegion.h`

```cpp
class OCCRegion : public GRegion {
protected:
  TopoDS_Solid _s;  // OCC实体

public:
  OCCRegion(GModel *model, TopoDS_Solid s, int num);

  // 获取OCC对象
  TopoDS_Solid getShape() const { return _s; }
};
```

### synchronize - 同步到GModel

```cpp
// OCC_Internals.cpp
void OCC_Internals::synchronize(GModel *model) {
  // 1. 遍历所有OCC顶点
  TopExp_Explorer exp0(_compound, TopAbs_VERTEX);
  for(; exp0.More(); exp0.Next()) {
    TopoDS_Vertex v = TopoDS::Vertex(exp0.Current());
    int tag = getTag(v);
    if(!model->getVertexByTag(tag)) {
      model->add(new OCCVertex(model, v, tag));
    }
  }

  // 2. 遍历所有OCC边
  TopExp_Explorer exp1(_compound, TopAbs_EDGE);
  for(; exp1.More(); exp1.Next()) {
    TopoDS_Edge e = TopoDS::Edge(exp1.Current());
    int tag = getTag(e);
    if(!model->getEdgeByTag(tag)) {
      // 找到端点
      TopoDS_Vertex v0 = TopExp::FirstVertex(e);
      TopoDS_Vertex v1 = TopExp::LastVertex(e);
      GVertex *gv0 = model->getVertexByTag(getTag(v0));
      GVertex *gv1 = model->getVertexByTag(getTag(v1));
      model->add(new OCCEdge(model, e, gv0, gv1, tag));
    }
  }

  // 3. 遍历所有OCC面
  // ...

  // 4. 遍历所有OCC体
  // ...
}
```

---

## 练习作业

### 1. 【基础】理解OCC类型映射

查找TopoDS到GEntity的映射：

```bash
# 查找OCCVertex构造
grep -A 10 "OCCVertex::OCCVertex" src/geo/OCCVertex.cpp

# 查找OCCEdge构造
grep -A 15 "OCCEdge::OCCEdge" src/geo/OCCEdge.cpp
```

记录映射关系：
```
TopoDS_Vertex → OCCVertex → GVertex
TopoDS_Edge   → OCCEdge   → GEdge
TopoDS_Face   → OCCFace   → GFace
TopoDS_Solid  → OCCRegion → GRegion
```

### 2. 【进阶】追踪布尔运算流程

分析布尔运算的实现：

```bash
# 查找布尔运算实现
grep -A 30 "booleanUnion\|booleanDifference" src/geo/OCC_Internals.cpp | head -50
```

理解流程：
```
gmsh::model::occ::fuse(...)
    ↓
OCC_Internals::booleanUnion(...)
    ↓
BRepAlgoAPI_Fuse (OCC布尔运算)
    ↓
更新_compound和tag映射
    ↓
synchronize()更新GModel
```

### 3. 【挑战】比较gmsh*和OCC*的参数化

对比同一曲线在两种实现中的参数化：

```cpp
// 圆弧参数化比较
// gmshEdge (内置):
//   参数范围可能是 [0, 2π] 或 [θ0, θ1]

// OCCEdge (OCC):
//   参数范围由 BRep_Tool::Curve 返回
//   可能与gmshEdge不同

// 查找参数化差异
grep -n "parBounds" src/geo/gmshEdge.cpp src/geo/OCCEdge.cpp
```

思考：为什么需要适配层？

---

## 今日检查点

- [ ] 理解OpenCASCADE的基本概念（TopoDS、Geom、BRep）
- [ ] 理解OCC_Internals的作用
- [ ] 理解OCCVertex/OCCEdge/OCCFace的实现
- [ ] 理解synchronize的同步流程

---

## 核心概念

### OCC类型体系

```
OCC类型层次：
┌─────────────────────────────────────────────────────────────┐
│                    TopoDS_Shape                             │
│                         │                                   │
│  ┌──────────┬──────────┼──────────┬──────────┐             │
│  │          │          │          │          │             │
│ Vertex    Edge       Wire       Face      Solid            │
│  │          │          │          │          │             │
│  └──────────┴──────────┴──────────┴──────────┘             │
│                         │                                   │
│              每个拓扑对象关联几何对象                        │
│                         │                                   │
│                    Geom_XXX                                 │
│  Geom_Point  Geom_Curve  Geom_Surface                      │
└─────────────────────────────────────────────────────────────┘
```

### Gmsh对OCC的封装

```
API层                    封装层                  OCC层
─────────────────────────────────────────────────────────
gmsh::model::occ::       OCC_Internals          OpenCASCADE
  addBox()          →      addBox()         →   BRepPrimAPI_MakeBox
  fuse()            →      booleanUnion()   →   BRepAlgoAPI_Fuse
  synchronize()     →      synchronize()    →   TopoDS遍历

同步后创建：
  OCCVertex, OCCEdge, OCCFace, OCCRegion
```

### 参数化适配

```cpp
// OCC参数化到Gmsh参数化的适配
//
// OCC: 使用自己的参数范围和方向
// Gmsh: 期望统一的参数化接口
//
// 适配发生在OCC*类中：

class OCCEdge : public GEdge {
  // 存储OCC的参数范围
  double _s0, _s1;

  // Gmsh接口使用OCC参数
  GPoint point(double p) const {
    return GPoint(_c->Value(p), this, p);
  }
};
```

---

## 源码导航

### 关键文件

```
src/geo/
├── OCC_Internals.h/cpp   # OCC封装（核心）
├── OCCVertex.h/cpp       # OCC顶点
├── OCCEdge.h/cpp         # OCC边
├── OCCFace.h/cpp         # OCC面
├── OCCRegion.h/cpp       # OCC体
├── OCCMeshConstraints.h  # 网格约束
└── GModelIO_OCC.cpp      # OCC文件IO
```

### 搜索命令

```bash
# 查找OCC类
grep -rn "class OCC" src/geo/*.h

# 查找布尔运算
grep -n "Boolean\|Fuse\|Cut\|Common" src/geo/OCC_Internals.cpp

# 查找synchronize
grep -n "synchronize" src/geo/OCC_Internals.cpp
```

---

## 产出

- 理解OCC集成架构
- 掌握TopoDS到GEntity的映射

---

## 导航

- **上一天**：[Day 30 - 内置几何引擎](day-30.md)
- **下一天**：[Day 32 - 几何操作](day-32.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
