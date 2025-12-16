# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 构建和开发命令

### 基本构建流程

```bash
# 创建并进入构建目录
mkdir build && cd build

# 配置和构建
cmake ..
make

# 安装（可选）
make install

# 构建带共享库和API支持的版本（推荐用于开发）
cmake -DENABLE_BUILD_DYNAMIC=1 ..
make && make install
```

### 运行测试

```bash
cd build
ctest              # 运行所有测试
ctest -V           # 详细输出
ctest -R <pattern> # 运行匹配的测试（如 ctest -R t1）
```

测试覆盖 `tutorials/`、`examples/` 和 `benchmarks/` 目录下的 .geo 文件。动态构建时还会测试 C++/C API tutorials。

### 生成API绑定

```bash
cd api
python gen.py  # 生成 gmsh.h, gmshc.h, gmsh.py, gmsh.jl, gmsh.f90
```

### 构建选项

- 使用 `ccmake ..` 进行交互式配置
- 使用 `make VERBOSE=1` 查看详细构建输出
- 默认构建类型为"RelWithDebInfo"，使用 `-DCMAKE_BUILD_TYPE=Debug` 进行调试构建
- 要求C++14标准，C代码使用C99

### 关键CMake选项

- `ENABLE_FLTK=0`: 不构建GUI界面
- `ENABLE_BUILD_LIB=1`: 构建静态库
- `ENABLE_BUILD_SHARED=1`: 构建共享库
- `ENABLE_BUILD_DYNAMIC=1`: 构建动态可执行文件（测试API必需）
- `ENABLE_TESTS=1`: 启用测试（默认开启）
- `CMAKE_PREFIX_PATH`: 指定外部包的位置
- `CMAKE_INSTALL_PREFIX`: 更改安装目录

### 问题报告

所有问题请报告至: https://gitlab.onelab.info/gmsh/gmsh/issues

## 架构概览

### 核心模块

Gmsh围绕四个主要模块组织：

1. **几何模块** (`src/geo/`)
   - `GModel`: 中央几何模型类
   - `GVertex`, `GEdge`, `GFace`, `GRegion`: 几何实体
   - `OCCVertex`, `OCCEdge`, `OCCFace`, `OCCRegion`: 基于OpenCASCADE的实体
   - `GModelIO_*.cpp` 文件中的CAD导入导出功能

2. **网格模块** (`src/mesh/`)
   - `meshGEdge`, `meshGFace`, `meshGRegion`: 针对特定实体的网格生成
   - `BackgroundMesh`: 自适应网格尺寸控制
   - `BoundaryLayers`: 网格边界层生成
   - `Field`: 尺寸场计算
   - 各种网格算法（Delaunay、BDS等）

3. **求解器模块** (`src/solver/`)
   - 支持线性系统的有限元框架
   - 各种求解器后端（PETSc、MUMPS、Eigen）
   - 弹性力学、热传导和其他物理模块

4. **后处理模块** (`src/post/`)
   - `PView`: 可视化视图
   - 数据处理和导出功能

### 支持组件

- **通用工具** (`src/common/`): 核心工具、选项、消息处理
- **图形** (`src/graphics/`): OpenGL渲染、图像导出
- **FLTK GUI** (`src/fltk/`): 图形用户界面
- **解析器** (`src/parser/`): .geo文件解析和脚本处理
- **插件系统** (`src/plugin/`): 可扩展功能
- **数值计算** (`src/numeric/`): 数学基础、基函数
- **API** (`api/`): 多语言绑定（C++、C、Python、Julia、Fortran）

### 关键数据结构

- **MElement**: 网格元素的基类（MTriangle、MTetrahedron等）
- **MVertex**: 网格顶点
- **GEntity**: 几何实体的基类
- **PViewData**: 后处理数据容器
- **fullMatrix**: 稠密矩阵运算

### 外部依赖

代码库在 `contrib/` 目录中集成了众多第三方库：

- **OpenCASCADE**: CAD几何内核
- **FLTK**: GUI框架
- **Eigen**: 线性代数
- **CGNS**、**MED**: 文件格式支持
- **Netgen**、**TetGen**: 外部网格生成器
- 各种网格优化和专用算法

### 多语言API

API系统 (`api/`) 提供：

- `gmsh.h`: C++ API头文件
- `gmsh.py`: Python绑定
- `gmsh.jl`: Julia绑定
- `gmsh.f90`: Fortran模块
- 从API规范自动生成

### 文件格式

对CAD和网格格式的广泛支持：

- **几何**: STEP、IGES、BREP、STL
- **网格**: MSH（原生）、CGNS、MED、UNV、VTK等
- **后处理**: POS、VTK和可视化格式

## 开发指南

### 代码组织

- 每个模块职责分离清晰
- 几何实体遵循继承层次结构（GEntity -> GVertex/GEdge/GFace/GRegion）
- 网格元素同样采用层次结构（MElement -> 具体元素类型）
- IO操作集中在GModelIO_*.cpp文件中

### 关键约定

- 要求符合C++14标准
- 头文件使用包含保护
- 在适用的地方使用OpenMP进行并行化
- 基于CMake的构建系统，配置选项丰富

### 几何操作

- 使用 `GModel` 作为几何数据的中央容器
- 几何操作通常委托给后端（OpenCASCADE、内置）
- 通过GModel方法创建和修改拓扑

### 网格操作

- 网格生成通过Generator类协调
- 元素创建使用工厂模式
- 质量度量和优化在专用模块中

这个代码库代表了一个全面的有限元网格生成套件，具有强大的数学基础和广泛的格式兼容性。