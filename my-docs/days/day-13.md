# Day 13：C++ API入门

**所属周次**：第2周 - GEO脚本进阶与API入门
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

开始程序化使用Gmsh（C++重点）

---

## 学习文件

- `tutorials/c++/t1.cpp`
- `api/gmsh.h`

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读api/gmsh.h，了解API结构 |
| 09:45-10:30 | 45min | 学习C++ API基本框架 |
| 10:30-11:15 | 45min | 学习CMake链接Gmsh库 |
| 11:15-12:00 | 45min | 编译并运行t1.cpp |
| 14:00-14:45 | 45min | 对比t1.cpp和t1.geo |
| 14:45-15:30 | 45min | 学习API命名空间 |
| 15:30-16:00 | 30min | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 阅读API头文件`api/gmsh.h`，了解API结构
- [ ] 学习C++ API基本结构：

```cpp
#include "gmsh.h"
int main() {
    gmsh::initialize();
    // ... 操作 ...
    gmsh::finalize();
}
```

- [ ] 学习CMake链接Gmsh库的方法

---

## 下午任务（2小时）

- [ ] 对比t1.cpp和t1.geo的差异
- [ ] 学习API命名空间：
  - gmsh::model - 模型操作
  - gmsh::model::geo - 几何操作
  - gmsh::model::mesh - 网格操作
  - gmsh::model::occ - OpenCASCADE操作
- [ ] 编译并调试t1.cpp

---

## 练习作业

### 1. 【基础】最小C++程序

编译运行一个最小的Gmsh C++程序

```cpp
// minimal.cpp - 最小Gmsh程序
#include <gmsh.h>

int main(int argc, char **argv) {
    // 初始化Gmsh
    gmsh::initialize(argc, argv);

    // 创建新模型
    gmsh::model::add("minimal");

    // 显示版本信息
    std::string version;
    gmsh::version(version);
    std::cout << "Gmsh version: " << version << std::endl;

    // 启动GUI（可选）
    // gmsh::fltk::run();

    // 清理
    gmsh::finalize();
    return 0;
}
```

**编译命令**：

```bash
# 假设Gmsh安装在 /usr/local 或 build目录
g++ -std=c++14 minimal.cpp -o minimal \
    -I/path/to/gmsh/include \
    -L/path/to/gmsh/lib \
    -lgmsh
```

### 2. 【进阶】C++创建正方形网格

用C++ API创建一个正方形并生成网格

```cpp
// square.cpp - C++创建正方形
#include <gmsh.h>
#include <vector>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("square");

    double lc = 0.1;  // 网格尺寸

    // 创建点
    int p1 = gmsh::model::geo::addPoint(0, 0, 0, lc);
    int p2 = gmsh::model::geo::addPoint(1, 0, 0, lc);
    int p3 = gmsh::model::geo::addPoint(1, 1, 0, lc);
    int p4 = gmsh::model::geo::addPoint(0, 1, 0, lc);

    // 创建线
    int l1 = gmsh::model::geo::addLine(p1, p2);
    int l2 = gmsh::model::geo::addLine(p2, p3);
    int l3 = gmsh::model::geo::addLine(p3, p4);
    int l4 = gmsh::model::geo::addLine(p4, p1);

    // 创建曲线环
    int cl = gmsh::model::geo::addCurveLoop({l1, l2, l3, l4});

    // 创建平面
    int s = gmsh::model::geo::addPlaneSurface({cl});

    // 同步几何
    gmsh::model::geo::synchronize();

    // 生成2D网格
    gmsh::model::mesh::generate(2);

    // 保存网格
    gmsh::write("square.msh");

    // 可选：启动GUI查看
    // gmsh::fltk::run();

    gmsh::finalize();
    return 0;
}
```

### 3. 【挑战】C++创建参数化几何

创建一个参数化的带孔圆盘

```cpp
// disk_with_hole.cpp - 参数化带孔圆盘
#include <gmsh.h>
#include <cmath>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("disk_with_hole");

    // 参数
    double outer_radius = 1.0;
    double inner_radius = 0.3;
    double lc_outer = 0.1;
    double lc_inner = 0.05;

    // 使用OpenCASCADE内核
    // 外圆盘
    int disk = gmsh::model::occ::addDisk(0, 0, 0, outer_radius, outer_radius);

    // 内圆（孔）
    int hole = gmsh::model::occ::addDisk(0, 0, 0, inner_radius, inner_radius);

    // 布尔差集
    std::vector<std::pair<int, int>> ov;
    std::vector<std::vector<std::pair<int, int>>> ovv;
    gmsh::model::occ::cut({{2, disk}}, {{2, hole}}, ov, ovv);

    // 同步
    gmsh::model::occ::synchronize();

    // 设置网格尺寸
    gmsh::option::setNumber("Mesh.CharacteristicLengthMax", lc_outer);

    // 获取内边界曲线，设置更细的网格
    std::vector<std::pair<int, int>> curves;
    gmsh::model::getBoundary({{2, ov[0].second}}, curves);

    // 为每条曲线设置网格尺寸（简化处理）
    for (auto &c : curves) {
        std::vector<std::pair<int, int>> points;
        gmsh::model::getBoundary({c}, points);
        for (auto &p : points) {
            gmsh::model::mesh::setSize({p}, lc_inner);
        }
    }

    // 生成网格
    gmsh::model::mesh::generate(2);

    // 保存
    gmsh::write("disk_with_hole.msh");

    gmsh::finalize();
    return 0;
}
```

---

## 今日检查点

- [ ] 能编译运行基本的C++ Gmsh程序
- [ ] 理解initialize/finalize的作用
- [ ] 理解geo和occ命名空间的区别

---

## 核心概念

### C++ API 基本框架

```cpp
#include <gmsh.h>

int main(int argc, char **argv) {
    // 1. 初始化
    gmsh::initialize(argc, argv);

    // 2. 创建模型
    gmsh::model::add("model_name");

    // 3. 创建几何
    // ... gmsh::model::geo::* 或 gmsh::model::occ::*

    // 4. 同步几何到模型
    gmsh::model::geo::synchronize();  // 或 occ::synchronize()

    // 5. 生成网格
    gmsh::model::mesh::generate(dim);  // dim = 1, 2, 或 3

    // 6. 保存或显示
    gmsh::write("output.msh");
    // gmsh::fltk::run();  // GUI

    // 7. 清理
    gmsh::finalize();
    return 0;
}
```

### API命名空间结构

```
gmsh::
├── initialize()              # 初始化
├── finalize()                # 清理
├── model::                   # 模型操作
│   ├── add()                 # 添加模型
│   ├── remove()              # 删除模型
│   ├── geo::                 # 内置几何内核
│   │   ├── addPoint()
│   │   ├── addLine()
│   │   ├── addCircleArc()
│   │   ├── addCurveLoop()
│   │   ├── addPlaneSurface()
│   │   └── synchronize()
│   ├── occ::                 # OpenCASCADE内核
│   │   ├── addBox()
│   │   ├── addSphere()
│   │   ├── addCylinder()
│   │   ├── cut()             # 布尔差集
│   │   ├── fuse()            # 布尔并集
│   │   └── synchronize()
│   └── mesh::                # 网格操作
│       ├── generate()
│       ├── refine()
│       └── setSize()
├── option::                  # 选项设置
│   ├── setNumber()
│   └── setString()
├── write()                   # 保存文件
└── fltk::                    # GUI
    └── run()
```

### CMakeLists.txt 示例

```cmake
cmake_minimum_required(VERSION 3.10)
project(GmshExample)

set(CMAKE_CXX_STANDARD 14)

# 查找Gmsh
find_package(gmsh REQUIRED)

# 添加可执行文件
add_executable(example example.cpp)

# 链接Gmsh库
target_link_libraries(example gmsh)
target_include_directories(example PRIVATE ${GMSH_INCLUDE_DIRS})
```

### 编译选项

```bash
# 方式1：直接编译
g++ -std=c++14 example.cpp -o example -lgmsh

# 方式2：指定路径
g++ -std=c++14 example.cpp -o example \
    -I/path/to/gmsh/include \
    -L/path/to/gmsh/lib \
    -lgmsh \
    -Wl,-rpath,/path/to/gmsh/lib

# 方式3：使用CMake
mkdir build && cd build
cmake ..
make
```

---

## geo vs occ API对比

| 操作 | geo API | occ API |
|------|---------|---------|
| 创建点 | addPoint() | addPoint() |
| 创建线 | addLine() | addLine() |
| 创建圆 | addCircleArc() | addCircle() |
| 创建矩形 | - | addRectangle() |
| 创建立方体 | - | addBox() |
| 创建球体 | - | addSphere() |
| 布尔运算 | 有限支持 | cut/fuse/intersect |
| 同步 | synchronize() | synchronize() |

---

## 产出

- 能用C++调用Gmsh API

---

## 导航

- **上一天**：[Day 12 - OpenCASCADE几何内核](day-12.md)
- **下一天**：[Day 14 - 第二周复习与API练习](day-14.md)
- **返回目录**：[学习计划总览](../study.md)
