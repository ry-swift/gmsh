# View基础

Gmsh的后处理模块使用"View"（视图）来存储和显示标量、向量和张量数据。

## View概念

```
模型数据              →         View（视图）        →        可视化
节点位移                        ┌─────────────┐            颜色映射
元素应力         →             │  数据存储   │     →      等值线
温度场                          │  显示控制   │            矢量图
速度场                          └─────────────┘            动画
```

## 创建View

### 基本创建

```python
import gmsh

gmsh.initialize()

# 创建View
# add(name) -> view_tag
view_tag = gmsh.view.add("Temperature")

# 获取所有View
tags = gmsh.view.getTags()
print(f"View数量: {len(tags)}")

# 获取View名称
name = gmsh.view.getIndex(view_tag)  # 返回索引
# 注：gmsh.view模块主要用于数据操作，名称通过option获取

gmsh.finalize()
```

### View属性

```python
# 设置View选项
# option.setNumber("View[index].Option", value)
# index是View的索引（从0开始）

gmsh.option.setNumber("View[0].Visible", 1)        # 可见性
gmsh.option.setNumber("View[0].ShowScale", 1)      # 显示色标
gmsh.option.setNumber("View[0].ColorTable", 2)     # 颜色表
gmsh.option.setNumber("View[0].RangeType", 1)      # 范围类型
```

## 添加数据

### 列表式数据（List-based）

```python
#!/usr/bin/env python3
"""
使用列表式数据创建View
"""
import gmsh

gmsh.initialize()
gmsh.model.add("view_list")

# 创建简单几何和网格
gmsh.model.occ.addRectangle(0, 0, 0, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(2)

# 创建View
view = gmsh.view.add("ScalarField")

# 添加点数据 (SP = Scalar Point)
# addListData(tag, dataType, numEle, data)
# data格式: [x1, y1, z1, v1, x2, y2, z2, v2, ...]
data = []
for i in range(11):
    for j in range(11):
        x, y, z = i/10, j/10, 0
        value = x * y  # 标量值
        data.extend([x, y, z, value])

gmsh.view.addListData(view, "SP", len(data)//4, data)

# 写入文件
gmsh.view.write(view, "scalar_field.pos")

gmsh.finalize()
```

### 模型式数据（Model-based）

```python
#!/usr/bin/env python3
"""
使用模型式数据创建View（基于网格）
"""
import gmsh
import numpy as np

gmsh.initialize()
gmsh.model.add("view_model")

# 创建几何和网格
rect = gmsh.model.occ.addRectangle(0, 0, 0, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(2)

# 获取节点
node_tags, node_coords, _ = gmsh.model.mesh.getNodes()
node_coords = np.array(node_coords).reshape(-1, 3)

# 创建View
view = gmsh.view.add("NodalValues")

# 计算节点值
values = []
for coords in node_coords:
    x, y, z = coords
    value = np.sin(np.pi * x) * np.sin(np.pi * y)
    values.append(value)

# 添加模型数据
# addModelData(tag, step, modelName, dataType, tags, data, time=0, numComponents=1, partition=0)
gmsh.view.addModelData(
    view,
    0,                      # 时间步
    "",                     # 模型名（空=当前模型）
    "NodeData",             # 数据类型
    list(node_tags),        # 节点tag列表
    [[v] for v in values]   # 每个节点的值（列表的列表）
)

gmsh.view.write(view, "nodal_values.msh")
gmsh.finalize()
```

## 数据类型

### 标量数据

```python
# 点标量 (SP)
# 格式: [x, y, z, value]
gmsh.view.addListData(view, "SP", n, data)

# 线标量 (SL)
# 格式: [x1, y1, z1, x2, y2, z2, v1, v2]
gmsh.view.addListData(view, "SL", n, data)

# 三角形标量 (ST)
# 格式: [x1, y1, z1, x2, y2, z2, x3, y3, z3, v1, v2, v3]
gmsh.view.addListData(view, "ST", n, data)

# 四面体标量 (SS)
# 格式: [x1,y1,z1, x2,y2,z2, x3,y3,z3, x4,y4,z4, v1,v2,v3,v4]
gmsh.view.addListData(view, "SS", n, data)
```

### 向量数据

```python
# 点向量 (VP)
# 格式: [x, y, z, vx, vy, vz]
gmsh.view.addListData(view, "VP", n, data)

# 三角形向量 (VT)
# 格式: [x1,y1,z1, x2,y2,z2, x3,y3,z3,
#        vx1,vy1,vz1, vx2,vy2,vz2, vx3,vy3,vz3]
gmsh.view.addListData(view, "VT", n, data)
```

### 张量数据

```python
# 点张量 (TP)
# 格式: [x, y, z, t11,t12,t13,t21,t22,t23,t31,t32,t33]
gmsh.view.addListData(view, "TP", n, data)
```

## View选项

### 显示控制

```python
# 可见性
gmsh.option.setNumber("View[0].Visible", 1)

# 显示类型
# 1: 颜色映射
# 2: 等值线
# 3: 等值面
# 4: 数值
gmsh.option.setNumber("View[0].IntervalsType", 1)

# 等值线/面数量
gmsh.option.setNumber("View[0].NbIso", 20)

# 光照
gmsh.option.setNumber("View[0].Light", 1)

# 边缘
gmsh.option.setNumber("View[0].ShowElement", 1)
```

### 颜色设置

```python
# 颜色表
# 0: 默认
# 1: Vis5D
# 2: Jet
# 3: Hot
# 4: Gray
# 5: HSV
gmsh.option.setNumber("View[0].ColorTable", 2)

# 自定义颜色范围
gmsh.option.setNumber("View[0].CustomMin", 0)
gmsh.option.setNumber("View[0].CustomMax", 1)
gmsh.option.setNumber("View[0].RangeType", 2)  # 2=自定义范围
```

### 向量显示

```python
# 向量类型
# 1: 线段
# 2: 箭头
# 3: 金字塔
# 4: 圆锥
gmsh.option.setNumber("View[0].VectorType", 2)

# 向量缩放
gmsh.option.setNumber("View[0].ArrowSizeMax", 30)
gmsh.option.setNumber("View[0].ArrowSizeMin", 0)

# 颜色模式
# 0: 按模值
# 1: 按方向
gmsh.option.setNumber("View[0].GlyphLocation", 1)
```

## 完整示例：有限元结果可视化

```python
#!/usr/bin/env python3
"""
可视化有限元分析结果
"""
import gmsh
import numpy as np

gmsh.initialize()
gmsh.model.add("fem_results")

# 创建简单梁
length, height = 10, 1
beam = gmsh.model.occ.addRectangle(0, 0, 0, length, height)
gmsh.model.occ.synchronize()

# 网格
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.3)
gmsh.model.mesh.generate(2)

# 获取节点信息
node_tags, node_coords, _ = gmsh.model.mesh.getNodes()
node_coords = np.array(node_coords).reshape(-1, 3)
num_nodes = len(node_tags)

# 模拟有限元结果

# 1. 位移场（向量）
view_disp = gmsh.view.add("Displacement")
disp_data = []
for i, coords in enumerate(node_coords):
    x, y, z = coords
    # 简化的悬臂梁位移（模拟）
    ux = 0.001 * x
    uy = -0.01 * x**2 / length**2
    uz = 0
    disp_data.append([ux, uy, uz])

gmsh.view.addModelData(
    view_disp, 0, "", "NodeData",
    list(node_tags), disp_data
)

# 2. 应力场（标量 - von Mises）
view_stress = gmsh.view.add("VonMises")
stress_data = []
for i, coords in enumerate(node_coords):
    x, y, z = coords
    # 简化的应力计算
    sigma = 100 * (1 - x/length) * (0.5 - abs(y - 0.5))
    stress_data.append([sigma])

gmsh.view.addModelData(
    view_stress, 0, "", "NodeData",
    list(node_tags), stress_data
)

# 3. 温度场
view_temp = gmsh.view.add("Temperature")
temp_data = []
for i, coords in enumerate(node_coords):
    x, y, z = coords
    temp = 20 + 80 * np.exp(-((x-5)**2 + (y-0.5)**2) / 4)
    temp_data.append([temp])

gmsh.view.addModelData(
    view_temp, 0, "", "NodeData",
    list(node_tags), temp_data
)

# 设置View选项
# 位移场显示为矢量
gmsh.option.setNumber("View[0].VectorType", 4)  # 圆锥
gmsh.option.setNumber("View[0].ArrowSizeMax", 50)

# 应力场使用热图
gmsh.option.setNumber("View[1].ColorTable", 2)  # Jet
gmsh.option.setNumber("View[1].IntervalsType", 1)  # 颜色映射

# 温度场使用等值线
gmsh.option.setNumber("View[2].IntervalsType", 2)  # 等值线
gmsh.option.setNumber("View[2].NbIso", 15)

# 保存
gmsh.view.write(view_disp, "displacement.pos")
gmsh.view.write(view_stress, "stress.pos")
gmsh.view.write(view_temp, "temperature.pos")

# 合并保存
gmsh.write("fem_results.msh")

gmsh.finalize()
```

## 读取View

### 从文件读取

```python
# 读取POS文件
gmsh.merge("results.pos")

# 读取后，View自动添加到列表
tags = gmsh.view.getTags()
print(f"读取了 {len(tags)} 个View")

# 读取MSH文件（可能包含View）
gmsh.merge("results.msh")
```

### 获取View数据

```python
# 获取模型数据
# getModelData(tag, step) -> (dataType, tags, data, time, numComponents)
dataType, tags, data, time, numComp = gmsh.view.getModelData(view_tag, 0)

print(f"数据类型: {dataType}")
print(f"节点/元素数: {len(tags)}")
print(f"时间: {time}")
print(f"分量数: {numComp}")

# 获取列表数据
# getListData(tag) -> (dataType, numElements, data)
# 返回所有数据类型的数据
```

## View操作

### 复制和删除

```python
# 复制View
new_tag = gmsh.view.copy(view_tag)

# 删除View
gmsh.view.remove(view_tag)
```

### 组合View

```python
# 使用插件组合View
gmsh.plugin.setNumber("MathEval", "View", 0)
gmsh.plugin.setString("MathEval", "Expression0", "v0 + v1")
gmsh.plugin.run("MathEval")
```

## 时间序列数据

```python
#!/usr/bin/env python3
"""
创建时间序列View
"""
import gmsh
import numpy as np

gmsh.initialize()
gmsh.model.add("time_series")

# 创建网格
disk = gmsh.model.occ.addDisk(0, 0, 0, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(2)

node_tags, node_coords, _ = gmsh.model.mesh.getNodes()
node_coords = np.array(node_coords).reshape(-1, 3)

# 创建时间序列View
view = gmsh.view.add("WavePropagation")

# 多个时间步
num_steps = 20
for step in range(num_steps):
    time = step * 0.1
    values = []

    for coords in node_coords:
        x, y, z = coords
        r = np.sqrt(x**2 + y**2)
        # 波动方程解（简化）
        value = np.sin(2 * np.pi * r - 2 * np.pi * time) * np.exp(-r**2)
        values.append([value])

    gmsh.view.addModelData(
        view, step, "", "NodeData",
        list(node_tags), values,
        time=time
    )

# 设置动画选项
gmsh.option.setNumber("View[0].ShowTime", 1)

gmsh.view.write(view, "wave.pos")
gmsh.finalize()
```

## View选项总结

| 选项 | 描述 | 常用值 |
|-----|------|-------|
| Visible | 可见性 | 0/1 |
| IntervalsType | 显示类型 | 1=颜色,2=等值线,3=等值面 |
| NbIso | 等值线数 | 10-50 |
| ColorTable | 颜色表 | 0-5 |
| RangeType | 范围类型 | 1=自动,2=自定义,3=每步 |
| VectorType | 向量类型 | 1=线,2=箭头,4=圆锥 |
| ShowElement | 显示单元 | 0/1 |
| Light | 光照 | 0/1 |
| ShowScale | 显示色标 | 0/1 |

## 下一步

- [02-列表式数据导入](./02-列表式数据导入.md) - 深入学习列表式数据格式
