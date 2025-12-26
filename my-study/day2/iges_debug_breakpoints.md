# IGES 文件读取调试断点指南

## 核心断点位置（按调用顺序）

### 1. 文件打开入口
**文件**: `src/common/OpenFile.cpp`
**行号**: 379
**说明**: 根据文件扩展名判断文件类型，调用对应的读取函数

### 2. IGES 读取入口
**文件**: `src/geo/GModelIO_OCC.cpp`
**行号**: 6783
**函数**: `GModel::readOCCIGES()`
**说明**: Gmsh 读取 IGES 文件的入口函数

### 3. OCC 导入形状
**文件**: `src/geo/GModelIO_OCC.cpp`
**行号**: 4790
**函数**: `OCC_Internals::importShapes()`
**说明**: 调用 OpenCASCADE 读取 CAD 文件的核心函数

### 4. IGES 文件解析（OCC 源码）
**文件**: `/Users/renyuxiao/Documents/gmsh/OCCT/src/DataExchange/TKDEIGES/IGESCAFControl/IGESCAFControl_Reader.cxx`
**行号**: 约 50-100
**函数**: `IGESCAFControl_Reader::ReadFile()`
**说明**: OCC 读取 IGES 文件的入口

### 5. IGES 实体转换（OCC 源码）
**文件**: `/Users/renyuxiao/Documents/gmsh/OCCT/src/DataExchange/TKDEIGES/IGESControl/IGESControl_Reader.cxx`
**行号**: 82
**函数**: `IGESControl_Reader::NbRootsForTransfer()`
**说明**: 准备转换 IGES 实体到 BRep

### 6. 几何修复
**文件**: `src/geo/GModelIO_OCC.cpp`
**行号**: 4867
**函数**: `_healShape()`
**说明**: 修复读取的几何形状（缝合面、修复小边等）

### 7. 实体绑定
**文件**: `src/geo/GModelIO_OCC.cpp`
**行号**: 4874
**函数**: `_multiBind()`
**说明**: 将 OCC 形状绑定到 Gmsh 几何实体

### 8. 同步到模型
**文件**: `src/geo/GModelIO_OCC.cpp`
**行号**: 6788
**函数**: `synchronize()`
**说明**: 将 OCC 内部数据同步到 GModel

---

## IGES 读取完整调用链

```
程序入口 (main)
    ↓
OpenFile.cpp:379  ← 根据扩展名 .igs/.iges 判断
    ↓
GModel::readOCCIGES()  (GModelIO_OCC.cpp:6783)
    ↓
OCC_Internals::importShapes()  (GModelIO_OCC.cpp:4790)
    ↓
IGESCAFControl_Reader::ReadFile()  ← OCC 源码 (读取 IGES 文件)
    ↓
reader.TransferRoots()  ← OCC 转换为 TopoDS_Shape
    ↓
_healShape()  ← 修复几何
    ↓
_multiBind()  ← 绑定到 Gmsh 实体
    ↓
synchronize()  ← 同步到 GModel
```

---

## 快速设置断点（在 Cursor 中）

1. 打开文件 `src/geo/GModelIO_OCC.cpp`
2. 按 `Ctrl+G` (或 `Cmd+G`)，输入行号 `6783`，回车
3. 点击行号左侧设置断点（红点）
4. 重复上述步骤设置其他断点

## OCC 源码断点

1. 打开 `/Users/renyuxiao/Documents/gmsh/OCCT/src/DataExchange/TKDEIGES/IGESControl/IGESControl_Reader.cxx`
2. 在第 52 行（构造函数）设置断点
3. 在 `ReadFile` 方法处设置断点

---

## 启动调试

1. 按 `F5` 启动调试
2. 选择 "Debug Gmsh + OCC (IGES)"
3. 程序会在断点处暂停
4. 使用调试工具栏：
   - `F10` - 单步跳过
   - `F11` - 单步进入（进入函数内部/OCC 源码）
   - `Shift+F11` - 单步跳出
   - `F5` - 继续运行到下一个断点

---

## 关键代码片段

### GModel::readOCCIGES (GModelIO_OCC.cpp:6783)
```cpp
int GModel::readOCCIGES(const std::string &fn)
{
  if(!_occ_internals) _occ_internals = new OCC_Internals;
  std::vector<std::pair<int, int>> outDimTags;
  _occ_internals->importShapes(fn, false, outDimTags, "iges");
  _occ_internals->synchronize(this);
  return 1;
}
```

### OCC_Internals::importShapes IGES 部分 (GModelIO_OCC.cpp:4834-4854)
```cpp
else if(format == "iges" || split[2] == ".iges" || split[2] == ".igs" ||
        split[2] == ".IGES" || split[2] == ".IGS") {
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
```

---

## 调试技巧

1. **查看变量**: 在调试时，将鼠标悬停在变量上可查看其值
2. **监视表达式**: 在"监视"面板添加 `result.ShapeType()` 查看形状类型
3. **调用堆栈**: 查看"调用堆栈"面板了解完整调用链
4. **条件断点**: 右键断点 → "编辑断点" → 添加条件（如 `fileName.find(".igs") != std::string::npos`）

---

## 相关文件路径

| 文件 | 路径 |
|------|------|
| OpenFile.cpp | src/common/OpenFile.cpp |
| GModelIO_OCC.cpp | src/geo/GModelIO_OCC.cpp |
| GModelIO_OCC.h | src/geo/GModelIO_OCC.h |
| IGESControl_Reader.cxx | /Users/renyuxiao/Documents/gmsh/OCCT/src/DataExchange/TKDEIGES/IGESControl/ |
| IGESCAFControl_Reader.cxx | /Users/renyuxiao/Documents/gmsh/OCCT/src/DataExchange/TKDEIGES/IGESCAFControl/ |
