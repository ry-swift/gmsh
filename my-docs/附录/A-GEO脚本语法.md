# 附录A: GEO脚本语法

本附录提供Gmsh GEO脚本语言的完整语法参考。GEO是Gmsh的原生脚本语言，用于定义几何、网格参数和后处理设置。

## 基本语法

### 注释

```geo
// 单行注释

/* 多行注释
   可以跨越多行 */
```

### 变量

```geo
// 数值变量
a = 1.5;
b = 2 * a;

// 字符串变量
name = "my_mesh";

// 数组
points[] = {1, 2, 3, 4};
coords[] = {0.0, 1.0, 2.0};

// 访问数组元素
x = coords[0];

// 数组长度
n = #points[];
```

### 表达式

```geo
// 算术运算
a = 1 + 2;
b = 3 - 1;
c = 2 * 3;
d = 6 / 2;
e = 2 ^ 3;  // 幂运算

// 数学函数
s = Sin(Pi/4);
c = Cos(0);
t = Tan(Pi/4);
sq = Sqrt(2);
lg = Log(10);
ex = Exp(1);
ab = Abs(-5);
fl = Floor(3.7);
ce = Ceil(3.2);
rd = Round(3.5);

// 比较运算
cond = (a > b) ? a : b;  // 条件表达式
```

### 控制结构

```geo
// 循环
For i In {1:10}
  Point(i) = {i, 0, 0, 0.1};
EndFor

For i In {1:10:2}  // 步长为2
  Printf("i = %g", i);
EndFor

// 条件
If (a > b)
  c = a;
ElseIf (a < b)
  c = b;
Else
  c = a;
EndIf
```

### 宏

```geo
// 定义宏
Macro MyMacro
  Point(newp) = {x, y, z, lc};
  Printf("Created point at (%g, %g, %g)", x, y, z);
Return

// 调用宏
x = 1; y = 2; z = 0; lc = 0.1;
Call MyMacro;
```

## 几何命令

### 点

```geo
// 基本点定义
Point(tag) = {x, y, z, mesh_size};

// 示例
Point(1) = {0, 0, 0, 0.1};
Point(2) = {1, 0, 0, 0.1};
Point(3) = {1, 1, 0, 0.1};

// 获取新标签
newp = newp;  // 下一个可用点标签
Point(newp) = {2, 0, 0, 0.1};
```

### 曲线

```geo
// 直线
Line(tag) = {start_point, end_point};
Line(1) = {1, 2};

// 圆弧 (起点, 圆心, 终点)
Circle(tag) = {start, center, end};
Circle(2) = {1, 5, 3};

// 椭圆弧 (起点, 圆心, 长轴端点, 终点)
Ellipse(tag) = {start, center, major_axis, end};

// 样条曲线
Spline(tag) = {point1, point2, point3, ...};
Spline(3) = {1, 2, 3, 4, 5};

// B样条
BSpline(tag) = {point1, point2, point3, ...};

// 贝塞尔曲线
Bezier(tag) = {point1, point2, point3, point4};

// 复合曲线
Compound Curve(tag) = {curve1, curve2, ...};
```

### 曲线环与曲面

```geo
// 曲线环
Curve Loop(tag) = {curve1, curve2, curve3, ...};
// 负号表示反向
Curve Loop(1) = {1, 2, -3, -4};

// 平面
Plane Surface(tag) = {curve_loop};
Plane Surface(1) = {1};

// 带孔的平面
Plane Surface(2) = {outer_loop, hole_loop1, hole_loop2};

// 参数曲面（填充）
Surface(tag) = {curve_loop};

// 复合曲面
Compound Surface(tag) = {surface1, surface2, ...};
```

### 曲面环与体积

```geo
// 曲面环
Surface Loop(tag) = {surface1, surface2, ...};
Surface Loop(1) = {1, 2, 3, 4, 5, 6};

// 体积
Volume(tag) = {surface_loop};
Volume(1) = {1};

// 带空腔的体积
Volume(2) = {outer_loop, cavity_loop};

// 复合体积
Compound Volume(tag) = {volume1, volume2, ...};
```

### 几何变换

```geo
// 平移
Translate {dx, dy, dz} { entity_list }

// 旋转 (绕经过(x,y,z)的轴(ax,ay,az)旋转angle弧度)
Rotate {{ax, ay, az}, {x, y, z}, angle} { entity_list }

// 缩放
Dilate {{x, y, z}, factor} { entity_list }
Dilate {{x, y, z}, {fx, fy, fz}} { entity_list }  // 各向异性

// 镜像 (关于平面ax+by+cz+d=0)
Symmetry {a, b, c, d} { entity_list }

// 复制并变换
new_entities[] = Translate {1, 0, 0} { Duplicata { Point{1}; Line{1}; } };
```

### 拉伸

```geo
// 线性拉伸
Extrude {dx, dy, dz} { entity_list }

// 旋转拉伸
Extrude {{ax, ay, az}, {x, y, z}, angle} { entity_list }

// 分层拉伸
Extrude {0, 0, 1} {
  Surface{1};
  Layers{10};           // 10层
  // 或
  Layers{{5, 5}, {0.3, 1.0}};  // 5层到0.3, 5层到1.0
  Recombine;            // 重组为六面体
}

// 获取拉伸结果
out[] = Extrude {0, 0, 1} { Surface{1}; };
// out[0] = 顶面
// out[1] = 生成的体积
```

### 布尔运算

```geo
// 并集
BooleanUnion { Volume{1}; Delete; } { Volume{2}; Delete; }

// 交集
BooleanIntersection { Volume{1}; Delete; } { Volume{2}; Delete; }

// 差集
BooleanDifference { Volume{1}; Delete; } { Volume{2}; Delete; }

// 分片
BooleanFragments { Volume{1}; Delete; } { Volume{2}; Delete; }

// 保留结果
result() = BooleanDifference { Volume{1}; } { Volume{2}; };
```

## 物理组

```geo
// 定义物理组
Physical Point("point_name") = {point_tags};
Physical Curve("curve_name") = {curve_tags};
Physical Surface("surface_name") = {surface_tags};
Physical Volume("volume_name") = {volume_tags};

// 示例
Physical Surface("inlet") = {1};
Physical Surface("outlet") = {2};
Physical Surface("walls") = {3, 4, 5, 6};
Physical Volume("fluid") = {1};
```

## 网格控制

### 网格尺寸

```geo
// 全局尺寸
Mesh.CharacteristicLengthMin = 0.1;
Mesh.CharacteristicLengthMax = 1.0;
Mesh.CharacteristicLengthFactor = 1.0;

// 点尺寸（在Point定义中）
Point(1) = {0, 0, 0, 0.1};

// 修改已有点的尺寸
Characteristic Length {1, 2, 3} = 0.05;
```

### 结构化网格

```geo
// Transfinite曲线
Transfinite Curve {curve_tags} = n_nodes;
Transfinite Curve {1, 3} = 20;
Transfinite Curve {2, 4} = 10 Using Progression 1.2;
Transfinite Curve {5} = 15 Using Bump 0.5;

// Transfinite曲面
Transfinite Surface {surface_tag};
Transfinite Surface {1} = {p1, p2, p3, p4};  // 指定角点
Transfinite Surface {1} Left;   // 或 Right, Alternate

// Transfinite体积
Transfinite Volume {volume_tag};
Transfinite Volume {1} = {p1, p2, p3, p4, p5, p6, p7, p8};

// 重组为四边形/六面体
Recombine Surface {surface_tags};
Recombine Volume {volume_tags};  // 需要SubdivisionAlgorithm
```

### 边界层

```geo
// 边界层拉伸
Field[1] = BoundaryLayer;
Field[1].CurvesList = {1, 2, 3};
Field[1].PointsList = {1, 2};
Field[1].Size = 0.01;
Field[1].Ratio = 1.2;
Field[1].NbLayers = 10;
Field[1].Quads = 1;
Field[1].FanPointsList = {1, 2};
Field[1].FanPointsSizesList = {5, 5};

Background Field = 1;
```

### 尺寸场

```geo
// 距离场
Field[1] = Distance;
Field[1].CurvesList = {1, 2};
Field[1].Sampling = 100;

// 阈值场
Field[2] = Threshold;
Field[2].InField = 1;
Field[2].SizeMin = 0.01;
Field[2].SizeMax = 0.1;
Field[2].DistMin = 0;
Field[2].DistMax = 0.5;

// 盒子场
Field[3] = Box;
Field[3].XMin = -1; Field[3].XMax = 1;
Field[3].YMin = -1; Field[3].YMax = 1;
Field[3].ZMin = -1; Field[3].ZMax = 1;
Field[3].VIn = 0.05;
Field[3].VOut = 0.2;

// 最小值组合
Field[4] = Min;
Field[4].FieldsList = {2, 3};

// 设置背景场
Background Field = 4;
```

### 嵌入实体

```geo
// 在曲面中嵌入点
Point{5, 6} In Surface{1};

// 在曲面中嵌入曲线
Curve{10} In Surface{1};

// 在体积中嵌入点/曲线/曲面
Point{5} In Volume{1};
Curve{10} In Volume{1};
Surface{5} In Volume{1};
```

## 选项设置

### 通用选项

```geo
General.Terminal = 1;           // 终端输出
General.Verbosity = 5;          // 详细程度
General.NumThreads = 4;         // 线程数
```

### 几何选项

```geo
Geometry.Tolerance = 1e-8;
Geometry.AutoCoherence = 1;
Geometry.OCCFixDegenerated = 1;
Geometry.OCCFixSmallEdges = 1;
Geometry.OCCFixSmallFaces = 1;
Geometry.OCCSewFaces = 1;
```

### 网格选项

```geo
Mesh.Algorithm = 6;             // 2D算法
Mesh.Algorithm3D = 1;           // 3D算法
Mesh.RecombineAll = 0;          // 全部重组
Mesh.RecombinationAlgorithm = 1; // Blossom
Mesh.Smoothing = 10;            // 光滑迭代
Mesh.OptimizeNetgen = 1;        // Netgen优化
Mesh.HighOrderOptimize = 2;     // 高阶优化
Mesh.ElementOrder = 1;          // 元素阶次
Mesh.SecondOrderLinear = 0;     // 二阶节点位置
Mesh.Binary = 0;                // 二进制输出
Mesh.MshFileVersion = 4.1;      // MSH版本
Mesh.SaveAll = 0;               // 保存所有元素
```

## 后处理

### 视图定义

```geo
// 创建视图
View "Temperature" {
  // 标量三角形
  ST(x1,y1,z1, x2,y2,z2, x3,y3,z3){v1, v2, v3};

  // 向量三角形
  VT(x1,y1,z1, x2,y2,z2, x3,y3,z3){vx1,vy1,vz1, vx2,vy2,vz2, vx3,vy3,vz3};

  // 标量四面体
  SS(x1,y1,z1, x2,y2,z2, x3,y3,z3, x4,y4,z4){v1, v2, v3, v4};

  // 文本标签
  T3(x, y, z, style){"label text"};
  T2(x, y, style){"2D text"};
};
```

### 视图选项

```geo
View[0].Visible = 1;
View[0].IntervalsType = 3;      // 等值面
View[0].NbIso = 20;             // 等值线数
View[0].RangeType = 2;          // 自定义范围
View[0].CustomMin = 0;
View[0].CustomMax = 100;
View[0].ColorTable = {
  {0, 0, 255},                  // 蓝色
  {0, 255, 0},                  // 绿色
  {255, 0, 0}                   // 红色
};
```

## 完整示例

### 2D矩形网格

```geo
// 参数
L = 10;
H = 5;
lc = 0.5;

// 几何
Point(1) = {0, 0, 0, lc};
Point(2) = {L, 0, 0, lc};
Point(3) = {L, H, 0, lc};
Point(4) = {0, H, 0, lc};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};

Curve Loop(1) = {1, 2, 3, 4};
Plane Surface(1) = {1};

// 结构化网格
Transfinite Curve {1, 3} = 21;
Transfinite Curve {2, 4} = 11;
Transfinite Surface {1};
Recombine Surface {1};

// 物理组
Physical Surface("domain") = {1};
Physical Curve("bottom") = {1};
Physical Curve("right") = {2};
Physical Curve("top") = {3};
Physical Curve("left") = {4};

// 生成网格
Mesh 2;
Save "rectangle.msh";
```

### 3D圆柱流场

```geo
// 参数
R = 0.5;
L = 5;
D = 3;
lc_cyl = 0.1;
lc_far = 0.5;

// 设置内核
SetFactory("OpenCASCADE");

// 几何
Cylinder(1) = {0, 0, -D/2, 0, 0, D, R};
Box(2) = {-L, -D/2, -D/2, 3*L, D, D};

// 布尔差集
BooleanDifference(3) = { Volume{2}; Delete; }{ Volume{1}; Delete; };

// 尺寸场
Field[1] = Distance;
Field[1].SurfacesList = {7};  // 圆柱面
Field[1].Sampling = 100;

Field[2] = Threshold;
Field[2].InField = 1;
Field[2].SizeMin = lc_cyl;
Field[2].SizeMax = lc_far;
Field[2].DistMin = 0;
Field[2].DistMax = 1;

Background Field = 2;

// 物理组
Physical Volume("fluid") = {3};
Physical Surface("inlet") = {1};
Physical Surface("outlet") = {2};
Physical Surface("walls") = {3, 4, 5, 6};
Physical Surface("cylinder") = {7};

// 生成网格
Mesh 3;
Save "cylinder_flow.msh";
```

## 与Python API对照

| GEO语法 | Python API |
|---------|-----------|
| `Point(1) = {0,0,0,0.1}` | `gmsh.model.geo.addPoint(0,0,0,0.1,1)` |
| `Line(1) = {1, 2}` | `gmsh.model.geo.addLine(1,2,1)` |
| `Curve Loop(1) = {1,2,3,4}` | `gmsh.model.geo.addCurveLoop([1,2,3,4],1)` |
| `Plane Surface(1) = {1}` | `gmsh.model.geo.addPlaneSurface([1],1)` |
| `Physical Surface("name") = {1}` | `gmsh.model.addPhysicalGroup(2,[1],name="name")` |
| `Mesh.Algorithm = 6` | `gmsh.option.setNumber("Mesh.Algorithm",6)` |
| `Transfinite Curve {1} = 10` | `gmsh.model.mesh.setTransfiniteCurve(1,10)` |
| `Mesh 2` | `gmsh.model.mesh.generate(2)` |

## 小结

GEO脚本语言特点：

1. **简洁直观**：语法接近数学表达
2. **完整功能**：支持所有Gmsh功能
3. **参数化**：变量和循环支持参数化建模
4. **可读性**：脚本文件便于版本控制和共享

使用建议：
- 简单几何用GEO脚本快速定义
- 复杂参数化用Python API
- 可以混合使用：Python调用GEO脚本
