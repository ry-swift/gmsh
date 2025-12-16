# Python API完整指南

本章系统介绍Gmsh Python API的完整使用方法。

## API结构概览

```
gmsh
├── initialize()              # 初始化
├── finalize()                # 终止
├── open()                    # 打开文件
├── merge()                   # 合并文件
├── write()                   # 写入文件
├── clear()                   # 清除模型
│
├── model                     # 模型命名空间
│   ├── add()                 # 添加模型
│   ├── remove()              # 移除模型
│   ├── list()                # 列出模型
│   ├── setCurrent()          # 设置当前模型
│   ├── getCurrent()          # 获取当前模型
│   │
│   ├── geo                   # 内置几何内核
│   │   ├── addPoint()
│   │   ├── addLine()
│   │   ├── addCurveLoop()
│   │   ├── addPlaneSurface()
│   │   └── ...
│   │
│   ├── occ                   # OpenCASCADE内核
│   │   ├── addPoint()
│   │   ├── addBox()
│   │   ├── fuse()
│   │   ├── cut()
│   │   └── ...
│   │
│   ├── mesh                  # 网格操作
│   │   ├── generate()
│   │   ├── getNodes()
│   │   ├── getElements()
│   │   └── ...
│   │
│   └── ...
│
├── view                      # 后处理视图
├── plugin                    # 插件
├── graphics                  # 图形
├── fltk                      # GUI
├── onelab                    # ONELAB
└── option                    # 选项
```

## 初始化与终止

### 基本用法

```python
import gmsh

# 初始化Gmsh
gmsh.initialize()

# 可选：在初始化时传入命令行参数
# gmsh.initialize(sys.argv)

# ... Gmsh操作 ...

# 终止Gmsh
gmsh.finalize()
```

### 使用上下文管理器

```python
#!/usr/bin/env python3
"""
使用上下文管理器确保资源释放
"""
import gmsh

class GmshContext:
    """Gmsh上下文管理器"""

    def __init__(self, args=None):
        self.args = args or []

    def __enter__(self):
        gmsh.initialize(self.args)
        return gmsh

    def __exit__(self, exc_type, exc_val, exc_tb):
        gmsh.finalize()
        return False

# 使用示例
with GmshContext() as g:
    g.model.add("test")
    g.model.occ.addBox(0, 0, 0, 1, 1, 1)
    g.model.occ.synchronize()
    g.model.mesh.generate(3)
    g.write("test.msh")
```

### 初始化选项

```python
import sys
import gmsh

# 传入命令行参数
gmsh.initialize(sys.argv)

# 常用初始化参数
# -v N: 设置详细级别 (0=静默, 99=最详细)
# -nopopup: 禁止弹出窗口
# -format msh4: 设置输出格式

# 代码中设置详细级别
gmsh.option.setNumber("General.Verbosity", 5)
```

## 模型管理

### 创建和切换模型

```python
import gmsh

gmsh.initialize()

# 添加新模型
gmsh.model.add("model1")
gmsh.model.add("model2")

# 获取当前模型名称
current = gmsh.model.getCurrent()
print(f"当前模型: {current}")

# 列出所有模型
models = gmsh.model.list()
print(f"所有模型: {models}")

# 切换模型
gmsh.model.setCurrent("model1")

# 在model1中创建几何
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

# 切换到model2
gmsh.model.setCurrent("model2")
gmsh.model.occ.addSphere(0, 0, 0, 1)
gmsh.model.occ.synchronize()

# 移除模型
gmsh.model.remove()  # 移除当前模型

gmsh.finalize()
```

### 清除与重置

```python
# 清除当前模型的所有内容
gmsh.clear()

# 清除特定模型
gmsh.model.setCurrent("model1")
gmsh.clear()
```

## 几何操作

### geo内核（内置几何）

```python
#!/usr/bin/env python3
"""
使用内置几何内核创建模型
"""
import gmsh

gmsh.initialize()
gmsh.model.add("geo_example")

# 添加点
# addPoint(x, y, z, meshSize=0, tag=-1)
p1 = gmsh.model.geo.addPoint(0, 0, 0, 0.1)
p2 = gmsh.model.geo.addPoint(1, 0, 0, 0.1)
p3 = gmsh.model.geo.addPoint(1, 1, 0, 0.1)
p4 = gmsh.model.geo.addPoint(0, 1, 0, 0.1)

# 添加直线
# addLine(startTag, endTag, tag=-1)
l1 = gmsh.model.geo.addLine(p1, p2)
l2 = gmsh.model.geo.addLine(p2, p3)
l3 = gmsh.model.geo.addLine(p3, p4)
l4 = gmsh.model.geo.addLine(p4, p1)

# 添加曲线环
# addCurveLoop(curveTags, tag=-1, reorient=False)
cl = gmsh.model.geo.addCurveLoop([l1, l2, l3, l4])

# 添加平面
# addPlaneSurface(wireTags, tag=-1)
s = gmsh.model.geo.addPlaneSurface([cl])

# 同步几何到模型
gmsh.model.geo.synchronize()

# 添加物理组
gmsh.model.addPhysicalGroup(2, [s], name="Surface")

gmsh.model.mesh.generate(2)
gmsh.write("geo_example.msh")
gmsh.finalize()
```

### occ内核（OpenCASCADE）

```python
#!/usr/bin/env python3
"""
使用OpenCASCADE内核创建模型
"""
import gmsh

gmsh.initialize()
gmsh.model.add("occ_example")

# 基本形状
box = gmsh.model.occ.addBox(0, 0, 0, 2, 2, 2)
sphere = gmsh.model.occ.addSphere(1, 1, 1, 0.8)

# 布尔运算
# cut(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)
result, resultMap = gmsh.model.occ.cut(
    [(3, box)],      # 被切对象
    [(3, sphere)],   # 切割工具
    removeTool=True
)

# 同步
gmsh.model.occ.synchronize()

# 获取实体
entities = gmsh.model.getEntities(dim=3)
print(f"3D实体: {entities}")

gmsh.model.mesh.generate(3)
gmsh.write("occ_example.msh")
gmsh.finalize()
```

### 布尔运算完整示例

```python
#!/usr/bin/env python3
"""
布尔运算完整示例
"""
import gmsh

gmsh.initialize()
gmsh.model.add("boolean")

# 创建基础形状
box1 = gmsh.model.occ.addBox(0, 0, 0, 2, 2, 2)
box2 = gmsh.model.occ.addBox(1, 1, 1, 2, 2, 2)
sphere = gmsh.model.occ.addSphere(1, 1, 1, 0.5)
cylinder = gmsh.model.occ.addCylinder(0, 0, 0, 0, 0, 3, 0.3)

# 并集 (Union/Fuse)
# fuse(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)
fused, _ = gmsh.model.occ.fuse([(3, box1)], [(3, box2)])
print(f"并集结果: {fused}")

# 差集 (Difference/Cut)
result, _ = gmsh.model.occ.cut(fused, [(3, sphere)])

# 交集 (Intersection/Intersect)
# intersect(objectDimTags, toolDimTags, ...)
# result, _ = gmsh.model.occ.intersect(...)

# 分片 (Fragment) - 保留所有部分
# fragment(objectDimTags, toolDimTags, ...)
# result, _ = gmsh.model.occ.fragment(...)

gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)
gmsh.write("boolean.msh")
gmsh.finalize()
```

## 网格操作

### 网格生成

```python
import gmsh

gmsh.initialize()
gmsh.model.add("mesh")
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

# 设置网格选项
gmsh.option.setNumber("Mesh.MeshSizeMin", 0.05)
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.2)

# 生成网格
# generate(dim=3)
gmsh.model.mesh.generate(1)  # 1D网格
gmsh.model.mesh.generate(2)  # 2D网格
gmsh.model.mesh.generate(3)  # 3D网格

# 或者直接生成3D网格（会自动生成低维网格）
# gmsh.model.mesh.generate(3)

gmsh.finalize()
```

### 获取网格数据

```python
#!/usr/bin/env python3
"""
获取网格节点和元素
"""
import gmsh
import numpy as np

gmsh.initialize()
gmsh.model.add("mesh_data")
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 获取所有节点
# getNodes(dim=-1, tag=-1, includeBoundary=False, returnParametricCoord=True)
node_tags, node_coords, param_coords = gmsh.model.mesh.getNodes()

print(f"节点数量: {len(node_tags)}")
print(f"节点tags: {node_tags[:5]}...")

# 重组坐标为(N, 3)数组
coords = np.array(node_coords).reshape(-1, 3)
print(f"坐标形状: {coords.shape}")
print(f"第一个节点坐标: {coords[0]}")

# 获取所有元素
# getElements(dim=-1, tag=-1)
elem_types, elem_tags, elem_node_tags = gmsh.model.mesh.getElements()

for etype, etags, enodes in zip(elem_types, elem_tags, elem_node_tags):
    # 获取元素属性
    props = gmsh.model.mesh.getElementProperties(etype)
    name = props[0]      # 元素名称
    dim = props[1]       # 维度
    order = props[2]     # 阶数
    num_nodes = props[3] # 节点数

    print(f"类型: {name}, 维度: {dim}, 阶数: {order}, "
          f"节点数: {num_nodes}, 元素数: {len(etags)}")

gmsh.finalize()
```

### 设置网格尺寸

```python
#!/usr/bin/env python3
"""
网格尺寸控制方法
"""
import gmsh

gmsh.initialize()
gmsh.model.add("size_control")

# 方法1: 点尺寸（geo内核）
gmsh.model.geo.addPoint(0, 0, 0, 0.1)  # meshSize=0.1

# 方法2: 全局选项
gmsh.option.setNumber("Mesh.MeshSizeMin", 0.01)
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.5)

# 方法3: 使用尺寸场
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

# 距离场
distance_field = gmsh.model.mesh.field.add("Distance")
gmsh.model.mesh.field.setNumbers(distance_field, "PointsList", [1])

# 阈值场
threshold_field = gmsh.model.mesh.field.add("Threshold")
gmsh.model.mesh.field.setNumber(threshold_field, "InField", distance_field)
gmsh.model.mesh.field.setNumber(threshold_field, "SizeMin", 0.02)
gmsh.model.mesh.field.setNumber(threshold_field, "SizeMax", 0.2)
gmsh.model.mesh.field.setNumber(threshold_field, "DistMin", 0.1)
gmsh.model.mesh.field.setNumber(threshold_field, "DistMax", 0.5)

# 设置背景场
gmsh.model.mesh.field.setAsBackgroundMesh(threshold_field)

gmsh.model.mesh.generate(3)
gmsh.finalize()
```

### 结构化网格（Transfinite）

```python
#!/usr/bin/env python3
"""
Transfinite结构化网格
"""
import gmsh

gmsh.initialize()
gmsh.model.add("transfinite")

# 创建矩形
gmsh.model.occ.addRectangle(0, 0, 0, 2, 1)
gmsh.model.occ.synchronize()

# 获取曲线和曲面
curves = gmsh.model.getEntities(dim=1)
surfaces = gmsh.model.getEntities(dim=2)

# 设置曲线上的节点数
# setTransfiniteCurve(tag, numNodes, meshType="Progression", coef=1.0)
for dim, tag in curves:
    # 根据曲线方向设置节点数
    if tag in [1, 3]:  # 水平边
        gmsh.model.mesh.setTransfiniteCurve(tag, 21)  # 21个节点
    else:  # 垂直边
        gmsh.model.mesh.setTransfiniteCurve(tag, 11)

# 设置结构化曲面
# setTransfiniteSurface(tag, arrangement="Left", cornerTags=[])
for dim, tag in surfaces:
    gmsh.model.mesh.setTransfiniteSurface(tag)

# 设置重组为四边形
# recombine(dim, tag, angle=45)
for dim, tag in surfaces:
    gmsh.model.mesh.setRecombine(dim, tag)

gmsh.model.mesh.generate(2)
gmsh.write("transfinite.msh")
gmsh.finalize()
```

## 物理组

### 创建物理组

```python
#!/usr/bin/env python3
"""
物理组管理
"""
import gmsh

gmsh.initialize()
gmsh.model.add("physical_groups")

# 创建几何
box = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()

# 获取边界面
boundary = gmsh.model.getBoundary([(3, box)], oriented=False)
surface_tags = [tag for dim, tag in boundary]

# 添加物理组
# addPhysicalGroup(dim, tags, tag=-1, name="")

# 3D物理组（体积）
gmsh.model.addPhysicalGroup(3, [box], tag=1, name="Volume")

# 2D物理组（面）
# 假设面1是底面，面2是顶面，其他是侧面
gmsh.model.addPhysicalGroup(2, [1], tag=10, name="Bottom")
gmsh.model.addPhysicalGroup(2, [2], tag=20, name="Top")
gmsh.model.addPhysicalGroup(2, [3, 4, 5, 6], tag=30, name="Sides")

# 获取物理组信息
# getPhysicalGroups(dim=-1)
groups = gmsh.model.getPhysicalGroups()
print("物理组列表:")
for dim, tag in groups:
    name = gmsh.model.getPhysicalName(dim, tag)
    entities = gmsh.model.getEntitiesForPhysicalGroup(dim, tag)
    print(f"  ({dim}, {tag}) '{name}': 实体 {entities}")

gmsh.model.mesh.generate(3)
gmsh.write("physical_groups.msh")
gmsh.finalize()
```

### 按名称查找物理组

```python
# 通过名称获取物理组的实体
def get_physical_group_entities(name):
    """根据名称获取物理组的实体"""
    groups = gmsh.model.getPhysicalGroups()
    for dim, tag in groups:
        group_name = gmsh.model.getPhysicalName(dim, tag)
        if group_name == name:
            return gmsh.model.getEntitiesForPhysicalGroup(dim, tag)
    return []

# 使用
inlet_entities = get_physical_group_entities("Inlet")
```

## 文件操作

### 读写文件

```python
import gmsh

gmsh.initialize()

# 打开文件（会清除现有模型）
gmsh.open("model.step")

# 合并文件（添加到现有模型）
gmsh.merge("additional.msh")

# 写入文件
gmsh.write("output.msh")      # MSH格式
gmsh.write("output.vtk")      # VTK格式
gmsh.write("output.stl")      # STL格式
gmsh.write("output.step")     # STEP格式

# 设置文件格式版本
gmsh.option.setNumber("Mesh.MshFileVersion", 4.1)  # MSH 4.1

gmsh.finalize()
```

### 支持的文件格式

```python
# 几何格式
# 输入: .geo, .step, .stp, .iges, .igs, .brep
# 输出: .geo_unrolled, .step, .brep

# 网格格式
# 输入/输出: .msh, .vtk, .stl, .mesh, .med, .cgns
# 输出: .pos (后处理), .unv, .bdf, .p3d

# 图像格式
# 输出: .png, .jpg, .gif, .pdf, .svg, .eps, .ps
```

## 后处理视图

### 创建和操作视图

```python
#!/usr/bin/env python3
"""
后处理视图操作
"""
import gmsh

gmsh.initialize()
gmsh.model.add("view_example")
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 添加基于模型的视图数据
# 获取节点
node_tags, coords, _ = gmsh.model.mesh.getNodes()

# 创建节点数据（例如温度场）
import math
data = []
for i in range(0, len(coords), 3):
    x, y, z = coords[i:i+3]
    T = 100 * math.exp(-(x**2 + y**2 + z**2))
    data.append(T)

# 添加视图
view_tag = gmsh.view.add("Temperature")

# 添加模型数据
# addModelData(tag, step, modelName, dataType, tags, data, time=0, numComponents=1, partition=0)
gmsh.view.addModelData(
    view_tag,
    0,                    # 步骤
    "view_example",       # 模型名称
    "NodeData",           # 数据类型
    list(node_tags),      # 节点tags
    [[d] for d in data]   # 数据（每个节点一个列表）
)

# 写入视图
gmsh.view.write(view_tag, "temperature.pos")

# 获取视图列表
views = gmsh.view.getTags()
print(f"视图列表: {views}")

# 获取视图名称
for v in views:
    name = gmsh.view.option.getString(v, "Name")
    print(f"视图 {v}: {name}")

gmsh.finalize()
```

### 列表式视图数据

```python
#!/usr/bin/env python3
"""
列表式视图数据
"""
import gmsh
import math

gmsh.initialize()

# 创建标量点视图
view = gmsh.view.add("Scalar Points")

# SP: 标量点
# 格式: [x, y, z, value]
data = []
for i in range(10):
    for j in range(10):
        x, y, z = i * 0.1, j * 0.1, 0
        value = math.sin(x * math.pi) * math.sin(y * math.pi)
        data.extend([x, y, z, value])

gmsh.view.addListData(view, "SP", 100, data)

# ST: 标量三角形
# 格式: [x1,y1,z1, x2,y2,z2, x3,y3,z3, v1,v2,v3]
tri_data = [
    0, 0, 0,  1, 0, 0,  0.5, 1, 0,  # 坐标
    0, 1, 0.5                        # 值
]
gmsh.view.addListData(view, "ST", 1, tri_data)

gmsh.view.write(view, "list_data.pos")
gmsh.finalize()
```

## 选项系统

### 设置和获取选项

```python
import gmsh

gmsh.initialize()

# 设置数值选项
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.5)
gmsh.option.setNumber("General.Verbosity", 5)

# 获取数值选项
size_max = gmsh.option.getNumber("Mesh.MeshSizeMax")
print(f"MeshSizeMax: {size_max}")

# 设置字符串选项
gmsh.option.setString("General.GraphicsFont", "Helvetica")

# 获取字符串选项
font = gmsh.option.getString("General.GraphicsFont")
print(f"Font: {font}")

# 设置颜色选项
gmsh.option.setColor("Mesh.SurfaceFaces", 255, 0, 0, 255)  # RGBA

# 获取颜色选项
r, g, b, a = gmsh.option.getColor("Mesh.SurfaceFaces")
print(f"颜色: R={r}, G={g}, B={b}, A={a}")

gmsh.finalize()
```

### 常用选项类别

```python
# 通用选项
gmsh.option.setNumber("General.Verbosity", 5)        # 详细级别
gmsh.option.setNumber("General.NumThreads", 4)       # 线程数
gmsh.option.setNumber("General.Terminal", 1)         # 终端输出

# 几何选项
gmsh.option.setNumber("Geometry.Tolerance", 1e-8)    # 几何容差
gmsh.option.setNumber("Geometry.OCCTargetUnit", 1)   # OCC单位(mm)

# 网格选项
gmsh.option.setNumber("Mesh.Algorithm", 6)           # 2D算法
gmsh.option.setNumber("Mesh.Algorithm3D", 10)        # 3D算法
gmsh.option.setNumber("Mesh.ElementOrder", 2)        # 元素阶数
gmsh.option.setNumber("Mesh.Optimize", 1)            # 优化网格
gmsh.option.setNumber("Mesh.OptimizeNetgen", 1)      # Netgen优化

# 视图选项
gmsh.option.setNumber("View.Visible", 1)             # 显示视图
gmsh.option.setNumber("View.RangeType", 1)           # 范围类型
```

## 错误处理

### 异常处理

```python
import gmsh

try:
    gmsh.initialize()

    # 尝试操作
    gmsh.model.add("test")
    gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
    gmsh.model.occ.synchronize()

    # 尝试访问不存在的实体
    try:
        gmsh.model.getBoundary([(3, 999)])  # 不存在的实体
    except Exception as e:
        print(f"获取边界失败: {e}")

    gmsh.model.mesh.generate(3)
    gmsh.write("test.msh")

except Exception as e:
    print(f"Gmsh错误: {e}")

finally:
    # 确保终止
    try:
        gmsh.finalize()
    except:
        pass
```

### 日志和调试

```python
import gmsh

gmsh.initialize()

# 设置日志级别
gmsh.option.setNumber("General.Verbosity", 99)  # 最详细

# 记录日志到文件
gmsh.logger.start()

# 执行操作
gmsh.model.add("debug")
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 获取日志
log = gmsh.logger.get()
print("日志内容:")
for line in log:
    print(f"  {line}")

# 停止记录
gmsh.logger.stop()

# 写入警告/错误
gmsh.logger.write("这是一条自定义警告", "warning")
gmsh.logger.write("这是一条自定义错误", "error")

gmsh.finalize()
```

## 完整工作流示例

```python
#!/usr/bin/env python3
"""
完整的Gmsh Python API工作流
"""
import gmsh
import numpy as np
import sys

def create_geometry():
    """创建几何模型"""
    # 创建带孔的板
    plate = gmsh.model.occ.addRectangle(0, 0, 0, 10, 5)
    hole1 = gmsh.model.occ.addDisk(2.5, 2.5, 0, 0.8, 0.8)
    hole2 = gmsh.model.occ.addDisk(7.5, 2.5, 0, 0.8, 0.8)

    # 布尔差集
    result, _ = gmsh.model.occ.cut(
        [(2, plate)],
        [(2, hole1), (2, hole2)]
    )

    gmsh.model.occ.synchronize()
    return result

def setup_physical_groups():
    """设置物理组"""
    # 获取边界
    entities = gmsh.model.getEntities(dim=2)
    surface_tags = [tag for dim, tag in entities]

    # 创建体积物理组
    gmsh.model.addPhysicalGroup(2, surface_tags, name="Plate")

    # 获取边界曲线
    boundary = gmsh.model.getBoundary(entities, oriented=False)

    # 分类边界（简化处理）
    left_curves = []
    right_curves = []
    hole_curves = []

    for dim, tag in boundary:
        com = gmsh.model.occ.getCenterOfMass(dim, tag)
        x = com[0]
        if abs(x) < 0.1:
            left_curves.append(tag)
        elif abs(x - 10) < 0.1:
            right_curves.append(tag)
        else:
            hole_curves.append(tag)

    if left_curves:
        gmsh.model.addPhysicalGroup(1, left_curves, name="Left")
    if right_curves:
        gmsh.model.addPhysicalGroup(1, right_curves, name="Right")
    if hole_curves:
        gmsh.model.addPhysicalGroup(1, hole_curves, name="Holes")

def setup_mesh_size():
    """设置网格尺寸"""
    # 全局尺寸
    gmsh.option.setNumber("Mesh.MeshSizeMin", 0.1)
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.5)

    # 在孔边缘加密
    # 获取孔的曲线
    boundary = gmsh.model.getBoundary(gmsh.model.getEntities(2))
    hole_curves = []
    for dim, tag in boundary:
        com = gmsh.model.occ.getCenterOfMass(dim, tag)
        # 检查是否是孔的边界
        if 1 < com[0] < 4 or 6 < com[0] < 9:
            if 1 < com[1] < 4:
                hole_curves.append(tag)

    if hole_curves:
        # 距离场
        dist = gmsh.model.mesh.field.add("Distance")
        gmsh.model.mesh.field.setNumbers(dist, "CurvesList", hole_curves)

        # 阈值场
        thresh = gmsh.model.mesh.field.add("Threshold")
        gmsh.model.mesh.field.setNumber(thresh, "InField", dist)
        gmsh.model.mesh.field.setNumber(thresh, "SizeMin", 0.05)
        gmsh.model.mesh.field.setNumber(thresh, "SizeMax", 0.5)
        gmsh.model.mesh.field.setNumber(thresh, "DistMin", 0.1)
        gmsh.model.mesh.field.setNumber(thresh, "DistMax", 1.0)

        gmsh.model.mesh.field.setAsBackgroundMesh(thresh)

def generate_mesh():
    """生成网格"""
    gmsh.option.setNumber("Mesh.Algorithm", 6)  # Frontal-Delaunay
    gmsh.model.mesh.generate(2)

def analyze_mesh():
    """分析网格"""
    # 获取节点
    node_tags, coords, _ = gmsh.model.mesh.getNodes()
    coords = np.array(coords).reshape(-1, 3)

    print(f"\n网格统计:")
    print(f"  节点数: {len(node_tags)}")

    # 获取元素
    elem_types, elem_tags, elem_nodes = gmsh.model.mesh.getElements()

    total_elems = 0
    for etype, etags, enodes in zip(elem_types, elem_tags, elem_nodes):
        props = gmsh.model.mesh.getElementProperties(etype)
        print(f"  {props[0]}: {len(etags)}个")
        total_elems += len(etags)

    print(f"  总元素数: {total_elems}")

    # 网格质量
    qualities = gmsh.model.mesh.getElementQualities([], "minSJ")
    if qualities:
        q = np.array(qualities)
        print(f"\n网格质量 (minSJ):")
        print(f"  最小: {q.min():.4f}")
        print(f"  最大: {q.max():.4f}")
        print(f"  平均: {q.mean():.4f}")

def main():
    gmsh.initialize(sys.argv)
    gmsh.model.add("workflow_example")

    print("1. 创建几何...")
    create_geometry()

    print("2. 设置物理组...")
    setup_physical_groups()

    print("3. 设置网格尺寸...")
    setup_mesh_size()

    print("4. 生成网格...")
    generate_mesh()

    print("5. 分析网格...")
    analyze_mesh()

    print("\n6. 保存结果...")
    gmsh.write("workflow_result.msh")
    gmsh.write("workflow_result.vtk")

    # 可选：显示GUI
    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()
    print("\n完成!")

if __name__ == "__main__":
    main()
```

## API函数速查表

### 初始化与终止

| 函数 | 描述 |
|------|------|
| `initialize(argv=[])` | 初始化Gmsh |
| `finalize()` | 终止Gmsh |
| `clear()` | 清除当前模型 |
| `open(fileName)` | 打开文件 |
| `merge(fileName)` | 合并文件 |
| `write(fileName)` | 写入文件 |

### 模型操作

| 函数 | 描述 |
|------|------|
| `model.add(name)` | 添加模型 |
| `model.remove()` | 移除当前模型 |
| `model.list()` | 列出所有模型 |
| `model.setCurrent(name)` | 设置当前模型 |
| `model.getCurrent()` | 获取当前模型名 |
| `model.getEntities(dim=-1)` | 获取实体 |
| `model.getBoundary(dimTags)` | 获取边界 |

### 几何操作

| 函数 | 描述 |
|------|------|
| `model.geo.addPoint()` | 添加点 |
| `model.geo.addLine()` | 添加直线 |
| `model.geo.addCurveLoop()` | 添加曲线环 |
| `model.geo.addPlaneSurface()` | 添加平面 |
| `model.geo.synchronize()` | 同步几何 |
| `model.occ.addBox()` | 添加立方体 |
| `model.occ.addSphere()` | 添加球体 |
| `model.occ.fuse()` | 布尔并集 |
| `model.occ.cut()` | 布尔差集 |
| `model.occ.synchronize()` | 同步几何 |

### 网格操作

| 函数 | 描述 |
|------|------|
| `model.mesh.generate(dim)` | 生成网格 |
| `model.mesh.getNodes()` | 获取节点 |
| `model.mesh.getElements()` | 获取元素 |
| `model.mesh.setTransfiniteCurve()` | 设置结构化曲线 |
| `model.mesh.setTransfiniteSurface()` | 设置结构化面 |
| `model.mesh.setRecombine()` | 设置重组 |

## 下一步

- [02-Cpp-API完整指南](./02-Cpp-API完整指南.md) - 学习C++ API使用方法
