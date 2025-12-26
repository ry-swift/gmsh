# Gmsh与OpenCASCADE联合调试完全指南

> **文档版本**: v1.0
> **适用版本**: Gmsh 4.14.0 + OpenCASCADE 7.x
> **创建日期**: 2024年12月

---

## 目录

1. [概述与架构](#1-概述与架构)
2. [调试环境搭建](#2-调试环境搭建)
3. [OCC核心架构详解](#3-occ核心架构详解)
4. [关键调试入口点](#4-关键调试入口点)
5. [布尔运算调试](#5-布尔运算调试)
6. [错误处理与日志系统](#6-错误处理与日志系统)
7. [断点设置速查表](#7-断点设置速查表)
8. [常见调试场景实战](#8-常见调试场景实战)
9. [调试工具与技巧](#9-调试工具与技巧)
10. [附录](#10-附录)

---

## 1. 概述与架构

### 1.1 Gmsh与OCC的关系

Gmsh是一个开源的有限元网格生成器，支持多种几何内核。OpenCASCADE (OCC)是其中最强大的几何内核之一，提供了：

- **CAD文件导入**: STEP、IGES、BREP格式支持
- **几何建模**: 参数化曲线、曲面、实体创建
- **布尔运算**: 并集、交集、差集、碎片化
- **几何修复**: 形状愈合、容差处理

### 1.2 集成架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                         Gmsh应用层                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │ 几何模块 │  │ 网格模块 │  │ 求解器   │  │ 后处理   │        │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘        │
│       │             │             │             │               │
│       └─────────────┴─────────────┴─────────────┘               │
│                           │                                      │
│                           ▼                                      │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                      GModel                              │    │
│  │  _occ_internals ──────────────────────────────────────►  │    │
│  └─────────────────────────────────────────────────────────┘    │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                      OCC_Internals                              │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  形状映射 (Shape Maps)                                   │    │
│  │  ┌─────┐ ┌─────┐ ┌─────┐ ┌──────┐ ┌──────┐ ┌──────┐    │    │
│  │  │_vmap│ │_emap│ │_wmap│ │_fmap │ │_shmap│ │_somap│    │    │
│  │  └─────┘ └─────┘ └─────┘ └──────┘ └──────┘ └──────┘    │    │
│  └─────────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  标签映射 (Tag Maps)                                     │    │
│  │  _tagVertex, _tagEdge, _tagFace, _tagSolid              │    │
│  │  _vertexTag, _edgeTag, _faceTag, _solidTag              │    │
│  └─────────────────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  属性管理 (OCCAttributesRTree)                           │    │
│  │  网格尺寸、挤出参数、标签、颜色                          │    │
│  └─────────────────────────────────────────────────────────┘    │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                   OpenCASCADE 几何内核                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐             │
│  │ TopoDS_Shape │ │ Geom_Surface │ │ BRep_Tool    │             │
│  └──────────────┘ └──────────────┘ └──────────────┘             │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐             │
│  │ STEPControl  │ │ IGESControl  │ │ BRepAlgoAPI  │             │
│  └──────────────┘ └──────────────┘ └──────────────┘             │
└─────────────────────────────────────────────────────────────────┘
```

### 1.3 核心模块关系

```
GModel (几何模型容器)
    │
    ├── _occ_internals: OCC_Internals*  ← OCC几何内核接口
    │       │
    │       ├── importShapes()     ← 导入CAD文件
    │       ├── addVertex/Edge/... ← 创建几何实体
    │       ├── boolean...()       ← 布尔运算
    │       └── synchronize()      ← 同步到GModel
    │
    ├── vertices_: std::set<GVertex*>
    │       └── OCCVertex* (OCC实现)
    │
    ├── edges_: std::set<GEdge*>
    │       └── OCCEdge* (OCC实现)
    │
    ├── faces_: std::set<GFace*>
    │       └── OCCFace* (OCC实现)
    │
    └── regions_: std::set<GRegion*>
            └── OCCRegion* (OCC实现)
```

### 1.4 OCC_Internals类设计理念

`OCC_Internals`是Gmsh与OpenCASCADE之间的核心适配器，其设计遵循以下原则：

1. **延迟同步**: 几何操作在OCC内部完成后，通过`synchronize()`统一同步到GModel
2. **双向映射**: 维护 OCC Shape ↔ Gmsh Tag 的双向映射关系
3. **属性管理**: 使用R树高效存储和查询几何属性
4. **版本兼容**: 通过条件编译适配不同版本的OpenCASCADE API

---

## 2. 调试环境搭建

### 2.1 CMake Debug模式构建

#### 2.1.1 基本Debug构建

```bash
# 创建并进入构建目录
mkdir build-debug && cd build-debug

# 配置Debug构建
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_BUILD_DYNAMIC=1 \
      ..

# 编译（使用多核加速）
make -j$(nproc)
```

#### 2.1.2 完整调试构建配置

```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_BUILD_DYNAMIC=1 \
      -DENABLE_BUILD_SHARED=1 \
      -DENABLE_OCC=1 \
      -DENABLE_OCC_CAF=1 \
      -DENABLE_PROFILE=1 \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..
```

#### 2.1.3 CMake构建类型说明

| 构建类型 | 优化级别 | 调试符号 | 适用场景 |
|---------|---------|---------|---------|
| `Debug` | -O0 | 完整 | 开发调试 |
| `Release` | -O3 | 无 | 生产部署 |
| `RelWithDebInfo` | -O2 | 完整 | 性能调试（默认）|
| `MinSizeRel` | -Os | 无 | 最小化体积 |

#### 2.1.4 验证OCC是否正确配置

```bash
# 在build目录中查看CMake配置摘要
grep -A5 "OpenCASCADE" CMakeCache.txt

# 预期输出应包含:
# ENABLE_OCC:BOOL=ON
# OCC_INC:PATH=/path/to/occ/include
# OCC_LIBS:FILEPATH=/path/to/occ/lib
```

### 2.2 IDE调试器配置

#### 2.2.1 VSCode配置

创建`.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Gmsh",
            "type": "lldb",
            "request": "launch",
            "program": "${workspaceFolder}/build-debug/gmsh",
            "args": ["-v", "99", "${workspaceFolder}/tutorials/t1.geo"],
            "cwd": "${workspaceFolder}",
            "stopOnEntry": false,
            "environment": [],
            "externalConsole": false,
            "MIMode": "lldb",
            "preLaunchTask": "build-debug"
        },
        {
            "name": "Debug IGES Import",
            "type": "lldb",
            "request": "launch",
            "program": "${workspaceFolder}/build-debug/gmsh",
            "args": ["-v", "99", "${workspaceFolder}/tutorials/hhh.igs"],
            "cwd": "${workspaceFolder}",
            "stopOnEntry": false
        }
    ]
}
```

创建`.vscode/tasks.json`:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build-debug",
            "type": "shell",
            "command": "cd ${workspaceFolder}/build-debug && make -j8",
            "group": {
                "kind": "build",
                "isDefault": true
            }
        }
    ]
}
```

#### 2.2.2 CLion配置

1. 打开 `Settings > Build, Execution, Deployment > CMake`
2. 添加Debug Profile:
   - Build type: `Debug`
   - CMake options: `-DENABLE_BUILD_DYNAMIC=1 -DENABLE_OCC=1`
3. 设置运行配置:
   - Program: `gmsh`
   - Program arguments: `-v 99 /path/to/file.igs`

#### 2.2.3 Xcode配置 (macOS)

```bash
# 生成Xcode项目
mkdir build-xcode && cd build-xcode
cmake -G Xcode \
      -DENABLE_BUILD_DYNAMIC=1 \
      -DENABLE_OCC=1 \
      ..

# 打开Xcode项目
open gmsh.xcodeproj
```

### 2.3 GDB/LLDB调试命令速查

#### 2.3.1 GDB常用命令

```bash
# 启动调试
gdb ./gmsh
(gdb) run -v 99 test.igs

# 设置断点
(gdb) break GModelIO_OCC.cpp:6783          # 按行号
(gdb) break GModel::readOCCIGES            # 按函数名
(gdb) break OCC_Internals::importShapes    # OCC核心函数
(gdb) break GModelIO_OCC.cpp:4860          # 异常捕获点

# 条件断点
(gdb) break GModelIO_OCC.cpp:4846 if format == "iges"

# 查看变量
(gdb) print fileName
(gdb) print *_occ_internals
(gdb) print result.ShapeType()             # OCC Shape类型

# 查看调用栈
(gdb) backtrace
(gdb) bt full                               # 完整栈信息

# 单步执行
(gdb) next                                  # 下一行（跳过函数）
(gdb) step                                  # 进入函数
(gdb) finish                                # 执行到函数返回
(gdb) continue                              # 继续运行

# 监视点
(gdb) watch _changed                        # 监视变量变化
(gdb) rwatch _toRemove.size()              # 监视读取

# 查看OCC数据结构
(gdb) print _vmap.Extent()                  # 顶点数量
(gdb) print _emap.Extent()                  # 边数量
(gdb) print _fmap.Extent()                  # 面数量
(gdb) print _somap.Extent()                 # 体数量
```

#### 2.3.2 LLDB常用命令 (macOS)

```bash
# 启动调试
lldb ./gmsh
(lldb) run -v 99 test.igs

# 设置断点
(lldb) breakpoint set -f GModelIO_OCC.cpp -l 6783
(lldb) b GModel::readOCCIGES
(lldb) b OCC_Internals::synchronize

# 条件断点
(lldb) breakpoint set -f GModelIO_OCC.cpp -l 4846 -c 'format == "iges"'

# 查看变量
(lldb) frame variable fileName
(lldb) p fileName
(lldb) expr result.ShapeType()

# 查看调用栈
(lldb) bt
(lldb) bt all                               # 所有线程

# 单步执行
(lldb) n                                    # next
(lldb) s                                    # step into
(lldb) finish                               # step out
(lldb) c                                    # continue

# 内存查看
(lldb) memory read _occ_internals
```

#### 2.3.3 常用GDB/LLDB脚本

创建`.gdbinit`文件:

```gdb
# OCC调试辅助宏
define occ_info
    printf "OCC Shape Maps:\n"
    printf "  Vertices: %d\n", _vmap.Extent()
    printf "  Edges:    %d\n", _emap.Extent()
    printf "  Wires:    %d\n", _wmap.Extent()
    printf "  Faces:    %d\n", _fmap.Extent()
    printf "  Shells:   %d\n", _shmap.Extent()
    printf "  Solids:   %d\n", _somap.Extent()
end

define break_occ_import
    break GModel::readOCCIGES
    break GModel::readOCCSTEP
    break OCC_Internals::importShapes
    break OCC_Internals::synchronize
end

define break_occ_boolean
    break OCC_Internals::booleanOperator
    break OCC_Internals::booleanUnion
    break OCC_Internals::booleanDifference
    break OCC_Internals::booleanIntersection
end
```

---

## 3. OCC核心架构详解

### 3.1 OCC相关源文件清单

| 文件 | 大小 | 用途 |
|------|------|------|
| `src/geo/GModelIO_OCC.cpp` | 232KB (6888行) | OCC_Internals完整实现 |
| `src/geo/GModelIO_OCC.h` | 41KB (959行) | OCC_Internals类定义 |
| `src/geo/OCCVertex.h/cpp` | 1.8KB/2KB | 顶点包装类 |
| `src/geo/OCCEdge.h/cpp` | 2.1KB/14KB | 边包装类 |
| `src/geo/OCCFace.h/cpp` | 2.6KB/17KB | 面包装类 |
| `src/geo/OCCRegion.h/cpp` | 861B/4.5KB | 体包装类 |
| `src/geo/OCCAttributes.h` | 11KB | 属性存储与R树索引 |

### 3.2 OCC_Internals类核心API

#### 3.2.1 形状导入

```cpp
// 从文件导入 (GModelIO_OCC.cpp:4790)
bool importShapes(const std::string &fileName,
                  bool highestDimOnly,
                  std::vector<std::pair<int, int>> &outDimTags,
                  const std::string &format = "");

// 从OCC形状导入
bool importShapes(const TopoDS_Shape *shape,
                  bool highestDimOnly,
                  std::vector<std::pair<int, int>> &outDimTags);
```

#### 3.2.2 基本几何创建

```cpp
// 顶点
bool addVertex(int &tag, double x, double y, double z, double meshSize=MAX_LC);

// 曲线
bool addLine(int &tag, int startTag, int endTag);
bool addCircle(int &tag, double x, double y, double z, double r, ...);
bool addSpline(int &tag, const std::vector<int> &pointTags, ...);
bool addBSpline(int &tag, const std::vector<int> &pointTags, ...);

// 曲面
bool addRectangle(int &tag, double x, double y, double z, double dx, double dy, ...);
bool addDisk(int &tag, double xc, double yc, double zc, double rx, double ry);
bool addPlaneSurface(int &tag, const std::vector<int> &wireTags, ...);

// 实体
bool addSphere(int &tag, double xc, double yc, double zc, double r, ...);
bool addBox(int &tag, double x, double y, double z, double dx, double dy, double dz);
bool addCylinder(int &tag, double x, double y, double z, double dx, double dy, double dz, double r, ...);
```

#### 3.2.3 布尔运算

```cpp
// 布尔并集
bool booleanUnion(int tag,
                  const std::vector<std::pair<int, int>> &objectDimTags,
                  const std::vector<std::pair<int, int>> &toolDimTags,
                  std::vector<std::pair<int, int>> &outDimTags, ...);

// 布尔交集
bool booleanIntersection(int tag, ...);

// 布尔差集
bool booleanDifference(int tag, ...);

// 碎片化（保留所有片段）
bool booleanFragments(int tag, ...);
```

#### 3.2.4 变换操作

```cpp
bool translate(const std::vector<std::pair<int, int>> &dimTags,
               double dx, double dy, double dz);

bool rotate(const std::vector<std::pair<int, int>> &dimTags,
            double x, double y, double z,
            double ax, double ay, double az, double angle);

bool dilate(const std::vector<std::pair<int, int>> &dimTags,
            double x, double y, double z,
            double a, double b, double c);

bool copy(const std::vector<std::pair<int, int>> &dimTags,
          std::vector<std::pair<int, int>> &outDimTags);
```

#### 3.2.5 同步机制

```cpp
// 核心同步函数 (GModelIO_OCC.cpp:5483)
void synchronize(GModel *model);
```

### 3.3 OCCVertex/Edge/Face/Region包装类

这些类是Gmsh几何实体（GVertex/GEdge/GFace/GRegion）的OCC实现。

#### 3.3.1 OCCVertex (src/geo/OCCVertex.h)

```cpp
class OCCVertex : public GVertex {
protected:
    TopoDS_Vertex _v;    // OCC顶点对象
    double _x, _y, _z;   // 坐标缓存

public:
    OCCVertex(GModel *m, TopoDS_Vertex v, int num);

    // 几何查询
    virtual double x() const { return _x; }
    virtual double y() const { return _y; }
    virtual double z() const { return _z; }
    virtual GPoint point() const;

    // OCC访问
    TopoDS_Vertex getShape() { return _v; }

    // 几何类型
    virtual ModelType geomType() const { return OpenCascadeModel; }
};
```

#### 3.3.2 OCCEdge (src/geo/OCCEdge.h)

```cpp
class OCCEdge : public GEdge {
protected:
    TopoDS_Edge _c;                    // OCC边对象
    double _s0, _s1;                   // 参数范围
    Handle(Geom_Curve) _curve;         // 3D曲线
    Handle(Geom2d_Curve) _curve2d;     // 2D曲线（面上的）

public:
    OCCEdge(GModel *m, TopoDS_Edge c, int num, GVertex *v1, GVertex *v2);

    // 参数化
    virtual Range<double> parBounds(int i) const;
    virtual GPoint point(double p) const;
    virtual SVector3 firstDer(double par) const;
    virtual SVector3 secondDer(double par) const;
    virtual double curvature(double par) const;

    // 几何类型识别
    virtual GeomType geomType() const;  // Line, Circle, Ellipse, BSpline...
    virtual bool degenerate(int) const;

    // OCC访问
    TopoDS_Edge getShape() { return _c; }
};
```

#### 3.3.3 OCCFace (src/geo/OCCFace.h)

```cpp
class OCCFace : public GFace {
protected:
    TopoDS_Face _s;                       // OCC面对象
    Handle(Geom_Surface) _occface;        // 几何曲面
    BRepAdaptor_Surface _sf;              // 曲面适配器
    bool _periodic[2];                     // UV方向周期性
    double _period[2];                     // 周期值

public:
    OCCFace(GModel *m, TopoDS_Face s, int num);

    // 参数化
    virtual Range<double> parBounds(int i) const;
    virtual GPoint point(double u, double v) const;
    virtual GPoint closestPoint(const SPoint3 &pt, const double uv[2]) const;

    // 微分几何
    virtual Pair<SVector3, SVector3> firstDer(const SPoint2 &param) const;
    virtual void secondDer(const SPoint2 &param, ...) const;
    virtual SVector3 normal(const SPoint2 &param) const;
    virtual double curvature(const SPoint2 &param, ...) const;

    // 几何类型识别
    virtual GeomType geomType() const;  // Plane, Cylinder, Cone, Sphere, BSpline...

    // OCC访问
    TopoDS_Face getShape() { return _s; }
};
```

#### 3.3.4 OCCRegion (src/geo/OCCRegion.h)

```cpp
class OCCRegion : public GRegion {
protected:
    TopoDS_Solid _s;    // OCC实体对象

public:
    OCCRegion(GModel *m, TopoDS_Solid s, int num);

    // 几何查询
    virtual bool containsPoint(const SPoint3 &pt) const;
    virtual GeomType geomType() const { return OpenCascadeModel; }

    // OCC访问
    TopoDS_Solid getShape() { return _s; }
};
```

### 3.4 属性管理系统（OCCAttributesRTree）

`OCCAttributesRTree`使用R树数据结构高效管理几何属性：

```cpp
// src/geo/OCCAttributes.h

class OCCAttributes {
    int _dim;                        // 维度 (0=顶点, 1=边, 2=面, 3=体)
    TopoDS_Shape _shape;             // 关联的OCC形状
    double _meshSize;                // 网格尺寸
    ExtrudeParams *_extrude;         // 挤出参数
    std::string _label;              // 标签
    std::vector<double> _color;      // RGBA颜色
};

class OCCAttributesRTree {
    RTree<OCCAttributes *, double, 3, double> *_rtree[4];  // 4个维度的R树
    std::vector<OCCAttributes *> _all;                      // 所有属性
    double _tol;                                            // 容差

public:
    // 插入属性
    void insert(OCCAttributes *attr);

    // 查询方法
    double getMeshSize(int dim, TopoDS_Shape shape);
    ExtrudeParams *getExtrudeParams(int dim, TopoDS_Shape shape, ...);
    void getLabels(int dim, TopoDS_Shape shape, std::vector<std::string> &labels);
    bool getColor(int dim, TopoDS_Shape shape, unsigned int &color, ...);
    void getSimilarShapes(int dim, TopoDS_Shape shape, ...);
};
```

**工作原理**:
1. 属性按形状的包围盒中心点存储在R树中
2. 查询时，先用R树找到可能匹配的属性
3. 再用`isSame()`或`isSimilar()`精确匹配形状

---

## 4. 关键调试入口点

### 4.1 IGES/STEP导入流程

#### 4.1.1 顶层入口函数

```cpp
// src/geo/GModelIO_OCC.cpp

// IGES导入入口 (第6783行)
int GModel::readOCCIGES(const std::string &fn)
{
    if(!_occ_internals) _occ_internals = new OCC_Internals;
    std::vector<std::pair<int, int>> outDimTags;
    _occ_internals->importShapes(fn, false, outDimTags, "iges");
    _occ_internals->synchronize(this);
    return 1;
}

// STEP导入入口 (第6774行)
int GModel::readOCCSTEP(const std::string &fn)
{
    if(!_occ_internals) _occ_internals = new OCC_Internals;
    std::vector<std::pair<int, int>> outDimTags;
    _occ_internals->importShapes(fn, false, outDimTags, "step");
    _occ_internals->synchronize(this);
    return 1;
}
```

#### 4.1.2 importShapes核心函数

```cpp
// GModelIO_OCC.cpp:4790
bool OCC_Internals::importShapes(const std::string &fileName,
                                  bool highestDimOnly,
                                  std::vector<std::pair<int, int>> &outDimTags,
                                  const std::string &format)
{
    // 第4795行: 检查文件是否存在
    if(!StatFile(fileName)) {
        Msg::Error("File '%s' does not exist", fileName.c_str());
        return false;
    }

    // 第4804行: 解析文件扩展名
    std::vector<std::string> split = SplitFileName(fileName);

    TopoDS_Shape result;

    try {
        // STEP格式处理 (第4808-4831行)
        if(format == "step" || split[2] == ".step" || split[2] == ".stp" || ...) {
            setTargetUnit(CTX::instance()->geom.occTargetUnit);
            Interface_Static::SetIVal("read.step.ideas", 1);
            Interface_Static::SetIVal("read.step.nonmanifold", 1);

#if defined(HAVE_OCC_CAF)
            STEPCAFControl_Reader cafreader;
            if(cafreader.ReadFile(fileName.c_str()) != IFSelect_RetDone) {
                Msg::Error("Could not read file '%s'", fileName.c_str());
                return false;
            }
            if(CTX::instance()->geom.occImportLabels)
                readAttributes(_attributes, cafreader, "STEP-XCAF");
            reader = cafreader.ChangeReader();
#else
            STEPControl_Reader reader;
            if(reader.ReadFile(fileName.c_str()) != IFSelect_RetDone) {
                Msg::Error("Could not read file '%s'", fileName.c_str());
                return false;
            }
#endif
            reader.NbRootsForTransfer();
            reader.TransferRoots();
            result = reader.OneShape();
        }
        // IGES格式处理 (第4834-4858行)
        else if(format == "iges" || split[2] == ".iges" || split[2] == ".igs" || ...) {
            setTargetUnit(CTX::instance()->geom.occTargetUnit);

#if defined(HAVE_OCC_CAF)
            IGESCAFControl_Reader reader;
            if(reader.ReadFile(fileName.c_str()) != IFSelect_RetDone) {
                Msg::Error("Could not read file '%s'", fileName.c_str());
                return false;
            }
            if(CTX::instance()->geom.occImportLabels)
                readAttributes(_attributes, reader, "IGES-XCAF");
#else
            IGESControl_Reader reader;
            if(reader.ReadFile(fileName.c_str()) != IFSelect_RetDone) {
                Msg::Error("Could not read file '%s'", fileName.c_str());
                return false;
            }
#endif
            reader.NbRootsForTransfer();
            reader.TransferRoots();
            result = reader.OneShape();
        }
    }
    catch(Standard_Failure &err) {
        // 第4860行: OCC异常捕获
        Msg::Error("OpenCASCADE exception %s", err.GetMessageString());
        return false;
    }

    // 第4865行: 清理拓扑
    BRepTools::Clean(result);

    // 第4867-4872行: 形状修复
    _healShape(
        result,
        CTX::instance()->geom.tolerance,
        CTX::instance()->geom.occFixDegenerated,
        CTX::instance()->geom.occFixSmallEdges,
        CTX::instance()->geom.occFixSmallFaces,
        CTX::instance()->geom.occSewFaces,
        CTX::instance()->geom.occMakeSolids,
        CTX::instance()->geom.occScaling
    );

    // 第4875行: 绑定拓扑实体到标签
    _multiBind(result, -1, outDimTags, highestDimOnly, true);

    return true;
}
```

#### 4.1.3 调用流程图

```
GModel::readOCCIGES(fileName)
    │
    ├─► new OCC_Internals() [如果是首次]
    │
    └─► OCC_Internals::importShapes(fileName, false, outDimTags, "iges")
        │
        ├─► StatFile(fileName)              // 检查文件存在性
        │
        ├─► setTargetUnit(targetUnit)       // 设置目标单位
        │
        ├─► IGESControl_Reader::ReadFile()  // 读取IGES文件
        │       │
        │       └─► IFSelect_RetDone ?      // 检查读取结果
        │
        ├─► reader.NbRootsForTransfer()     // 准备转换
        │
        ├─► reader.TransferRoots()          // 执行转换
        │
        ├─► reader.OneShape()               // 获取结果Shape
        │
        ├─► BRepTools::Clean(result)        // 清理拓扑
        │
        ├─► _healShape(...)                 // 形状修复
        │       │
        │       ├─► 缩放 (如果需要)
        │       ├─► 修复退化边/面
        │       ├─► 修复小边
        │       └─► 缝合面
        │
        └─► _multiBind(...)                 // 绑定标签
                │
                └─► 遍历所有子形状，分配标签
    │
    └─► OCC_Internals::synchronize(this)
        │
        ├─► 删除_toRemove中的实体
        │
        ├─► 重建Shape映射表
        │
        ├─► 创建OCCVertex/Edge/Face/Region
        │
        └─► 恢复属性（网格尺寸、标签、颜色）
```

### 4.2 形状修复（_healShape）

```cpp
// GModelIO_OCC.cpp:5836
static void _healShape(TopoDS_Shape &shape,
                       double tolerance,
                       bool fixDegenerated,
                       bool fixSmallEdges,
                       bool fixSmallFaces,
                       bool sewFaces,
                       bool makeSolids,
                       double scaling)
{
    // 第5841行: 缩放操作
    if(scaling != 1.0) {
        gp_Trsf t;
        t.SetScaleFactor(scaling);
        BRepBuilderAPI_Transform trans(shape, t, Standard_True);
        shape = trans.Shape();
    }

    // 第5854行: 开始计时
    double t1 = Cpu(), w1 = TimeOfDay();

    // 第5866-5869行: 统计形状信息（调试用）
    int nrso = ..., nrsh = ..., nrf = ..., nrw = ..., nre = ..., nrv = ...;

    // 第5879-5930行: 修复退化边和面
    if(fixDegenerated) {
        TopExp_Explorer exp;
        for(exp.Init(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(exp.Current());
            if(BRep_Tool::Degenerated(edge)) {
                // 删除退化边
            }
        }
        // ShapeFix_Face修复
    }

    // 第5933-5990行: 修复小边
    if(fixSmallEdges) {
        Handle(ShapeFix_Wire) sfw;
        for(...) {
            sfw->FixSmall(Standard_True, tolerance);
            // 移除短边
        }
    }

    // 第6006-6026行: 焊接线框
    Handle(ShapeFix_Wireframe) sfwf = new ShapeFix_Wireframe(shape);
    sfwf->ModeDropSmallEdges() = Standard_True;
    sfwf->SetPrecision(tolerance);
    sfwf->FixSmallEdges();
    sfwf->FixWireGaps();
    shape = sfwf->Shape();
}
```

**关键调试变量**:
- `nrso`: 实体(Solid)数量
- `nrsh`: 壳体(Shell)数量
- `nrf`: 面(Face)数量
- `nrw`: 线框(Wire)数量
- `nre`: 边(Edge)数量
- `nrv`: 顶点(Vertex)数量

### 4.3 模型同步（synchronize）

```cpp
// GModelIO_OCC.cpp:5483
void OCC_Internals::synchronize(GModel *model)
{
    Msg::Debug("Syncing OCC_Internals with GModel");

    // 阶段1: 清除已删除的实体 (第5489-5505行)
    if(_toRemove.size() || _toPreserve.size()) {
        std::vector<std::pair<int, int>> toRemove(_toRemove.begin(), _toRemove.end());
        // 按倒序维度排序（高维先删）
        std::sort(toRemove.begin(), toRemove.end(), sortByInvDim);
        std::vector<GEntity *> removed;
        model->remove(toRemove, removed);
        // ...
    }

    // 阶段2: 重建Shape地图 (第5509-5520行)
    _vmap.Clear(); _emap.Clear(); _fmap.Clear(); _somap.Clear();
    // ...
    TopTools_DataMapIteratorOfDataMapOfIntegerShape exp0(_tagVertex);
    for(; exp0.More(); exp0.Next()) {
        _addShapeToMaps(exp0.Value());
    }
    // 对Edge, Face, Solid重复

    // 阶段3: 创建顶点实体 (第5523-5546行)
    for(int i = 1; i <= _vmap.Extent(); i++) {
        TopoDS_Vertex vertex = TopoDS::Vertex(_vmap(i));
        GVertex *occv = getVertexForOCCShape(model, vertex);
        if(!occv) {
            int tag = (_vertexTag.IsBound(vertex)) ?
                       _vertexTag.Find(vertex) : ++vTagMax;
            occv = new OCCVertex(model, vertex, tag);
            model->add(occv);
            Msg::Debug("Binding unbound OpenCASCADE point to tag %d", tag);
        }
        // 恢复网格尺寸属性
        double lc = _attributes->getMeshSize(0, vertex);
        if(lc != MAX_LC) occv->setPrescribedMeshSizeAtVertex(lc);
        // 恢复标签和颜色
    }

    // 阶段4: 创建边实体 (第5548-5572行)
    for(int i = 1; i <= _emap.Extent(); i++) {
        TopoDS_Edge edge = TopoDS::Edge(_emap(i));
        GEdge *occe = getEdgeForOCCShape(model, edge);
        if(!occe) {
            int tag = (_edgeTag.IsBound(edge)) ?
                       _edgeTag.Find(edge) : ++eTagMax;
            // 找到起点和终点
            GVertex *v1 = getVertexForOCCShape(model, TopoDS::Vertex(...));
            GVertex *v2 = getVertexForOCCShape(model, TopoDS::Vertex(...));
            occe = new OCCEdge(model, edge, tag, v1, v2);
            model->add(occe);
        }
        // 恢复属性
    }

    // 阶段5: 创建面实体 (第5574-5602行)
    // 类似逻辑...

    // 阶段6: 创建体实体 (第5604-5631行)
    // 类似逻辑...

    // 重置变化标记
    _changed = false;
    _toRemove.clear();
    _toPreserve.clear();
}
```

---

## 5. 布尔运算调试

### 5.1 布尔运算类型与入口函数

```cpp
// GModelIO_OCC.cpp

// 布尔运算主函数 (第3834行)
bool OCC_Internals::booleanOperator(
    int tag,
    BooleanOperator op,
    const std::vector<std::pair<int, int>> &objectDimTags,
    const std::vector<std::pair<int, int>> &toolDimTags,
    std::vector<std::pair<int, int>> &outDimTags,
    std::vector<std::vector<std::pair<int, int>>> &outDimTagsMap,
    bool removeObject,
    bool removeTool)
{
    // ...
}

// 公共接口函数
bool booleanUnion(int tag, ...);         // 并集
bool booleanIntersection(int tag, ...);  // 交集
bool booleanDifference(int tag, ...);    // 差集
bool booleanFragments(int tag, ...);     // 碎片化
```

### 5.2 布尔运算选项设置

```cpp
// GModelIO_OCC.cpp:3787
template <class T>
static void _setBooleanOptions(T &algo)
{
    // 并行计算
    algo.SetRunParallel(CTX::instance()->geom.occParallel);

    // 模糊值（容差）
    algo.SetFuzzyValue(CTX::instance()->geom.toleranceBoolean);

    // OCC 7.1+: 非破坏性操作
#if OCC_VERSION_HEX > 0x070100
    algo.SetNonDestructive(true);
#endif

    // OCC 7.2+: 粘合模式
#if OCC_VERSION_HEX > 0x070200
    if(CTX::instance()->geom.occBooleanGlue == 1)
        algo.SetGlue(BOPAlgo_GlueShift);
    else if(CTX::instance()->geom.occBooleanGlue == 2)
        algo.SetGlue(BOPAlgo_GlueFull);
#endif

    // OCC 7.3+: 检查反转
#if OCC_VERSION_HEX > 0x070300
    algo.SetCheckInverted(true);
#endif
}
```

### 5.3 布尔运算实现详解

```cpp
// GModelIO_OCC.cpp:3834
bool OCC_Internals::booleanOperator(...)
{
    // 第3844-3878行: 验证输入实体
    for(auto &dimTag : objectDimTags) {
        if(!_isBound(dimTag.first, dimTag.second)) {
            Msg::Error("Unknown OpenCASCADE entity of dimension %d with tag %d",
                       dimTag.first, dimTag.second);
            return false;
        }
        objectShapes.Append(_find(dimTag.first, dimTag.second));
    }
    // toolDimTags同理

    // 第3884-4035行: 执行布尔运算
    try {
        TopoDS_Shape result;

        switch(op) {
        case Union: {  // 第3887行
            BRepAlgoAPI_Fuse fuse;
            fuse.SetArguments(objectShapes);
            fuse.SetTools(toolShapes);
            _setBooleanOptions(fuse);
            fuse.Build();
            if(!fuse.IsDone()) {
                _printBooleanErrors(fuse, "Fuse");
                return false;
            }
            _printBooleanWarnings(fuse, "Fuse");
            result = fuse.Shape();
            break;
        }

        case Intersection: {  // 第3932行
            BRepAlgoAPI_Common common;
            // 类似设置...
            break;
        }

        case Difference: {  // 第3965行
            BRepAlgoAPI_Cut cut;
            // 类似设置...
            break;
        }

        case Fragments: {  // 第4001行
            BRepAlgoAPI_BuilderAlgo fragments;
            fragments.SetArguments(allShapes);
            _setBooleanOptions(fragments);
            fragments.Build();
            // ...
            break;
        }
        }

        // 后处理...
    }
    catch(Standard_Failure &err) {
        // 第4036行: 异常捕获
        Msg::Error("OpenCASCADE exception %s", err.GetMessageString());
        return false;
    }
}
```

### 5.4 布尔运算错误处理

```cpp
// GModelIO_OCC.cpp:3809
template <class T>
static bool _printBooleanErrors(T &algo, const std::string &what)
{
#if OCC_VERSION_HEX > 0x070200
    std::ostringstream os;
    algo.DumpErrors(os);
    Msg::Error("OpenCASCADE %s failed:\n%s", what.c_str(), os.str().c_str());
#else
    Msg::Error("OpenCASCADE %s failed", what.c_str());
#endif
    return false;
}

// GModelIO_OCC.cpp:3822
template <class T>
static void _printBooleanWarnings(T &algo, const std::string &what)
{
#if OCC_VERSION_HEX > 0x070200
    if(algo.HasWarnings()) {
        std::ostringstream os;
        algo.DumpWarnings(os);
        Msg::Warning("OpenCASCADE %s generated warnings:\n%s",
                     what.c_str(), os.str().c_str());
    }
#endif
}
```

### 5.5 常见布尔运算失败原因

| 错误类型 | 可能原因 | 解决方案 |
|---------|---------|---------|
| `BOP_SelfIntersect` | 自相交几何 | 检查输入形状的有效性 |
| `BOP_TooSmallEdge` | 边太短 | 增加容差或简化几何 |
| `BOP_NotValid` | 无效形状 | 使用_healShape修复 |
| `BOP_BadTopology` | 拓扑错误 | 检查面的方向和连接性 |

---

## 6. 错误处理与日志系统

### 6.1 Msg日志系统详解

#### 6.1.1 核心API

```cpp
// src/common/GmshMessage.h

class Msg {
public:
    // 消息输出
    static void Error(const char *fmt, ...);      // 错误（红色）
    static void Warning(const char *fmt, ...);    // 警告（黄色）
    static void Info(const char *fmt, ...);       // 信息（白色）
    static void Direct(const char *fmt, ...);     // 直接输出
    static void Debug(const char *fmt, ...);      // 调试（仅verbosity=99）

    // Verbosity管理
    static void SetVerbosity(int val);            // 设置冗长级别
    static int GetVerbosity();                    // 获取冗长级别

    // 日志文件
    static void SetLogFileName(const std::string &name);

    // 进度显示
    static void StartProgressMeter(int ntotal);
    static void ProgressMeter(int n, bool log, const char *fmt, ...);
    static void StopProgressMeter();
};
```

#### 6.1.2 Verbosity级别定义

```
级别 0: 仅致命错误 (Fatal errors only)
级别 1: + 错误 (+ Errors)
级别 2: + 警告 (+ Warnings)
级别 3: + 直接输出 (+ Direct output)
级别 4: + 信息 (+ Info messages)
级别 5: + 状态栏 (+ Status bar) [正常模式]
级别 99: 调试模式 (Debug mode) [启用Debug()输出]
```

#### 6.1.3 命令行使用

```bash
# 静默模式
gmsh -v 0 file.geo

# 正常模式（默认）
gmsh -v 5 file.geo

# 调试模式
gmsh -v 99 file.geo

# 输出到日志文件
gmsh -logfile debug.log -v 99 file.geo
```

### 6.2 OCC异常捕获机制

#### 6.2.1 标准异常捕获模式

```cpp
try {
    // OCC操作
    IGESControl_Reader reader;
    reader.ReadFile(fileName.c_str());
    // ...
}
catch(Standard_Failure &err) {
    Msg::Error("OpenCASCADE exception %s", err.GetMessageString());
    return false;
}
```

#### 6.2.2 常见OCC异常类型

| 异常类 | 描述 |
|-------|------|
| `Standard_Failure` | 通用异常基类 |
| `Standard_ConstructionError` | 构造失败 |
| `Standard_NoSuchObject` | 对象不存在 |
| `Standard_RangeError` | 范围错误 |
| `Standard_NullObject` | 空对象访问 |
| `StdFail_NotDone` | 操作未完成 |

#### 6.2.3 OCC版本特定的错误输出

```cpp
// OCC 7.2+ 支持详细错误转储
#if OCC_VERSION_HEX > 0x070200
    std::ostringstream os;
    algo.DumpErrors(os);
    Msg::Error("Details:\n%s", os.str().c_str());
#endif
```

### 6.3 调试输出示例

```cpp
// 在OCC_Internals中常见的调试输出

// 信息级别
Msg::Info("Cannot bind existing OpenCASCADE point %d to second tag %d",
          tag1, tag2);

// 调试级别（需要 -v 99）
Msg::Debug("Circle through point sense %d: Alpha1=%g, Alpha2=%g, AlphaC=%g",
           sense, alpha1, alpha2, alphaC);

Msg::Debug("Syncing OCC_Internals with GModel");

Msg::Debug("Binding unbound OpenCASCADE point to tag %d", tag);

Msg::Debug("BOOL (%d,%d) deleted", dim, tag);
```

---

## 7. 断点设置速查表

### 7.1 按功能模块分类

#### 7.1.1 文件导入断点

| 断点位置 | 行号 | 用途 |
|---------|------|------|
| `GModel::readOCCIGES` | 6783 | IGES导入入口 |
| `GModel::readOCCSTEP` | 6774 | STEP导入入口 |
| `GModel::readOCCBREP` | 6519 | BREP导入入口 |
| `OCC_Internals::importShapes` | 4790 | 核心导入函数 |
| `IGESControl_Reader::ReadFile` | 4846 | IGES文件读取 |
| `STEPControl_Reader::ReadFile` | 4810 | STEP文件读取 |
| `catch(Standard_Failure)` | 4860 | OCC异常捕获 |

#### 7.1.2 形状修复断点

| 断点位置 | 行号 | 用途 |
|---------|------|------|
| `_healShape` | 5836 | 修复入口 |
| `scaling` | 5841 | 缩放操作 |
| `fixDegenerated` | 5879 | 修复退化边/面 |
| `ShapeFix_Face` | 5896 | 面修复 |
| `fixSmallEdges` | 5933 | 修复小边 |
| `ShapeFix_Wire::FixSmall` | 5951 | 线框小边修复 |
| `ShapeFix_Wireframe` | 6006 | 线框焊接 |

#### 7.1.3 同步断点

| 断点位置 | 行号 | 用途 |
|---------|------|------|
| `OCC_Internals::synchronize` | 5483 | 同步入口 |
| `model->remove` | 5497 | 删除旧实体 |
| `_addShapeToMaps` | 5512 | 重建映射 |
| `new OCCVertex` | 5535 | 创建顶点 |
| `new OCCEdge` | 5559 | 创建边 |
| `new OCCFace` | 5586 | 创建面 |
| `new OCCRegion` | 5615 | 创建体 |

#### 7.1.4 布尔运算断点

| 断点位置 | 行号 | 用途 |
|---------|------|------|
| `booleanOperator` | 3834 | 运算入口 |
| `_isBound` 检查 | 3847 | 验证输入 |
| `BRepAlgoAPI_Fuse` | 3887 | 并集操作 |
| `BRepAlgoAPI_Common` | 3932 | 交集操作 |
| `BRepAlgoAPI_Cut` | 3965 | 差集操作 |
| `BRepAlgoAPI_BuilderAlgo` | 4001 | 碎片化 |
| `catch` | 4036 | 异常捕获 |

### 7.2 GDB断点设置命令

```bash
# 文件导入
break GModelIO_OCC.cpp:6783
break GModelIO_OCC.cpp:4790
break GModelIO_OCC.cpp:4860

# 形状修复
break GModelIO_OCC.cpp:5836
break GModelIO_OCC.cpp:5879
break GModelIO_OCC.cpp:5933

# 同步
break GModelIO_OCC.cpp:5483
break GModelIO_OCC.cpp:5535
break GModelIO_OCC.cpp:5559

# 布尔运算
break GModelIO_OCC.cpp:3834
break GModelIO_OCC.cpp:3887
break GModelIO_OCC.cpp:4036

# 按函数名
break GModel::readOCCIGES
break OCC_Internals::importShapes
break OCC_Internals::synchronize
break OCC_Internals::booleanOperator
```

### 7.3 LLDB断点设置命令

```bash
# 文件导入
breakpoint set -f GModelIO_OCC.cpp -l 6783
breakpoint set -f GModelIO_OCC.cpp -l 4790
breakpoint set -f GModelIO_OCC.cpp -l 4860

# 按函数名
breakpoint set -n GModel::readOCCIGES
breakpoint set -n OCC_Internals::importShapes
breakpoint set -n OCC_Internals::synchronize

# 条件断点
breakpoint set -f GModelIO_OCC.cpp -l 4846 -c 'format == "iges"'
```

---

## 8. 常见调试场景实战

### 8.1 IGES文件导入失败调试

#### 8.1.1 症状
```
Error   : Could not read file 'xxx.igs'
```
或
```
Error   : OpenCASCADE exception [some message]
```

#### 8.1.2 调试步骤

```bash
# 1. 启用调试模式运行
gdb ./gmsh
(gdb) break GModelIO_OCC.cpp:4846      # IGESControl_Reader
(gdb) break GModelIO_OCC.cpp:4860      # 异常捕获
(gdb) run -v 99 problem.igs

# 2. 检查文件读取结果
(gdb) print reader.ReadFile(fileName.c_str())  # 应返回 IFSelect_RetDone

# 3. 如果进入异常处理
(gdb) print err.GetMessageString()     # 查看异常详情

# 4. 检查形状转换
(gdb) next                              # 执行 NbRootsForTransfer
(gdb) print reader.NbRootsForTransfer() # 查看根实体数量
```

#### 8.1.3 常见问题与解决

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 返回 `IFSelect_RetFail` | 文件格式错误 | 检查IGES文件有效性 |
| `NbRootsForTransfer()=0` | 无可导入实体 | 检查文件内容 |
| 转换异常 | 几何问题 | 使用CAD软件修复 |

### 8.2 布尔运算结果异常调试

#### 8.2.1 症状
```
Error   : OpenCASCADE Fuse failed
Warning : OpenCASCADE Fuse generated warnings: [details]
```

#### 8.2.2 调试步骤

```bash
# 1. 设置断点
(gdb) break GModelIO_OCC.cpp:3834      # booleanOperator入口
(gdb) break GModelIO_OCC.cpp:3897      # fuse.IsDone()检查
(gdb) break GModelIO_OCC.cpp:4036      # 异常捕获

# 2. 检查输入形状
(gdb) run -v 99 problem.geo
(gdb) print objectShapes.Size()         # 对象数量
(gdb) print toolShapes.Size()           # 工具数量

# 3. 检查布尔运算结果
(gdb) print fuse.IsDone()              # 是否成功
(gdb) print fuse.HasErrors()           # 是否有错误
(gdb) print fuse.HasWarnings()         # 是否有警告

# 4. 查看详细错误（OCC 7.2+）
# 在代码中临时添加:
# std::ostringstream os; fuse.DumpErrors(os);
# std::cout << os.str() << std::endl;
```

#### 8.2.3 常见问题与解决

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 自相交 | 输入形状自交 | 修复输入几何 |
| 容差问题 | 形状间距太小 | 调整 `toleranceBoolean` |
| 无效结果 | 结果形状退化 | 检查输入几何有效性 |

### 8.3 网格生成失败调试（OCC几何问题）

#### 8.3.1 症状
```
Error   : Surface mesh generation failed
Warning : Meshing curve X failed
```

#### 8.3.2 检查几何有效性

```bash
# 1. 检查曲面类型
(gdb) break OCCFace.cpp:95             # geomType()
(gdb) run -v 99 problem.geo

# 2. 检查参数范围
(gdb) print _occface->Bounds(umin, umax, vmin, vmax)
(gdb) print umin, umax, vmin, vmax

# 3. 检查曲率
(gdb) break OCCFace.cpp:200            # curvature()
```

#### 8.3.3 常见问题与解决

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 奇异点 | 曲面有奇异点 | 分割曲面 |
| 退化边 | 边长度为0 | 使用_healShape修复 |
| 自交曲面 | 曲面自相交 | CAD软件修复 |

### 8.4 性能瓶颈定位

#### 8.4.1 使用Profiling构建

```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DENABLE_PROFILE=1 \
      ..
make -j8
```

#### 8.4.2 使用perf分析 (Linux)

```bash
# 记录性能数据
perf record -g ./gmsh -v 5 large_model.stp

# 查看报告
perf report
```

#### 8.4.3 使用Instruments (macOS)

```bash
# Time Profiler
instruments -t "Time Profiler" ./gmsh -v 5 large_model.stp
```

#### 8.4.4 关键性能点

| 操作 | 位置 | 优化建议 |
|------|------|---------|
| 文件导入 | `importShapes` | 减少修复步骤 |
| 形状修复 | `_healShape` | 关闭不必要的修复 |
| 布尔运算 | `booleanOperator` | 启用并行：`occParallel=1` |
| 同步 | `synchronize` | 批量操作后再同步 |

---

## 9. 调试工具与技巧

### 9.1 OCC DRAW调试工具

OpenCASCADE提供了DRAW测试工具，可用于独立调试OCC操作：

```bash
# 启动DRAW
/path/to/occ/bin/DRAWEXE

# 加载IGES文件
Draw[1]> pload ALL
Draw[2]> igesread problem.igs a *

# 检查形状
Draw[3]> checkshape a

# 查看形状信息
Draw[4]> nbshapes a

# 显示形状
Draw[5]> vinit
Draw[6]> vdisplay a
Draw[7]> vfit
```

### 9.2 形状可视化检查

#### 9.2.1 在Gmsh中查看几何信息

```bash
# 打印几何统计
gmsh problem.igs -0 -v 5
# 输出包含顶点、边、面、体的数量

# 保存为BREP格式以便后续分析
gmsh problem.igs -0 -o output.brep
```

#### 9.2.2 使用CAD Exchanger检查

免费的在线工具：https://cadexchanger.com/

### 9.3 内存泄漏检测

#### 9.3.1 使用Valgrind (Linux)

```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         ./gmsh -v 5 test.geo -0
```

#### 9.3.2 使用AddressSanitizer

```bash
# CMake配置
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
      ..

# 运行
ASAN_OPTIONS=detect_leaks=1 ./gmsh -v 5 test.geo
```

### 9.4 调试技巧汇总

#### 9.4.1 快速定位问题

1. **先用 `-v 99` 查看完整日志**
2. **使用 `-logfile debug.log` 保存日志**
3. **在异常捕获点设置断点**
4. **使用条件断点缩小范围**

#### 9.4.2 分析OCC形状

```cpp
// 临时添加调试代码
#include <BRepTools.hxx>

// 输出形状到文件
BRepTools::Write(shape, "debug_shape.brep");

// 打印形状类型
std::cout << "Shape type: " << shape.ShapeType() << std::endl;
// TopAbs_COMPOUND=0, TopAbs_COMPSOLID=1, TopAbs_SOLID=2,
// TopAbs_SHELL=3, TopAbs_FACE=4, TopAbs_WIRE=5,
// TopAbs_EDGE=6, TopAbs_VERTEX=7
```

#### 9.4.3 追踪标签映射

```cpp
// 在synchronize中添加调试输出
Msg::Debug("Tag mapping: Vertex %d -> Shape %p", tag, &vertex);
Msg::Debug("Shape mapping: Shape %p -> Tag %d", &shape, _find(dim, shape));
```

---

## 10. 附录

### 10.1 OCC版本兼容性

| OCC版本 | 特性 |
|---------|------|
| 6.9.1 | 最低要求版本 |
| 7.1.0 | 添加 NonDestructive 选项 |
| 7.2.0 | 添加 DumpErrors/Warnings, Glue模式 |
| 7.3.0 | 添加 CheckInverted |
| 7.4.0 | 添加 SimplifyResult |
| 7.7.0 | 更新 Progress 指示器 |
| 7.8.0 | DataExchange模块重组 |

### 10.2 关键配置参数

| 参数 | 默认值 | 说明 |
|------|-------|------|
| `Geometry.OCCTargetUnit` | mm | 目标单位 |
| `Geometry.Tolerance` | 1e-8 | 几何容差 |
| `Geometry.ToleranceBoolean` | 0.0 | 布尔运算容差 |
| `Geometry.OCCFixDegenerated` | 1 | 修复退化元素 |
| `Geometry.OCCFixSmallEdges` | 1 | 修复小边 |
| `Geometry.OCCFixSmallFaces` | 1 | 修复小面 |
| `Geometry.OCCSewFaces` | 1 | 缝合面 |
| `Geometry.OCCMakeSolids` | 1 | 创建实体 |
| `Geometry.OCCParallel` | 0 | 并行计算 |
| `Geometry.OCCBooleanGlue` | 0 | 粘合模式 |

### 10.3 常用OCC头文件

```cpp
// STEP/IGES读写
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>

// CAF支持
#include <STEPCAFControl_Reader.hxx>
#include <IGESCAFControl_Reader.hxx>

// 几何操作
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>

// 布尔运算
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_BuilderAlgo.hxx>

// 形状修复
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeFix_Wireframe.hxx>

// 拓扑查询
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>

// 几何对象
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Geom2d_Curve.hxx>
```

### 10.4 参考资源

- **Gmsh官方文档**: https://gmsh.info/doc/texinfo/gmsh.html
- **OpenCASCADE文档**: https://dev.opencascade.org/doc/overview/html/
- **Gmsh GitLab**: https://gitlab.onelab.info/gmsh/gmsh
- **问题报告**: https://gitlab.onelab.info/gmsh/gmsh/issues

---

**文档编写者**: renyong
**最后更新**: 2024年12月
