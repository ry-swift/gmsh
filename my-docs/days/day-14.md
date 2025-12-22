# Day 14：第二周复习与API练习

**所属周次**：第2周 - GEO脚本进阶与API入门
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

巩固进阶概念，完成第一阶段

---

## 学习文件

- `tutorials/c++/t2.cpp`
- `tutorials/c++/t3.cpp`

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 学习t2.cpp的变换操作 |
| 09:45-10:30 | 45min | 学习t3.cpp的参数化 |
| 10:30-11:15 | 45min | 理解C++与GEO的对应关系 |
| 11:15-12:00 | 45min | 复习第2周核心概念 |
| 14:00-15:00 | 1h | 综合练习：参数化网格生成器 |
| 15:00-16:00 | 1h | 整理笔记，更新学习计划 |

---

## 上午任务（2小时）

- [ ] 学习`tutorials/c++/t2.cpp`和`t3.cpp`
- [ ] 练习C++ API的网格生成流程
- [ ] 理解C++与GEO脚本的等价对应关系

---

## 下午任务（2小时）

- [ ] **综合练习**：
  - 用C++创建一个参数化的几何模型
  - 应用不同的Field控制网格
  - 导出MSH文件并在GUI中可视化
- [ ] 更新学习笔记

---

## 练习作业

### 1. 【基础】C++变换操作

使用C++ API进行几何变换

```cpp
// transform.cpp - 几何变换
#include <gmsh.h>
#include <cmath>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("transform");

    double lc = 0.1;

    // 创建一个正方形
    gmsh::model::geo::addPoint(0, 0, 0, lc, 1);
    gmsh::model::geo::addPoint(0.5, 0, 0, lc, 2);
    gmsh::model::geo::addPoint(0.5, 0.5, 0, lc, 3);
    gmsh::model::geo::addPoint(0, 0.5, 0, lc, 4);

    gmsh::model::geo::addLine(1, 2, 1);
    gmsh::model::geo::addLine(2, 3, 2);
    gmsh::model::geo::addLine(3, 4, 3);
    gmsh::model::geo::addLine(4, 1, 4);

    gmsh::model::geo::addCurveLoop({1, 2, 3, 4}, 1);
    gmsh::model::geo::addPlaneSurface({1}, 1);

    // 复制并平移
    std::vector<std::pair<int, int>> out;
    gmsh::model::geo::copy({{2, 1}}, out);  // 复制Surface 1
    gmsh::model::geo::translate(out, 0.6, 0, 0);  // 平移

    // 复制并旋转
    gmsh::model::geo::copy({{2, 1}}, out);
    gmsh::model::geo::rotate(out, 0, 0, 0, 0, 0, 1, M_PI/4);  // 绕Z轴旋转45度

    gmsh::model::geo::synchronize();
    gmsh::model::mesh::generate(2);
    gmsh::write("transform.msh");

    // gmsh::fltk::run();
    gmsh::finalize();
    return 0;
}
```

### 2. 【进阶】C++网格尺寸控制

使用C++ API设置Field

```cpp
// field_control.cpp - 网格尺寸控制
#include <gmsh.h>
#include <cmath>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("field_control");

    // 使用OCC创建矩形
    gmsh::model::occ::addRectangle(0, 0, 0, 2, 2, 1);
    gmsh::model::occ::synchronize();

    // 创建距离场（到点的距离）
    gmsh::model::mesh::field::add("Distance", 1);
    gmsh::model::mesh::field::setNumbers(1, "PointsList", {1});  // 左下角点

    // 创建阈值场
    gmsh::model::mesh::field::add("Threshold", 2);
    gmsh::model::mesh::field::setNumber(2, "InField", 1);
    gmsh::model::mesh::field::setNumber(2, "SizeMin", 0.02);
    gmsh::model::mesh::field::setNumber(2, "SizeMax", 0.2);
    gmsh::model::mesh::field::setNumber(2, "DistMin", 0.1);
    gmsh::model::mesh::field::setNumber(2, "DistMax", 1.0);

    // 设置背景场
    gmsh::model::mesh::field::setAsBackgroundMesh(2);

    // 生成网格
    gmsh::model::mesh::generate(2);
    gmsh::write("field_control.msh");

    gmsh::finalize();
    return 0;
}
```

### 3. 【挑战】完整的参数化网格生成器

创建一个功能完整的参数化网格生成器

```cpp
// parametric_mesh_generator.cpp
// 参数化网格生成器：带孔的板
#include <gmsh.h>
#include <iostream>
#include <vector>
#include <cmath>

// 参数结构
struct Parameters {
    double plate_width = 4.0;
    double plate_height = 3.0;
    double hole_x = 2.0;
    double hole_y = 1.5;
    double hole_radius = 0.5;
    double mesh_size_coarse = 0.2;
    double mesh_size_fine = 0.03;
};

void generateMesh(const Parameters& params) {
    gmsh::model::add("parametric_plate");

    // 使用OCC内核
    // 创建矩形板
    int plate = gmsh::model::occ::addRectangle(
        0, 0, 0,
        params.plate_width, params.plate_height
    );

    // 创建圆孔
    int hole = gmsh::model::occ::addDisk(
        params.hole_x, params.hole_y, 0,
        params.hole_radius, params.hole_radius
    );

    // 布尔差集
    std::vector<std::pair<int, int>> ov;
    std::vector<std::vector<std::pair<int, int>>> ovv;
    gmsh::model::occ::cut({{2, plate}}, {{2, hole}}, ov, ovv);

    gmsh::model::occ::synchronize();

    // 获取孔的边界曲线
    std::vector<std::pair<int, int>> boundaries;
    gmsh::model::getBoundary(ov, boundaries, false, false, false);

    // 设置距离场
    std::vector<double> hole_curves;
    for (const auto& b : boundaries) {
        if (b.first == 1) {  // 曲线
            // 检查是否是圆弧（孔的边界）
            std::string type;
            gmsh::model::getType(b.first, b.second, type);
            if (type == "Circle" || type == "Ellipse") {
                hole_curves.push_back(b.second);
            }
        }
    }

    if (!hole_curves.empty()) {
        gmsh::model::mesh::field::add("Distance", 1);
        gmsh::model::mesh::field::setNumbers(1, "CurvesList", hole_curves);

        gmsh::model::mesh::field::add("Threshold", 2);
        gmsh::model::mesh::field::setNumber(2, "InField", 1);
        gmsh::model::mesh::field::setNumber(2, "SizeMin", params.mesh_size_fine);
        gmsh::model::mesh::field::setNumber(2, "SizeMax", params.mesh_size_coarse);
        gmsh::model::mesh::field::setNumber(2, "DistMin", params.hole_radius * 0.5);
        gmsh::model::mesh::field::setNumber(2, "DistMax", params.hole_radius * 3);

        gmsh::model::mesh::field::setAsBackgroundMesh(2);
    }

    // 设置物理组
    std::vector<int> outer_curves, hole_curves_int;
    for (const auto& b : boundaries) {
        if (b.first == 1) {
            std::string type;
            gmsh::model::getType(b.first, b.second, type);
            if (type == "Circle" || type == "Ellipse") {
                hole_curves_int.push_back(b.second);
            } else {
                outer_curves.push_back(b.second);
            }
        }
    }

    gmsh::model::addPhysicalGroup(1, outer_curves, -1, "outer_boundary");
    gmsh::model::addPhysicalGroup(1, hole_curves_int, -1, "hole_boundary");
    gmsh::model::addPhysicalGroup(2, {ov[0].second}, -1, "plate");

    // 生成网格
    gmsh::model::mesh::generate(2);
}

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);

    // 默认参数
    Parameters params;

    // 命令行参数解析（简化版）
    if (argc > 1) params.plate_width = std::stod(argv[1]);
    if (argc > 2) params.plate_height = std::stod(argv[2]);
    if (argc > 3) params.hole_radius = std::stod(argv[3]);

    // 生成网格
    generateMesh(params);

    // 保存
    gmsh::write("parametric_plate.msh");

    // 打印统计信息
    std::vector<int> elementTypes;
    std::vector<std::vector<std::size_t>> elementTags;
    std::vector<std::vector<std::size_t>> nodeTags;
    gmsh::model::mesh::getElements(elementTypes, elementTags, nodeTags, 2);

    std::size_t totalElements = 0;
    for (const auto& tags : elementTags) {
        totalElements += tags.size();
    }

    std::cout << "Mesh generated successfully!" << std::endl;
    std::cout << "Total 2D elements: " << totalElements << std::endl;

    // 可选：启动GUI
    // gmsh::fltk::run();

    gmsh::finalize();
    return 0;
}
```

---

## 第二周检查点总览

### GEO进阶
- [ ] 掌握Coherence、Split Curve、Compound
- [ ] 能使用各种Field控制网格
- [ ] 理解网格细化和各向异性网格
- [ ] 能创建边界层网格

### OpenCASCADE
- [ ] 掌握SetFactory("OpenCASCADE")
- [ ] 能使用布尔运算
- [ ] 能使用Fillet/Chamfer

### C++ API
- [ ] 能编写基本的C++ Gmsh程序
- [ ] 理解命名空间结构
- [ ] 能用C++设置Field

---

## 第一阶段完成里程碑

恭喜完成第一阶段（基础入门）！

**掌握的技能**：
1. GEO脚本编写（基础和进阶）
2. 结构化和非结构化网格生成
3. Field系统控制网格密度
4. 边界层网格
5. OpenCASCADE布尔运算
6. C++ API基础

**下一阶段预告**：
- 第二阶段（第3-6周）：源码阅读基础
- 将深入Gmsh源码，理解核心模块实现

---

## GEO vs C++ API 快速对照表

| GEO脚本 | C++ API |
|---------|---------|
| `Point(1) = {x,y,z,lc};` | `gmsh::model::geo::addPoint(x,y,z,lc,1);` |
| `Line(1) = {1,2};` | `gmsh::model::geo::addLine(1,2,1);` |
| `Curve Loop(1) = {1,2,3,4};` | `gmsh::model::geo::addCurveLoop({1,2,3,4},1);` |
| `Plane Surface(1) = {1};` | `gmsh::model::geo::addPlaneSurface({1},1);` |
| `SetFactory("OpenCASCADE");` | 使用 `gmsh::model::occ::*` |
| `Box(1) = {...};` | `gmsh::model::occ::addBox(...);` |
| `BooleanDifference` | `gmsh::model::occ::cut(...)` |
| `Mesh 2;` | `gmsh::model::mesh::generate(2);` |
| `Field[1] = Distance;` | `gmsh::model::mesh::field::add("Distance",1);` |
| `Background Field = 1;` | `gmsh::model::mesh::field::setAsBackgroundMesh(1);` |

---

## 产出

- 能用C++完成完整的网格生成流程
- 第一阶段学习完成

---

## 导航

- **上一天**：[Day 13 - C++ API入门](day-13.md)
- **下一天**：[Day 15 - 项目架构全览](day-15.md)（第二阶段开始）
- **返回目录**：[学习计划总览](../study.md)
