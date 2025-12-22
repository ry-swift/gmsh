# Day 34：几何修复与简化

**所属周次**：第5周 - 几何模块深入
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解几何修复、去特征化和简化方法

---

## 学习文件

- `src/geo/OCC_Internals.cpp`（修复和简化部分）
- `src/geo/GModelGeom.cpp`

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 学习几何修复的必要性 |
| 09:45-10:30 | 45min | 分析healShapes实现 |
| 10:30-11:15 | 45min | 学习去特征化方法 |
| 11:15-12:00 | 45min | 分析removeSmallFeatures |
| 14:00-14:45 | 45min | 学习几何简化方法 |
| 14:45-15:30 | 45min | 分析合并重复实体 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 几何修复的必要性

```
CAD几何常见问题：
┌─────────────────────────────────────────────────────────────┐
│ 1. 拓扑问题                                                 │
│    - 边界不闭合（gap）                                      │
│    - 面方向不一致                                           │
│    - 非流形边（被3个以上面共享）                            │
├─────────────────────────────────────────────────────────────┤
│ 2. 几何问题                                                 │
│    - 退化边（长度为0）                                      │
│    - 退化面（面积为0）                                      │
│    - 自相交                                                 │
├─────────────────────────────────────────────────────────────┤
│ 3. 精度问题                                                 │
│    - 相邻面边界不匹配（微小gap）                            │
│    - 顶点坐标精度不足                                       │
├─────────────────────────────────────────────────────────────┤
│ 4. 小特征                                                   │
│    - 微小边（对网格无影响但增加计算量）                      │
│    - 微小面（薄片）                                         │
│    - 窄缝（sliver）                                         │
└─────────────────────────────────────────────────────────────┘
```

### healShapes实现分析

```cpp
// src/geo/OCC_Internals.cpp

bool OCC_Internals::healShapes(
    const std::vector<std::pair<int, int>> &inDimTags,
    std::vector<std::pair<int, int>> &outDimTags,
    double tolerance,
    bool fixDegenerated,
    bool fixSmallEdges,
    bool fixSmallFaces,
    bool sewFaces,
    bool makeSolids) {

  for(auto &dt : inDimTags) {
    TopoDS_Shape shape = getShape(dt.first, dt.second);

    // 步骤1：基本形状修复
    if(fixDegenerated) {
      ShapeFix_Shape fix(shape);
      fix.SetPrecision(tolerance);
      fix.SetMinTolerance(tolerance);
      fix.SetMaxTolerance(tolerance * 10);

      // 修复各级别
      fix.FixSolidMode() = Standard_True;
      fix.FixShellMode() = Standard_True;
      fix.FixFaceMode() = Standard_True;
      fix.FixWireMode() = Standard_True;
      fix.FixEdgeMode() = Standard_True;

      fix.Perform();
      shape = fix.Shape();
    }

    // 步骤2：修复小边
    if(fixSmallEdges) {
      ShapeFix_Wireframe wireframe(shape);
      wireframe.ModeDropSmallEdges() = Standard_True;
      wireframe.SetPrecision(tolerance);
      wireframe.FixSmallEdges();
      wireframe.FixWireGaps();
      shape = wireframe.Shape();
    }

    // 步骤3：缝合面
    if(sewFaces) {
      BRepBuilderAPI_Sewing sewing(tolerance);
      sewing.Add(shape);
      sewing.Perform();

      if(sewing.SewedShape().IsNull()) {
        Msg::Warning("Sewing failed");
      } else {
        shape = sewing.SewedShape();
      }
    }

    // 步骤4：生成实体
    if(makeSolids) {
      TopExp_Explorer exp(shape, TopAbs_SHELL);
      if(exp.More()) {
        BRepBuilderAPI_MakeSolid solid;
        for(; exp.More(); exp.Next()) {
          TopoDS_Shell shell = TopoDS::Shell(exp.Current());
          solid.Add(shell);
        }
        if(solid.IsDone()) {
          shape = solid.Solid();
        }
      }
    }

    updateShape(dt.first, dt.second, shape);
    outDimTags.push_back(dt);
  }

  return true;
}
```

### ShapeFix详解

```cpp
// ShapeFix_Shape 的子修复器

ShapeFix_Shape fix(shape);

// 1. 边修复 (ShapeFix_Edge)
//    - 修复退化边
//    - 修复曲线-顶点关联
fix.FixEdgeMode() = Standard_True;

// 2. 线环修复 (ShapeFix_Wire)
//    - 修复边顺序
//    - 闭合gap
//    - 移除自相交
fix.FixWireMode() = Standard_True;

// 3. 面修复 (ShapeFix_Face)
//    - 修复线环方向
//    - 修复面-曲面关联
fix.FixFaceMode() = Standard_True;

// 4. 壳修复 (ShapeFix_Shell)
//    - 修复面方向一致性
fix.FixShellMode() = Standard_True;

// 5. 实体修复 (ShapeFix_Solid)
//    - 修复壳方向
fix.FixSolidMode() = Standard_True;
```

---

## 下午任务（2小时）

### 去特征化方法

```cpp
// 移除小边
bool OCC_Internals::removeSmallEdges(
    const std::vector<std::pair<int, int>> &dimTags,
    double minLen) {

  for(auto &dt : dimTags) {
    TopoDS_Shape shape = getShape(dt.first, dt.second);

    // 使用ShapeFix_Wireframe移除小边
    ShapeFix_Wireframe fix(shape);
    fix.SetPrecision(minLen);
    fix.ModeDropSmallEdges() = Standard_True;
    fix.FixSmallEdges();

    // 或者手动遍历移除
    TopExp_Explorer exp(shape, TopAbs_EDGE);
    for(; exp.More(); exp.Next()) {
      TopoDS_Edge edge = TopoDS::Edge(exp.Current());
      GProp_GProps props;
      BRepGProp::LinearProperties(edge, props);
      double length = props.Mass();

      if(length < minLen) {
        // 标记为需要移除
        // 后续用BRepAlgo_DefeatFeature处理
      }
    }

    updateShape(dt.first, dt.second, shape);
  }
  return true;
}

// 移除小面
bool OCC_Internals::removeSmallFaces(
    const std::vector<std::pair<int, int>> &dimTags,
    double minArea) {

  for(auto &dt : dimTags) {
    TopoDS_Shape shape = getShape(dt.first, dt.second);

    TopExp_Explorer exp(shape, TopAbs_FACE);
    for(; exp.More(); exp.Next()) {
      TopoDS_Face face = TopoDS::Face(exp.Current());
      GProp_GProps props;
      BRepGProp::SurfaceProperties(face, props);
      double area = props.Mass();

      if(area < minArea) {
        // 标记为需要移除
      }
    }
  }
  return true;
}
```

### 去除圆角/倒角

```cpp
// 去除圆角（defeaturing）
bool OCC_Internals::removeFillets(
    const std::vector<int> &faceTags,
    std::vector<std::pair<int, int>> &outDimTags) {

  // 使用BRepAlgoAPI_Defeaturing（OCC 7.4+）
  BRepAlgoAPI_Defeaturing aDF;

  // 添加要移除的面
  for(int tag : faceTags) {
    TopoDS_Face face = TopoDS::Face(getShape(2, tag));
    aDF.AddFaceToRemove(face);
  }

  // 设置要处理的形状
  TopoDS_Shape shape = ...; // 包含这些面的实体
  aDF.SetShape(shape);

  // 执行去特征化
  aDF.Build();

  if(!aDF.IsDone()) {
    Msg::Error("Defeaturing failed");
    return false;
  }

  TopoDS_Shape result = aDF.Shape();
  // 更新形状
  return true;
}
```

### 合并重复实体

```cpp
// 移除重复实体
bool OCC_Internals::removeAllDuplicates() {
  // 1. 合并重复顶点
  std::vector<TopoDS_Vertex> uniqueVertices;
  for(auto &pair : _vertexTag) {
    TopoDS_Vertex v = pair.first;
    gp_Pnt p = BRep_Tool::Pnt(v);

    bool found = false;
    for(auto &uv : uniqueVertices) {
      gp_Pnt up = BRep_Tool::Pnt(uv);
      if(p.Distance(up) < CTX::instance()->geom.tolerance) {
        // 找到重复，需要合并
        found = true;
        break;
      }
    }
    if(!found) {
      uniqueVertices.push_back(v);
    }
  }

  // 2. 合并重复边
  // 基于端点和几何相似性判断

  // 3. 合并重复面
  // 基于边界和几何相似性判断

  // 4. 重建拓扑关系
  synchronize(GModel::current());

  return true;
}

// 缝合分离的面
bool OCC_Internals::sewFaces(
    const std::vector<int> &faceTags,
    double tolerance) {

  BRepBuilderAPI_Sewing sewing(tolerance);

  for(int tag : faceTags) {
    TopoDS_Face face = TopoDS::Face(getShape(2, tag));
    sewing.Add(face);
  }

  sewing.Perform();
  TopoDS_Shape result = sewing.SewedShape();

  // 更新模型
  return true;
}
```

---

## 练习作业

### 1. 【基础】使用几何修复

编写使用healShapes的代码：

```python
import gmsh
gmsh.initialize()
gmsh.model.add("heal_test")

# 导入有问题的CAD文件
gmsh.merge("problematic.step")

# 获取所有实体
entities = gmsh.model.getEntities(3)

# 执行修复
gmsh.model.occ.healShapes(
    dimTags=entities,
    tolerance=1e-6,
    fixDegenerated=True,
    fixSmallEdges=True,
    fixSmallFaces=True,
    sewFaces=True,
    makeSolids=True
)
gmsh.model.occ.synchronize()

# 验证修复结果
entities_after = gmsh.model.getEntities(3)
print(f"Before: {len(entities)}, After: {len(entities_after)}")

gmsh.fltk.run()
gmsh.finalize()
```

### 2. 【进阶】分析修复代码

查找修复相关的实现：

```bash
# 查找healShapes实现
grep -A 80 "OCC_Internals::healShapes" src/geo/OCC_Internals.cpp

# 查找ShapeFix使用
grep -n "ShapeFix" src/geo/OCC_Internals.cpp

# 查找缝合实现
grep -n "Sewing\|sew" src/geo/OCC_Internals.cpp
```

### 3. 【挑战】实现几何诊断

编写诊断几何问题的代码：

```python
import gmsh

def diagnose_geometry(filename):
    """诊断几何问题"""
    gmsh.initialize()
    gmsh.model.add("diagnose")
    gmsh.merge(filename)

    issues = []

    # 检查退化边
    edges = gmsh.model.getEntities(1)
    for dim, tag in edges:
        bounds = gmsh.model.getParametrizationBounds(dim, tag)
        if bounds[1][0] - bounds[0][0] < 1e-10:
            issues.append(f"Degenerate edge: {tag}")

    # 检查小面
    faces = gmsh.model.getEntities(2)
    for dim, tag in faces:
        mass = gmsh.model.occ.getMass(dim, tag)
        if mass < 1e-12:
            issues.append(f"Small face (area={mass}): {tag}")

    # 检查未闭合的壳
    volumes = gmsh.model.getEntities(3)
    if len(volumes) == 0 and len(faces) > 0:
        issues.append("No closed volumes found - may have gaps")

    # 检查自相交
    # (通过尝试网格化检测)
    for dim, tag in faces:
        try:
            # 尝试对单个面网格化
            gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)
        except:
            issues.append(f"Face {tag} may have self-intersection")

    print("Geometry Issues Found:")
    for issue in issues:
        print(f"  - {issue}")

    gmsh.finalize()
    return issues

diagnose_geometry("model.step")
```

---

## 今日检查点

- [ ] 理解CAD几何常见问题
- [ ] 理解healShapes的修复流程
- [ ] 了解去特征化方法
- [ ] 能使用Python API进行几何修复

---

## 核心概念

### 几何修复流程

```
几何修复推荐流程：
┌─────────────────────────────────────────────────────────────┐
│ 1. 导入CAD文件                                              │
│    gmsh.merge("model.step")                                │
├─────────────────────────────────────────────────────────────┤
│ 2. 基本修复                                                 │
│    healShapes(fixDegenerated=True)                         │
│    修复退化边、面                                           │
├─────────────────────────────────────────────────────────────┤
│ 3. 缝合面                                                   │
│    healShapes(sewFaces=True)                               │
│    闭合微小间隙                                             │
├─────────────────────────────────────────────────────────────┤
│ 4. 移除小特征                                               │
│    healShapes(fixSmallEdges=True, fixSmallFaces=True)      │
│    移除对网格无影响的小特征                                 │
├─────────────────────────────────────────────────────────────┤
│ 5. 生成实体                                                 │
│    healShapes(makeSolids=True)                             │
│    从壳创建实体                                             │
├─────────────────────────────────────────────────────────────┤
│ 6. 同步并验证                                               │
│    synchronize()                                           │
│    检查实体数量和有效性                                     │
└─────────────────────────────────────────────────────────────┘
```

### 修复选项总结

| 选项 | 作用 | 使用场景 |
|------|------|----------|
| fixDegenerated | 修复退化几何 | 总是启用 |
| fixSmallEdges | 移除小边 | 有微小特征时 |
| fixSmallFaces | 移除小面 | 有薄片时 |
| sewFaces | 缝合面 | 面不闭合时 |
| makeSolids | 生成实体 | 只有壳没有实体时 |

### 修复失败的处理

```
修复失败时的处理策略：
┌─────────────────────────────────────────────────────────────┐
│ 1. 调整容差                                                 │
│    增大tolerance参数                                        │
│    gmsh.option.setNumber("Geometry.Tolerance", 1e-4)       │
├─────────────────────────────────────────────────────────────┤
│ 2. 分步修复                                                 │
│    逐个启用修复选项，找出问题步骤                           │
├─────────────────────────────────────────────────────────────┤
│ 3. 手动清理                                                 │
│    在CAD软件中手动修复问题几何                              │
│    或使用Gmsh的remove命令删除问题实体                       │
├─────────────────────────────────────────────────────────────┤
│ 4. 替代方法                                                 │
│    使用STL导入（无拓扑）                                    │
│    使用其他CAD格式                                          │
└─────────────────────────────────────────────────────────────┘
```

---

## 源码导航

### 关键文件

```
src/geo/
├── OCC_Internals.cpp     # healShapes, defeaturing
├── GModelGeom.cpp        # 几何操作辅助
└── GModelIO_OCC.cpp      # CAD导入修复
```

### 搜索命令

```bash
# 查找修复函数
grep -n "healShapes\|fixSmall\|sewFaces" src/geo/OCC_Internals.cpp

# 查找OCC修复类使用
grep -n "ShapeFix\|Sewing\|Defeaturing" src/geo/OCC_Internals.cpp

# 查找容差相关
grep -n "tolerance\|Precision" src/geo/OCC_Internals.cpp
```

---

## 产出

- 理解几何修复方法
- 掌握去特征化技术

---

## 导航

- **上一天**：[Day 33 - CAD文件导入流程](day-33.md)
- **下一天**：[Day 35 - 第五周复习](day-35.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
