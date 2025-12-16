# API函数索引

本节提供Gmsh Python API的完整函数索引，按命名空间组织，方便快速查阅。

## 命名空间概览

```
gmsh
├── initialize()           # 初始化
├── finalize()            # 清理
├── open()                # 打开文件
├── write()               # 保存文件
├── clear()               # 清空模型
│
├── model                 # 模型操作
│   ├── add()             # 添加模型
│   ├── remove()          # 删除模型
│   ├── list()            # 列出模型
│   ├── setCurrent()      # 设置当前模型
│   ├── getEntities()     # 获取实体
│   ├── addPhysicalGroup()# 添加物理组
│   │
│   ├── geo               # 内置几何内核
│   ├── occ               # OpenCASCADE内核
│   └── mesh              # 网格操作
│
├── option                # 选项设置
├── view                  # 后处理视图
├── plugin               # 插件
├── graphics             # 图形
├── fltk                 # GUI
└── logger               # 日志
```

## gmsh (顶层命名空间)

### 初始化与清理

| 函数 | 描述 |
|------|------|
| `initialize(argv=[], readConfigFiles=True, run=False)` | 初始化Gmsh，必须在任何其他API调用之前 |
| `finalize()` | 清理Gmsh资源，释放所有内存 |
| `isInitialized()` | 检查Gmsh是否已初始化 |

### 文件操作

| 函数 | 描述 |
|------|------|
| `open(fileName)` | 打开文件（.geo, .msh, .step等） |
| `merge(fileName)` | 合并文件到当前模型 |
| `write(fileName)` | 保存到文件 |
| `clear()` | 清空所有模型 |

### 示例

```python
import gmsh

# 初始化
gmsh.initialize()

# 打开文件
gmsh.open("model.step")

# 保存
gmsh.write("output.msh")

# 清理
gmsh.finalize()
```

## gmsh.model

### 模型管理

| 函数 | 描述 |
|------|------|
| `add(name)` | 创建新模型 |
| `remove()` | 删除当前模型 |
| `list()` | 返回所有模型名称列表 |
| `getCurrent()` | 获取当前模型名称 |
| `setCurrent(name)` | 设置当前模型 |
| `getFileName()` | 获取当前模型文件名 |
| `setFileName(fileName)` | 设置当前模型文件名 |

### 实体操作

| 函数 | 描述 |
|------|------|
| `getEntities(dim=-1)` | 获取所有实体，dim=-1返回所有维度 |
| `setEntityName(dim, tag, name)` | 设置实体名称 |
| `getEntityName(dim, tag)` | 获取实体名称 |
| `removeEntities(dimTags, recursive=False)` | 删除实体 |
| `getType(dim, tag)` | 获取实体类型（Point, Line, Surface, Volume） |
| `getParent(dim, tag)` | 获取父实体 |

### 物理组

| 函数 | 描述 |
|------|------|
| `addPhysicalGroup(dim, tags, tag=-1, name="")` | 添加物理组 |
| `removePhysicalGroups(dimTags=[])` | 删除物理组 |
| `getPhysicalGroups(dim=-1)` | 获取所有物理组 |
| `getEntitiesForPhysicalGroup(dim, tag)` | 获取物理组中的实体 |
| `getPhysicalGroupsForEntity(dim, tag)` | 获取实体所属的物理组 |
| `setPhysicalName(dim, tag, name)` | 设置物理组名称 |
| `getPhysicalName(dim, tag)` | 获取物理组名称 |

### 边界与邻接

| 函数 | 描述 |
|------|------|
| `getBoundary(dimTags, combined=True, oriented=True, recursive=False)` | 获取边界 |
| `getAdjacencies(dim, tag)` | 获取邻接实体 |
| `getPartitions(dim, tag)` | 获取分区信息 |

### 几何查询

| 函数 | 描述 |
|------|------|
| `getBoundingBox(dim, tag)` | 获取包围盒 (xmin,ymin,zmin,xmax,ymax,zmax) |
| `getDimension()` | 获取模型维度 |
| `getValue(dim, tag, parametricCoord)` | 在参数坐标处求值 |
| `getDerivative(dim, tag, parametricCoord)` | 获取导数 |
| `getCurvature(dim, tag, parametricCoord)` | 获取曲率 |
| `getPrincipalCurvatures(tag, parametricCoord)` | 获取主曲率 |
| `getNormal(tag, parametricCoord)` | 获取法向量 |
| `getParameterization(dim, tag, coord)` | 获取参数化坐标 |
| `getParametrizationBounds(dim, tag)` | 获取参数范围 |
| `isInside(dim, tag, coord)` | 检查点是否在实体内 |
| `getClosestPoint(dim, tag, coord)` | 获取最近点 |

### 示例

```python
import gmsh

gmsh.initialize()
gmsh.model.add("example")

# 创建几何
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

# 获取所有面
surfaces = gmsh.model.getEntities(2)
print(f"面数量: {len(surfaces)}")

# 创建物理组
gmsh.model.addPhysicalGroup(2, [1, 2], name="walls")
gmsh.model.addPhysicalGroup(3, [1], name="domain")

# 获取物理组
groups = gmsh.model.getPhysicalGroups()
for dim, tag in groups:
    name = gmsh.model.getPhysicalName(dim, tag)
    print(f"物理组: dim={dim}, tag={tag}, name={name}")

gmsh.finalize()
```

## gmsh.model.geo

### 点

| 函数 | 描述 |
|------|------|
| `addPoint(x, y, z, meshSize=0., tag=-1)` | 添加点 |

### 曲线

| 函数 | 描述 |
|------|------|
| `addLine(startTag, endTag, tag=-1)` | 添加直线 |
| `addCircleArc(startTag, centerTag, endTag, tag=-1, nx=0, ny=0, nz=0)` | 添加圆弧 |
| `addEllipseArc(startTag, centerTag, majorTag, endTag, tag=-1, nx=0, ny=0, nz=0)` | 添加椭圆弧 |
| `addSpline(pointTags, tag=-1)` | 添加Catmull-Rom样条 |
| `addBSpline(pointTags, tag=-1, degree=-1, weights=[], knots=[], multiplicities=[])` | 添加B样条 |
| `addBezier(pointTags, tag=-1)` | 添加贝塞尔曲线 |
| `addPolyline(pointTags, tag=-1)` | 添加折线 |
| `addCompoundSpline(curveTags, numIntervals=5, tag=-1)` | 添加复合样条 |
| `addCompoundBSpline(curveTags, numIntervals=20, tag=-1)` | 添加复合B样条 |

### 曲面

| 函数 | 描述 |
|------|------|
| `addCurveLoop(curveTags, tag=-1, reorient=False)` | 添加曲线环 |
| `addCurveLoops(curveTags)` | 自动创建曲线环 |
| `addPlaneSurface(wireTags, tag=-1)` | 添加平面 |
| `addSurfaceFilling(wireTags, tag=-1, sphereCenterTag=-1)` | 添加填充曲面 |

### 实体

| 函数 | 描述 |
|------|------|
| `addSurfaceLoop(surfaceTags, tag=-1)` | 添加曲面环 |
| `addVolume(shellTags, tag=-1)` | 添加体积 |

### 几何变换

| 函数 | 描述 |
|------|------|
| `copy(dimTags)` | 复制实体 |
| `remove(dimTags, recursive=False)` | 删除实体 |
| `translate(dimTags, dx, dy, dz)` | 平移 |
| `rotate(dimTags, x, y, z, ax, ay, az, angle)` | 旋转 |
| `dilate(dimTags, x, y, z, a, b, c)` | 缩放 |
| `mirror(dimTags, a, b, c, d)` | 镜像 |
| `symmetrize(dimTags, a, b, c, d)` | 对称（同mirror） |
| `affineTransform(dimTags, a)` | 仿射变换 |

### 拉伸

| 函数 | 描述 |
|------|------|
| `extrude(dimTags, dx, dy, dz, numElements=[], heights=[], recombine=False)` | 线性拉伸 |
| `revolve(dimTags, x, y, z, ax, ay, az, angle, numElements=[], heights=[], recombine=False)` | 旋转拉伸 |
| `twist(dimTags, x, y, z, dx, dy, dz, ax, ay, az, angle, numElements=[], heights=[], recombine=False)` | 扭转拉伸 |
| `extrudeBoundaryLayer(dimTags, numElements=[], heights=[], recombine=False, second=False, viewIndex=-1)` | 边界层拉伸 |

### 同步

| 函数 | 描述 |
|------|------|
| `synchronize()` | 同步几何到模型（必须调用！） |

### Transfinite

| 函数 | 描述 |
|------|------|
| `mesh.setTransfiniteCurve(tag, numNodes, meshType="Progression", coef=1.)` | 设置曲线Transfinite |
| `mesh.setTransfiniteSurface(tag, arrangement="Left", cornerTags=[])` | 设置曲面Transfinite |
| `mesh.setTransfiniteVolume(tag, cornerTags=[])` | 设置体积Transfinite |
| `mesh.setRecombine(dim, tag, angle=45.)` | 设置重组（四边形/六面体） |
| `mesh.setSmoothing(dim, tag, val)` | 设置光滑迭代次数 |

## gmsh.model.occ

### 基本实体

| 函数 | 描述 |
|------|------|
| `addPoint(x, y, z, meshSize=0., tag=-1)` | 添加点 |
| `addLine(startTag, endTag, tag=-1)` | 添加直线 |
| `addCircleArc(startTag, centerTag, endTag, tag=-1)` | 添加圆弧 |
| `addCircle(x, y, z, r, tag=-1, angle1=0, angle2=2*pi, nx=0, ny=0, nz=1)` | 添加完整圆 |
| `addEllipseArc(startTag, centerTag, majorTag, endTag, tag=-1)` | 添加椭圆弧 |
| `addEllipse(x, y, z, r1, r2, tag=-1, angle1=0, angle2=2*pi, nx=0, ny=0, nz=1)` | 添加椭圆 |
| `addSpline(pointTags, tag=-1)` | 添加B样条插值曲线 |
| `addBSpline(pointTags, tag=-1, degree=-1, weights=[], knots=[], multiplicities=[])` | 添加B样条 |
| `addBezier(pointTags, tag=-1)` | 添加贝塞尔曲线 |

### 曲线环与曲面

| 函数 | 描述 |
|------|------|
| `addCurveLoop(curveTags, tag=-1)` | 添加曲线环 |
| `addRectangle(x, y, z, dx, dy, tag=-1, roundedRadius=0.)` | 添加矩形 |
| `addDisk(xc, yc, zc, rx, ry, tag=-1, zAxis=[], xAxis=[])` | 添加圆盘 |
| `addPlaneSurface(wireTags, tag=-1)` | 添加平面 |
| `addSurfaceFilling(wireTag, tag=-1, pointTags=[], degree=2, numPointsOnCurves=15, numIter=2, anisotropic=False, tol2d=1e-8, tol3d=1e-4, tolAng=1e-2, tolCurv=1e-2, maxSegments=0, maxDegree=8)` | 添加曲面填充 |
| `addBSplineFilling(wireTag, tag=-1, type="Stretch")` | 添加B样条填充 |
| `addBezierFilling(wireTag, tag=-1, type="Stretch")` | 添加贝塞尔填充 |
| `addTrimmedSurface(surfaceTag, wireTags=[], tag=-1)` | 添加裁剪曲面 |

### 3D基本体

| 函数 | 描述 |
|------|------|
| `addSurfaceLoop(surfaceTags, tag=-1, sewing=False)` | 添加曲面环 |
| `addVolume(shellTags, tag=-1)` | 添加体积 |
| `addSphere(xc, yc, zc, radius, tag=-1, angle1=-pi/2, angle2=pi/2, angle3=2*pi)` | 添加球 |
| `addBox(x, y, z, dx, dy, dz, tag=-1)` | 添加立方体 |
| `addCylinder(x, y, z, dx, dy, dz, r, tag=-1, angle=2*pi)` | 添加圆柱 |
| `addCone(x, y, z, dx, dy, dz, r1, r2, tag=-1, angle=2*pi)` | 添加圆锥 |
| `addWedge(x, y, z, dx, dy, dz, tag=-1, ltx=0., zAxis=[])` | 添加楔形 |
| `addTorus(x, y, z, r1, r2, tag=-1, angle=2*pi, zAxis=[])` | 添加圆环 |

### 高级建模

| 函数 | 描述 |
|------|------|
| `addThruSections(wireTags, tag=-1, makeSolid=True, makeRuled=False, maxDegree=-1, continuity="")` | 过渡曲面/实体 |
| `addThruSectionsI(wireTags, tag=-1, makeSolid=True, makeRuled=False, maxDegree=-1, continuity="")` | 增量过渡 |
| `addPipe(dimTags, wireTag, trihedron="DiscreteTrihedron")` | 沿路径扫掠 |
| `fillet(volumeTags, curveTags, radii, removeVolume=True)` | 倒圆角 |
| `chamfer(volumeTags, curveTags, surfaceTags, distances, removeVolume=True)` | 倒斜角 |

### 布尔运算

| 函数 | 描述 |
|------|------|
| `fuse(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)` | 并集 |
| `cut(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)` | 差集 |
| `intersect(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)` | 交集 |
| `fragment(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)` | 分片 |

### 几何变换

| 函数 | 描述 |
|------|------|
| `copy(dimTags)` | 复制 |
| `remove(dimTags, recursive=False)` | 删除 |
| `translate(dimTags, dx, dy, dz)` | 平移 |
| `rotate(dimTags, x, y, z, ax, ay, az, angle)` | 旋转 |
| `dilate(dimTags, x, y, z, a, b, c)` | 缩放 |
| `mirror(dimTags, a, b, c, d)` | 镜像 |
| `affineTransform(dimTags, a)` | 仿射变换 |

### 拉伸

| 函数 | 描述 |
|------|------|
| `extrude(dimTags, dx, dy, dz, numElements=[], heights=[], recombine=False)` | 线性拉伸 |
| `revolve(dimTags, x, y, z, ax, ay, az, angle, numElements=[], heights=[], recombine=False)` | 旋转拉伸 |
| `addPipe(dimTags, wireTag, ...)` | 沿路径拉伸 |

### CAD导入导出

| 函数 | 描述 |
|------|------|
| `importShapes(fileName, highestDimOnly=True, format="")` | 导入CAD文件 |
| `importShapesNativePointer(shape, highestDimOnly=True)` | 从OpenCASCADE指针导入 |
| `getEntities(dim=-1)` | 获取OCC实体 |
| `getEntitiesInBoundingBox(xmin, ymin, zmin, xmax, ymax, zmax, dim=-1)` | 获取包围盒内实体 |
| `getBoundingBox(dim, tag)` | 获取包围盒 |
| `getMass(dim, tag)` | 获取质量/面积/长度 |
| `getCenterOfMass(dim, tag)` | 获取质心 |

### 同步与修复

| 函数 | 描述 |
|------|------|
| `synchronize()` | 同步到Gmsh模型（必须调用！） |
| `healShapes(dimTags=[], tolerance=1e-8, fixDegenerated=True, fixSmallEdges=True, fixSmallFaces=True, sewFaces=True, makeSolids=True)` | 修复几何 |
| `convertToNURBS(dimTags)` | 转换为NURBS |

## gmsh.model.mesh

### 网格生成

| 函数 | 描述 |
|------|------|
| `generate(dim=3)` | 生成网格 |
| `recombine()` | 重组（三角形→四边形） |
| `refine()` | 全局细化 |
| `optimize(method="", force=False, niter=1, dimTags=[])` | 优化网格 |
| `setOrder(order)` | 设置单元阶次 |
| `getLastEntityError()` | 获取最后错误的实体 |
| `getLastNodeError()` | 获取最后错误的节点 |
| `clear(dimTags=[])` | 清除网格 |

### 网格尺寸

| 函数 | 描述 |
|------|------|
| `setSize(dimTags, size)` | 设置实体的网格尺寸 |
| `setSizeAtParametricPoints(tag, parametricCoord, sizes)` | 在参数点设置尺寸 |
| `setSizeCallback(callback)` | 设置尺寸回调函数 |

### 节点操作

| 函数 | 描述 |
|------|------|
| `getNodes(dim=-1, tag=-1, includeBoundary=False, returnParametricCoord=True)` | 获取节点 |
| `getNodesByElementType(elementType, tag=-1, returnParametricCoord=True)` | 按元素类型获取节点 |
| `setNodes(dim, tag, nodeTags, coord, parametricCoord=[])` | 设置节点 |
| `addNodes(dim, tag, nodeTags, coord, parametricCoord=[])` | 添加节点 |
| `reclassifyNodes()` | 重分类节点 |
| `relocateNodes(dim=-1, tag=-1)` | 重定位节点 |

### 元素操作

| 函数 | 描述 |
|------|------|
| `getElements(dim=-1, tag=-1)` | 获取元素 |
| `getElement(elementTag)` | 获取单个元素 |
| `getElementByCoordinates(x, y, z, dim=-1, strict=False)` | 按坐标获取元素 |
| `setElements(dim, tag, elementTypes, elementTags, nodeTags)` | 设置元素 |
| `addElements(dim, tag, elementTypes, elementTags, nodeTags)` | 添加元素 |
| `addElementsByType(tag, elementType, elementTags, nodeTags)` | 按类型添加元素 |
| `getElementTypes(dim=-1, tag=-1)` | 获取元素类型 |
| `getElementProperties(elementType)` | 获取元素属性 |
| `getElementsByType(elementType, tag=-1)` | 按类型获取元素 |

### 雅可比与积分

| 函数 | 描述 |
|------|------|
| `getJacobian(elementTag, localCoord)` | 获取雅可比矩阵 |
| `getJacobians(elementType, localCoord, tag=-1, task=0, numTasks=1)` | 批量获取雅可比 |
| `preallocateJacobians(elementType, numEvaluationPoints, allocateJacobians, allocateDeterminants, allocateCoord, tag=-1)` | 预分配空间 |
| `getBasisFunctions(elementType, localCoord, functionSpaceType, numComponents=-1, wantedOrientations=[])` | 获取基函数 |
| `getBasisFunctionsOrientation(elementType, functionSpaceType, tag=-1, task=0, numTasks=1)` | 获取基函数方向 |
| `getIntegrationPoints(elementType, integrationType)` | 获取积分点 |

### 质量检查

| 函数 | 描述 |
|------|------|
| `getElementQualities(elementTags, qualityType="minSICN", task=0, numTasks=1)` | 获取单元质量 |

### 网格分区

| 函数 | 描述 |
|------|------|
| `partition(numPart)` | 分区网格 |
| `unpartition()` | 取消分区 |
| `getPartitions(dim, tag)` | 获取实体分区 |

### 周期性

| 函数 | 描述 |
|------|------|
| `setPeriodic(dim, tags, tagsMaster, affineTransform)` | 设置周期性约束 |
| `getPeriodicNodes(dim, tag, includeHighOrderNodes=False)` | 获取周期节点 |
| `getPeriodicKeys(elementType, functionSpaceType, tag, returnCoord=True)` | 获取周期键 |

### 嵌入实体

| 函数 | 描述 |
|------|------|
| `embed(dim, tags, inDim, inTag)` | 嵌入实体 |
| `removeEmbedded(dimTags, dim=-1)` | 移除嵌入 |
| `getEmbedded(dim, tag)` | 获取嵌入实体 |

## gmsh.model.mesh.field

### 尺寸场

| 函数 | 描述 |
|------|------|
| `add(fieldType, tag=-1)` | 添加尺寸场 |
| `remove(tag)` | 删除尺寸场 |
| `list()` | 列出所有尺寸场 |
| `getType(tag)` | 获取场类型 |
| `setNumber(tag, option, value)` | 设置数值选项 |
| `getNumber(tag, option)` | 获取数值选项 |
| `setString(tag, option, value)` | 设置字符串选项 |
| `getString(tag, option)` | 获取字符串选项 |
| `setNumbers(tag, option, values)` | 设置数值列表选项 |
| `getNumbers(tag, option)` | 获取数值列表选项 |
| `setAsBackgroundMesh(tag)` | 设置为背景网格 |

### 常用场类型

| 类型 | 描述 |
|------|------|
| `Distance` | 距离场 |
| `Threshold` | 阈值场 |
| `Box` | 盒子场 |
| `Cylinder` | 圆柱场 |
| `Ball` | 球场 |
| `MathEval` | 数学表达式场 |
| `PostView` | 后处理视图场 |
| `Min` | 最小值场 |
| `Max` | 最大值场 |
| `Restrict` | 限制场 |
| `Constant` | 常数场 |
| `BoundaryLayer` | 边界层场 |

## gmsh.option

### 选项操作

| 函数 | 描述 |
|------|------|
| `setNumber(name, value)` | 设置数值选项 |
| `getNumber(name)` | 获取数值选项 |
| `setString(name, value)` | 设置字符串选项 |
| `getString(name)` | 获取字符串选项 |
| `setColor(name, r, g, b, a=255)` | 设置颜色选项 |
| `getColor(name)` | 获取颜色选项 |

### 示例

```python
import gmsh

gmsh.initialize()

# 设置网格算法
gmsh.option.setNumber("Mesh.Algorithm", 6)  # Frontal-Delaunay

# 设置网格尺寸
gmsh.option.setNumber("Mesh.MeshSizeMin", 0.1)
gmsh.option.setNumber("Mesh.MeshSizeMax", 1.0)

# 获取当前值
alg = gmsh.option.getNumber("Mesh.Algorithm")
print(f"网格算法: {alg}")

gmsh.finalize()
```

## gmsh.view

### 视图管理

| 函数 | 描述 |
|------|------|
| `add(name, tag=-1)` | 添加视图 |
| `remove(tag)` | 删除视图 |
| `getIndex(tag)` | 获取视图索引 |
| `getTags()` | 获取所有视图标签 |
| `addHomogeneousModelData(tag, step, modelName, dataType, tags, data, ...)` | 添加均匀模型数据 |
| `addModelData(tag, step, modelName, dataType, tags, data, ...)` | 添加模型数据 |
| `addListData(tag, dataType, numElements, data)` | 添加列表数据 |
| `addListDataString(tag, coord, data, style=[])` | 添加字符串数据 |
| `getHomogeneousModelData(tag, step)` | 获取均匀模型数据 |
| `getModelData(tag, step)` | 获取模型数据 |
| `getListData(tag)` | 获取列表数据 |
| `probe(tag, x, y, z, step=-1, ...)` | 探测视图值 |
| `write(tag, fileName, append=False)` | 写入文件 |
| `combine(how, what, remove=True, ...)` | 合并视图 |

### 视图选项

| 函数 | 描述 |
|------|------|
| `option.setNumber(tag, option, value)` | 设置视图数值选项 |
| `option.getNumber(tag, option)` | 获取视图数值选项 |
| `option.setString(tag, option, value)` | 设置视图字符串选项 |
| `option.getString(tag, option)` | 获取视图字符串选项 |
| `option.setColor(tag, option, r, g, b, a=255)` | 设置视图颜色 |
| `option.getColor(tag, option)` | 获取视图颜色 |

## gmsh.plugin

| 函数 | 描述 |
|------|------|
| `setNumber(name, option, value)` | 设置插件数值选项 |
| `setString(name, option, value)` | 设置插件字符串选项 |
| `run(name)` | 运行插件 |

## gmsh.fltk

### GUI操作

| 函数 | 描述 |
|------|------|
| `initialize()` | 初始化GUI |
| `finalize()` | 关闭GUI |
| `wait(time=-1.)` | 等待用户输入 |
| `update()` | 更新GUI |
| `awake(action="")` | 唤醒GUI |
| `lock()` | 锁定GUI |
| `unlock()` | 解锁GUI |
| `run()` | 运行GUI事件循环 |
| `isAvailable()` | 检查GUI是否可用 |
| `selectEntities(dim=-1)` | 选择实体 |
| `selectElements()` | 选择元素 |
| `selectViews()` | 选择视图 |
| `setStatusMessage(message, graphics=False)` | 设置状态栏消息 |
| `showContextWindow(dim, tag)` | 显示实体上下文窗口 |
| `openProject(fileName)` | 打开项目 |

## gmsh.logger

| 函数 | 描述 |
|------|------|
| `write(message, level="info")` | 写入日志 |
| `start()` | 开始记录日志 |
| `stop()` | 停止记录日志 |
| `get()` | 获取日志内容 |
| `getLastError()` | 获取最后错误 |

## 快速参考示例

### 完整工作流

```python
import gmsh
import numpy as np

# 初始化
gmsh.initialize()
gmsh.model.add("complete_example")

# 创建几何（OCC内核）
box = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
sphere = gmsh.model.occ.addSphere(0.5, 0.5, 0.5, 0.3)
result = gmsh.model.occ.cut([(3, box)], [(3, sphere)])
gmsh.model.occ.synchronize()

# 物理组
volumes = gmsh.model.getEntities(3)
gmsh.model.addPhysicalGroup(3, [v[1] for v in volumes], name="Domain")

surfaces = gmsh.model.getEntities(2)
for i, (dim, tag) in enumerate(surfaces):
    gmsh.model.addPhysicalGroup(2, [tag], name=f"Surface_{i}")

# 网格尺寸场
gmsh.model.mesh.field.add("Ball", 1)
gmsh.model.mesh.field.setNumber(1, "XCenter", 0.5)
gmsh.model.mesh.field.setNumber(1, "YCenter", 0.5)
gmsh.model.mesh.field.setNumber(1, "ZCenter", 0.5)
gmsh.model.mesh.field.setNumber(1, "Radius", 0.4)
gmsh.model.mesh.field.setNumber(1, "VIn", 0.02)
gmsh.model.mesh.field.setNumber(1, "VOut", 0.1)
gmsh.model.mesh.field.setAsBackgroundMesh(1)

# 生成网格
gmsh.model.mesh.generate(3)
gmsh.model.mesh.optimize("Netgen")

# 获取网格数据
node_tags, node_coords, _ = gmsh.model.mesh.getNodes()
elem_types, elem_tags, elem_nodes = gmsh.model.mesh.getElements(3)

print(f"节点数: {len(node_tags)}")
print(f"四面体数: {len(elem_tags[0]) if elem_tags else 0}")

# 保存
gmsh.write("example.msh")

# 清理
gmsh.finalize()
```

本索引涵盖了Gmsh Python API的主要函数。更多详细信息请参阅官方文档。
