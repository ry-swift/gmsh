# 附录C: 术语表

本附录提供Gmsh及网格生成相关的中英对照术语表。

## 几何术语

| 英文 | 中文 | 说明 |
|------|------|------|
| Point | 点 | 0维几何实体 |
| Curve / Line / Edge | 曲线/线/边 | 1维几何实体 |
| Surface / Face | 曲面/面 | 2维几何实体 |
| Volume / Solid | 体积/实体 | 3维几何实体 |
| Entity | 实体 | 几何对象的通用术语 |
| Vertex | 顶点 | 几何的角点 |
| Edge | 边 | 几何的边界线 |
| Loop | 环 | 闭合的曲线集合 |
| Shell | 壳 | 闭合的曲面集合 |
| Boundary | 边界 | 实体的边界 |
| Interior | 内部 | 实体的内部区域 |

## 曲线与曲面

| 英文 | 中文 | 说明 |
|------|------|------|
| Straight Line | 直线 | 两点间的直线段 |
| Circle Arc | 圆弧 | 圆的一部分 |
| Ellipse Arc | 椭圆弧 | 椭圆的一部分 |
| Spline | 样条曲线 | 经过控制点的光滑曲线 |
| B-Spline | B样条 | 基于基函数的样条 |
| Bezier Curve | 贝塞尔曲线 | 由控制点定义的多项式曲线 |
| NURBS | 非均匀有理B样条 | 工业标准曲线/曲面表示 |
| Plane Surface | 平面 | 平坦的2D曲面 |
| Ruled Surface | 直纹面 | 由直线族生成的曲面 |
| Freeform Surface | 自由曲面 | 任意形状的曲面 |
| Trimmed Surface | 裁剪曲面 | 被边界裁剪的曲面 |

## 几何操作

| 英文 | 中文 | 说明 |
|------|------|------|
| Boolean Operation | 布尔运算 | 几何的集合运算 |
| Union / Fuse | 并集/融合 | 合并两个几何 |
| Intersection | 交集 | 取公共部分 |
| Difference / Cut | 差集/切除 | 减去部分几何 |
| Fragment | 分片 | 分割并保留所有部分 |
| Translation | 平移 | 沿向量移动 |
| Rotation | 旋转 | 绕轴旋转 |
| Scaling / Dilation | 缩放 | 放大或缩小 |
| Mirroring / Symmetry | 镜像/对称 | 关于面对称 |
| Extrusion | 拉伸 | 沿方向延伸 |
| Revolution / Revolve | 旋转体 | 绕轴旋转生成 |
| Sweep / Pipe | 扫掠/管道 | 沿路径扫掠 |
| Loft / ThruSections | 放样/过渡 | 连接多个截面 |
| Fillet | 倒圆角 | 圆角过渡 |
| Chamfer | 倒斜角 | 斜角过渡 |

## CAD内核

| 英文 | 中文 | 说明 |
|------|------|------|
| Geometry Kernel | 几何内核 | 几何计算引擎 |
| Built-in Kernel | 内置内核 | Gmsh原生几何内核 |
| OpenCASCADE / OCC | 开放级联 | 开源CAD内核 |
| BRep | 边界表示 | 几何表示方法 |
| CSG | 构造实体几何 | 布尔运算建模方法 |
| Parametric Model | 参数化模型 | 由参数驱动的模型 |

## 网格术语

| 英文 | 中文 | 说明 |
|------|------|------|
| Mesh | 网格 | 离散化的几何 |
| Meshing | 网格生成/网格划分 | 生成网格的过程 |
| Discretization | 离散化 | 连续域的离散表示 |
| Node / Vertex | 节点/顶点 | 网格的点 |
| Element / Cell | 单元/单元格 | 网格的基本组成单元 |
| Connectivity | 连接性 | 单元的节点组成关系 |
| Mesh Size | 网格尺寸 | 单元的特征大小 |
| Mesh Density | 网格密度 | 单位体积内的单元数 |
| Mesh Quality | 网格质量 | 单元形状质量度量 |

## 单元类型

| 英文 | 中文 | 说明 |
|------|------|------|
| Triangle | 三角形 | 3节点2D单元 |
| Quadrilateral / Quad | 四边形 | 4节点2D单元 |
| Tetrahedron / Tet | 四面体 | 4节点3D单元 |
| Hexahedron / Hex | 六面体 | 8节点3D单元 |
| Prism / Wedge | 棱柱/楔形 | 6节点3D单元 |
| Pyramid | 金字塔/锥形 | 5节点3D单元 |
| Simplex | 单纯形 | 三角形/四面体的统称 |
| Linear Element | 线性单元 | 一阶单元 |
| Quadratic Element | 二次单元 | 二阶单元 |
| Higher-Order Element | 高阶单元 | 三阶及以上单元 |
| Lagrange Element | 拉格朗日单元 | 完全多项式基函数单元 |
| Serendipity Element | 塞伦迪皮蒂单元 | 无内部节点的单元 |

## 网格类型

| 英文 | 中文 | 说明 |
|------|------|------|
| Structured Mesh | 结构化网格 | 规则排列的网格 |
| Unstructured Mesh | 非结构化网格 | 不规则排列的网格 |
| Hybrid Mesh | 混合网格 | 多种单元类型的网格 |
| Conformal Mesh | 协调网格 | 单元面对面匹配 |
| Non-conformal Mesh | 非协调网格 | 界面不匹配的网格 |
| Transfinite Mesh | 横截网格 | 指定边界节点数的结构化网格 |
| Body-fitted Mesh | 贴体网格 | 与几何边界匹配的网格 |
| Cartesian Mesh | 笛卡尔网格 | 轴对齐的规则网格 |

## 网格算法

| 英文 | 中文 | 说明 |
|------|------|------|
| Delaunay Triangulation | Delaunay三角剖分 | 最大化最小角的三角化 |
| Frontal Advancing | 前沿推进 | 从边界向内生成网格 |
| Octree | 八叉树 | 空间划分数据结构 |
| Mesh Adaptation | 网格自适应 | 根据解自动细化 |
| Mesh Refinement | 网格细化 | 增加网格密度 |
| Mesh Coarsening | 网格粗化 | 减少网格密度 |
| Mesh Smoothing | 网格光滑 | 改善节点位置 |
| Mesh Optimization | 网格优化 | 改善单元质量 |
| Recombination | 重组 | 三角形合并为四边形 |

## 边界层

| 英文 | 中文 | 说明 |
|------|------|------|
| Boundary Layer | 边界层 | 壁面附近的薄层网格 |
| Inflation Layer | 膨胀层 | 同边界层 |
| Prism Layer | 棱柱层 | 边界层的棱柱单元 |
| First Layer Height | 第一层高度 | 最靠近壁面的层厚 |
| Growth Rate / Ratio | 增长率 | 相邻层厚度比 |
| Number of Layers | 层数 | 边界层的层数 |
| y+ | y+ | 无量纲壁面距离 |

## 尺寸场

| 英文 | 中文 | 说明 |
|------|------|------|
| Size Field | 尺寸场 | 空间变化的网格尺寸定义 |
| Background Mesh | 背景网格 | 用于尺寸控制的参考网格 |
| Distance Field | 距离场 | 到几何的距离 |
| Threshold Field | 阈值场 | 基于距离的尺寸渐变 |
| Box Field | 盒子场 | 矩形区域的尺寸定义 |
| Curvature | 曲率 | 几何的弯曲程度 |
| Anisotropic | 各向异性 | 方向相关的尺寸 |
| Metric Tensor | 度量张量 | 各向异性尺寸的张量表示 |

## 物理组与边界条件

| 英文 | 中文 | 说明 |
|------|------|------|
| Physical Group | 物理组 | 用于边界条件的实体集合 |
| Physical Surface | 物理面 | 2D物理组 |
| Physical Volume | 物理体 | 3D物理组 |
| Boundary Condition | 边界条件 | 边界上的约束 |
| Dirichlet BC | 狄利克雷边界条件 | 指定值的边界条件 |
| Neumann BC | 诺伊曼边界条件 | 指定导数的边界条件 |
| Periodic BC | 周期边界条件 | 周期性约束 |
| Inlet | 入口 | 流体入口边界 |
| Outlet | 出口 | 流体出口边界 |
| Wall | 壁面 | 固体壁面边界 |
| Symmetry | 对称面 | 对称边界 |

## 网格分区

| 英文 | 中文 | 说明 |
|------|------|------|
| Partitioning | 分区 | 将网格分割为多个部分 |
| Partition | 分区/部分 | 网格的一个子集 |
| Domain Decomposition | 域分解 | 将计算域分解 |
| Load Balancing | 负载均衡 | 平衡各分区的计算量 |
| Ghost Elements | 幽灵单元 | 分区边界的重叠单元 |
| Interface | 界面 | 分区之间的边界 |

## 后处理

| 英文 | 中文 | 说明 |
|------|------|------|
| Post-processing | 后处理 | 结果的可视化和分析 |
| View | 视图 | 后处理数据容器 |
| Scalar Field | 标量场 | 单值数据（温度、压力） |
| Vector Field | 向量场 | 向量数据（速度、力） |
| Tensor Field | 张量场 | 张量数据（应力、应变） |
| Contour | 等值线 | 等值线显示 |
| Isosurface | 等值面 | 等值面显示 |
| Streamline | 流线 | 向量场的流线 |
| Glyph | 符号 | 向量/张量的图形表示 |
| Color Map | 色图 | 数值到颜色的映射 |
| Animation | 动画 | 时序数据的动态显示 |

## 文件格式

| 英文 | 中文 | 说明 |
|------|------|------|
| MSH | MSH格式 | Gmsh原生网格格式 |
| GEO | GEO格式 | Gmsh几何脚本 |
| POS | POS格式 | Gmsh后处理格式 |
| STEP | STEP格式 | ISO CAD交换格式 |
| IGES | IGES格式 | 初始图形交换规范 |
| STL | STL格式 | 立体光刻格式 |
| VTK | VTK格式 | 可视化工具包格式 |
| CGNS | CGNS格式 | CFD通用标注系统 |
| UNV | UNV格式 | Universal通用格式 |

## 有限元相关

| 英文 | 中文 | 说明 |
|------|------|------|
| Finite Element Method | 有限元法 | FEM |
| Basis Function | 基函数 | 插值函数 |
| Shape Function | 形函数 | 单元内的插值函数 |
| Jacobian | 雅可比矩阵 | 坐标变换矩阵 |
| Integration Point | 积分点 | 数值积分的采样点 |
| Gauss Point | 高斯点 | 高斯积分点 |
| Degrees of Freedom | 自由度 | DOF |
| Stiffness Matrix | 刚度矩阵 | FEM系统矩阵 |
| Mass Matrix | 质量矩阵 | 动态分析的质量矩阵 |

## 其他常用术语

| 英文 | 中文 | 说明 |
|------|------|------|
| API | 应用程序接口 | 编程接口 |
| GUI | 图形用户界面 | 可视化操作界面 |
| Command Line | 命令行 | 文本命令界面 |
| Script | 脚本 | 命令序列文件 |
| Option | 选项 | 可配置的参数 |
| Tag | 标签 | 实体的唯一标识符 |
| Dimension | 维度 | 0D/1D/2D/3D |
| Coordinate | 坐标 | 空间位置 |
| Tolerance | 容差 | 数值精度阈值 |

## 小结

理解这些术语对于：

1. **阅读文档**：理解官方文档和教程
2. **编程使用**：正确使用API函数
3. **技术交流**：与同行有效沟通
4. **问题排查**：理解错误信息

都是非常重要的。
