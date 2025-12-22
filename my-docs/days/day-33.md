# Day 33：CAD文件导入流程

**所属周次**：第5周 - 几何模块深入
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解STEP、IGES等CAD文件的导入流程

---

## 学习文件

- `src/geo/GModelIO_OCC.cpp`
- `src/geo/OCC_Internals.cpp`（导入部分）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 了解常见CAD文件格式 |
| 09:45-10:30 | 45min | 分析STEP文件导入流程 |
| 10:30-11:15 | 45min | 分析IGES文件导入流程 |
| 11:15-12:00 | 45min | 理解BREP格式 |
| 14:00-14:45 | 45min | 理解导入后的拓扑处理 |
| 14:45-15:30 | 45min | 学习导入选项和错误处理 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 常见CAD文件格式

```
CAD文件格式对比：
┌─────────────────────────────────────────────────────────────┐
│ STEP (.step, .stp)                                          │
│   - ISO标准 (ISO 10303)                                     │
│   - 支持完整的几何和拓扑信息                                │
│   - 跨平台兼容性最好                                        │
│   - 推荐用于工程数据交换                                    │
├─────────────────────────────────────────────────────────────┤
│ IGES (.iges, .igs)                                          │
│   - 较老的标准                                              │
│   - 支持基本几何                                            │
│   - 可能丢失拓扑信息                                        │
│   - 逐渐被STEP取代                                          │
├─────────────────────────────────────────────────────────────┤
│ BREP (.brep)                                                │
│   - OpenCASCADE原生格式                                     │
│   - 完整保留所有信息                                        │
│   - 仅用于OCC生态系统                                       │
├─────────────────────────────────────────────────────────────┤
│ STL (.stl)                                                  │
│   - 三角面片格式                                            │
│   - 无拓扑信息                                              │
│   - 主要用于3D打印和可视化                                  │
└─────────────────────────────────────────────────────────────┘
```

### STEP文件导入流程

```cpp
// src/geo/GModelIO_OCC.cpp

int GModel::readSTEP(const std::string &fn) {
  // 1. 创建STEP读取器
  STEPControl_Reader reader;

  // 2. 读取文件
  IFSelect_ReturnStatus status = reader.ReadFile(fn.c_str());
  if(status != IFSelect_RetDone) {
    Msg::Error("Could not read STEP file %s", fn.c_str());
    return 0;
  }

  // 3. 检查和打印诊断信息
  reader.PrintCheckLoad(Standard_False, IFSelect_ListByItem);

  // 4. 传输根形状
  int nbRoots = reader.NbRootsForTransfer();
  for(int i = 1; i <= nbRoots; i++) {
    reader.TransferRoot(i);
  }

  // 5. 获取结果形状
  TopoDS_Shape shape = reader.OneShape();

  // 6. 调用OCC_Internals处理
  if(_occ_internals)
    _occ_internals->importShapes(&shape, true);

  // 7. 同步到GModel
  _occ_internals->synchronize(this);

  return 1;
}

// OCC_Internals::importShapes
void OCC_Internals::importShapes(TopoDS_Shape *shape, bool highestDimOnly) {
  // 遍历形状，分配tag
  if(highestDimOnly) {
    // 只导入最高维度的实体
    int dim = getShapeDim(*shape);
    if(dim == 3) {
      TopExp_Explorer exp(*shape, TopAbs_SOLID);
      for(; exp.More(); exp.Next()) {
        TopoDS_Solid solid = TopoDS::Solid(exp.Current());
        int tag = getMaxTag(3) + 1;
        _solidTag[solid] = tag;
      }
    }
    // ... 其他维度
  }
}
```

### IGES文件导入流程

```cpp
int GModel::readIGES(const std::string &fn) {
  // 1. 创建IGES读取器
  IGESControl_Reader reader;

  // 2. 设置读取参数
  Interface_Static::SetCVal("xstep.cascade.unit", "MM");

  // 3. 读取文件
  IFSelect_ReturnStatus status = reader.ReadFile(fn.c_str());
  if(status != IFSelect_RetDone) {
    Msg::Error("Could not read IGES file %s", fn.c_str());
    return 0;
  }

  // 4. 传输实体
  reader.TransferRoots();

  // 5. 获取结果
  TopoDS_Shape shape = reader.OneShape();

  // 6. 导入到GModel
  _occ_internals->importShapes(&shape, true);
  _occ_internals->synchronize(this);

  return 1;
}
```

### BREP格式导入

```cpp
int GModel::readBREP(const std::string &fn) {
  TopoDS_Shape shape;

  // BREP是OCC原生格式，直接读取
  BRep_Builder builder;
  BRepTools::Read(shape, fn.c_str(), builder);

  if(shape.IsNull()) {
    Msg::Error("Could not read BREP file %s", fn.c_str());
    return 0;
  }

  _occ_internals->importShapes(&shape, true);
  _occ_internals->synchronize(this);

  return 1;
}
```

---

## 下午任务（2小时）

### 导入后的拓扑处理

```cpp
// 导入后需要进行拓扑修复和清理

void OCC_Internals::healShapes(
    const std::vector<std::pair<int, int>> &dimTags,
    double tolerance, bool fixDegenerated, bool fixSmallEdges,
    bool fixSmallFaces, bool sewFaces, bool makeSolids) {

  for(auto &dt : dimTags) {
    TopoDS_Shape shape = getShape(dt.first, dt.second);

    // 1. 修复退化几何
    if(fixDegenerated) {
      ShapeFix_Shape fix(shape);
      fix.FixSolidMode() = Standard_True;
      fix.FixShellMode() = Standard_True;
      fix.FixFaceMode() = Standard_True;
      fix.FixWireMode() = Standard_True;
      fix.FixEdgeMode() = Standard_True;
      fix.SetPrecision(tolerance);
      fix.Perform();
      shape = fix.Shape();
    }

    // 2. 移除小边
    if(fixSmallEdges) {
      ShapeFix_Wireframe fix(shape);
      fix.ModeDropSmallEdges() = Standard_True;
      fix.SetPrecision(tolerance);
      fix.FixSmallEdges();
      shape = fix.Shape();
    }

    // 3. 缝合面
    if(sewFaces) {
      BRepBuilderAPI_Sewing sew(tolerance);
      sew.Add(shape);
      sew.Perform();
      shape = sew.SewedShape();
    }

    // 4. 生成实体
    if(makeSolids) {
      BRepBuilderAPI_MakeSolid solid;
      // ...
    }

    updateShape(dt.first, dt.second, shape);
  }
}
```

### 导入选项

```cpp
// 导入相关的选项（Context.h）
struct contextGeometryOptions {
  // STEP/IGES读取选项
  double tolerance;           // 几何容差
  double toleranceBoolean;    // 布尔运算容差

  // 单位
  std::string occTargetUnit;  // 目标单位（MM, M, etc.）

  // 修复选项
  int occAutoFix;             // 自动修复
  int occFixDegenerated;      // 修复退化几何
  int occFixSmallEdges;       // 修复小边
  int occFixSmallFaces;       // 修复小面
  int occSewFaces;            // 缝合面
  int occMakeSolids;          // 生成实体

  // 导入选项
  int occImportLabels;        // 导入标签/名称
  int occHighestDimensionOnly;// 只导入最高维度
  int occMerge;               // 合并重复实体
};

// API设置示例
gmsh::option::setNumber("Geometry.Tolerance", 1e-6);
gmsh::option::setNumber("Geometry.OCCFixDegenerated", 1);
gmsh::option::setNumber("Geometry.OCCFixSmallEdges", 1);
gmsh::option::setNumber("Geometry.OCCSewFaces", 1);
```

### 错误处理

```cpp
// 常见导入错误和处理

int GModel::readSTEP(const std::string &fn) {
  STEPControl_Reader reader;
  IFSelect_ReturnStatus status = reader.ReadFile(fn.c_str());

  switch(status) {
    case IFSelect_RetDone:
      // 成功
      break;
    case IFSelect_RetError:
      Msg::Error("STEP read error");
      return 0;
    case IFSelect_RetFail:
      Msg::Error("STEP read failed");
      return 0;
    case IFSelect_RetVoid:
      Msg::Warning("STEP file is empty");
      return 0;
  }

  // 检查传输过程中的警告
  reader.PrintCheckTransfer(Standard_False, IFSelect_ListByItem);

  // 获取失败的实体数量
  int nbFail = reader.NbRootsForTransfer() -
               reader.TransferRoots();
  if(nbFail > 0) {
    Msg::Warning("%d entities could not be transferred", nbFail);
  }

  // ...
}
```

---

## 练习作业

### 1. 【基础】导入STEP文件

使用Python API导入STEP文件：

```python
import gmsh
gmsh.initialize()
gmsh.model.add("step_import")

# 设置导入选项
gmsh.option.setNumber("Geometry.Tolerance", 1e-6)
gmsh.option.setNumber("Geometry.OCCFixDegenerated", 1)

# 导入STEP文件
gmsh.merge("model.step")

# 检查导入结果
entities = gmsh.model.getEntities()
print(f"Imported entities: {entities}")

# 获取边界盒
bbox = gmsh.model.getBoundingBox(-1, -1)
print(f"Bounding box: {bbox}")

gmsh.fltk.run()
gmsh.finalize()
```

### 2. 【进阶】追踪STEP导入代码

分析从API到OCC的完整导入流程：

```bash
# 查找merge函数
grep -rn "GModel::readOCC\|GModel::readSTEP" src/geo/GModelIO*.cpp

# 查找importShapes
grep -A 30 "importShapes" src/geo/OCC_Internals.cpp | head -50
```

绘制流程图：
```
gmsh.merge("model.step")
    ↓
GModel::readOCC() / GModel::readSTEP()
    ↓
STEPControl_Reader::ReadFile()
    ↓
STEPControl_Reader::TransferRoots()
    ↓
OCC_Internals::importShapes()
    ↓
OCC_Internals::synchronize()
    ↓
创建 OCCVertex, OCCEdge, OCCFace, OCCRegion
```

### 3. 【挑战】实现导入后的几何检查

编写检查导入几何有效性的代码：

```python
import gmsh

def check_geometry(filename):
    """检查导入几何的有效性"""
    gmsh.initialize()
    gmsh.model.add("check")

    # 导入
    gmsh.merge(filename)

    # 检查各维度实体
    for dim in range(4):
        entities = gmsh.model.getEntities(dim)
        print(f"Dimension {dim}: {len(entities)} entities")

    # 检查每个面的边界
    faces = gmsh.model.getEntities(2)
    for dim, tag in faces:
        boundary = gmsh.model.getBoundary([(dim, tag)])
        print(f"Face {tag}: {len(boundary)} boundary edges")

    # 检查每个体的边界面
    volumes = gmsh.model.getEntities(3)
    for dim, tag in volumes:
        boundary = gmsh.model.getBoundary([(dim, tag)])
        print(f"Volume {tag}: {len(boundary)} boundary faces")

    # 尝试生成网格检测几何问题
    try:
        gmsh.model.mesh.generate(2)
        print("2D mesh generated successfully")
    except Exception as e:
        print(f"2D mesh failed: {e}")

    gmsh.finalize()

# 使用
check_geometry("model.step")
```

---

## 今日检查点

- [ ] 了解STEP、IGES、BREP格式的特点
- [ ] 理解STEP文件的导入流程
- [ ] 理解导入后的拓扑修复过程
- [ ] 能使用Python API导入CAD文件

---

## 核心概念

### CAD导入流程总结

```
CAD文件导入完整流程：
┌─────────────────────────────────────────────────────────────┐
│ 1. 文件读取                                                 │
│    STEPControl_Reader / IGESControl_Reader                  │
│    读取文件内容到OCC数据结构                                │
├─────────────────────────────────────────────────────────────┤
│ 2. 实体传输                                                 │
│    TransferRoots() / TransferRoot()                        │
│    将CAD实体转换为TopoDS形状                                │
├─────────────────────────────────────────────────────────────┤
│ 3. 几何修复（可选）                                         │
│    ShapeFix_Shape / BRepBuilderAPI_Sewing                  │
│    修复退化几何、缝合面、生成实体                           │
├─────────────────────────────────────────────────────────────┤
│ 4. 导入到Gmsh                                               │
│    OCC_Internals::importShapes()                           │
│    分配tag，建立映射关系                                    │
├─────────────────────────────────────────────────────────────┤
│ 5. 同步到GModel                                             │
│    OCC_Internals::synchronize()                            │
│    创建OCCVertex/OCCEdge/OCCFace/OCCRegion                 │
└─────────────────────────────────────────────────────────────┘
```

### 常见导入问题

| 问题 | 原因 | 解决方法 |
|------|------|----------|
| 导入为空 | 文件损坏或格式错误 | 在其他软件中验证文件 |
| 缺少实体 | 拓扑不完整 | 启用OCCSewFaces |
| 网格失败 | 几何退化 | 启用OCCFixDegenerated |
| 小特征问题 | 边/面过小 | 启用OCCFixSmallEdges |
| 单位错误 | CAD单位不匹配 | 设置OCCTargetUnit |

### 导入选项速查

```python
# 推荐的导入选项设置
gmsh.option.setNumber("Geometry.Tolerance", 1e-6)
gmsh.option.setNumber("Geometry.ToleranceBoolean", 1e-6)
gmsh.option.setNumber("Geometry.OCCFixDegenerated", 1)
gmsh.option.setNumber("Geometry.OCCFixSmallEdges", 1)
gmsh.option.setNumber("Geometry.OCCFixSmallFaces", 1)
gmsh.option.setNumber("Geometry.OCCSewFaces", 1)
gmsh.option.setNumber("Geometry.OCCMakeSolids", 1)
gmsh.option.setString("Geometry.OCCTargetUnit", "M")  # 米制单位
```

---

## 源码导航

### 关键文件

```
src/geo/
├── GModelIO_OCC.cpp      # STEP/IGES/BREP导入导出
├── OCC_Internals.cpp     # OCC封装（importShapes, healShapes）
├── GModelIO_STL.cpp      # STL导入导出
└── GModelIO_STEP.cpp     # STEP专用处理
```

### 搜索命令

```bash
# 查找文件读取函数
grep -n "readSTEP\|readIGES\|readBREP" src/geo/GModelIO*.cpp

# 查找导入选项
grep -n "OCCFix\|OCCSew\|OCCMake" src/geo/OCC_Internals.cpp

# 查找错误处理
grep -n "IFSelect_Ret\|Msg::Error.*STEP\|Msg::Error.*IGES" src/geo/
```

---

## 产出

- 理解CAD文件导入流程
- 掌握导入选项设置

---

## 导航

- **上一天**：[Day 32 - 几何操作](day-32.md)
- **下一天**：[Day 34 - 几何修复与简化](day-34.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
