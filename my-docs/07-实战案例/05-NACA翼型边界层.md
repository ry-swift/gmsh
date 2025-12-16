# NACA翼型边界层

本案例演示空气动力学应用中的翼型边界层网格生成。

## 案例目标

- 生成NACA翼型几何
- 创建边界层网格
- 设置入口、出口、壁面边界
- 为CFD分析准备网格

## NACA翼型几何生成

```python
#!/usr/bin/env python3
"""
案例5: NACA 4位数翼型几何生成
"""
import gmsh
import sys
import math

def naca4_airfoil(code, num_points=100, chord=1.0):
    """
    生成NACA 4位数翼型坐标

    Parameters:
        code: NACA代码，如 "0012", "2412"
        num_points: 每侧点数
        chord: 弦长

    Returns:
        upper: 上表面点 [(x, y), ...]
        lower: 下表面点 [(x, y), ...]
    """
    # 解析NACA代码
    m = int(code[0]) / 100.0      # 最大弯度
    p = int(code[1]) / 10.0       # 最大弯度位置
    t = int(code[2:4]) / 100.0    # 最大厚度

    def thickness(x):
        """厚度分布"""
        return 5 * t * (0.2969 * math.sqrt(x) - 0.1260 * x
                        - 0.3516 * x**2 + 0.2843 * x**3 - 0.1015 * x**4)

    def camber(x):
        """弯度线"""
        if m == 0:
            return 0, 0
        if x < p:
            yc = m / p**2 * (2 * p * x - x**2)
            dyc = 2 * m / p**2 * (p - x)
        else:
            yc = m / (1-p)**2 * ((1 - 2*p) + 2*p*x - x**2)
            dyc = 2 * m / (1-p)**2 * (p - x)
        return yc, dyc

    # 使用余弦分布获得更好的前缘分辨率
    upper = []
    lower = []

    for i in range(num_points):
        beta = math.pi * i / (num_points - 1)
        x = 0.5 * (1 - math.cos(beta))  # 余弦分布

        yt = thickness(x)
        yc, dyc = camber(x)
        theta = math.atan(dyc) if m > 0 else 0

        # 上表面
        xu = x - yt * math.sin(theta)
        yu = yc + yt * math.cos(theta)
        upper.append((xu * chord, yu * chord))

        # 下表面
        xl = x + yt * math.sin(theta)
        yl = yc - yt * math.cos(theta)
        lower.append((xl * chord, yl * chord))

    return upper, lower

def create_naca_airfoil():
    """创建NACA翼型"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("naca_airfoil")

    # 参数
    naca_code = "0012"  # NACA 0012
    chord = 1.0
    num_points = 50

    # 生成翼型坐标
    upper, lower = naca4_airfoil(naca_code, num_points, chord)

    # 创建样条曲线点
    lc_le = 0.005   # 前缘网格尺寸
    lc_te = 0.01    # 后缘网格尺寸

    # 上表面点（从后缘到前缘）
    upper_points = []
    for i, (x, y) in enumerate(reversed(upper)):
        # 根据位置设置网格尺寸
        t = i / (len(upper) - 1)
        lc = lc_le * t + lc_te * (1 - t)
        p = gmsh.model.geo.addPoint(x, y, 0, lc)
        upper_points.append(p)

    # 下表面点（从前缘到后缘，跳过第一点避免重复）
    lower_points = []
    for i, (x, y) in enumerate(lower[1:]):
        t = i / (len(lower) - 2)
        lc = lc_le * (1 - t) + lc_te * t
        p = gmsh.model.geo.addPoint(x, y, 0, lc)
        lower_points.append(p)

    # 创建样条曲线
    # 上表面样条
    upper_spline = gmsh.model.geo.addSpline(upper_points)

    # 下表面样条（连接前缘到后缘）
    lower_spline = gmsh.model.geo.addSpline([upper_points[-1]] + lower_points + [upper_points[0]])

    # 创建翼型曲线环
    airfoil_loop = gmsh.model.geo.addCurveLoop([upper_spline, lower_spline])

    gmsh.model.geo.synchronize()

    # 物理组
    gmsh.model.addPhysicalGroup(1, [upper_spline], name="UpperSurface")
    gmsh.model.addPhysicalGroup(1, [lower_spline], name="LowerSurface")

    # 生成1D网格预览
    gmsh.model.mesh.generate(1)

    gmsh.write("naca_curve.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

    return airfoil_loop

if __name__ == "__main__":
    create_naca_airfoil()
```

## 完整CFD网格（C型域）

```python
#!/usr/bin/env python3
"""
案例5: NACA翼型C型CFD网格
"""
import gmsh
import sys
import math

def naca4_coordinates(code, num_points=50, chord=1.0):
    """生成NACA坐标（简化版）"""
    t = int(code[2:4]) / 100.0

    points = []
    # 使用余弦分布
    for i in range(num_points):
        beta = math.pi * i / (num_points - 1)
        x = 0.5 * (1 - math.cos(beta))
        yt = 5 * t * (0.2969 * math.sqrt(x) - 0.1260 * x
                      - 0.3516 * x**2 + 0.2843 * x**3 - 0.1015 * x**4)
        points.append((x * chord, yt * chord))

    # 返回完整轮廓（上表面 + 下表面反向）
    upper = points
    lower = [(x, -y) for x, y in reversed(points[1:-1])]

    return upper + lower

def create_cfd_mesh():
    """创建CFD网格"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("naca_cfd")

    # 参数
    chord = 1.0
    far_field = 20 * chord   # 远场距离
    wake_length = 10 * chord # 尾流长度

    # NACA翼型
    naca_points = naca4_coordinates("0012", 40, chord)

    # 创建翼型点
    lc_airfoil = 0.01
    airfoil_pts = []
    for x, y in naca_points:
        p = gmsh.model.geo.addPoint(x, y, 0, lc_airfoil)
        airfoil_pts.append(p)

    # 闭合翼型
    airfoil_pts.append(airfoil_pts[0])

    # 创建翼型样条
    airfoil_spline = gmsh.model.geo.addSpline(airfoil_pts)

    # 创建C型外边界
    lc_far = 2.0

    # C型边界点
    p_wake_up = gmsh.model.geo.addPoint(chord + wake_length, far_field, 0, lc_far)
    p_wake_down = gmsh.model.geo.addPoint(chord + wake_length, -far_field, 0, lc_far)
    p_inlet_up = gmsh.model.geo.addPoint(-far_field, far_field, 0, lc_far)
    p_inlet_down = gmsh.model.geo.addPoint(-far_field, -far_field, 0, lc_far)
    p_inlet_center = gmsh.model.geo.addPoint(-far_field, 0, 0, lc_far)

    # 创建外边界曲线
    # 出口上部
    outlet_up = gmsh.model.geo.addLine(airfoil_pts[-2], p_wake_up)  # 从后缘上表面
    # 出口边界
    outlet = gmsh.model.geo.addLine(p_wake_up, p_wake_down)
    # 出口下部
    outlet_down = gmsh.model.geo.addLine(p_wake_down, airfoil_pts[1])  # 到后缘下表面

    # 远场边界（半圆弧）
    far_top = gmsh.model.geo.addLine(p_wake_up, p_inlet_up)
    far_inlet = gmsh.model.geo.addCircleArc(p_inlet_up, p_inlet_center, p_inlet_down)
    far_bottom = gmsh.model.geo.addLine(p_inlet_down, p_wake_down)

    # 创建域
    # 外边界环
    outer_loop = gmsh.model.geo.addCurveLoop([
        outlet_up, far_top, far_inlet, -far_bottom, -outlet, -outlet_down
    ])

    # 翼型环
    airfoil_loop = gmsh.model.geo.addCurveLoop([airfoil_spline])

    # 创建曲面（外边界减去翼型）
    surface = gmsh.model.geo.addPlaneSurface([outer_loop, airfoil_loop])

    gmsh.model.geo.synchronize()

    # 物理组
    gmsh.model.addPhysicalGroup(1, [airfoil_spline], name="Airfoil")
    gmsh.model.addPhysicalGroup(1, [far_inlet], name="Inlet")
    gmsh.model.addPhysicalGroup(1, [outlet], name="Outlet")
    gmsh.model.addPhysicalGroup(1, [far_top, far_bottom], name="Farfield")
    gmsh.model.addPhysicalGroup(2, [surface], name="Fluid")

    # 边界层网格
    # 在翼型表面创建边界层
    bl = gmsh.model.mesh.field.add("BoundaryLayer")
    gmsh.model.mesh.field.setNumbers(bl, "CurvesList", [airfoil_spline])
    gmsh.model.mesh.field.setNumber(bl, "Size", 0.002)       # 第一层厚度
    gmsh.model.mesh.field.setNumber(bl, "Ratio", 1.2)        # 增长比
    gmsh.model.mesh.field.setNumber(bl, "NbLayers", 15)      # 层数
    gmsh.model.mesh.field.setNumber(bl, "Quads", 1)          # 使用四边形

    gmsh.model.mesh.field.setAsBoundaryLayer(bl)

    # 设置远场网格尺寸
    gmsh.option.setNumber("Mesh.MeshSizeMax", 2.0)

    # 使用Frontal-Delaunay算法
    gmsh.option.setNumber("Mesh.Algorithm", 6)

    gmsh.model.mesh.generate(2)

    gmsh.write("naca_cfd.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    create_cfd_mesh()
```

## O型网格

```python
#!/usr/bin/env python3
"""
案例5: O型网格（翼型周围）
"""
import gmsh
import sys
import math

def create_o_grid():
    """创建O型网格"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("o_grid")

    # 参数
    chord = 1.0
    t = 0.12  # NACA 0012
    outer_radius = 5 * chord

    # 简化翼型（椭圆近似）
    a = chord / 2  # 半长轴
    b = chord * t / 2  # 半短轴

    # 翼型中心
    cx, cy = a, 0

    num_points = 60
    lc_inner = 0.02
    lc_outer = 0.3

    # 创建翼型点（椭圆近似）
    inner_points = []
    for i in range(num_points):
        theta = 2 * math.pi * i / num_points
        x = cx + a * math.cos(theta)
        y = cy + b * math.sin(theta)
        p = gmsh.model.geo.addPoint(x, y, 0, lc_inner)
        inner_points.append(p)

    # 创建外边界点
    outer_points = []
    for i in range(num_points):
        theta = 2 * math.pi * i / num_points
        x = cx + outer_radius * math.cos(theta)
        y = cy + outer_radius * math.sin(theta)
        p = gmsh.model.geo.addPoint(x, y, 0, lc_outer)
        outer_points.append(p)

    # 创建内边界样条
    inner_spline = gmsh.model.geo.addSpline(inner_points + [inner_points[0]])

    # 创建外边界样条
    outer_spline = gmsh.model.geo.addSpline(outer_points + [outer_points[0]])

    # 创建曲面
    inner_loop = gmsh.model.geo.addCurveLoop([inner_spline])
    outer_loop = gmsh.model.geo.addCurveLoop([outer_spline])
    surface = gmsh.model.geo.addPlaneSurface([outer_loop, inner_loop])

    gmsh.model.geo.synchronize()

    # 物理组
    gmsh.model.addPhysicalGroup(1, [inner_spline], name="Airfoil")
    gmsh.model.addPhysicalGroup(1, [outer_spline], name="Farfield")
    gmsh.model.addPhysicalGroup(2, [surface], name="Fluid")

    # 边界层
    bl = gmsh.model.mesh.field.add("BoundaryLayer")
    gmsh.model.mesh.field.setNumbers(bl, "CurvesList", [inner_spline])
    gmsh.model.mesh.field.setNumber(bl, "Size", 0.003)
    gmsh.model.mesh.field.setNumber(bl, "Ratio", 1.15)
    gmsh.model.mesh.field.setNumber(bl, "NbLayers", 20)
    gmsh.model.mesh.field.setNumber(bl, "Quads", 1)

    gmsh.model.mesh.field.setAsBoundaryLayer(bl)

    gmsh.option.setNumber("Mesh.Algorithm", 6)

    gmsh.model.mesh.generate(2)

    gmsh.write("o_grid.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    create_o_grid()
```

## 多段翼型

```python
#!/usr/bin/env python3
"""
案例5: 多段翼（主翼+襟翼）
"""
import gmsh
import sys
import math

def create_multi_element():
    """创建多段翼网格"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("multi_element")

    chord_main = 1.0
    chord_flap = 0.3

    # 主翼（简化为椭圆）
    main_a = chord_main / 2
    main_b = chord_main * 0.06
    main_cx, main_cy = main_a, 0

    # 襟翼
    flap_a = chord_flap / 2
    flap_b = chord_flap * 0.04
    flap_angle = math.radians(30)  # 偏转角度
    flap_gap = 0.02  # 缝隙
    flap_cx = chord_main + flap_gap + flap_a * math.cos(flap_angle)
    flap_cy = -flap_a * math.sin(flap_angle)

    # 远场
    far_field = 15 * chord_main
    lc_far = 1.0

    def create_airfoil_points(cx, cy, a, b, angle, num_pts, lc):
        """创建旋转的椭圆点"""
        points = []
        for i in range(num_pts):
            theta = 2 * math.pi * i / num_pts
            # 局部坐标
            x_local = a * math.cos(theta)
            y_local = b * math.sin(theta)
            # 旋转并平移
            x = cx + x_local * math.cos(angle) - y_local * math.sin(angle)
            y = cy + x_local * math.sin(angle) + y_local * math.cos(angle)
            p = gmsh.model.geo.addPoint(x, y, 0, lc)
            points.append(p)
        return points

    # 创建主翼
    main_pts = create_airfoil_points(main_cx, main_cy, main_a, main_b, 0, 40, 0.02)
    main_spline = gmsh.model.geo.addSpline(main_pts + [main_pts[0]])

    # 创建襟翼
    flap_pts = create_airfoil_points(flap_cx, flap_cy, flap_a, flap_b, -flap_angle, 30, 0.01)
    flap_spline = gmsh.model.geo.addSpline(flap_pts + [flap_pts[0]])

    # 创建远场边界
    center = gmsh.model.geo.addPoint(main_cx, 0, 0)
    far_pts = []
    for i in range(40):
        theta = 2 * math.pi * i / 40
        x = main_cx + far_field * math.cos(theta)
        y = far_field * math.sin(theta)
        p = gmsh.model.geo.addPoint(x, y, 0, lc_far)
        far_pts.append(p)

    far_spline = gmsh.model.geo.addSpline(far_pts + [far_pts[0]])

    # 创建曲面
    main_loop = gmsh.model.geo.addCurveLoop([main_spline])
    flap_loop = gmsh.model.geo.addCurveLoop([flap_spline])
    far_loop = gmsh.model.geo.addCurveLoop([far_spline])

    surface = gmsh.model.geo.addPlaneSurface([far_loop, main_loop, flap_loop])

    gmsh.model.geo.synchronize()

    # 物理组
    gmsh.model.addPhysicalGroup(1, [main_spline], name="MainWing")
    gmsh.model.addPhysicalGroup(1, [flap_spline], name="Flap")
    gmsh.model.addPhysicalGroup(1, [far_spline], name="Farfield")
    gmsh.model.addPhysicalGroup(2, [surface], name="Fluid")

    # 边界层
    bl = gmsh.model.mesh.field.add("BoundaryLayer")
    gmsh.model.mesh.field.setNumbers(bl, "CurvesList", [main_spline, flap_spline])
    gmsh.model.mesh.field.setNumber(bl, "Size", 0.002)
    gmsh.model.mesh.field.setNumber(bl, "Ratio", 1.2)
    gmsh.model.mesh.field.setNumber(bl, "NbLayers", 12)
    gmsh.model.mesh.field.setNumber(bl, "Quads", 1)

    gmsh.model.mesh.field.setAsBoundaryLayer(bl)

    gmsh.option.setNumber("Mesh.Algorithm", 6)

    gmsh.model.mesh.generate(2)

    gmsh.write("multi_element.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    create_multi_element()
```

## 3D翼型拉伸

```python
#!/usr/bin/env python3
"""
案例5: 3D翼型拉伸
"""
import gmsh
import sys
import math

def create_3d_wing():
    """创建3D机翼"""
    gmsh.initialize(sys.argv)
    gmsh.model.add("wing_3d")

    # 参数
    chord = 1.0
    span = 3.0
    t = 0.12

    # 创建翼型剖面（使用OCC的椭圆近似）
    # 前缘点
    p_le = gmsh.model.occ.addPoint(0, 0, 0)
    # 后缘点
    p_te = gmsh.model.occ.addPoint(chord, 0, 0)
    # 上表面控制点
    p_up = gmsh.model.occ.addPoint(chord * 0.3, chord * t/2, 0)
    # 下表面控制点
    p_down = gmsh.model.occ.addPoint(chord * 0.3, -chord * t/2, 0)

    # 创建样条
    upper = gmsh.model.occ.addSpline([p_te, p_up, p_le])
    lower = gmsh.model.occ.addSpline([p_le, p_down, p_te])

    # 创建曲线环和平面
    loop = gmsh.model.occ.addCurveLoop([upper, lower])
    surface = gmsh.model.occ.addPlaneSurface([loop])

    # 拉伸
    extrusion = gmsh.model.occ.extrude([(2, surface)], 0, 0, span)

    gmsh.model.occ.synchronize()

    # 获取体积
    volumes = gmsh.model.getEntities(dim=3)

    # 识别边界
    boundary = gmsh.model.getBoundary(volumes, oriented=False)

    wing_surface = []
    tip_surfaces = []

    for dim, tag in boundary:
        com = gmsh.model.occ.getCenterOfMass(dim, tag)
        z = com[2]

        if abs(z) < 0.1 or abs(z - span) < 0.1:
            tip_surfaces.append(tag)
        else:
            wing_surface.append(tag)

    gmsh.model.addPhysicalGroup(2, wing_surface, name="WingSurface")
    gmsh.model.addPhysicalGroup(2, tip_surfaces, name="Tips")
    gmsh.model.addPhysicalGroup(3, [v[1] for v in volumes], name="Wing")

    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)
    gmsh.model.mesh.generate(3)

    gmsh.write("wing_3d.msh")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    create_3d_wing()
```

## 总结

本案例涵盖了：

1. **NACA翼型生成**: 4位数翼型参数化
2. **C型网格**: 适用于外流问题
3. **O型网格**: 翼型周围结构化网格
4. **边界层网格**: 使用BoundaryLayer场
5. **多段翼**: 处理复杂配置
6. **3D拉伸**: 从2D剖面生成3D几何

## 下一步

- [06-地形网格生成](./06-地形网格生成.md) - 处理离散数据
