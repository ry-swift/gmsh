# OCCAttributesRTree 深度解析

## 1. 问题背景：为什么需要这个类？

### 1.1 核心问题

在 CAD 建模过程中，用户会为几何实体设置各种属性：

- **网格尺寸 (meshSize)**：控制该实体附近的网格密度
- **拉伸参数 (ExtrudeParams)**：记录拉伸/旋转操作的参数
- **标签 (label)**：从 STEP/IGES 文件导入的实体名称
- **颜色 (color)**：实体的显示颜色

但是，当执行 **布尔操作**（特别是 `BooleanFragments`）时，原始的 `TopoDS_Shape` 对象会被销毁并替换成新的形状对象。这导致之前绑定到旧形状上的属性会丢失。

### 1.2 典型场景示例

```geo
// 用户创建一个点并设置网格尺寸
Point(1) = {0, 0, 0, 0.1};  // meshSize = 0.1

// 创建两个相交的圆
Circle(1) = {0, 0, 0, 1, 0, Pi};
Circle(2) = {1, 0, 0, 1, 0, Pi};

// 执行布尔碎片化操作
BooleanFragments{ Curve{1}; Curve{2}; }{ }
```

在 `BooleanFragments` 执行后：
- 原始的 `TopoDS_Vertex` (Point 1) 被替换成新的顶点对象
- 但用户设置的 `meshSize = 0.1` 仍需要保留

### 1.3 解决思路

`OCCAttributesRTree` 采用了一种巧妙的方法：
> **不依赖 TopoDS_Shape 的对象标识，而是通过形状的包围盒中心点进行空间匹配**

这样，即使形状对象被替换，只要新形状的包围盒中心点与旧形状相同（或在容差范围内），就能找回原来的属性。

---

## 2. 核心数据结构

### 2.1 R-Tree 空间索引

R-Tree（Rectangle Tree）是一种用于空间数据的树形索引结构，由 Antonin Guttman 于 1984 年提出。

```cpp
// 位置：src/common/rtree.h
template<class DATATYPE, class ELEMTYPE, int NUMDIMS,
         class ELEMTYPEREAL = ELEMTYPE,
         int TMAXNODES = 8, int TMINNODES = TMAXNODES / 2>
class RTree { ... }
```

**特点**：
- 支持高效的空间范围查询
- 插入、删除、搜索的时间复杂度为 O(log n)
- 适合处理多维空间数据

### 2.2 OCCAttributesRTree 的数据成员

```cpp
class OCCAttributesRTree {
private:
  // 4 棵 R-Tree，分别对应 0维(点)、1维(边)、2维(面)、3维(体)
  RTree<OCCAttributes *, double, 3, double> *_rtree[4];

  // 所有属性对象的列表（用于内存管理）
  std::vector<OCCAttributes *> _all;

  // 匹配容差，默认 1e-8
  double _tol;
  ...
};
```

**为什么分成 4 棵树？**

不同维度的实体分开存储，可以：
1. 减少搜索范围，提高查询效率
2. 避免不同维度实体的误匹配

---

## 3. OCCAttributes 类详解

`OCCAttributes` 是存储单个形状属性的容器类。

### 3.1 数据成员

```cpp
class OCCAttributes {
private:
  int _dim;                    // 维度：0=点, 1=边, 2=面, 3=体
  TopoDS_Shape _shape;         // 关联的形状对象
  double _meshSize;            // 网格尺寸（默认 MAX_LC）
  ExtrudeParams *_extrude;     // 拉伸参数
  int _sourceDim;              // 拉伸源实体的维度
  TopoDS_Shape _sourceShape;   // 拉伸源实体的形状
  std::string _label;          // 标签（如 STEP 文件中的实体名）
  std::vector<double> _color;  // 颜色 [r, g, b, a, boundary?]
};
```

### 3.2 构造函数重载

根据不同的属性类型，提供多种构造方式：

```cpp
// 1. 仅形状（用于基本绑定）
OCCAttributes(int dim, TopoDS_Shape shape)

// 2. 形状 + 网格尺寸
OCCAttributes(int dim, TopoDS_Shape shape, double size)

// 3. 形状 + 拉伸参数
OCCAttributes(int dim, TopoDS_Shape shape, ExtrudeParams *e,
              int sourceDim, TopoDS_Shape sourceShape)

// 4. 形状 + 标签
OCCAttributes(int dim, TopoDS_Shape shape, const std::string &label)

// 5. 形状 + 颜色
OCCAttributes(int dim, TopoDS_Shape shape,
              double r, double g, double b, double a = 1., int boundary = 0)
```

---

## 4. OCCAttributesRTree 核心方法详解

### 4.1 插入操作 `insert()`

```cpp
void insert(OCCAttributes *v)
{
  _all.push_back(v);  // 加入列表用于内存管理

  if(v->getDim() < 0 || v->getDim() > 3) return;

  // 计算形状的包围盒
  Bnd_Box box;
  BRepBndLib::Add(v->getShape(), box, Standard_False);

  if(box.IsVoid()) {
    Msg::Debug("Inserting (null or degenerate) shape with void bounding box");
    return;
  }

  // 获取包围盒的边界
  double xmin, ymin, zmin, xmax, ymax, zmax;
  box.Get(xmin, ymin, zmin, xmax, ymax, zmax);

  // 计算包围盒中心点
  double x = 0.5 * (xmin + xmax);
  double y = 0.5 * (ymin + ymax);
  double z = 0.5 * (zmin + zmax);

  // 以中心点为索引键插入 R-Tree
  // 使用 [center - tol, center + tol] 作为包围盒
  double bmin[3] = {x - _tol, y - _tol, z - _tol};
  double bmax[3] = {x + _tol, y + _tol, z + _tol};

  _rtree[v->getDim()]->Insert(bmin, bmax, v);
}
```

**关键点**：
- 使用形状包围盒的**中心点**作为索引键
- 索引范围是 `[center - tol, center + tol]`，而不是整个包围盒
- 这种设计使得空间查询更加精确

### 4.2 查找操作 `_find()`

这是核心的私有方法，所有公开的查询方法都依赖它：

```cpp
void _find(int dim, const TopoDS_Shape &shape,
           std::vector<OCCAttributes *> &attr,
           bool requireMeshSize,
           bool requireExtrudeParams,
           bool requireLabel,
           bool requireColor,
           bool excludeSame)
{
  attr.clear();
  if(dim < 0 || dim > 3) return;

  // 步骤1：计算目标形状的包围盒中心
  Bnd_Box box;
  BRepBndLib::Add(shape, box, Standard_False);

  double xmin, ymin, zmin, xmax, ymax, zmax;
  box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
  double x = 0.5 * (xmin + xmax);
  double y = 0.5 * (ymin + ymax);
  double z = 0.5 * (zmin + zmax);

  // 步骤2：在 R-Tree 中搜索 [center ± tol] 范围内的候选
  double bmin[3] = {x - _tol, y - _tol, z - _tol};
  double bmax[3] = {x + _tol, y + _tol, z + _tol};

  std::vector<OCCAttributes *> tmp;
  _rtree[dim]->Search(bmin, bmax, rtree_callback, &tmp);

  // 步骤3：优先精确匹配（shape.IsSame）
  if(!excludeSame) {
    for(std::size_t i = 0; i < tmp.size(); i++) {
      // 检查是否满足所需属性条件
      if(requireMeshSize && tmp[i]->getMeshSize() == MAX_LC) continue;
      if(requireExtrudeParams && !tmp[i]->getExtrudeParams()) continue;
      if(requireLabel && tmp[i]->getLabel().empty()) continue;
      if(requireColor && tmp[i]->getColor().empty()) continue;

      // 精确匹配：同一个 TopoDS_Shape 对象
      if(shape.IsSame(tmp[i]->getShape())) {
        attr.push_back(tmp[i]);
        return;  // 找到精确匹配，立即返回
      }
    }
  }

  // 步骤4：包围盒匹配（用于布尔操作后的形状）
  for(std::size_t i = 0; i < tmp.size(); i++) {
    if(requireMeshSize && tmp[i]->getMeshSize() == MAX_LC) continue;
    if(requireExtrudeParams && !tmp[i]->getExtrudeParams()) continue;
    if(requireLabel && tmp[i]->getLabel().empty()) continue;
    if(requireColor && tmp[i]->getColor().empty()) continue;

    // 比较两个形状的包围盒边界
    Bnd_Box box2;
    BRepBndLib::Add(tmp[i]->getShape(), box2, Standard_False);
    double xmin2, ymin2, zmin2, xmax2, ymax2, zmax2;
    box2.Get(xmin2, ymin2, zmin2, xmax2, ymax2, zmax2);

    // 6 个边界值都在容差范围内，则认为匹配
    if(std::abs(xmin - xmin2) < _tol && std::abs(xmax - xmax2) < _tol &&
       std::abs(ymin - ymin2) < _tol && std::abs(ymax - ymax2) < _tol &&
       std::abs(zmin - zmin2) < _tol && std::abs(zmax - zmax2) < _tol) {
      attr.push_back(tmp[i]);
    }
  }
}
```

**匹配策略（两级匹配）**：

```
               ┌─────────────────────────────────────┐
               │      计算目标形状的包围盒中心        │
               └─────────────────┬───────────────────┘
                                 │
                                 ▼
               ┌─────────────────────────────────────┐
               │  R-Tree 范围搜索 [center ± tol]     │
               └─────────────────┬───────────────────┘
                                 │
                        找到候选列表 tmp
                                 │
                                 ▼
               ┌─────────────────────────────────────┐
               │ 第一级：精确匹配 shape.IsSame()      │
               │ （形状对象未被修改的情况）           │
               └─────────────────┬───────────────────┘
                                 │
                         找到？──┬── 是 → 返回
                                 │
                                 否
                                 │
                                 ▼
               ┌─────────────────────────────────────┐
               │ 第二级：包围盒边界匹配               │
               │ （形状被布尔操作替换后的情况）       │
               │ 比较 6 个边界值是否在容差内          │
               └─────────────────┴───────────────────┘
```

### 4.3 公开查询方法

```cpp
// 获取网格尺寸
double getMeshSize(int dim, TopoDS_Shape shape) {
  std::vector<OCCAttributes *> attr;
  _find(dim, shape, attr, true, false, false, false, false);
  for(auto a : attr) {
    if(a->getMeshSize() < MAX_LC) return a->getMeshSize();
  }
  return MAX_LC;
}

// 获取拉伸参数
ExtrudeParams *getExtrudeParams(int dim, TopoDS_Shape shape,
                                 int &sourceDim, TopoDS_Shape &sourceShape) {
  std::vector<OCCAttributes *> attr;
  _find(dim, shape, attr, false, true, false, false, false);
  for(auto a : attr) {
    if(a->getExtrudeParams()) {
      sourceDim = a->getSourceDim();
      sourceShape = a->getSourceShape();
      return a->getExtrudeParams();
    }
  }
  return nullptr;
}

// 获取标签
void getLabels(int dim, TopoDS_Shape shape, std::vector<std::string> &labels) {
  labels.clear();
  std::vector<OCCAttributes *> attr;
  _find(dim, shape, attr, false, false, true, false, false);
  for(auto a : attr) {
    if(!a->getLabel().empty()) labels.push_back(a->getLabel());
  }
}

// 获取颜色
bool getColor(int dim, TopoDS_Shape shape,
              unsigned int &color, unsigned int &boundary) {
  std::vector<OCCAttributes *> attr;
  _find(dim, shape, attr, false, false, false, true, false);
  for(auto a : attr) {
    const std::vector<double> &col = a->getColor();
    if(col.size() >= 3) {
      // 转换为 RGBA 格式
      int r = static_cast<int>(col[0] * 255. + 0.5);
      int g = static_cast<int>(col[1] * 255. + 0.5);
      int b = static_cast<int>(col[2] * 255. + 0.5);
      int a = (col.size() >= 4) ? static_cast<int>(col[3] * 255. + 0.5) : 255;
      color = CTX::instance()->packColor(r, g, b, a);
      boundary = (col.size() == 5) ? col[4] : 0;
      return true;
    }
  }
  return false;
}

// 获取相似形状（用于调试或特殊场景）
void getSimilarShapes(int dim, TopoDS_Shape shape,
                      std::vector<TopoDS_Shape> &other) {
  std::vector<OCCAttributes *> attr;
  _find(dim, shape, attr, false, false, false, false, true);  // excludeSame=true
  for(auto a : attr) {
    TopoDS_Shape s = a->getShape();
    if(!s.IsNull()) other.push_back(s);
  }
}
```

---

## 5. 在 OCC_Internals 中的使用

### 5.1 初始化与销毁

```cpp
// 文件：src/geo/GModelIO_OCC.cpp

OCC_Internals::OCC_Internals() {
  // 使用几何容差初始化
  _attributes = new OCCAttributesRTree(CTX::instance()->geom.tolerance);
  // ...
}

OCC_Internals::~OCC_Internals() {
  delete _attributes;
}

void OCC_Internals::reset() {
  _attributes->clear();
  // ...
}
```

### 5.2 绑定形状时插入属性

当创建几何实体时，会自动插入基本属性：

```cpp
void OCC_Internals::_bind(const TopoDS_Vertex &vertex, int tag, bool recursive)
{
  // ...
  _attributes->insert(new OCCAttributes(0, vertex));
}

void OCC_Internals::_bind(const TopoDS_Edge &edge, int tag, bool recursive)
{
  // ...
  _attributes->insert(new OCCAttributes(1, edge));
}

void OCC_Internals::_bind(const TopoDS_Face &face, int tag, bool recursive)
{
  // ...
  _attributes->insert(new OCCAttributes(2, face));
}

void OCC_Internals::_bind(const TopoDS_Solid &solid, int tag, bool recursive)
{
  // ...
  _attributes->insert(new OCCAttributes(3, solid));
}
```

### 5.3 设置网格尺寸

```cpp
bool OCC_Internals::addVertex(int &tag, double x, double y, double z,
                              double meshSize)
{
  // 创建顶点
  TopoDS_Vertex result;
  // ...

  // 如果指定了网格尺寸，插入属性
  if(meshSize < MAX_LC) {
    _attributes->insert(new OCCAttributes(0, result, meshSize));
  }
}
```

### 5.4 拉伸操作中的属性传递

```cpp
void OCC_Internals::_setExtrudedAttributes(...)
{
  bool extrude_attributes = (e ? true : false);

  // 对于拉伸生成的顶面和侧面，设置拉伸参数
  if(extrude_attributes) {
    ExtrudeParams *ee = new ExtrudeParams(COPIED_ENTITY);
    ee->fill(p, r, ...);
    // 将顶面与底面关联
    _attributes->insert(new OCCAttributes(2, top, ee, 2, bot));
  }

  // 传递网格尺寸
  double lc = _attributes->getMeshSize(0, bot);
  if(lc < MAX_LC) {
    _attributes->insert(new OCCAttributes(0, top, lc));
  }
}
```

### 5.5 从 CAD 文件导入属性

```cpp
// 从 STEP/IGES 文件读取颜色和标签
static void setShapeAttributes(OCCAttributesRTree *attributes, ...)
{
  // 读取标签
  std::string phys = pathName;
  Handle(TDataStd_Name) n;
  if(label.FindAttribute(TDataStd_Name::GetID(), n)) {
    phys += TCollection_AsciiString(n->Get()).ToCString();
  }

  if(phys.size()) {
    attributes->insert(new OCCAttributes(dim, shape, phys));
  }

  // 读取颜色
  Quantity_Color col;
  if(colorTool->GetColor(label, XCAFDoc_ColorGen, col)) {
    double r, g, b;
    getColorRGB(col, r, g, b);
    attributes->insert(new OCCAttributes(dim, shape, r, g, b, 1.));
  }
}
```

### 5.6 同步到 GModel 时应用属性

```cpp
void OCC_Internals::synchronize(GModel *model)
{
  // 对每个顶点
  for(...) {
    TopoDS_Vertex vertex = ...;

    // 获取网格尺寸
    double lc = _attributes->getMeshSize(0, vertex);
    if(lc < MAX_LC) {
      // 应用到 GVertex
      gv->setPrescribedMeshSizeAtVertex(lc);
    }

    // 获取标签
    std::vector<std::string> labels;
    _attributes->getLabels(0, vertex, labels);
    for(auto &label : labels) {
      model->setPhysicalName(label, 0, ...);
    }

    // 获取颜色
    unsigned int col, boundary;
    if(_attributes->getColor(0, vertex, col, boundary)) {
      occv->setColor(col);
    }
  }

  // 对边、面、体同样处理...
}
```

---

## 6. 设计亮点与工程考量

### 6.1 空间索引的选择

使用 R-Tree 而非简单的哈希表或列表，原因：

| 方案 | 优点 | 缺点 |
|------|------|------|
| 哈希表 | O(1) 精确查找 | 无法处理形状替换后的模糊匹配 |
| 列表遍历 | 简单实现 | O(n) 时间复杂度，大模型性能差 |
| **R-Tree** | O(log n) 范围查询，支持模糊匹配 | 实现复杂度较高 |

### 6.2 两级匹配策略

```
精确匹配优先 → 包围盒匹配兜底
```

这种设计兼顾了：
- **效率**：大多数情况下精确匹配即可
- **鲁棒性**：布尔操作后仍能恢复属性

### 6.3 维度分离存储

4 棵独立的 R-Tree：
- 减少搜索空间
- 避免点和体积碰巧重合导致的误匹配
- 便于按维度独立管理

### 6.4 容差设计

```cpp
OCCAttributesRTree(double tolerance = 1.e-8)
```

- 使用 `CTX::instance()->geom.tolerance` 初始化
- 与几何建模的精度保持一致
- 过大会导致误匹配，过小会导致漏匹配

---

## 7. 潜在问题与局限性

### 7.1 包围盒碰撞

如果两个不同的形状恰好具有相同的包围盒中心和边界，可能会错误地共享属性。

**缓解措施**：
- 维度分离减少碰撞概率
- 精确匹配优先

### 7.2 复杂布尔操作

在极端的布尔操作（如大量碎片化）后，可能存在：
- 多个候选匹配
- 包围盒发生微小变化导致匹配失败

### 7.3 内存管理

`_all` 列表保存所有 `OCCAttributes` 对象，在 `clear()` 时统一释放：

```cpp
void clear() {
  for(int dim = 0; dim < 4; dim++) _rtree[dim]->RemoveAll();
  for(std::size_t i = 0; i < _all.size(); i++) delete _all[i];
  _all.clear();
}
```

**注意**：`remove()` 方法只从 R-Tree 中移除索引，不删除对象本身。

---

## 8. 调试技巧

### 8.1 打印 R-Tree 内容

```cpp
void OCCAttributesRTree::print(const std::string &fileName = "")
{
  // 输出所有点的网格尺寸到 Gmsh 视图格式
  fprintf(fp, "View(\"rtree mesh sizes\"){\n");
  for(std::size_t i = 0; i < _all.size(); i++) {
    if(_all[i]->getDim() != 0) continue;
    if(_all[i]->getMeshSize() == MAX_LC) continue;
    gp_Pnt pnt = BRep_Tool::Pnt(TopoDS::Vertex(_all[i]->getShape()));
    fprintf(fp, "SP(%g,%g,%g){%g};\n", pnt.X(), pnt.Y(), pnt.Z(),
            _all[i]->getMeshSize());
  }
  fprintf(fp, "};\n");
}
```

### 8.2 调试日志

```cpp
Msg::Debug("OCCRTree found %d matches at (%g,%g,%g) in tree of size %d",
           (int)tmp.size(), x, y, z, (int)_all.size());

Msg::Debug("OCCRTree exact match");

Msg::Debug("OCCRtree %d matches after bounding box filtering", (int)attr.size());
```

启用调试模式可以看到匹配过程。

---

## 9. 相关文件

| 文件路径 | 说明 |
|----------|------|
| `src/geo/OCCAttributes.h` | OCCAttributes 和 OCCAttributesRTree 定义 |
| `src/geo/GModelIO_OCC.h` | OCC_Internals 类定义，包含 `_attributes` 成员 |
| `src/geo/GModelIO_OCC.cpp` | 属性的插入和使用逻辑 |
| `src/common/rtree.h` | R-Tree 模板实现 |

---

## 10. 总结

`OCCAttributesRTree` 是 Gmsh 中一个精心设计的空间索引组件，解决了 CAD 布尔操作后属性保持的核心问题。

**核心思想**：
> 将属性与形状的空间位置（包围盒中心）绑定，而非与形状对象标识绑定

**关键技术**：
- R-Tree 空间索引实现高效范围查询
- 两级匹配策略（精确 + 模糊）
- 维度分离减少误匹配

**应用场景**：
- 网格尺寸控制
- 拉伸操作参数传递
- CAD 文件（STEP/IGES）导入时的颜色和标签保留
- 布尔操作后的属性恢复
