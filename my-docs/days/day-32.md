# Day 32：几何操作

**所属周次**：第5周 - 几何模块深入
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

深入理解布尔运算、几何变换等操作的实现

---

## 学习文件

- `src/geo/OCC_Internals.cpp`（布尔运算部分）
- `src/geo/GModelFactory.cpp`

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 学习布尔运算原理 |
| 09:45-10:30 | 45min | 分析Union/Fuse实现 |
| 10:30-11:15 | 45min | 分析Difference/Cut实现 |
| 11:15-12:00 | 45min | 分析Intersection/Common实现 |
| 14:00-14:45 | 45min | 学习几何变换（平移、旋转、缩放） |
| 14:45-15:30 | 45min | 学习倒角和圆角 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 布尔运算原理

```
布尔运算类型：
┌─────────────────────────────────────────────────────────────┐
│ Union/Fuse（并集）                                          │
│   A ∪ B = A和B的合并                                        │
│   ┌───┐      ┌───────┐                                     │
│   │ A │  ∪   │   B   │  =  合并后的形状                     │
│   └───┘      └───────┘                                     │
├─────────────────────────────────────────────────────────────┤
│ Difference/Cut（差集）                                      │
│   A - B = A减去A和B的交集                                   │
│   ┌───┐      ┌───────┐                                     │
│   │ A │  -   │   B   │  =  A中去除B的部分                   │
│   └───┘      └───────┘                                     │
├─────────────────────────────────────────────────────────────┤
│ Intersection/Common（交集）                                 │
│   A ∩ B = A和B的公共部分                                    │
│   ┌───┐      ┌───────┐                                     │
│   │ A │  ∩   │   B   │  =  重叠部分                         │
│   └───┘      └───────┘                                     │
├─────────────────────────────────────────────────────────────┤
│ Fragments（分割）                                           │
│   将A和B分割成不重叠的片段                                  │
│   用于创建共形网格                                          │
└─────────────────────────────────────────────────────────────┘
```

### Union/Fuse实现

```cpp
// src/geo/OCC_Internals.cpp

bool OCC_Internals::booleanUnion(
    int tag,
    const std::vector<std::pair<int, int>> &objectDimTags,
    const std::vector<std::pair<int, int>> &toolDimTags,
    std::vector<std::pair<int, int>> &outDimTags,
    std::vector<std::vector<std::pair<int, int>>> &outDimTagsMap,
    bool removeObject, bool removeTool) {

  // 1. 收集输入形状
  TopoDS_Shape object, tool;
  for(auto &dt : objectDimTags) {
    TopoDS_Shape s = getShape(dt.first, dt.second);
    // 合并到object
  }
  for(auto &dt : toolDimTags) {
    TopoDS_Shape s = getShape(dt.first, dt.second);
    // 合并到tool
  }

  // 2. 执行OCC布尔运算
  BRepAlgoAPI_Fuse fuse(object, tool);
  fuse.SetFuzzyValue(CTX::instance()->geom.tolerance);
  fuse.Build();

  if(!fuse.IsDone()) {
    Msg::Error("Boolean union failed");
    return false;
  }

  // 3. 获取结果
  TopoDS_Shape result = fuse.Shape();

  // 4. 分配tag并更新映射
  _assignTag(result, tag);

  // 5. 删除原对象（如果要求）
  if(removeObject) {
    for(auto &dt : objectDimTags)
      _deleteShape(dt.first, dt.second);
  }

  return true;
}
```

### Difference/Cut实现

```cpp
bool OCC_Internals::booleanDifference(
    int tag,
    const std::vector<std::pair<int, int>> &objectDimTags,
    const std::vector<std::pair<int, int>> &toolDimTags,
    std::vector<std::pair<int, int>> &outDimTags,
    ...) {

  // 收集形状
  TopoDS_Shape object = collectShapes(objectDimTags);
  TopoDS_Shape tool = collectShapes(toolDimTags);

  // 执行差运算
  BRepAlgoAPI_Cut cut(object, tool);
  cut.SetFuzzyValue(CTX::instance()->geom.tolerance);
  cut.Build();

  if(!cut.IsDone()) {
    Msg::Error("Boolean difference failed");
    return false;
  }

  TopoDS_Shape result = cut.Shape();
  // ...
}
```

### Intersection/Common实现

```cpp
bool OCC_Internals::booleanIntersection(
    int tag,
    const std::vector<std::pair<int, int>> &objectDimTags,
    const std::vector<std::pair<int, int>> &toolDimTags,
    ...) {

  TopoDS_Shape object = collectShapes(objectDimTags);
  TopoDS_Shape tool = collectShapes(toolDimTags);

  // 执行交运算
  BRepAlgoAPI_Common common(object, tool);
  common.Build();

  TopoDS_Shape result = common.Shape();
  // ...
}
```

---

## 下午任务（2小时）

### 几何变换

```cpp
// src/geo/OCC_Internals.cpp

// 平移
bool OCC_Internals::translate(
    const std::vector<std::pair<int, int>> &dimTags,
    double dx, double dy, double dz) {

  gp_Trsf trsf;
  trsf.SetTranslation(gp_Vec(dx, dy, dz));

  for(auto &dt : dimTags) {
    TopoDS_Shape shape = getShape(dt.first, dt.second);
    BRepBuilderAPI_Transform transform(shape, trsf);
    TopoDS_Shape newShape = transform.Shape();
    updateShape(dt.first, dt.second, newShape);
  }
  return true;
}

// 旋转
bool OCC_Internals::rotate(
    const std::vector<std::pair<int, int>> &dimTags,
    double x, double y, double z,      // 轴上的点
    double ax, double ay, double az,   // 轴方向
    double angle) {

  gp_Ax1 axis(gp_Pnt(x, y, z), gp_Dir(ax, ay, az));
  gp_Trsf trsf;
  trsf.SetRotation(axis, angle);

  for(auto &dt : dimTags) {
    TopoDS_Shape shape = getShape(dt.first, dt.second);
    BRepBuilderAPI_Transform transform(shape, trsf);
    TopoDS_Shape newShape = transform.Shape();
    updateShape(dt.first, dt.second, newShape);
  }
  return true;
}

// 缩放
bool OCC_Internals::dilate(
    const std::vector<std::pair<int, int>> &dimTags,
    double x, double y, double z,   // 缩放中心
    double a, double b, double c) { // 各方向缩放因子

  gp_GTrsf gtrsf;
  gtrsf.SetValue(1, 1, a);
  gtrsf.SetValue(2, 2, b);
  gtrsf.SetValue(3, 3, c);
  // 设置平移以保持中心点不变

  for(auto &dt : dimTags) {
    TopoDS_Shape shape = getShape(dt.first, dt.second);
    BRepBuilderAPI_GTransform transform(shape, gtrsf);
    TopoDS_Shape newShape = transform.Shape();
    updateShape(dt.first, dt.second, newShape);
  }
  return true;
}

// 镜像
bool OCC_Internals::symmetry(
    const std::vector<std::pair<int, int>> &dimTags,
    double a, double b, double c, double d) { // 平面 ax+by+cz+d=0

  gp_Ax2 ax2(gp_Pnt(-a*d, -b*d, -c*d), gp_Dir(a, b, c));
  gp_Trsf trsf;
  trsf.SetMirror(ax2);
  // ...
}
```

### 倒角和圆角

```cpp
// 圆角（Fillet）
bool OCC_Internals::fillet(
    const std::vector<int> &volumeTags,
    const std::vector<int> &edgeTags,
    const std::vector<double> &radii,
    std::vector<std::pair<int, int>> &outDimTags,
    bool removeVolume) {

  for(int vTag : volumeTags) {
    TopoDS_Shape shape = getShape(3, vTag);

    BRepFilletAPI_MakeFillet fillet(TopoDS::Solid(shape));

    // 为每条边添加圆角半径
    for(int i = 0; i < edgeTags.size(); i++) {
      TopoDS_Edge edge = TopoDS::Edge(getShape(1, edgeTags[i]));
      double r = (i < radii.size()) ? radii[i] : radii[0];
      fillet.Add(r, edge);
    }

    fillet.Build();
    if(!fillet.IsDone()) {
      Msg::Error("Fillet operation failed");
      return false;
    }

    TopoDS_Shape result = fillet.Shape();
    // 更新或创建新形状
  }
  return true;
}

// 倒角（Chamfer）
bool OCC_Internals::chamfer(
    const std::vector<int> &volumeTags,
    const std::vector<int> &edgeTags,
    const std::vector<int> &faceTags,
    const std::vector<double> &distances,
    std::vector<std::pair<int, int>> &outDimTags,
    bool removeVolume) {

  for(int vTag : volumeTags) {
    TopoDS_Shape shape = getShape(3, vTag);

    BRepFilletAPI_MakeChamfer chamfer(TopoDS::Solid(shape));

    // 为每条边添加倒角
    for(int i = 0; i < edgeTags.size(); i++) {
      TopoDS_Edge edge = TopoDS::Edge(getShape(1, edgeTags[i]));
      TopoDS_Face face = TopoDS::Face(getShape(2, faceTags[i]));
      double d = distances[i];
      chamfer.Add(d, edge, face);
    }

    chamfer.Build();
    TopoDS_Shape result = chamfer.Shape();
  }
  return true;
}
```

---

## 练习作业

### 1. 【基础】理解布尔运算结果

分析以下代码的输出：

```python
import gmsh
gmsh.initialize()
gmsh.model.add("boolean_test")

# 创建两个相交的盒子
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1, 1)
gmsh.model.occ.addBox(0.5, 0.5, 0.5, 1, 1, 1, 2)

# 布尔并集
gmsh.model.occ.fuse([(3, 1)], [(3, 2)])
gmsh.model.occ.synchronize()

# 检查结果
entities = gmsh.model.getEntities(3)
print(f"Volume entities: {entities}")

gmsh.fltk.run()
gmsh.finalize()
```

### 2. 【进阶】追踪布尔运算代码

分析布尔运算的完整流程：

```bash
# 查找API入口
grep -rn "fuse\|cut\|intersect" api/gmsh.cpp | head -20

# 查找OCC实现
grep -A 50 "booleanUnion\|booleanDifference" src/geo/OCC_Internals.cpp | head -80
```

绘制调用链：
```
gmsh::model::occ::fuse()
    ↓
OCC_Internals::booleanUnion()
    ↓
BRepAlgoAPI_Fuse
    ↓
结果处理和tag分配
```

### 3. 【挑战】实现几何对称操作

编写创建对称几何的代码：

```python
import gmsh
gmsh.initialize()
gmsh.model.add("symmetry_test")

# 创建原始几何（半个翼型）
# ...

# 复制并镜像
gmsh.model.occ.copy([(3, 1)])  # 返回新tag
# 获取新创建的实体
new_tags = gmsh.model.occ.copy([(3, 1)])
# 镜像变换
gmsh.model.occ.mirror(new_tags, 0, 1, 0, 0)  # 关于xz平面镜像

# 合并
gmsh.model.occ.fuse([(3, 1)], new_tags)
gmsh.model.occ.synchronize()

gmsh.fltk.run()
gmsh.finalize()
```

---

## 今日检查点

- [ ] 理解四种布尔运算（Union、Difference、Intersection、Fragments）
- [ ] 理解几何变换的实现（平移、旋转、缩放、镜像）
- [ ] 理解圆角和倒角操作
- [ ] 能使用Python API执行布尔运算

---

## 核心概念

### 布尔运算总结

| 运算 | API | OCC类 | 结果 |
|------|-----|-------|------|
| 并集 | fuse | BRepAlgoAPI_Fuse | A ∪ B |
| 差集 | cut | BRepAlgoAPI_Cut | A - B |
| 交集 | intersect | BRepAlgoAPI_Common | A ∩ B |
| 分割 | fragment | BRepAlgoAPI_BuilderAlgo | 不重叠片段 |

### 几何变换总结

| 变换 | API | OCC类 | 参数 |
|------|-----|-------|------|
| 平移 | translate | gp_Trsf::SetTranslation | dx, dy, dz |
| 旋转 | rotate | gp_Trsf::SetRotation | 轴、角度 |
| 缩放 | dilate | gp_GTrsf | 中心、因子 |
| 镜像 | mirror | gp_Trsf::SetMirror | 平面 |

### 布尔运算失败的常见原因

```
布尔运算可能失败的情况：
┌─────────────────────────────────────────────────────────────┐
│ 1. 几何不相交或相切                                         │
│    - 两个形状没有交集                                       │
│    - 仅在边或面上接触（需要增加容差）                        │
│                                                             │
│ 2. 几何退化                                                 │
│    - 零厚度的薄片                                           │
│    - 自相交的几何                                           │
│                                                             │
│ 3. 数值精度问题                                             │
│    - 坐标值过大或过小                                       │
│    - 需要调整 Geometry.Tolerance                            │
│                                                             │
│ 4. 复杂拓扑                                                 │
│    - 非流形边                                               │
│    - 内部孔洞处理                                           │
└─────────────────────────────────────────────────────────────┘

解决方法：
- 调整 gmsh.option.setNumber("Geometry.Tolerance", 1e-6)
- 使用 gmsh.model.occ.removeAllDuplicates() 清理
- 检查输入几何的有效性
```

---

## 源码导航

### 关键文件

```
src/geo/
├── OCC_Internals.cpp    # 布尔运算实现（主要）
├── GModelFactory.cpp    # 几何创建工厂
└── GeoStringInterface.cpp # GEO脚本接口

api/
├── gmsh.cpp             # API入口
└── gmshc.cpp            # C API包装
```

### 搜索命令

```bash
# 查找布尔运算
grep -n "booleanUnion\|booleanDifference\|booleanIntersection" src/geo/OCC_Internals.cpp

# 查找几何变换
grep -n "translate\|rotate\|dilate\|mirror" src/geo/OCC_Internals.cpp

# 查找圆角倒角
grep -n "fillet\|chamfer" src/geo/OCC_Internals.cpp
```

---

## 产出

- 理解布尔运算实现
- 掌握几何变换方法

---

## 导航

- **上一天**：[Day 31 - OpenCASCADE集成](day-31.md)
- **下一天**：[Day 33 - CAD文件导入流程](day-33.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
