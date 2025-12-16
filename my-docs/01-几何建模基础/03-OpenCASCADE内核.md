# OpenCASCADE 几何内核 (gmsh.model.occ)

OpenCASCADE (OCC) 是工业级的开源CAD内核，提供了比内置内核更强大的几何建模能力。

## API 命名空间

```python
gmsh.model.occ.*  # 所有OCC几何操作
```

## OCC vs Geo 对比

| 功能 | geo (内置) | occ (OpenCASCADE) |
|------|-----------|-------------------|
| 布尔运算 | ❌ | ✅ |
| 基本图元 | 需手动构建 | ✅ 直接创建 |
| CAD导入 | 有限 | ✅ STEP/IGES/BREP |
| 圆角/倒角 | ❌ | ✅ |
| 偏移/壳体 | ❌ | ✅ |
| 几何修复 | ❌ | ✅ |
| 高级曲面 | 有限 | ✅ NURBS/BSpline |

## 基本图元

### 2D 基本图元

```python
# 矩形
# addRectangle(x, y, z, dx, dy, tag=-1, roundedRadius=0)
rect = gmsh.model.occ.addRectangle(0, 0, 0, 2, 1)  # 2x1矩形

# 带圆角的矩形
rect_rounded = gmsh.model.occ.addRectangle(0, 0, 0, 2, 1, roundedRadius=0.1)

# 圆盘/椭圆盘
# addDisk(xc, yc, zc, rx, ry, tag=-1, zAxis=[], xAxis=[])
disk = gmsh.model.occ.addDisk(0, 0, 0, 1, 1)       # 圆（rx=ry）
ellipse = gmsh.model.occ.addDisk(0, 0, 0, 2, 1)    # 椭圆（rx≠ry）
```

### 3D 基本图元

```python
# 长方体
# addBox(x, y, z, dx, dy, dz, tag=-1)
box = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)

# 球
# addSphere(xc, yc, zc, radius, tag=-1, angle1=-pi/2, angle2=pi/2, angle3=2*pi)
sphere = gmsh.model.occ.addSphere(0, 0, 0, 1)

# 部分球（半球）
hemisphere = gmsh.model.occ.addSphere(0, 0, 0, 1, angle2=0)

# 圆柱
# addCylinder(x, y, z, dx, dy, dz, r, tag=-1, angle=2*pi)
cylinder = gmsh.model.occ.addCylinder(0, 0, 0, 0, 0, 1, 0.5)  # 沿Z轴，半径0.5

# 部分圆柱（90度扇形）
quarter_cyl = gmsh.model.occ.addCylinder(0, 0, 0, 0, 0, 1, 0.5, angle=math.pi/2)

# 圆锥
# addCone(x, y, z, dx, dy, dz, r1, r2, tag=-1, angle=2*pi)
cone = gmsh.model.occ.addCone(0, 0, 0, 0, 0, 1, 0.5, 0)  # 底半径0.5，顶半径0
truncated_cone = gmsh.model.occ.addCone(0, 0, 0, 0, 0, 1, 0.5, 0.2)  # 截锥

# 楔形
# addWedge(x, y, z, dx, dy, dz, tag=-1, ltx=0, zAxis=[])
wedge = gmsh.model.occ.addWedge(0, 0, 0, 1, 1, 1)

# 圆环
# addTorus(xc, yc, zc, r1, r2, tag=-1, angle=-1, zAxis=[])
torus = gmsh.model.occ.addTorus(0, 0, 0, 1, 0.3)  # 大半径1，小半径0.3
```

## 点和曲线

### 点

```python
# addPoint(x, y, z, meshSize=0, tag=-1)
p = gmsh.model.occ.addPoint(0, 0, 0, 0.1)
```

### 曲线

```python
# 直线
# addLine(startTag, endTag, tag=-1)
line = gmsh.model.occ.addLine(p1, p2)

# 圆弧
# addCircleArc(startTag, centerTag, endTag, tag=-1)
arc = gmsh.model.occ.addCircleArc(p1, p_center, p2)

# 完整圆
# addCircle(xc, yc, zc, r, tag=-1, angle1=0, angle2=2*pi, zAxis=[], xAxis=[])
circle = gmsh.model.occ.addCircle(0, 0, 0, 1)

# 椭圆
# addEllipse(xc, yc, zc, r1, r2, tag=-1, angle1=0, angle2=2*pi, zAxis=[], xAxis=[])
ellipse = gmsh.model.occ.addEllipse(0, 0, 0, 2, 1)

# 样条
# addSpline(pointTags, tag=-1)
spline = gmsh.model.occ.addSpline([p1, p2, p3, p4])

# B样条
# addBSpline(pointTags, tag=-1, degree=3, weights=[], knots=[], multiplicities=[])
bspline = gmsh.model.occ.addBSpline([p1, p2, p3, p4])

# Bezier曲线
# addBezier(pointTags, tag=-1)
bezier = gmsh.model.occ.addBezier([p1, p2, p3, p4])

# 折线/多段线
# addWire(curveTags, tag=-1, checkClosed=False)
wire = gmsh.model.occ.addWire([l1, l2, l3])
```

## 曲面

### 曲线环和平面

```python
# 曲线环
# addCurveLoop(curveTags, tag=-1)
cl = gmsh.model.occ.addCurveLoop([l1, l2, l3, l4])

# 平面曲面
# addPlaneSurface(wireTags, tag=-1)
surface = gmsh.model.occ.addPlaneSurface([cl])

# 带孔平面
surface_with_hole = gmsh.model.occ.addPlaneSurface([outer_loop, hole_loop])
```

### 高级曲面

```python
# B样条曲面
# addBSplineSurface(pointTags, numPointsU, tag=-1, degreeU=3, degreeV=3, ...)
# pointTags: 控制点网格（行优先）
surface = gmsh.model.occ.addBSplineSurface(
    control_points,
    numPointsU=4  # U方向4个点
)

# Bezier曲面
# addBezierSurface(pointTags, numPointsU, tag=-1)
bezier_surface = gmsh.model.occ.addBezierSurface(control_points, numPointsU=4)

# 曲面填充
# addSurfaceFilling(wireTag, tag=-1, pointTags=[], degree=3, ...)
filling = gmsh.model.occ.addSurfaceFilling(wire_tag)

# 偏移曲面
# addOffset(dimTags, offset, tag=-1, removeOriginal=False)
offset_surfaces = gmsh.model.occ.addOffset([(2, 1)], 0.1)
```

## 高级实体建模

### 通过截面 (ThruSections)

```python
# addThruSections(wireTags, tag=-1, makeSolid=True, makeRuled=False, maxDegree=-1)
# 通过多个截面轮廓创建实体

# 创建截面轮廓
wire1 = gmsh.model.occ.addCurveLoop([gmsh.model.occ.addCircle(0, 0, 0, 1)])
wire2 = gmsh.model.occ.addCurveLoop([gmsh.model.occ.addCircle(0, 0, 1, 0.5)])
wire3 = gmsh.model.occ.addCurveLoop([gmsh.model.occ.addCircle(0, 0, 2, 0.8)])

# 创建通过截面的实体
solid = gmsh.model.occ.addThruSections([wire1, wire2, wire3])
```

### 管道 (Pipe)

```python
# addPipe(dimTags, wireTag, tag=-1, trihedron="")
# 沿路径拉伸截面

# 创建路径
path = gmsh.model.occ.addSpline([p1, p2, p3, p4])
path_wire = gmsh.model.occ.addWire([path])

# 创建截面
section = gmsh.model.occ.addDisk(0, 0, 0, 0.1, 0.1)

# 沿路径拉伸
pipe = gmsh.model.occ.addPipe([(2, section)], path_wire)
```

### 拉伸

```python
# extrude(dimTags, dx, dy, dz, numElements=[], heights=[], recombine=False)
extruded = gmsh.model.occ.extrude([(2, 1)], 0, 0, 1)

# revolve(dimTags, x, y, z, ax, ay, az, angle, numElements=[], heights=[], recombine=False)
revolved = gmsh.model.occ.revolve([(2, 1)], 0, 0, 0, 0, 0, 1, math.pi)
```

## 布尔运算

### 并集 (Fuse/Union)

```python
# fuse(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)
# 返回：(结果dimTags, 映射关系)

box1 = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
box2 = gmsh.model.occ.addBox(0.5, 0, 0, 1, 1, 1)

# A ∪ B
result, map = gmsh.model.occ.fuse([(3, box1)], [(3, box2)])
```

### 差集 (Cut)

```python
# cut(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)
# A - B：从object中减去tool

box = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
hole = gmsh.model.occ.addCylinder(0.5, 0.5, 0, 0, 0, 1, 0.2)

# 从立方体挖出圆柱孔
result, map = gmsh.model.occ.cut([(3, box)], [(3, hole)])
```

### 交集 (Intersect)

```python
# intersect(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)
# A ∩ B

box = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
sphere = gmsh.model.occ.addSphere(0.5, 0.5, 0.5, 0.8)

# 立方体与球的交集
result, map = gmsh.model.occ.intersect([(3, box)], [(3, sphere)])
```

### 分片 (Fragment) - 最重要！

```python
# fragment(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)
# 将重叠区域分割成多个共形部分

# 分片对于多域问题非常重要：
# - 确保共享边界只有一份网格
# - 相邻域的节点完全匹配
# - 适合有限元组装

box1 = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
box2 = gmsh.model.occ.addBox(0.5, 0, 0, 1, 1, 1)

# 分片后两个盒子共享接触面
result, map = gmsh.model.occ.fragment([(3, box1)], [(3, box2)])
```

### 布尔运算选项

```python
# removeObject/removeTool 控制是否删除原实体
result, map = gmsh.model.occ.cut(
    [(3, box)],
    [(3, hole)],
    removeObject=True,   # 删除原object
    removeTool=True      # 删除原tool
)

# 保留原实体
result, map = gmsh.model.occ.cut(
    [(3, box)],
    [(3, hole)],
    removeObject=False,
    removeTool=False
)
```

## 特征操作

### 圆角 (Fillet)

```python
# fillet(volumeTags, curveTags, radii, removeVolume=True)

box = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

# 获取所有边
edges = [e[1] for e in gmsh.model.getEntities(1)]

# 为所有边添加圆角
gmsh.model.occ.fillet([box], edges, [0.1])
```

### 倒角 (Chamfer)

```python
# chamfer(volumeTags, curveTags, surfaceTags, distances, removeVolume=True)

box = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

edges = [e[1] for e in gmsh.model.getEntities(1)]
faces = [f[1] for f in gmsh.model.getEntities(2)]

# 添加倒角
gmsh.model.occ.chamfer([box], edges, faces, [0.05])
```

## CAD 文件导入

```python
# importShapes(fileName, highestDimOnly=True, format="")
# 支持：STEP, IGES, BREP, XAO

# 导入STEP文件
shapes = gmsh.model.occ.importShapes("model.step")

# 导入IGES文件
shapes = gmsh.model.occ.importShapes("model.igs")

# 导入所有实体（包括低维实体）
shapes = gmsh.model.occ.importShapes("model.step", highestDimOnly=False)

# 必须同步！
gmsh.model.occ.synchronize()
```

## 几何查询

```python
# 获取质心
x, y, z = gmsh.model.occ.getCenterOfMass(dim, tag)

# 获取质量/长度/面积/体积
mass = gmsh.model.occ.getMass(dim, tag)

# 获取惯性矩阵
matrix = gmsh.model.occ.getMatrixOfInertia(dim, tag)

# 获取边界框
xmin, ymin, zmin, xmax, ymax, zmax = gmsh.model.occ.getBoundingBox(dim, tag)

# 获取曲面的曲线环
loops = gmsh.model.occ.getCurveLoops(surfaceTag)

# 获取体的曲面环
shells = gmsh.model.occ.getSurfaceLoops(volumeTag)
```

## 几何修复

```python
# healShapes(dimTags=[], tolerance=1e-8, fixDegenerated=True, fixSmallEdges=True,
#            fixSmallFaces=True, sewFaces=True, makeSolids=True)
# 修复导入的CAD模型中的问题

gmsh.model.occ.importShapes("problematic.step")
gmsh.model.occ.healShapes()  # 修复所有问题
gmsh.model.occ.synchronize()
```

## 同步

```python
# synchronize()
# 将OCC数据同步到Gmsh模型
# 必须在网格生成前调用！

gmsh.model.occ.synchronize()
```

## 完整示例：CSG建模

```python
#!/usr/bin/env python3
"""
使用OpenCASCADE进行CSG建模
创建经典的"球体-立方体交集减去三个正交圆柱"
"""
import gmsh
import math

gmsh.initialize()
gmsh.model.add("csg_example")

# 参数
R = 1.4          # 立方体半边长
Rs = R * 0.7     # 球半径
Rc = R * 0.5     # 圆柱半径

# 创建立方体
box = gmsh.model.occ.addBox(-R, -R, -R, 2*R, 2*R, 2*R, 1)

# 创建球
sphere = gmsh.model.occ.addSphere(0, 0, 0, Rs * 1.25, 2)

# 立方体与球的交集
intersection, _ = gmsh.model.occ.intersect([(3, box)], [(3, sphere)], 3)

# 创建三个正交圆柱
cyl_x = gmsh.model.occ.addCylinder(-2*R, 0, 0, 4*R, 0, 0, Rc, 4)
cyl_y = gmsh.model.occ.addCylinder(0, -2*R, 0, 0, 4*R, 0, Rc, 5)
cyl_z = gmsh.model.occ.addCylinder(0, 0, -2*R, 0, 0, 4*R, Rc, 6)

# 合并三个圆柱
cylinders, _ = gmsh.model.occ.fuse([(3, cyl_x), (3, cyl_y)], [(3, cyl_z)], 7)

# 从交集中减去圆柱
result, _ = gmsh.model.occ.cut(intersection, cylinders, 8)

# 同步
gmsh.model.occ.synchronize()

# 添加圆角（可选）
try:
    edges = [e[1] for e in gmsh.model.getEntities(1)]
    if edges:
        gmsh.model.occ.fillet([8], edges[:12], [0.05])  # 部分边圆角
        gmsh.model.occ.synchronize()
except:
    pass  # 如果圆角失败，继续

# 定义物理组
volumes = gmsh.model.getEntities(3)
surfaces = gmsh.model.getEntities(2)
gmsh.model.addPhysicalGroup(3, [v[1] for v in volumes], name="solid")
gmsh.model.addPhysicalGroup(2, [s[1] for s in surfaces], name="boundary")

# 网格设置
gmsh.option.setNumber("Mesh.MeshSizeMin", 0.05)
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.2)

# 生成网格
gmsh.model.mesh.generate(3)

# 保存
gmsh.write("csg_example.msh")
gmsh.write("csg_example.vtk")

# 统计
nodes, _, _ = gmsh.model.mesh.getNodes()
print(f"节点数: {len(nodes)}")

# 可视化
# gmsh.fltk.run()

gmsh.finalize()
print("CSG模型创建完成！")
```

## 下一步

- [04-几何变换](./04-几何变换.md) - 学习几何变换操作
