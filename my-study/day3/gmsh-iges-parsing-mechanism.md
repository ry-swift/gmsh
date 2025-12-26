# Gmsh如何识别和解析IGES文件数据

本文档详细分析Gmsh源码中IGES文件的读取和解析机制，解释IGES中的实体类型是如何被转换为Gmsh内部几何对象的。

## 1. 整体架构概述

Gmsh通过**OpenCASCADE (OCC)几何内核**来处理IGES文件，核心流程如下：

```
IGES文件 → IGESControl_Reader → TopoDS_Shape → _healShape → _multiBind → synchronize → GModel
```

关键类和职责：

| 类名 | 职责 |
| --- | --- |
| `IGESControl_Reader` | OpenCASCADE提供的IGES读取器，负责解析IGES格式 |
| `OCC_Internals` | Gmsh的OCC接口封装，管理形状和标签的映射 |
| `GModel` | Gmsh的几何模型容器，存储所有几何实体 |
| `OCCVertex/Edge/Face/Region` | Gmsh对OCC形状的封装类 |

## 2. 主入口函数：GModel::readOCCIGES()

**文件位置**: `src/geo/GModelIO_OCC.cpp:6783`

```cpp
int GModel::readOCCIGES(const std::string &fn)
{
  // 步骤1: 延迟初始化 OCC_Internals
  if(!_occ_internals) _occ_internals = new OCC_Internals;

  // 步骤2: 导入IGES形状
  std::vector<std::pair<int, int>> outDimTags;
  _occ_internals->importShapes(fn, false, outDimTags, "iges");

  // 步骤3: 同步到GModel
  _occ_internals->synchronize(this);

  return 1;
}
```

## 3. IGES文件读取流程详解

### 3.1 importShapes()函数核心实现

**文件位置**: `src/geo/GModelIO_OCC.cpp:4790`

```cpp
bool OCC_Internals::importShapes(const std::string &fileName,
                                 bool highestDimOnly,
                                 std::vector<std::pair<int, int>> &outDimTags,
                                 const std::string &format)
{
  // ========== 第1步：文件格式检查 ==========
  std::vector<std::string> split = SplitFileName(fileName);

  // ========== 第2步：选择IGES读取器 ==========
  if(format == "iges" || split[2] == ".iges" || split[2] == ".igs") {

    // 设置目标单位
    setTargetUnit(CTX::instance()->geom.occTargetUnit);

    // 创建IGES读取器
    IGESControl_Reader reader;

    // 读取IGES文件
    if(reader.ReadFile(fileName.c_str()) != IFSelect_RetDone) {
      return false;  // 文件读取失败
    }

    // ========== 第3步：转换IGES实体为OCC形状 ==========
    reader.NbRootsForTransfer();   // 获取根实体数量
    reader.TransferRoots();        // 转换所有根实体
    result = reader.OneShape();    // 合并为单个形状
  }

  // ========== 第4步：形状清理 ==========
  BRepTools::Clean(result);

  // ========== 第5步：几何修复 ==========
  _healShape(result, tolerance, ...);

  // ========== 第6步：绑定形状到标签 ==========
  _multiBind(result, -1, outDimTags, highestDimOnly, true);

  return true;
}
```

### 3.2 IGES实体类型到OCC形状的自动映射

OpenCASCADE的`IGESControl_Reader`会自动将IGES实体转换为`TopoDS_Shape`：

| IGES实体类型 | IGES编号 | OCC映射 | Gmsh映射 | 维度 |
| --- | --- | --- | --- | --- |
| 直线(Line) | 110 | TopoDS_Edge | OCCEdge | 1 |
| 圆弧(Circular Arc) | 100 | TopoDS_Edge | OCCEdge | 1 |
| 椭圆弧(Ellipse Arc) | 104 | TopoDS_Edge | OCCEdge | 1 |
| B样条曲线 | 126 | TopoDS_Edge | OCCEdge | 1 |
| 平面(Plane) | 108 | TopoDS_Face | OCCFace | 2 |
| B样条曲面 | 128 | TopoDS_Face | OCCFace | 2 |
| 旋转曲面 | 120 | TopoDS_Face | OCCFace | 2 |
| 实体(Solid) | 186/187 | TopoDS_Solid | OCCRegion | 3 |
| 点(Point) | 116 | TopoDS_Vertex | OCCVertex | 0 |

## 4. 几何修复机制：_healShape()

**文件位置**: `src/geo/GModelIO_OCC.cpp:5836`

IGES文件导入后，Gmsh会自动进行几何修复：

```cpp
void OCC_Internals::_healShape(TopoDS_Shape &myshape,
                               double tolerance,
                               bool fixDegenerated,    // 修复退化边
                               bool fixSmallEdges,     // 修复小边
                               bool fixSmallFaces,     // 修复小面
                               bool sewFaces,          // 缝合面
                               bool makeSolids,        // 创建实体
                               double scaling)         // 缩放
```

### 4.1 修复步骤详解

**修复退化边**:

```cpp
if(fixDegenerated) {
  ShapeBuild_ReShape rebuild;
  for(exp1.Init(myshape, TopAbs_EDGE); exp1.More(); exp1.Next()) {
    TopoDS_Edge edge = TopoDS::Edge(exp1.Current());
    if(BRep_Tool::Degenerated(edge))
      rebuild.Remove(edge);  // 移除退化边
  }
  myshape = rebuild.Apply(myshape);
}
```

**修复小边**:

```cpp
if(fixSmallEdges) {
  ShapeFix_Wire sfw(oldwire, face, tolerance);
  sfw.FixReorder();           // 重新排序边
  sfw.FixConnected();         // 修复连接性
  sfw.FixSmall();             // 修复小边
  sfw.FixEdgeCurves();        // 修复边曲线
  sfw.FixDegenerated();       // 修复退化边
  sfw.FixSelfIntersection();  // 修复自相交
}
```

**缝合面**:

```cpp
if(sewFaces) {
  BRepOffsetAPI_Sewing sewer;
  sewer.SetTolerance(tolerance);
  for(/* 遍历所有面 */) {
    sewer.Add(face);
  }
  sewer.Perform();
  myshape = sewer.SewedShape();
}
```

## 5. 形状绑定机制：_multiBind()

**文件位置**: `src/geo/GModelIO_OCC.cpp:715`

此函数遍历OCC形状的拓扑层次，为每个子形状分配唯一标签：

```cpp
void OCC_Internals::_multiBind(const TopoDS_Shape &shape,
                               int tag,
                               std::vector<std::pair<int, int>> &outDimTags,
                               bool highestDimOnly,
                               bool recursive)
{
  // 处理3D实体
  for(exp0.Init(shape, TopAbs_SOLID); exp0.More(); exp0.Next()) {
    TopoDS_Solid solid = TopoDS::Solid(exp0.Current());
    int t = getMaxTag(3) + 1;
    _bind(solid, t, recursive);
    outDimTags.push_back(std::make_pair(3, t));
  }

  if(highestDimOnly) return;

  // 处理2D面
  for(exp0.Init(shape, TopAbs_FACE); exp0.More(); exp0.Next()) { ... }

  // 处理1D边
  for(exp0.Init(shape, TopAbs_EDGE); exp0.More(); exp0.Next()) { ... }

  // 处理0D顶点
  for(exp0.Init(shape, TopAbs_VERTEX); exp0.More(); exp0.Next()) { ... }
}
```

### 5.1 拓扑层次结构

```
TopoDS_Shape (根形状)
    ├─ TopoDS_Solid (3D实体)     → OCCRegion
    │   └─ TopoDS_Shell (壳)
    │       └─ TopoDS_Face (面)  → OCCFace
    │           └─ TopoDS_Wire (线环)
    │               └─ TopoDS_Edge (边) → OCCEdge
    │                   └─ TopoDS_Vertex (顶点) → OCCVertex
    ├─ TopoDS_Face (独立面)
    ├─ TopoDS_Edge (独立边)
    └─ TopoDS_Vertex (独立点)
```

## 6. 同步到GModel：synchronize()

**文件位置**: `src/geo/GModelIO_OCC.cpp:5483`

这是将OCC数据转换为Gmsh几何实体的最终步骤：

### 6.1 四个关键阶段

**阶段1：删除过期实体**

```cpp
std::vector<std::pair<int, int>> toRemove;
toRemove.insert(toRemove.end(), _toRemove.begin(), _toRemove.end());
model->remove(toRemove, removed);
_toRemove.clear();
```

**阶段2：重建形状映射**

```cpp
// 清空现有映射
_somap.Clear(); _shmap.Clear(); _fmap.Clear();
_wmap.Clear(); _emap.Clear(); _vmap.Clear();

// 从Tag映射重建索引映射
for(exp0.Init(...); exp0.More(); exp0.Next())
  _addShapeToMaps(exp0.Value());
```

**阶段3：创建GModel实体**

```cpp
// 创建顶点
for(int i = 1; i <= _vmap.Extent(); i++) {
  TopoDS_Vertex vertex = TopoDS::Vertex(_vmap(i));
  OCCVertex *occv = new OCCVertex(model, vertex, tag);
  model->add(occv);
}

// 创建边
for(int i = 1; i <= _emap.Extent(); i++) {
  TopoDS_Edge edge = TopoDS::Edge(_emap(i));
  GVertex *v1 = getVertexForOCCShape(model, TopExp::FirstVertex(edge));
  GVertex *v2 = getVertexForOCCShape(model, TopExp::LastVertex(edge));
  OCCEdge *occe = new OCCEdge(model, edge, tag, v1, v2);
  model->add(occe);
}

// 创建面
for(int i = 1; i <= _fmap.Extent(); i++) {
  TopoDS_Face face = TopoDS::Face(_fmap(i));
  OCCFace *occf = new OCCFace(model, face, tag);
  model->add(occf);
}

// 创建体
for(int i = 1; i <= _somap.Extent(); i++) {
  TopoDS_Solid region = TopoDS::Solid(_somap(i));
  OCCRegion *occr = new OCCRegion(model, region, tag);
  model->add(occr);
}
```

**阶段4：后处理**

```cpp
if(CTX::instance()->geom.toleranceBoolean)
  model->snapVertices();
SetBoundingBox();
_changed = false;
```

## 7. 完整调用流程图

```
用户调用: gmsh.merge("hhh.igs")
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  GModel::readOCCIGES("hhh.igs")                         │
│  文件位置: src/geo/GModelIO_OCC.cpp:6783                │
└─────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  OCC_Internals::importShapes()                          │
│  ┌────────────────────────────────────────────────┐    │
│  │ IGESControl_Reader::ReadFile()                  │    │
│  │ - 解析IGES文件的5个段(S,G,D,P,T)               │    │
│  │ - 识别实体类型(110,100,116,186等)               │    │
│  ├────────────────────────────────────────────────┤    │
│  │ IGESControl_Reader::TransferRoots()             │    │
│  │ - IGES Type 110 (Line) → TopoDS_Edge           │    │
│  │ - IGES Type 100 (Arc) → TopoDS_Edge            │    │
│  │ - IGES Type 116 (Point) → TopoDS_Vertex        │    │
│  │ - IGES Type 120 (Revolution) → TopoDS_Face     │    │
│  │ - IGES Type 186 (Solid) → TopoDS_Solid         │    │
│  ├────────────────────────────────────────────────┤    │
│  │ reader.OneShape()                               │    │
│  │ - 合并所有转换结果为单个TopoDS_Shape           │    │
│  └────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  OCC_Internals::_healShape()                            │
│  - 修复退化边 (ShapeBuild_ReShape)                     │
│  - 修复小边 (ShapeFix_Wire)                            │
│  - 修复小面 (GProp_GProps)                             │
│  - 缝合面 (BRepOffsetAPI_Sewing)                       │
└─────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  OCC_Internals::_multiBind()                            │
│  - 遍历TopoDS_Shape的拓扑层次                          │
│  - 为每个子形状分配唯一标签(tag)                        │
│  - 建立双向映射: Shape↔Tag                             │
└─────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  OCC_Internals::synchronize(GModel*)                    │
│  - 创建OCCVertex (0D点)                                │
│  - 创建OCCEdge (1D边)                                  │
│  - 创建OCCFace (2D面)                                  │
│  - 创建OCCRegion (3D体)                                │
│  - 添加到GModel                                        │
└─────────────────────────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────────────────────────┐
│  完成! GModel现在包含所有导入的几何实体                  │
│  可以进行: 网格生成、拓扑查询、布尔运算等               │
└─────────────────────────────────────────────────────────┘
```

## 8. hhh.igs文件的具体解析示例

以hhh.igs文件为例，展示IGES数据如何被Gmsh识别：

### 8.1 IGES中的Type 110直线

**IGES数据**:

```
110,0.,0.,0.,0.,0.,1000.;    (P段第1行)
```

**解析过程**:

1. `IGESControl_Reader`识别实体类型110（直线）
2. 解析参数：起点(0,0,0)，终点(0,0,1000)
3. 创建`TopoDS_Edge`对象，内部包含`Geom_Line`几何曲线
4. `_multiBind()`为此边分配tag=1
5. `synchronize()`创建`OCCEdge`对象，添加到GModel

### 8.2 IGES中的Type 186实体

**IGES数据**:

```
186,37,1,0;    (P段第20行)
```

**解析过程**:

1. `IGESControl_Reader`识别实体类型186（Manifold Solid B-Rep）
2. 递归解析引用的壳(Type 514)、面(Type 510)、环(Type 508)等
3. 构建完整的拓扑结构`TopoDS_Solid`
4. `_multiBind()`为此实体及其所有子形状分配tag
5. `synchronize()`创建`OCCRegion`及其边界`OCCFace`

## 9. OCC_Internals的核心数据结构

```cpp
class OCC_Internals {
private:
  // Shape → Tag 映射
  TopTools_DataMapOfShapeInteger _vertexTag;
  TopTools_DataMapOfShapeInteger _edgeTag;
  TopTools_DataMapOfShapeInteger _faceTag;
  TopTools_DataMapOfShapeInteger _solidTag;

  // Tag → Shape 映射
  TopTools_DataMapOfIntegerShape _tagVertex;
  TopTools_DataMapOfIntegerShape _tagEdge;
  TopTools_DataMapOfIntegerShape _tagFace;
  TopTools_DataMapOfIntegerShape _tagSolid;

  // 索引映射（按顺序访问）
  TopTools_IndexedMapOfShape _vmap, _emap, _fmap, _somap;

  // 最大标签数组
  int _maxTag[6];  // 各维度的最大标签值
};
```

## 10. 关键配置参数

这些参数控制IGES导入行为：

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `Geometry.OCCTargetUnit` | "mm" | 目标单位 |
| `Geometry.OCCFixDegenerated` | 1 | 修复退化边 |
| `Geometry.OCCFixSmallEdges` | 1 | 修复小边 |
| `Geometry.OCCFixSmallFaces` | 1 | 修复小面 |
| `Geometry.OCCSewFaces` | 1 | 缝合面 |
| `Geometry.OCCMakeSolids` | 1 | 创建实体 |
| `Geometry.Tolerance` | 1e-8 | 几何容差 |

## 11. 调试断点建议

```bash
# GDB调试时可在以下位置设置断点
(gdb) b GModelIO_OCC.cpp:6783    # readOCCIGES 入口
(gdb) b GModelIO_OCC.cpp:4838    # IGESControl_Reader::ReadFile
(gdb) b GModelIO_OCC.cpp:4852    # TransferRoots
(gdb) b GModelIO_OCC.cpp:5836    # _healShape 入口
(gdb) b GModelIO_OCC.cpp:5483    # synchronize 入口
```

## 12. 总结

Gmsh识别IGES数据的核心机制：

1. **格式解析**: 由OpenCASCADE的`IGESControl_Reader`完成，Gmsh本身不直接解析IGES格式
2. **实体转换**: IGES实体类型自动映射为OCC的`TopoDS_*`形状类
3. **几何修复**: 自动修复导入模型的拓扑缺陷
4. **标签绑定**: 建立形状与整数标签的双向映射
5. **GModel同步**: 将OCC形状转换为Gmsh的`OCC*`几何实体类

关键文件：
- `src/geo/GModelIO_OCC.cpp` - IGES导入的核心实现
- `src/geo/GModelIO_OCC.h` - OCC_Internals类声明
- `src/geo/OCCVertex.cpp`, `OCCEdge.cpp`, `OCCFace.cpp`, `OCCRegion.cpp` - 几何实体封装
