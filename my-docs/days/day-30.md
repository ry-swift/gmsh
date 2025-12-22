# Day 30：内置几何引擎

**所属周次**：第5周 - 几何模块深入
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解Gmsh内置几何引擎（gmsh*系列类）的实现

---

## 学习文件

- `src/geo/gmshVertex.h`
- `src/geo/gmshEdge.h`
- `src/geo/gmshFace.h`
- `src/geo/gmshRegion.h`

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读gmshVertex实现 |
| 09:45-10:30 | 45min | 阅读gmshEdge实现 |
| 10:30-11:15 | 45min | 理解曲线类型（直线、圆弧、样条） |
| 11:15-12:00 | 45min | 阅读gmshFace实现 |
| 14:00-14:45 | 45min | 理解曲面类型 |
| 14:45-15:30 | 45min | 阅读gmshRegion实现 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### gmshVertex - 内置顶点

**文件路径**：`src/geo/gmshVertex.h`

```cpp
// 内置几何顶点
class gmshVertex : public GVertex {
protected:
  // 存储顶点定义（来自GEO脚本解析）
  Vertex *_v;

public:
  gmshVertex(GModel *model, Vertex *v);

  // 实现GVertex的纯虚函数
  virtual double x() const { return _v->Pos.X; }
  virtual double y() const { return _v->Pos.Y; }
  virtual double z() const { return _v->Pos.Z; }

  // 网格尺寸
  virtual double prescribedMeshSizeAtVertex() const {
    return _v->lc;
  }

  // 获取底层Vertex
  Vertex *getVertex() const { return _v; }
};

// Vertex结构（来自GEO解析器）
// 定义在 src/geo/Geo.h
struct Vertex {
  int Num;           // 标签
  int Typ;           // 类型
  Coord Pos;         // 坐标 (X, Y, Z)
  double lc;         // 网格尺寸
  double u, v, w;    // 参数坐标
};
```

### gmshEdge - 内置边

**文件路径**：`src/geo/gmshEdge.h`

```cpp
// 内置几何边（曲线）
class gmshEdge : public GEdge {
protected:
  // 存储曲线定义
  Curve *_c;

public:
  gmshEdge(GModel *model, Curve *c, GVertex *v1, GVertex *v2);

  // 参数范围
  virtual Range<double> parBounds(int i) const {
    return Range<double>(_c->ubeg, _c->uend);
  }

  // 参数到点的映射
  virtual GPoint point(double p) const;

  // 切向量
  virtual SVector3 firstDer(double par) const;

  // 曲率
  virtual double curvature(double par) const;

  // 获取底层Curve
  Curve *getCurve() const { return _c; }
};

// Curve结构
struct Curve {
  int Num;           // 标签
  int Typ;           // 类型
  double ubeg, uend; // 参数范围
  List_T *Control_Points;  // 控制点
  // 不同曲线类型的参数...
};
```

### 曲线类型

```cpp
// src/geo/Geo.h 中定义的曲线类型
#define MSH_SEGM_LINE       1   // 直线
#define MSH_SEGM_SPLINE     2   // 样条曲线
#define MSH_SEGM_CIRC       3   // 圆弧
#define MSH_SEGM_ELLIPSE    4   // 椭圆弧
#define MSH_SEGM_BSPLINE    5   // B样条
#define MSH_SEGM_BEZIER     6   // 贝塞尔曲线
#define MSH_SEGM_NURBS      7   // NURBS曲线
#define MSH_SEGM_COMPOUND   8   // 复合曲线

// 直线的point()实现
GPoint gmshEdge::point(double p) const {
  if(_c->Typ == MSH_SEGM_LINE) {
    // 线性插值
    Vertex *v1 = _c->Control_Points[0];
    Vertex *v2 = _c->Control_Points[1];
    double t = (p - _c->ubeg) / (_c->uend - _c->ubeg);
    return GPoint(
      v1->Pos.X + t * (v2->Pos.X - v1->Pos.X),
      v1->Pos.Y + t * (v2->Pos.Y - v1->Pos.Y),
      v1->Pos.Z + t * (v2->Pos.Z - v1->Pos.Z),
      this, p
    );
  }
  // 其他曲线类型...
}

// 圆弧的point()实现
GPoint gmshEdge::point(double p) const {
  if(_c->Typ == MSH_SEGM_CIRC) {
    // 圆弧参数化
    double theta = p;  // 角度参数
    double r = _c->radius;
    Vertex *center = _c->Circle.center;
    return GPoint(
      center->Pos.X + r * cos(theta),
      center->Pos.Y + r * sin(theta),
      center->Pos.Z,
      this, p
    );
  }
}
```

---

## 下午任务（2小时）

### gmshFace - 内置面

**文件路径**：`src/geo/gmshFace.h`

```cpp
// 内置几何面（曲面）
class gmshFace : public GFace {
protected:
  Surface *_s;

public:
  gmshFace(GModel *model, Surface *s);

  // 参数范围
  virtual Range<double> parBounds(int i) const;

  // 参数到点的映射
  virtual GPoint point(double u, double v) const;

  // 法向量
  virtual SVector3 normal(const SPoint2 &param) const;

  // 一阶偏导数
  virtual Pair<SVector3, SVector3> firstDer(const SPoint2 &param) const;

  // 获取底层Surface
  Surface *getSurface() const { return _s; }
};

// 曲面类型
#define MSH_SURF_PLAN       1   // 平面
#define MSH_SURF_REGL       2   // 规则面（拉伸）
#define MSH_SURF_TRIC       3   // 三边面
#define MSH_SURF_DISCRETE   4   // 离散面
```

### 平面的实现

```cpp
// 平面由边界曲线定义
// 参数化通过投影到局部坐标系实现

GPoint gmshFace::point(double u, double v) const {
  if(_s->Typ == MSH_SURF_PLAN) {
    // 平面方程: ax + by + cz + d = 0
    // 或者用基向量表示
    SVector3 origin = _s->origin;
    SVector3 axis1 = _s->axis1;
    SVector3 axis2 = _s->axis2;

    return GPoint(
      origin.x() + u * axis1.x() + v * axis2.x(),
      origin.y() + u * axis1.y() + v * axis2.y(),
      origin.z() + u * axis1.z() + v * axis2.z(),
      this, u, v
    );
  }
}

SVector3 gmshFace::normal(const SPoint2 &param) const {
  if(_s->Typ == MSH_SURF_PLAN) {
    // 平面法向量恒定
    return _s->normal;
  }
  // 其他曲面：通过叉积计算
  auto der = firstDer(param);
  return crossprod(der.first(), der.second()).unit();
}
```

### gmshRegion - 内置体

**文件路径**：`src/geo/gmshRegion.h`

```cpp
// 内置几何体
class gmshRegion : public GRegion {
protected:
  Volume *_v;

public:
  gmshRegion(GModel *model, Volume *v);

  // 获取边界面
  virtual std::vector<GFace*> faces() const {
    return l_faces;  // 从GRegion继承
  }

  // 边界面方向
  virtual std::vector<int> orientations() const {
    return l_dirs;
  }

  // 获取底层Volume
  Volume *getVolume() const { return _v; }
};

// Volume结构
struct Volume {
  int Num;           // 标签
  int Typ;           // 类型
  List_T *Surfaces;  // 边界面
  List_T *SurfacesOrientations;  // 面的方向
};
```

---

## 练习作业

### 1. 【基础】分析曲线类型

查找不同曲线类型的实现：

```bash
# 查找曲线类型定义
grep -n "MSH_SEGM_" src/geo/Geo.h

# 查找point函数中的类型分支
grep -A 5 "case MSH_SEGM" src/geo/gmshEdge.cpp
```

记录各曲线类型的参数化方法。

### 2. 【进阶】追踪GEO解析到gmsh*类

分析从GEO脚本到内置类的转换：

```bash
# 查找Vertex创建
grep -rn "new Vertex\|Create_Vertex" src/geo/

# 查找gmshVertex创建
grep -rn "new gmshVertex" src/geo/
```

绘制流程图：
```
GEO文件解析
    ↓
Geo.cpp: Vertex/Curve/Surface 结构
    ↓
GModel::importGEO()
    ↓
new gmshVertex/gmshEdge/gmshFace/gmshRegion
```

### 3. 【挑战】实现新的曲线类型

设计一个新的曲线类型（如抛物线）：

```cpp
// 伪代码
class ParabolicCurve : public gmshEdge {
  double a, b, c;  // y = ax² + bx + c

public:
  GPoint point(double t) const override {
    // t ∈ [0, 1] 映射到 x ∈ [x0, x1]
    double x = x0 + t * (x1 - x0);
    double y = a*x*x + b*x + c;
    return GPoint(x, y, 0, this, t);
  }

  SVector3 firstDer(double t) const override {
    double x = x0 + t * (x1 - x0);
    double dx = x1 - x0;
    double dy = (2*a*x + b) * dx;
    return SVector3(dx, dy, 0);
  }
};
```

---

## 今日检查点

- [ ] 理解gmshVertex的坐标存储和访问
- [ ] 理解gmshEdge的参数化实现
- [ ] 能区分不同曲线类型（直线、圆弧、样条）
- [ ] 理解gmshFace的参数化方法

---

## 核心概念

### 内置几何vs OCC几何

```
┌─────────────────────────────────────────────────────────────┐
│              内置几何（gmsh*）                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ 优点：                                               │   │
│  │   - 轻量级，无外部依赖                               │   │
│  │   - 支持GEO脚本的全部功能                            │   │
│  │   - 代码简单，易于理解                               │   │
│  │                                                     │   │
│  │ 缺点：                                               │   │
│  │   - 不支持复杂布尔运算                               │   │
│  │   - 不支持STEP/IGES等CAD格式                         │   │
│  │   - 曲面类型有限                                     │   │
│  └─────────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────────┤
│              OCC几何（OCC*）                                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ 优点：                                               │   │
│  │   - 完整的CAD功能                                    │   │
│  │   - 支持布尔运算、倒角、圆角                          │   │
│  │   - 支持STEP、IGES、BREP等格式                       │   │
│  │                                                     │   │
│  │ 缺点：                                               │   │
│  │   - 需要OpenCASCADE库                                │   │
│  │   - 代码复杂                                         │   │
│  │   - 有时不稳定                                       │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 继承体系

```
GVertex           GEdge           GFace           GRegion
   │                │               │                │
   ├─gmshVertex     ├─gmshEdge      ├─gmshFace       ├─gmshRegion
   │                │               │                │
   ├─OCCVertex      ├─OCCEdge       ├─OCCFace        ├─OCCRegion
   │                │               │                │
   └─discreteVertex └─discreteEdge  └─discreteFace   └─discreteRegion
```

### GEO脚本到内部结构

```
GEO脚本                     内部结构              GModel
─────────────────────────────────────────────────────────
Point(1) = {0,0,0,1};   →   Vertex           →   gmshVertex
Line(1) = {1, 2};       →   Curve            →   gmshEdge
Circle(2) = {1,2,3};    →   Curve(CIRC)      →   gmshEdge
Plane Surface(1) = {1}; →   Surface          →   gmshFace
Volume(1) = {1};        →   Volume           →   gmshRegion
```

---

## 源码导航

### 关键文件

```
src/geo/
├── gmshVertex.h/cpp    # 内置顶点
├── gmshEdge.h/cpp      # 内置边（曲线）
├── gmshFace.h/cpp      # 内置面
├── gmshRegion.h/cpp    # 内置体
├── Geo.h/cpp           # GEO解析器数据结构
├── GeoInterpolation.cpp # 曲线插值计算
└── GeoStringInterface.cpp # GEO脚本接口
```

### 搜索命令

```bash
# 查找gmsh*类定义
grep -rn "class gmsh" src/geo/*.h

# 查找曲线参数化
grep -n "point.*double p" src/geo/gmshEdge.cpp

# 查找曲面参数化
grep -n "point.*double u.*double v" src/geo/gmshFace.cpp
```

---

## 产出

- 理解内置几何引擎架构
- 掌握曲线/曲面参数化方法

---

## 导航

- **上一天**：[Day 29 - GModel几何模型类](day-29.md)
- **下一天**：[Day 31 - OpenCASCADE集成](day-31.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
