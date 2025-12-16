# CSG建模实例

本案例演示使用构造实体几何（CSG）方法创建复杂模型。

## 案例目标

- 掌握布尔运算的组合使用
- 创建复杂工程几何
- 处理多材料域
- 优化布尔运算性能

## 基础布尔运算

```python
#!/usr/bin/env python3
"""
案例4: 基础布尔运算示例
"""
import gmsh
import sys

def basic_boolean_operations():
    """基础布尔运算"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("basic_boolean")

    # 创建基本形状
    box = gmsh.model.occ.addBox(0, 0, 0, 2, 2, 2)
    sphere = gmsh.model.occ.addSphere(1, 1, 1, 1.2)
    cylinder = gmsh.model.occ.addCylinder(1, 1, -1, 0, 0, 4, 0.5)

    # 并集示例
    # fuse(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)
    # fused, _ = gmsh.model.occ.fuse([(3, box)], [(3, sphere)])

    # 差集示例
    # cut(objectDimTags, toolDimTags, tag=-1, removeObject=True, removeTool=True)
    result1, _ = gmsh.model.occ.cut([(3, box)], [(3, sphere)])

    # 从结果中减去圆柱
    result2, _ = gmsh.model.occ.cut(result1, [(3, cylinder)])

    gmsh.model.occ.synchronize()

    # 获取最终体积
    volumes = gmsh.model.getEntities(dim=3)
    print(f"最终体积数: {len(volumes)}")

    gmsh.model.addPhysicalGroup(3, [v[1] for v in volumes], name="Solid")

    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.15)
    gmsh.model.mesh.generate(3)

    gmsh.write("basic_boolean.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    basic_boolean_operations()
```

## 机械零件建模

```python
#!/usr/bin/env python3
"""
案例4: 机械零件 - 法兰盘
"""
import gmsh
import sys
import math

def create_flange():
    """创建法兰盘"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("flange")

    # 几何参数
    outer_radius = 2.0      # 外半径
    inner_radius = 0.5      # 内孔半径
    thickness = 0.3         # 厚度
    hub_radius = 0.8        # 轴毂半径
    hub_height = 0.5        # 轴毂高度
    bolt_radius = 0.15      # 螺栓孔半径
    bolt_circle = 1.5       # 螺栓圆周半径
    num_bolts = 6           # 螺栓数量

    # 创建主盘
    disk = gmsh.model.occ.addCylinder(0, 0, 0, 0, 0, thickness, outer_radius)

    # 创建轴毂
    hub = gmsh.model.occ.addCylinder(0, 0, thickness, 0, 0, hub_height, hub_radius)

    # 合并盘和轴毂
    result, _ = gmsh.model.occ.fuse([(3, disk)], [(3, hub)])

    # 创建中心孔
    center_hole = gmsh.model.occ.addCylinder(0, 0, -0.1, 0, 0,
                                              thickness + hub_height + 0.2, inner_radius)

    # 减去中心孔
    result, _ = gmsh.model.occ.cut(result, [(3, center_hole)])

    # 创建螺栓孔
    bolt_holes = []
    for i in range(num_bolts):
        angle = 2 * math.pi * i / num_bolts
        x = bolt_circle * math.cos(angle)
        y = bolt_circle * math.sin(angle)
        hole = gmsh.model.occ.addCylinder(x, y, -0.1, 0, 0,
                                           thickness + 0.2, bolt_radius)
        bolt_holes.append((3, hole))

    # 减去螺栓孔
    if bolt_holes:
        result, _ = gmsh.model.occ.cut(result, bolt_holes)

    gmsh.model.occ.synchronize()

    # 获取所有边界面
    volumes = gmsh.model.getEntities(dim=3)
    boundary = gmsh.model.getBoundary(volumes, oriented=False)

    # 分类表面
    top_faces = []
    bottom_faces = []
    outer_faces = []
    hole_faces = []

    for dim, tag in boundary:
        com = gmsh.model.occ.getCenterOfMass(dim, tag)
        z = com[2]

        if z > thickness + hub_height - 0.01:
            top_faces.append(tag)
        elif z < 0.01:
            bottom_faces.append(tag)
        else:
            # 判断是外表面还是孔表面
            x, y = com[0], com[1]
            r = math.sqrt(x*x + y*y)
            if r > outer_radius - 0.1:
                outer_faces.append(tag)
            else:
                hole_faces.append(tag)

    # 物理组
    if top_faces:
        gmsh.model.addPhysicalGroup(2, top_faces, name="Top")
    if bottom_faces:
        gmsh.model.addPhysicalGroup(2, bottom_faces, name="Bottom")
    if outer_faces:
        gmsh.model.addPhysicalGroup(2, outer_faces, name="Outer")
    if hole_faces:
        gmsh.model.addPhysicalGroup(2, hole_faces, name="Holes")

    gmsh.model.addPhysicalGroup(3, [v[1] for v in volumes], name="Flange")

    # 网格设置
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)
    gmsh.option.setNumber("Mesh.Algorithm3D", 10)

    gmsh.model.mesh.generate(3)

    gmsh.write("flange.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    create_flange()
```

## 多材料域

```python
#!/usr/bin/env python3
"""
案例4: 多材料域 - 夹层结构
"""
import gmsh
import sys

def create_multilayer():
    """创建多层结构"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("multilayer")

    # 几何参数
    Lx, Ly = 4.0, 2.0
    layer_heights = [0.3, 0.5, 0.3]  # 三层厚度
    layer_names = ["Bottom", "Core", "Top"]

    # 创建各层
    layers = []
    z = 0
    for i, h in enumerate(layer_heights):
        layer = gmsh.model.occ.addBox(0, 0, z, Lx, Ly, h)
        layers.append(layer)
        z += h

    # 使用fragment确保共享界面
    # fragment会将重叠的实体分割，保留所有部分
    all_layers = [(3, l) for l in layers]

    result, result_map = gmsh.model.occ.fragment(all_layers, [])

    gmsh.model.occ.synchronize()

    # 根据位置识别各层
    volumes = gmsh.model.getEntities(dim=3)

    layer_volumes = {name: [] for name in layer_names}
    z_boundaries = [0]
    for h in layer_heights:
        z_boundaries.append(z_boundaries[-1] + h)

    for dim, tag in volumes:
        com = gmsh.model.occ.getCenterOfMass(dim, tag)
        z = com[2]

        for i, name in enumerate(layer_names):
            z_min = z_boundaries[i]
            z_max = z_boundaries[i + 1]
            if z_min < z < z_max:
                layer_volumes[name].append(tag)
                break

    # 定义物理组
    for name, tags in layer_volumes.items():
        if tags:
            gmsh.model.addPhysicalGroup(3, tags, name=name)

    # 识别界面
    interfaces = []
    for dim, tag in gmsh.model.getEntities(dim=2):
        # 获取连接到此面的体积
        up_down = gmsh.model.getAdjacencies(dim, tag)

        # 如果面连接两个体积，可能是界面
        # 这里简化处理
        com = gmsh.model.occ.getCenterOfMass(dim, tag)
        z = com[2]

        for zb in z_boundaries[1:-1]:  # 内部界面
            if abs(z - zb) < 0.01:
                interfaces.append(tag)
                break

    if interfaces:
        gmsh.model.addPhysicalGroup(2, interfaces, name="Interfaces")

    # 网格设置
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.15)

    gmsh.model.mesh.generate(3)

    gmsh.write("multilayer.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    create_multilayer()
```

## 复杂装配体

```python
#!/usr/bin/env python3
"""
案例4: 复杂装配体 - 轴承座
"""
import gmsh
import sys
import math

def create_bearing_housing():
    """创建轴承座"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("bearing_housing")

    # 基座参数
    base_l, base_w, base_h = 4.0, 2.5, 0.5

    # 轴承座参数
    housing_radius = 1.0
    housing_height = 2.0
    bore_radius = 0.6

    # 螺栓孔参数
    bolt_radius = 0.15
    bolt_positions = [
        (-1.5, -0.8),
        (-1.5, 0.8),
        (1.5, -0.8),
        (1.5, 0.8)
    ]

    # 创建基座
    base = gmsh.model.occ.addBox(-base_l/2, -base_w/2, 0,
                                  base_l, base_w, base_h)

    # 创建轴承座圆柱
    housing = gmsh.model.occ.addCylinder(0, 0, base_h,
                                          0, 0, housing_height,
                                          housing_radius)

    # 合并基座和轴承座
    result, _ = gmsh.model.occ.fuse([(3, base)], [(3, housing)])

    # 创建轴承孔
    bore = gmsh.model.occ.addCylinder(0, 0, base_h - 0.1,
                                       0, 0, housing_height + 0.2,
                                       bore_radius)

    result, _ = gmsh.model.occ.cut(result, [(3, bore)])

    # 创建螺栓孔
    bolt_holes = []
    for x, y in bolt_positions:
        hole = gmsh.model.occ.addCylinder(x, y, -0.1,
                                           0, 0, base_h + 0.2,
                                           bolt_radius)
        bolt_holes.append((3, hole))

    result, _ = gmsh.model.occ.cut(result, bolt_holes)

    # 添加倒角（可选）
    # 获取需要倒角的边
    gmsh.model.occ.synchronize()

    volumes = gmsh.model.getEntities(dim=3)

    # 物理组
    gmsh.model.addPhysicalGroup(3, [v[1] for v in volumes], name="Housing")

    # 获取边界面并分类
    boundary = gmsh.model.getBoundary(volumes, oriented=False)

    bottom_faces = []
    bore_faces = []
    bolt_faces = []
    outer_faces = []

    for dim, tag in boundary:
        com = gmsh.model.occ.getCenterOfMass(dim, tag)
        x, y, z = com

        # 底面
        if z < 0.01:
            bottom_faces.append(tag)
        # 轴承孔内表面
        elif abs(math.sqrt(x*x + y*y) - bore_radius) < 0.1 and z > base_h:
            bore_faces.append(tag)
        # 螺栓孔
        elif z < base_h + 0.01:
            is_bolt = False
            for bx, by in bolt_positions:
                if math.sqrt((x-bx)**2 + (y-by)**2) < bolt_radius + 0.1:
                    is_bolt = True
                    break
            if is_bolt:
                bolt_faces.append(tag)
            else:
                outer_faces.append(tag)
        else:
            outer_faces.append(tag)

    if bottom_faces:
        gmsh.model.addPhysicalGroup(2, bottom_faces, name="Bottom")
    if bore_faces:
        gmsh.model.addPhysicalGroup(2, bore_faces, name="Bore")
    if bolt_faces:
        gmsh.model.addPhysicalGroup(2, bolt_faces, name="BoltHoles")
    if outer_faces:
        gmsh.model.addPhysicalGroup(2, outer_faces, name="Outer")

    # 网格加密
    dist = gmsh.model.mesh.field.add("Distance")
    gmsh.model.mesh.field.setNumbers(dist, "SurfacesList", bore_faces)

    thresh = gmsh.model.mesh.field.add("Threshold")
    gmsh.model.mesh.field.setNumber(thresh, "InField", dist)
    gmsh.model.mesh.field.setNumber(thresh, "SizeMin", 0.03)
    gmsh.model.mesh.field.setNumber(thresh, "SizeMax", 0.15)
    gmsh.model.mesh.field.setNumber(thresh, "DistMin", 0.05)
    gmsh.model.mesh.field.setNumber(thresh, "DistMax", 0.5)

    gmsh.model.mesh.field.setAsBackgroundMesh(thresh)

    gmsh.option.setNumber("Mesh.MeshSizeExtendFromBoundary", 0)
    gmsh.option.setNumber("Mesh.MeshSizeFromPoints", 0)

    gmsh.model.mesh.generate(3)

    gmsh.write("bearing_housing.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    create_bearing_housing()
```

## 参数化建模

```python
#!/usr/bin/env python3
"""
案例4: 参数化建模
"""
import gmsh
import sys
import math

class ParametricModel:
    """参数化模型类"""

    def __init__(self, name="parametric"):
        gmsh.initialize(sys.argv)
        gmsh.model.add(name)
        self.volumes = []
        self.surfaces = {}

    def create_gear_like_shape(self, num_teeth=12, outer_radius=2.0,
                                inner_radius=0.5, tooth_depth=0.3,
                                thickness=0.5):
        """创建齿轮状形状"""

        # 创建基圆柱
        base = gmsh.model.occ.addCylinder(0, 0, 0, 0, 0, thickness, outer_radius)

        # 创建齿槽
        tooth_angle = 2 * math.pi / num_teeth
        slot_width = tooth_angle * 0.3  # 槽宽为周期的30%

        slots = []
        for i in range(num_teeth):
            angle = i * tooth_angle

            # 创建槽的近似（使用小圆柱）
            x = outer_radius * math.cos(angle)
            y = outer_radius * math.sin(angle)

            slot = gmsh.model.occ.addCylinder(
                x, y, -0.1,
                0, 0, thickness + 0.2,
                tooth_depth
            )
            slots.append((3, slot))

        # 减去齿槽
        result, _ = gmsh.model.occ.cut([(3, base)], slots)

        # 创建中心孔
        center_hole = gmsh.model.occ.addCylinder(
            0, 0, -0.1,
            0, 0, thickness + 0.2,
            inner_radius
        )

        result, _ = gmsh.model.occ.cut(result, [(3, center_hole)])

        gmsh.model.occ.synchronize()

        self.volumes = gmsh.model.getEntities(dim=3)
        return self

    def add_physical_groups(self):
        """添加物理组"""
        if self.volumes:
            gmsh.model.addPhysicalGroup(3, [v[1] for v in self.volumes], name="Solid")

        return self

    def generate_mesh(self, size=0.1):
        """生成网格"""
        gmsh.option.setNumber("Mesh.MeshSizeMax", size)
        gmsh.model.mesh.generate(3)
        return self

    def save(self, filename):
        """保存网格"""
        gmsh.write(filename)
        return self

    def show(self):
        """显示GUI"""
        if "-nopopup" not in sys.argv:
            gmsh.fltk.run()

    def finalize(self):
        """清理"""
        gmsh.finalize()

def main():
    model = ParametricModel("gear")
    model.create_gear_like_shape(
        num_teeth=16,
        outer_radius=2.0,
        inner_radius=0.4,
        tooth_depth=0.25,
        thickness=0.5
    )
    model.add_physical_groups()
    model.generate_mesh(size=0.08)
    model.save("gear.msh")
    model.show()
    model.finalize()

if __name__ == "__main__":
    main()
```

## 布尔运算优化

```python
#!/usr/bin/env python3
"""
案例4: 布尔运算优化技巧
"""
import gmsh
import sys
import time

def optimize_boolean():
    """优化布尔运算性能"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("boolean_optimized")

    # 技巧1: 批量操作
    # 不好的做法：逐个进行布尔运算
    # 好的做法：收集所有工具体，一次性操作

    # 创建主体
    main_box = gmsh.model.occ.addBox(0, 0, 0, 10, 10, 10)

    # 创建多个孔
    holes = []
    for i in range(3):
        for j in range(3):
            for k in range(3):
                sphere = gmsh.model.occ.addSphere(
                    i * 3 + 1.5,
                    j * 3 + 1.5,
                    k * 3 + 1.5,
                    0.8
                )
                holes.append((3, sphere))

    print(f"创建 {len(holes)} 个孔")

    # 一次性差集
    start = time.time()
    result, _ = gmsh.model.occ.cut([(3, main_box)], holes)
    elapsed = time.time() - start
    print(f"布尔运算耗时: {elapsed:.2f}s")

    gmsh.model.occ.synchronize()

    # 技巧2: 使用fragment而不是多次cut/fuse
    # fragment保留所有部分，适合多材料建模

    volumes = gmsh.model.getEntities(dim=3)
    gmsh.model.addPhysicalGroup(3, [v[1] for v in volumes], name="Result")

    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.3)
    gmsh.option.setNumber("Mesh.Algorithm3D", 10)

    gmsh.model.mesh.generate(3)

    gmsh.write("boolean_optimized.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    optimize_boolean()
```

## 总结

本案例涵盖了：

1. **基础布尔运算**: fuse, cut, intersect, fragment
2. **机械零件**: 法兰盘等复杂零件
3. **多材料域**: 使用fragment创建共享界面
4. **复杂装配体**: 组合多个基本形状
5. **参数化建模**: 可配置的几何模型
6. **性能优化**: 批量操作减少计算时间

## 下一步

- [05-NACA翼型边界层](./05-NACA翼型边界层.md) - CFD应用案例
