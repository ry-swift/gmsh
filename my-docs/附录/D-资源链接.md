# 附录D: 资源链接

本附录汇总Gmsh相关的官方资源、社区资源和学习资料。

## 官方资源

### 主要网站

| 资源 | 链接 | 说明 |
|------|------|------|
| Gmsh官网 | https://gmsh.info | 官方主页，下载和文档入口 |
| GitLab仓库 | https://gitlab.onelab.info/gmsh/gmsh | 源代码仓库 |
| 文档主页 | https://gmsh.info/doc/texinfo/gmsh.html | 官方参考手册 |
| API文档 | https://gmsh.info/doc/texinfo/gmsh.html#Gmsh-API | API完整参考 |
| 教程 | https://gmsh.info/doc/texinfo/gmsh.html#Tutorial | 官方教程 |

### 下载链接

| 平台 | 链接 |
|------|------|
| 官方下载 | https://gmsh.info/#Download |
| PyPI (Python) | https://pypi.org/project/gmsh/ |
| GitHub镜像 | https://github.com/gmsh/gmsh |

### 官方示例

| 资源 | 位置 | 说明 |
|------|------|------|
| 基础教程 | `tutorials/` | t1-t21教程文件 |
| API示例 | `examples/api/` | 130+个API使用示例 |
| 扩展示例 | `examples/` | 更多应用示例 |

## 社区资源

### 论坛与邮件列表

| 资源 | 链接 | 说明 |
|------|------|------|
| Gmsh邮件列表 | gmsh@onelab.info | 官方邮件列表 |
| 邮件列表存档 | https://gitlab.onelab.info/gmsh/gmsh/-/issues | GitLab Issues |

### 问答网站

在以下网站搜索"Gmsh"可以找到大量讨论：

- Stack Overflow: https://stackoverflow.com/questions/tagged/gmsh
- ResearchGate: https://www.researchgate.net
- CFD Online Forum: https://www.cfd-online.com/Forums

### 第三方教程

#### 视频教程

- YouTube搜索"Gmsh tutorial"
- B站搜索"Gmsh教程"

#### 博客文章

许多研究者和工程师在个人博客中分享Gmsh使用经验：

- 搜索"Gmsh mesh generation tutorial"
- 搜索"Gmsh Python API example"

## 相关软件

### CAD软件

| 软件 | 链接 | 与Gmsh集成 |
|------|------|-----------|
| FreeCAD | https://www.freecadweb.org | 可导出STEP/IGES |
| OpenSCAD | https://openscad.org | 参数化建模 |
| Salome | https://www.salome-platform.org | 集成Gmsh网格器 |
| ONELAB | https://onelab.info | Gmsh的上层平台 |

### 有限元求解器

| 软件 | 链接 | Gmsh支持 |
|------|------|---------|
| FEniCS/FEniCSx | https://fenicsproject.org | 直接读取MSH |
| deal.II | https://www.dealii.org | 支持MSH格式 |
| GetDP | https://getdp.info | 与Gmsh集成 |
| Elmer | https://www.csc.fi/web/elmer | 支持MSH输入 |
| Code_Aster | https://www.code-aster.org | 支持MED格式 |
| CalculiX | http://www.calculix.de | 支持INP格式 |

### CFD软件

| 软件 | 链接 | Gmsh支持 |
|------|------|---------|
| OpenFOAM | https://www.openfoam.com | gmshToFoam转换器 |
| SU2 | https://su2code.github.io | 支持SU2格式 |
| Nektar++ | https://www.nektar.info | 支持多种格式 |

### 可视化软件

| 软件 | 链接 | 说明 |
|------|------|------|
| ParaView | https://www.paraview.org | 读取VTK/VTU |
| VisIt | https://visit-dav.github.io/visit-website | 读取多种格式 |
| Gmsh GUI | 内置 | Gmsh自带可视化 |

## 学习资料

### 书籍

虽然没有专门针对Gmsh的书籍，但以下书籍对理解网格生成有帮助：

1. **Mesh Generation** - Pascal Jean Frey, Paul-Louis George
   - 网格生成理论的经典著作

2. **Computational Geometry: Algorithms and Applications** - de Berg et al.
   - 计算几何基础

3. **The Finite Element Method** - O.C. Zienkiewicz
   - 有限元方法，包含网格章节

### 在线课程

- Coursera/edX搜索"Finite Element Method"
- MIT OpenCourseWare相关课程

### 学术论文

Gmsh相关的引用论文：

```
C. Geuzaine and J.-F. Remacle,
"Gmsh: a three-dimensional finite element mesh generator with built-in
pre- and post-processing facilities",
International Journal for Numerical Methods in Engineering 79(11),
pp. 1309-1331, 2009.
```

## Python生态系统

### 相关库

| 库 | 安装 | 说明 |
|------|------|------|
| gmsh | `pip install gmsh` | 官方Python API |
| meshio | `pip install meshio` | 网格格式转换 |
| pygmsh | `pip install pygmsh` | Gmsh的Pythonic封装 |
| pyvista | `pip install pyvista` | 3D可视化 |
| numpy | `pip install numpy` | 数值计算 |
| scipy | `pip install scipy` | 科学计算 |

### meshio使用示例

```python
import meshio

# 读取网格
mesh = meshio.read("mesh.msh")

# 转换格式
meshio.write("mesh.vtk", mesh)
meshio.write("mesh.xdmf", mesh)

# 访问数据
points = mesh.points
cells = mesh.cells
```

### pygmsh使用示例

```python
import pygmsh

with pygmsh.geo.Geometry() as geom:
    geom.add_circle([0.0, 0.0], 1.0, mesh_size=0.1)
    mesh = geom.generate_mesh()

# mesh是meshio.Mesh对象
```

## 开发资源

### API开发

| 语言 | 头文件/模块 | 说明 |
|------|-------------|------|
| C++ | `gmsh.h` | C++ API |
| C | `gmshc.h` | C API |
| Python | `gmsh.py` | Python模块 |
| Julia | `gmsh.jl` | Julia模块 |
| Fortran | `gmsh.f90` | Fortran模块 |

### 编译Gmsh

```bash
# 克隆源码
git clone https://gitlab.onelab.info/gmsh/gmsh.git
cd gmsh

# 创建构建目录
mkdir build && cd build

# 配置（启用所有功能）
cmake -DENABLE_BUILD_DYNAMIC=1 \
      -DENABLE_OPENMP=1 \
      -DENABLE_FLTK=1 \
      -DENABLE_OCC=1 \
      ..

# 编译
make -j$(nproc)

# 安装
make install
```

### 贡献代码

1. Fork官方仓库
2. 创建特性分支
3. 提交更改
4. 创建Merge Request

## 常用命令速查

### 安装

```bash
# Python
pip install gmsh

# 升级
pip install --upgrade gmsh

# 验证安装
python -c "import gmsh; print(gmsh.__version__)"
```

### 命令行

```bash
# 生成3D网格
gmsh model.geo -3 -o mesh.msh

# 格式转换
gmsh input.msh -o output.vtk

# 查看帮助
gmsh -help
```

### Python快速开始

```python
import gmsh

gmsh.initialize()
gmsh.model.add("my_model")

# ... 创建几何 ...

gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)
gmsh.write("mesh.msh")

gmsh.finalize()
```

## 获取帮助

### 报告问题

1. 搜索现有Issues
2. 准备最小可复现示例
3. 在GitLab创建Issue
4. 提供版本信息和错误日志

### 提问技巧

1. 清楚描述问题
2. 提供代码示例
3. 说明预期结果和实际结果
4. 附上相关文件

## 版本历史

| 版本 | 日期 | 主要特性 |
|------|------|---------|
| 4.14 | 2024 | 当前稳定版 |
| 4.0 | 2018 | 新API引入 |
| 3.0 | 2016 | OCC集成改进 |
| 2.0 | 2009 | 重大重构 |

## 小结

本附录提供的资源可以帮助您：

1. **深入学习**：官方文档、教程、示例
2. **解决问题**：社区论坛、邮件列表
3. **扩展应用**：相关软件、Python库
4. **参与开发**：源码、贡献指南

建议的学习路径：

1. 阅读官方教程t1-t5
2. 运行API示例
3. 参考本文档的实战案例
4. 在社区提问和交流
