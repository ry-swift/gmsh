# Gmsh CMakeLists.txt 阅读指南

## 文件概览

这是 Gmsh 4.14.0 的主 CMake 构建配置文件，共约 2352 行，是一个典型的大型 C++ 项目构建脚本。

---

## 文件结构分解（按行号顺序）

### 1. 基础配置（第 1-27 行）

```cmake
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)  # CMake 最低版本要求
project(gmsh CXX C)                               # 项目名称和语言
set(CMAKE_CXX_STANDARD 14)                        # C++14 标准
set(CMAKE_C_STANDARD 99)                          # C99 标准
```

**要点**：
- Gmsh 需要 CMake 3.5 以上版本
- 使用 C++14 和 C99 标准
- 默认构建类型为 `RelWithDebInfo`（优化 + 调试信息）

---

### 2. 构建选项定义（第 29-112 行）

使用自定义宏 `opt()` 定义了约 80 个 `ENABLE_*` 选项：

| 分类 | 选项 | 说明 |
|------|------|------|
| **核心模块** | `ENABLE_MESH` | 网格生成模块 |
| | `ENABLE_POST` | 后处理模块 |
| | `ENABLE_PARSER` | .geo 文件解析器 |
| | `ENABLE_SOLVER` | 内置有限元求解器 |
| **GUI** | `ENABLE_FLTK` | 图形用户界面 |
| **CAD 内核** | `ENABLE_OCC` | OpenCASCADE |
| | `ENABLE_OCC_CAF` | STEP/IGES 属性支持 |
| **网格生成器** | `ENABLE_NETGEN` | 3D 前沿法网格生成 |
| | `ENABLE_BAMG` | 2D 各向异性网格 |
| | `ENABLE_HXT` | 重参数化和网格生成 |
| **线性代数** | `ENABLE_EIGEN` | Eigen 库（默认开启） |
| | `ENABLE_BLAS_LAPACK` | BLAS/LAPACK |
| | `ENABLE_PETSC` | PETSc 求解器 |
| **文件格式** | `ENABLE_CGNS` | CGNS 格式 |
| | `ENABLE_MED` | MED 格式 |
| **构建类型** | `ENABLE_BUILD_LIB` | 静态库 |
| | `ENABLE_BUILD_SHARED` | 共享库 |
| | `ENABLE_BUILD_DYNAMIC` | 动态链接可执行文件 |

**`opt()` 宏定义**（第 29-32 行）：
```cmake
macro(opt OPTION HELP VALUE)
  option(ENABLE_${OPTION} ${HELP} ${VALUE})
  set(OPT_TEXI "${OPT_TEXI}\n@item ENABLE_${OPTION}\n${HELP} (default: ${VALUE})")
endmacro()
```

---

### 3. 版本信息（第 114-152 行）

```cmake
set(GMSH_MAJOR_VERSION 4)
set(GMSH_MINOR_VERSION 14)
set(GMSH_PATCH_VERSION 0)
```

**Git 哈希获取**（第 124-137 行）：
```cmake
find_package(Git)
if(GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
                  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} ERROR_QUIET
                  OUTPUT_VARIABLE GIT_COMMIT_HASH
                  OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
```

---

### 4. 平台检测（第 207-262 行）

检测操作系统和架构：

```cmake
if(APPLE)
  if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64")
    set(GMSH_OS "MacOSARM")     # Apple Silicon
  else()
    set(GMSH_OS "MacOSX")       # Intel Mac
  endif()
elseif(CYGWIN OR MSYS)
  # Windows MinGW 编译器检测
elseif(WIN32)
  # Windows 原生
else()
  set(GMSH_OS "${CMAKE_SYSTEM_NAME}")  # Linux 等
endif()
```

**64位检测**（第 250-262 行）：
```cmake
check_type_size("size_t" SIZEOF_SIZE_T)
if(SIZEOF_SIZE_T EQUAL 8)
  set_config_option(HAVE_64BIT_SIZE_T "64Bit")
endif()
```

---

### 5. 编译器配置（第 302-428 行）

**MSVC 特殊处理**（第 302-327 行）：
```cmake
if(MSVC)
  set(GMSH_CONFIG_PRAGMAS "#pragma warning(disable:4800 4244 4267)")
  # 启用并行编译
  if(NOT ${VAR} MATCHES "/MP")
    set(${VAR} "${${VAR}} /MP")
  endif()
endif()
```

**OpenMP 支持**（第 335-371 行）：
```cmake
if(ENABLE_OPENMP)
  find_package(OpenMP)
  if(OpenMP_FOUND OR OPENMP_FOUND)
    set_config_option(HAVE_OPENMP "OpenMP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  endif()
endif()
```

**重要的自定义宏**：

1. `append_gmsh_src()`（第 391-397 行）：添加源文件到 GMSH_SRC
2. `find_all_libraries()`（第 399-417 行）：查找一组库文件
3. `set_compile_flags()`（第 419-428 行）：为文件设置编译标志

---

### 6. 核心依赖库查找（第 440-582 行）

**Eigen 查找**（第 440-456 行）：
```cmake
if(ENABLE_EIGEN)
  if(ENABLE_SYSTEM_CONTRIB)
    find_path(EIGEN_INC "Eigen/Dense" PATH_SUFFIXES eigen3)
  endif()
  if(NOT HAVE_EIGEN AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/contrib/eigen)
    include_directories(contrib/eigen)
    set_config_option(HAVE_EIGEN "Eigen[contrib]")
  endif()
endif()
```

**线性代数库查找优先级**：
1. Eigen（内置 contrib/eigen 或系统版本）
2. Intel MKL
3. ATLAS
4. OpenBLAS
5. 通用 BLAS/LAPACK

---

### 7. 源代码目录添加（第 592-618 行）

```cmake
add_subdirectory(src/common)    # 通用工具
add_subdirectory(src/numeric)   # 数值计算
add_subdirectory(src/geo)       # 几何模块

# 条件添加的模块
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/mesh AND ENABLE_MESH)
  add_subdirectory(src/mesh)
  set_config_option(HAVE_MESH "Mesh")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/solver AND ENABLE_SOLVER)
  add_subdirectory(src/solver)
  set_config_option(HAVE_SOLVER "Solver")
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/post AND ENABLE_POST)
  add_subdirectory(src/post)
  set_config_option(HAVE_POST "Post")
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin AND ENABLE_PLUGINS)
    add_subdirectory(src/plugin)
  endif()
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/parser AND ENABLE_PARSER)
  add_subdirectory(src/parser)
endif()
```

---

### 8. GUI 和图形库（第 624-758 行）

**FLTK 查找**（第 624-667 行）：
```cmake
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/fltk AND ENABLE_FLTK)
  # 优先使用 fltk-config 脚本
  find_program(FLTK_CONFIG_SCRIPT fltk-config)
  if(FLTK_CONFIG_SCRIPT)
    execute_process(COMMAND ${FLTK_CONFIG_SCRIPT} --api-version
                    OUTPUT_VARIABLE FLTK_VERSION)
    if(FLTK_VERSION VERSION_GREATER_EQUAL "1.3.0")
      add_subdirectory(src/fltk)
      set_config_option(HAVE_FLTK "Fltk")
    endif()
  endif()
  # 备选：使用 CMake 的 FindFLTK 模块
  if(NOT HAVE_FLTK)
    find_package(FLTK)
  endif()
endif()
```

**OpenGL 和图像库**（第 699-758 行）：
```cmake
if(HAVE_FLTK OR ENABLE_GRAPHICS)
  find_package(JPEG)
  find_package(ZLIB)
  find_package(PNG)
  find_package(OpenGL REQUIRED)
endif()
```

---

### 9. 第三方库集成（第 760-1052 行）

位于 `contrib/` 目录的库：

| 库 | 行号 | 用途 |
|----|------|------|
| ANN | 760-773 | 快速最近邻点搜索 |
| ALGLIB | 775-787 | 网格优化算法 |
| DiscreteIntegration | 799-804 | 离散积分（levelsets） |
| Kbipack | 806-821 | 同调求解器 |
| MathEx | 823-835 | 表达式解析器 |
| Metis | 904-920 | 网格分区 |
| Voro++ | 927-940 | 六面体网格（实验性） |
| HighOrderMeshOptimizer | 942-954 | 高阶网格优化 |
| Netgen | 987-993 | 3D 前沿法网格生成 |
| Bamg | 995-998 | 2D 各向异性网格 |
| HXT | 1011-1037 | 重参数化和网格生成 |

**典型的库集成模式**：
```cmake
if(ENABLE_ANN)
  find_library(ANN_LIB ANN PATH_SUFFIXES lib)
  find_path(ANN_INC "ANN.h" PATH_SUFFIXES src include ANN)
  if(ENABLE_SYSTEM_CONTRIB AND ANN_LIB AND ANN_INC)
    # 使用系统版本
    list(APPEND EXTERNAL_LIBRARIES ${ANN_LIB})
    list(APPEND EXTERNAL_INCLUDES ${ANN_INC})
    set_config_option(HAVE_ANN "ANN")
  elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/contrib/ANN)
    # 使用内置版本
    add_subdirectory(contrib/ANN)
    include_directories(contrib/ANN/include)
    set_config_option(HAVE_ANN "ANN[contrib]")
  endif()
endif()
```

---

### 10. 高级依赖配置（第 1054-1286 行）

**HDF5/MED/CGNS 文件格式**（第 1054-1097 行）：
```cmake
if(ENABLE_MED OR ENABLE_CGNS)
  find_package(HDF5)
  if(HDF5_FOUND)
    if(ENABLE_MED)
      find_library(MED_LIB medC HINTS ENV MED3HOME)
    endif()
    if(ENABLE_CGNS)
      find_library(CGNS_LIB cgns HINTS ENV CGNS_ROOT)
    endif()
  endif()
endif()
```

**PETSc 配置**（第 1112-1248 行）：
- 支持新旧两种 PETSc 安装方式
- 自动解析 `petscvariables` 文件
- 集成 SLEPc 特征值求解器

**MUMPS 直接求解器**（第 1250-1275 行）：
```cmake
if(ENABLE_MUMPS AND HAVE_BLAS AND HAVE_LAPACK)
  set(MUMPS_LIBS_REQUIRED smumps dmumps cmumps zmumps mumps_common pord)
  if(NOT ENABLE_MPI)
    list(APPEND MUMPS_LIBS_REQUIRED mpiseq)
  endif()
  find_all_libraries(MUMPS_LIBRARIES MUMPS_LIBS_REQUIRED "" "lib")
endif()
```

---

### 11. OpenCASCADE 配置（第 1292-1428 行）

**版本检测**（第 1303-1320 行）：
```cmake
find_path(OCC_INC "Standard_Version.hxx" HINTS ENV CASROOT
          PATH_SUFFIXES inc include opencascade)
if(OCC_INC)
  file(STRINGS ${OCC_INC}/Standard_Version.hxx
       OCC_MAJOR REGEX "#define OCC_VERSION_MAJOR.*")
  # 解析版本号...
endif()
```

**OCC 7.8+ 新库名**（第 1326-1348 行）：
```cmake
if(OCC_VERSION VERSION_GREATER_EQUAL "7.8.0")
  set(OCC_LIBS_REQUIRED
      TKDESTEP TKDEIGES TKXSBase  # DataExchange (新名称)
      TKOffset TKFeat TKFillet TKBool TKMesh TKHLR TKBO TKPrim TKShHealing
      TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKernel)
else()
  set(OCC_LIBS_REQUIRED
      TKSTEP TKSTEP209 TKSTEPAttr TKSTEPBase TKIGES TKXSBase  # 旧名称
      ...)
endif()
```

---

### 12. 配置文件生成（第 1648-1658 行）

```cmake
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/common/GmshConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/src/common/GmshConfig.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/common/GmshVersion.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/src/common/GmshVersion.h)
```

这些 `.h.in` 模板文件会被替换为实际的配置值。

---

### 13. 构建目标定义（第 1762-1877 行）

**静态库**（第 1762-1771 行）：
```cmake
if(ENABLE_BUILD_LIB)
  add_library(lib STATIC ${GMSH_SRC})
  set_target_properties(lib PROPERTIES OUTPUT_NAME gmsh)
endif()
```

**共享库**（第 1774-1806 行）：
```cmake
if(ENABLE_BUILD_SHARED OR ENABLE_BUILD_DYNAMIC)
  add_library(shared SHARED ${GMSH_SRC})
  set_target_properties(shared PROPERTIES OUTPUT_NAME gmsh
     VERSION ${GMSH_MAJOR_VERSION}.${GMSH_MINOR_VERSION}.${GMSH_PATCH_VERSION}
     SOVERSION ${GMSH_MAJOR_VERSION}.${GMSH_MINOR_VERSION})
  target_link_libraries(shared ${LINK_LIBRARIES})
endif()
```

**可执行文件**（第 1809-1829 行）：
```cmake
if(HAVE_FLTK)
  if(ENABLE_BUILD_DYNAMIC)
    add_executable(gmsh WIN32 src/common/Main.cpp)
    target_link_libraries(gmsh shared)  # 动态链接
  else()
    add_executable(gmsh WIN32 src/common/Main.cpp ${GMSH_SRC})  # 静态链接
  endif()
endif()
target_link_libraries(gmsh ${LINK_LIBRARIES})
```

**`target_link_libraries` 说明**（第 2303 行等）：
```cmake
target_link_libraries(${TEST} shared)
```
此命令将目标与共享库链接，用于测试程序。

---

### 14. 安装规则（第 2006-2073 行）

```cmake
install(TARGETS gmsh RUNTIME DESTINATION ${GMSH_BIN} OPTIONAL)

if(ENABLE_BUILD_LIB)
  install(TARGETS lib EXPORT gmshTargets ARCHIVE DESTINATION ${GMSH_LIB})
endif()

if(ENABLE_BUILD_SHARED OR ENABLE_BUILD_DYNAMIC)
  install(TARGETS shared EXPORT gmshTargets
    RUNTIME DESTINATION ${GMSH_LIB}
    ARCHIVE DESTINATION ${GMSH_LIB}
    LIBRARY DESTINATION ${GMSH_LIB})
endif()

# API 头文件
install(FILES ${GMSH_API} DESTINATION ${GMSH_INC})

# Python/Julia 绑定
install(FILES ${GMSH_PY} DESTINATION ${GMSH_LIB})
install(FILES ${GMSH_JL} DESTINATION ${GMSH_LIB})
```

---

### 15. 文档生成（第 2097-2143 行）

```cmake
find_program(MAKEINFO makeinfo)
if(MAKEINFO)
  add_custom_target(info DEPENDS ${TEX_DIR}/gmsh.info)
  add_custom_target(txt DEPENDS ${TEX_DIR}/gmsh.txt)
  add_custom_target(html DEPENDS ${TEX_DIR}/gmsh.html)
endif()

find_program(TEXI2PDF texi2pdf)
if(TEXI2PDF)
  add_custom_target(pdf DEPENDS ${TEX_DIR}/gmsh.pdf)
endif()
```

**自定义目标**：
- `make info` - 生成 info 格式文档
- `make html` - 生成 HTML 文档
- `make pdf` - 生成 PDF 文档
- `make doc` - 生成所有文档并打包

---

### 16. 打包配置（第 2180-2256 行）

```cmake
set(CPACK_PACKAGE_VENDOR "Christophe Geuzaine and Jean-Francois Remacle")
set(CPACK_PACKAGE_VERSION_MAJOR ${GMSH_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${GMSH_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION_PATCH ${GMSH_PATCH_VERSION})

# 根据平台选择打包器
if(APPLE AND ENABLE_OS_SPECIFIC_INSTALL AND HAVE_FLTK)
  set(CPACK_GENERATOR Bundle)  # macOS .app 包
elseif(WIN32)
  set(CPACK_GENERATOR ZIP)
else()
  set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
```

---

### 17. 测试配置（第 2258-2334 行）

```cmake
if(ENABLE_TESTS AND NOT DISABLE_GMSH_TESTS)
  include(CTest)

  # 收集测试文件
  file(GLOB_RECURSE ALLFILES
       tutorials/*.geo examples/*.geo benchmarks/?d/*.geo)

  # 过滤需要特定功能的测试
  filter_tests("${ALLFILES}" TESTFILES)

  # 添加 .geo 文件测试
  foreach(TESTFILE ${TESTFILES})
    add_test(${TEST} ./gmsh ${TEST} -3 -nopopup -o ./tmp.msh)
  endforeach()

  # 动态构建时添加 API 测试
  if(ENABLE_BUILD_DYNAMIC)
    file(GLOB_RECURSE ALLFILES tutorials/c++/*.cpp tutorials/c/*.c)
    foreach(TESTFILE ${TESTFILES})
      add_executable(${TEST} WIN32 ${TESTFILE})
      target_link_libraries(${TEST} shared)
      add_test(${TEST} ${TEST} -nopopup)
    endforeach()
  endif()
endif()
```

---

### 18. 构建摘要（第 2336-2351 行）

```cmake
message(STATUS "")
message(STATUS "Gmsh ${GMSH_VERSION} has been configured for ${GMSH_OS}")
message(STATUS "")
message(STATUS " * Build options:" ${GMSH_CONFIG_OPTIONS})
message(STATUS " * Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS " * C compiler: " ${CMAKE_C_COMPILER})
message(STATUS " * C++ compiler: " ${CMAKE_CXX_COMPILER})
message(STATUS " * Install prefix: " ${CMAKE_INSTALL_PREFIX})
message(STATUS "")
```

---

## 阅读建议

### 入门阅读顺序

1. **第 1-30 行**: 基础设置，了解项目要求
2. **第 29-112 行**: 所有可配置选项，快速了解功能
3. **第 592-618 行**: 源码目录结构
4. **第 1762-1829 行**: 构建目标，了解最终产物

### 进阶阅读

5. **第 440-582 行**: 依赖库查找逻辑
6. **第 1292-1428 行**: OpenCASCADE 集成（CAD 核心）
7. **第 760-1052 行**: 第三方库集成

### 实用搜索技巧

| 搜索关键字 | 用途 |
|-----------|------|
| `ENABLE_` | 查看所有可配置选项 |
| `add_subdirectory` | 了解模块结构 |
| `find_library` | 查看依赖库查找 |
| `find_package` | 查看 CMake 包查找 |
| `set_config_option` | 了解功能检测结果 |
| `add_executable` | 查看可执行目标 |
| `add_library` | 查看库目标 |
| `target_link_libraries` | 查看链接依赖 |

---

## 常用构建命令

```bash
# 创建构建目录
mkdir build && cd build

# 基本配置
cmake ..

# 最小化构建（无 GUI）
cmake -DENABLE_FLTK=0 ..

# 开发者构建（带共享库，推荐用于 API 开发）
cmake -DENABLE_BUILD_DYNAMIC=1 ..

# 静态库构建
cmake -DENABLE_BUILD_LIB=1 ..

# 禁用 OpenCASCADE
cmake -DENABLE_OCC=0 ..

# 交互式配置（查看和修改所有选项）
ccmake ..

# 构建
make -j$(nproc)

# 详细构建输出
make VERBOSE=1

# 运行测试
ctest

# 安装
make install

# 生成文档
make html
make pdf
```

---

## 关键变量参考

| 变量 | 说明 |
|------|------|
| `GMSH_SRC` | 所有源文件列表 |
| `GMSH_DIRS` | 所有源文件目录 |
| `EXTERNAL_LIBRARIES` | 外部链接库 |
| `EXTERNAL_INCLUDES` | 外部头文件目录 |
| `LINK_LIBRARIES` | 最终链接库列表 |
| `CONFIG_OPTIONS` | 启用的功能列表 |
| `GMSH_CONFIG_OPTIONS` | 配置选项字符串 |
