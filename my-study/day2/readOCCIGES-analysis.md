# GModel::readOCCIGES 方法详解

## 1. 方法概述

**文件位置**: `src/geo/GModelIO_OCC.cpp:6783`

**功能**: 使用 OpenCASCADE 几何内核读取 IGES 格式的 CAD 文件，并将其导入到 Gmsh 的 GModel 几何模型中。

**IGES 格式说明**: IGES (Initial Graphics Exchange Specification) 是工业界通用的 CAD 数据交换标准，支持从 SolidWorks、CATIA、Pro/E、UG、Creo 等主流 CAD 软件导出的几何模型。

---

## 2. 源代码及逐行注释

```cpp
/**
 * @brief 读取 IGES 格式 CAD 文件并导入到 GModel
 * @param fn IGES 文件路径（.igs/.iges）
 * @return 1 表示执行完成
 */
int GModel::readOCCIGES(const std::string &fn)
{
  // ============================================================
  // 第1步: 延迟初始化 OCC_Internals 对象
  // ============================================================
  // _occ_internals 是 GModel 的成员变量，类型为 OCC_Internals*
  // OCC_Internals 封装了所有与 OpenCASCADE 几何内核交互的功能
  // 采用延迟初始化模式：仅在首次需要时创建，节省资源
  if(!_occ_internals) _occ_internals = new OCC_Internals;

  // ============================================================
  // 第2步: 准备输出参数容器
  // ============================================================
  // outDimTags 用于存储导入后创建的几何实体信息
  // 每个元素是一个 pair<int, int>:
  //   - first:  维度 (0=顶点, 1=边, 2=面, 3=体)
  //   - second: 实体标签/ID
  // 示例: {(3,1), (2,1), (2,2)} 表示导入了1个体积和2个面
  std::vector<std::pair<int, int>> outDimTags;

  // ============================================================
  // 第3步: 调用 importShapes 读取 IGES 文件
  // ============================================================
  // 函数签名: importShapes(fileName, highestDimOnly, outDimTags, format)
  // 参数说明:
  //   - fn: IGES 文件的完整路径
  //   - false: highestDimOnly=false，导入所有维度的实体（不仅是最高维度）
  //   - outDimTags: 输出参数，返回所有导入实体的维度和标签
  //   - "iges": 指定文件格式
  //
  // importShapes 内部执行流程:
  //   1. 使用 IGESControl_Reader 读取 IGES 文件
  //   2. 调用 TransferRoots() 将 IGES 实体转换为 TopoDS_Shape
  //   3. 调用 BRepTools::Clean() 清理形状
  //   4. 调用 _healShape() 执行几何修复（修复退化边、小边、小面等）
  //   5. 调用 _multiBind() 将形状绑定到内部映射表
  _occ_internals->importShapes(fn, false, outDimTags, "iges");

  // ============================================================
  // 第4步: 同步 OCC 数据到 GModel
  // ============================================================
  // synchronize 将 OpenCASCADE 的几何数据同步到 Gmsh 的 GModel
  // 主要工作:
  //   1. 删除 GModel 中已在 OCC 中被删除的实体
  //   2. 遍历所有 OCC 形状，创建对应的 Gmsh 几何实体:
  //      - TopoDS_Vertex -> OCCVertex (GVertex 的子类)
  //      - TopoDS_Edge   -> OCCEdge   (GEdge 的子类)
  //      - TopoDS_Face   -> OCCFace   (GFace 的子类)
  //      - TopoDS_Solid  -> OCCRegion (GRegion 的子类)
  //   3. 建立实体之间的拓扑边界关系
  //   4. 将新创建的实体添加到 GModel 的实体列表中
  _occ_internals->synchronize(this);

  // 返回成功标志
  return 1;
}
```

---

## 3. 核心数据流程图

```
┌─────────────────────────────────────────────────────────────────┐
│                    readOCCIGES("model.igs")                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  Step 1: 初始化 OCC_Internals（延迟初始化模式）                 │
│          if(!_occ_internals) _occ_internals = new OCC_Internals │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  Step 2: importShapes() - 读取并转换 IGES 文件                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  2.1 IGESControl_Reader.ReadFile()    // 读取 IGES 文件    │ │
│  │  2.2 IGESControl_Reader.TransferRoots() // 转换为 OCC 形状 │ │
│  │  2.3 BRepTools::Clean()               // 清理形状数据      │ │
│  │  2.4 _healShape()                     // 几何修复          │ │
│  │      - 修复退化边 (degenerated edges)                      │ │
│  │      - 修复小边 (small edges)                              │ │
│  │      - 修复小面 (small faces)                              │ │
│  │      - 缝合面 (sew faces)                                  │ │
│  │  2.5 _multiBind()                     // 绑定到内部映射表  │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  Step 3: synchronize() - 同步到 GModel                          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  3.1 清理已删除的实体                                      │ │
│  │  3.2 形状类型映射:                                         │ │
│  │      TopoDS_Vertex ──► OCCVertex (dim=0)                   │ │
│  │      TopoDS_Edge   ──► OCCEdge   (dim=1)                   │ │
│  │      TopoDS_Face   ──► OCCFace   (dim=2)                   │ │
│  │      TopoDS_Solid  ──► OCCRegion (dim=3)                   │ │
│  │  3.3 建立拓扑边界关系                                      │ │
│  │  3.4 注册到 GModel 实体列表                                │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      返回 1 (成功)                              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. 关键类和数据结构

### 4.1 类型映射关系

| OpenCASCADE 类型 | Gmsh 类型   | 几何维度 | 说明           |
|------------------|-------------|----------|----------------|
| TopoDS_Vertex    | OCCVertex   | 0        | 顶点/点        |
| TopoDS_Edge      | OCCEdge     | 1        | 边/曲线        |
| TopoDS_Wire      | -           | -        | 线环(内部使用) |
| TopoDS_Face      | OCCFace     | 2        | 面/曲面        |
| TopoDS_Shell     | -           | -        | 壳(内部使用)   |
| TopoDS_Solid     | OCCRegion   | 3        | 体/实体        |

### 4.2 核心成员变量

```cpp
class GModel {
  OCC_Internals *_occ_internals;  // OpenCASCADE 内部数据管理器
  // ...
};

class OCC_Internals {
  // 形状到标签的映射
  TopTools_DataMapOfIntegerShape _tagVertex;  // 顶点映射
  TopTools_DataMapOfIntegerShape _tagEdge;    // 边映射
  TopTools_DataMapOfIntegerShape _tagFace;    // 面映射
  TopTools_DataMapOfIntegerShape _tagSolid;   // 体映射
  // ...
};
```

---

## 5. 相关方法

| 方法名 | 功能 |
|--------|------|
| `readOCCSTEP()` | 读取 STEP 格式文件 |
| `readOCCBREP()` | 读取 BREP 格式文件 |
| `writeOCCIGES()` | 导出 IGES 格式文件 |
| `importShapes()` | 通用形状导入方法 |
| `synchronize()` | OCC 到 GModel 同步 |

---

## 6. 调试断点建议

在 VSCode/CLion 中调试时，建议在以下位置设置断点：

1. **入口点**: `GModelIO_OCC.cpp:6783` - `readOCCIGES` 函数入口
2. **文件读取**: `GModelIO_OCC.cpp:4838` - `IGESControl_Reader.ReadFile()`
3. **形状转换**: `GModelIO_OCC.cpp:4852` - `reader.TransferRoots()`
4. **几何修复**: `GModelIO_OCC.cpp:4867` - `_healShape()`
5. **同步入口**: `GModelIO_OCC.cpp:5483` - `synchronize()` 函数入口

---

## 7. 使用示例

### 7.1 通过 .geo 脚本调用

```geo
// 使用 OpenCASCADE 内核
SetFactory("OpenCASCADE");

// 导入 IGES 文件
Merge "model.igs";

// 生成网格
Mesh 3;
```

### 7.2 通过 C++ API 调用

```cpp
#include "gmsh.h"

int main() {
    gmsh::initialize();
    gmsh::model::add("model");

    // 读取 IGES 文件
    gmsh::merge("model.igs");

    // 生成 3D 网格
    gmsh::model::mesh::generate(3);

    gmsh::finalize();
    return 0;
}
```

---

## 8. 常见问题

### Q1: IGES 导入后几何缺失？
**A**: 可能是几何修复过程中被移除。尝试调整修复参数：
```geo
Geometry.OCCFixDegenerated = 0;
Geometry.OCCFixSmallEdges = 0;
Geometry.OCCFixSmallFaces = 0;
```

### Q2: 导入速度很慢？
**A**: IGES 文件可能包含大量细节。可以尝试在导出时简化模型，或使用 STEP 格式（通常更高效）。

### Q3: 拓扑关系不正确？
**A**: 尝试启用面缝合：
```geo
Geometry.OCCSewFaces = 1;
Geometry.Tolerance = 1e-6;
```

---

*文档生成时间: 2024-12-24*
*源代码版本: Gmsh 4.14.0*
