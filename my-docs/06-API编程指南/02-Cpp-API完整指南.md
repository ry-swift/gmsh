# C++ API完整指南

本章系统介绍Gmsh C++ API的完整使用方法。

## 环境配置

### CMake配置

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(GmshExample)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Gmsh
find_package(gmsh REQUIRED)

# 或者手动指定路径
# set(GMSH_DIR "/path/to/gmsh")
# include_directories(${GMSH_DIR}/include)
# link_directories(${GMSH_DIR}/lib)

add_executable(example main.cpp)
target_link_libraries(example gmsh)
```

### 编译选项

```bash
# 使用pkg-config
g++ -o example main.cpp `pkg-config --cflags --libs gmsh`

# 手动指定
g++ -o example main.cpp -I/usr/local/include -L/usr/local/lib -lgmsh

# 使用CMake
mkdir build && cd build
cmake ..
make
```

### 头文件

```cpp
#include <gmsh.h>

// 主要命名空间
// gmsh::               全局函数
// gmsh::model::        模型操作
// gmsh::model::geo::   内置几何内核
// gmsh::model::occ::   OpenCASCADE内核
// gmsh::model::mesh::  网格操作
// gmsh::view::         后处理视图
// gmsh::option::       选项设置
// gmsh::fltk::         GUI操作
```

## 基本结构

### 初始化与终止

```cpp
#include <gmsh.h>
#include <iostream>

int main(int argc, char **argv) {
    // 初始化
    gmsh::initialize(argc, argv);

    // 设置选项
    gmsh::option::setNumber("General.Verbosity", 5);

    // ... Gmsh操作 ...

    // 终止
    gmsh::finalize();
    return 0;
}
```

### RAII封装

```cpp
#include <gmsh.h>
#include <stdexcept>

class GmshSession {
public:
    GmshSession(int argc = 0, char** argv = nullptr) {
        gmsh::initialize(argc, argv);
    }

    ~GmshSession() {
        try {
            gmsh::finalize();
        } catch (...) {
            // 忽略析构时的异常
        }
    }

    // 禁止拷贝
    GmshSession(const GmshSession&) = delete;
    GmshSession& operator=(const GmshSession&) = delete;
};

int main(int argc, char **argv) {
    GmshSession session(argc, argv);

    gmsh::model::add("test");
    gmsh::model::occ::addBox(0, 0, 0, 1, 1, 1);
    gmsh::model::occ::synchronize();
    gmsh::model::mesh::generate(3);
    gmsh::write("test.msh");

    return 0;
}
```

## 几何建模

### 内置几何内核

```cpp
#include <gmsh.h>
#include <vector>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("geo_example");

    // 添加点
    // addPoint(x, y, z, meshSize=0, tag=-1) -> int
    int p1 = gmsh::model::geo::addPoint(0, 0, 0, 0.1);
    int p2 = gmsh::model::geo::addPoint(1, 0, 0, 0.1);
    int p3 = gmsh::model::geo::addPoint(1, 1, 0, 0.1);
    int p4 = gmsh::model::geo::addPoint(0, 1, 0, 0.1);

    // 添加直线
    // addLine(startTag, endTag, tag=-1) -> int
    int l1 = gmsh::model::geo::addLine(p1, p2);
    int l2 = gmsh::model::geo::addLine(p2, p3);
    int l3 = gmsh::model::geo::addLine(p3, p4);
    int l4 = gmsh::model::geo::addLine(p4, p1);

    // 添加曲线环
    // addCurveLoop(curveTags, tag=-1, reorient=false) -> int
    std::vector<int> curves = {l1, l2, l3, l4};
    int cl = gmsh::model::geo::addCurveLoop(curves);

    // 添加平面
    // addPlaneSurface(wireTags, tag=-1) -> int
    std::vector<int> wires = {cl};
    int s = gmsh::model::geo::addPlaneSurface(wires);

    // 同步
    gmsh::model::geo::synchronize();

    // 添加物理组
    std::vector<int> surfaces = {s};
    gmsh::model::addPhysicalGroup(2, surfaces, -1, "Surface");

    gmsh::model::mesh::generate(2);
    gmsh::write("geo_example.msh");

    gmsh::finalize();
    return 0;
}
```

### OpenCASCADE内核

```cpp
#include <gmsh.h>
#include <vector>
#include <utility>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("occ_example");

    // 添加基本形状
    // addBox(x, y, z, dx, dy, dz, tag=-1) -> int
    int box = gmsh::model::occ::addBox(0, 0, 0, 2, 2, 2);

    // addSphere(xc, yc, zc, radius, tag=-1, angle1=-M_PI/2, angle2=M_PI/2, angle3=2*M_PI) -> int
    int sphere = gmsh::model::occ::addSphere(1, 1, 1, 0.8);

    // addCylinder(x, y, z, dx, dy, dz, r, tag=-1, angle=2*M_PI) -> int
    int cylinder = gmsh::model::occ::addCylinder(0, 0, 0, 0, 0, 3, 0.5);

    // 布尔运算
    std::vector<std::pair<int, int>> objectDimTags = {{3, box}};
    std::vector<std::pair<int, int>> toolDimTags = {{3, sphere}};
    std::vector<std::pair<int, int>> outDimTags;
    std::vector<std::vector<std::pair<int, int>>> outDimTagsMap;

    // cut(objectDimTags, toolDimTags, outDimTags, outDimTagsMap,
    //     tag=-1, removeObject=true, removeTool=true)
    gmsh::model::occ::cut(objectDimTags, toolDimTags,
                          outDimTags, outDimTagsMap);

    // 同步
    gmsh::model::occ::synchronize();

    // 获取实体
    std::vector<std::pair<int, int>> entities;
    gmsh::model::getEntities(entities, 3);

    std::cout << "3D实体数量: " << entities.size() << std::endl;

    gmsh::model::mesh::generate(3);
    gmsh::write("occ_example.msh");

    gmsh::finalize();
    return 0;
}
```

### 布尔运算

```cpp
#include <gmsh.h>
#include <vector>
#include <utility>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("boolean");

    // 创建形状
    int box1 = gmsh::model::occ::addBox(0, 0, 0, 2, 2, 2);
    int box2 = gmsh::model::occ::addBox(1, 1, 1, 2, 2, 2);
    int sphere = gmsh::model::occ::addSphere(1.5, 1.5, 1.5, 1);

    std::vector<std::pair<int, int>> outDimTags;
    std::vector<std::vector<std::pair<int, int>>> outDimTagsMap;

    // 并集 (Fuse)
    // fuse(objectDimTags, toolDimTags, outDimTags, outDimTagsMap,
    //      tag=-1, removeObject=true, removeTool=true)
    gmsh::model::occ::fuse(
        {{3, box1}}, {{3, box2}},
        outDimTags, outDimTagsMap
    );

    // 差集 (Cut)
    gmsh::model::occ::cut(
        outDimTags, {{3, sphere}},
        outDimTags, outDimTagsMap
    );

    // 交集 (Intersect)
    // gmsh::model::occ::intersect(...)

    // 分片 (Fragment)
    // gmsh::model::occ::fragment(...)

    gmsh::model::occ::synchronize();
    gmsh::model::mesh::generate(3);
    gmsh::write("boolean.msh");

    gmsh::finalize();
    return 0;
}
```

## 网格操作

### 网格生成

```cpp
#include <gmsh.h>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("mesh");

    gmsh::model::occ::addBox(0, 0, 0, 1, 1, 1);
    gmsh::model::occ::synchronize();

    // 设置网格选项
    gmsh::option::setNumber("Mesh.MeshSizeMin", 0.05);
    gmsh::option::setNumber("Mesh.MeshSizeMax", 0.2);
    gmsh::option::setNumber("Mesh.Algorithm", 6);      // Frontal-Delaunay
    gmsh::option::setNumber("Mesh.Algorithm3D", 10);   // HXT

    // 生成网格
    // generate(dim=3)
    gmsh::model::mesh::generate(3);

    // 优化网格
    // optimize(method="", force=false, niter=1, dimTags={})
    gmsh::model::mesh::optimize("Netgen");

    gmsh::write("mesh.msh");
    gmsh::finalize();
    return 0;
}
```

### 获取网格数据

```cpp
#include <gmsh.h>
#include <vector>
#include <iostream>
#include <cmath>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("mesh_data");

    gmsh::model::occ::addBox(0, 0, 0, 1, 1, 1);
    gmsh::model::occ::synchronize();
    gmsh::model::mesh::generate(3);

    // 获取所有节点
    // getNodes(nodeTags, coord, parametricCoord,
    //          dim=-1, tag=-1, includeBoundary=false, returnParametricCoord=true)
    std::vector<std::size_t> nodeTags;
    std::vector<double> nodeCoords;
    std::vector<double> parametricCoords;

    gmsh::model::mesh::getNodes(nodeTags, nodeCoords, parametricCoords);

    std::cout << "节点数量: " << nodeTags.size() << std::endl;
    std::cout << "第一个节点坐标: ("
              << nodeCoords[0] << ", "
              << nodeCoords[1] << ", "
              << nodeCoords[2] << ")" << std::endl;

    // 获取所有元素
    // getElements(elementTypes, elementTags, nodeTags, dim=-1, tag=-1)
    std::vector<int> elementTypes;
    std::vector<std::vector<std::size_t>> elementTags;
    std::vector<std::vector<std::size_t>> elementNodeTags;

    gmsh::model::mesh::getElements(elementTypes, elementTags, elementNodeTags);

    for (size_t i = 0; i < elementTypes.size(); i++) {
        // 获取元素属性
        // getElementProperties(elementType, elementName, dim, order,
        //                      numNodes, localNodeCoord, numPrimaryNodes)
        std::string elementName;
        int dim, order, numNodes, numPrimaryNodes;
        std::vector<double> localNodeCoord;

        gmsh::model::mesh::getElementProperties(
            elementTypes[i], elementName, dim, order,
            numNodes, localNodeCoord, numPrimaryNodes
        );

        std::cout << "类型: " << elementName
                  << ", 维度: " << dim
                  << ", 阶数: " << order
                  << ", 节点数: " << numNodes
                  << ", 元素数: " << elementTags[i].size()
                  << std::endl;
    }

    gmsh::finalize();
    return 0;
}
```

### 结构化网格

```cpp
#include <gmsh.h>
#include <vector>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("transfinite");

    // 创建矩形
    gmsh::model::occ::addRectangle(0, 0, 0, 2, 1);
    gmsh::model::occ::synchronize();

    // 获取曲线
    std::vector<std::pair<int, int>> curves;
    gmsh::model::getEntities(curves, 1);

    // 设置曲线节点数
    // setTransfiniteCurve(tag, numNodes, meshType="Progression", coef=1.0)
    for (const auto& [dim, tag] : curves) {
        // 根据曲线长度设置不同节点数
        double mass;
        gmsh::model::occ::getMass(dim, tag, mass);

        if (mass > 1.5) {
            gmsh::model::mesh::setTransfiniteCurve(tag, 21);
        } else {
            gmsh::model::mesh::setTransfiniteCurve(tag, 11);
        }
    }

    // 获取曲面
    std::vector<std::pair<int, int>> surfaces;
    gmsh::model::getEntities(surfaces, 2);

    // 设置结构化曲面
    // setTransfiniteSurface(tag, arrangement="Left", cornerTags={})
    for (const auto& [dim, tag] : surfaces) {
        gmsh::model::mesh::setTransfiniteSurface(tag);
        // 重组为四边形
        // setRecombine(dim, tag, angle=45)
        gmsh::model::mesh::setRecombine(dim, tag);
    }

    gmsh::model::mesh::generate(2);
    gmsh::write("transfinite.msh");

    gmsh::finalize();
    return 0;
}
```

### 尺寸场

```cpp
#include <gmsh.h>
#include <vector>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("size_field");

    gmsh::model::occ::addBox(0, 0, 0, 1, 1, 1);
    gmsh::model::occ::synchronize();

    // 获取所有点
    std::vector<std::pair<int, int>> points;
    gmsh::model::getEntities(points, 0);

    std::vector<double> pointTags;
    for (const auto& [dim, tag] : points) {
        pointTags.push_back(tag);
    }

    // 创建距离场
    // field::add(fieldType, tag=-1) -> int
    int distField = gmsh::model::mesh::field::add("Distance");
    gmsh::model::mesh::field::setNumbers(distField, "PointsList", pointTags);

    // 创建阈值场
    int threshField = gmsh::model::mesh::field::add("Threshold");
    gmsh::model::mesh::field::setNumber(threshField, "InField", distField);
    gmsh::model::mesh::field::setNumber(threshField, "SizeMin", 0.02);
    gmsh::model::mesh::field::setNumber(threshField, "SizeMax", 0.2);
    gmsh::model::mesh::field::setNumber(threshField, "DistMin", 0.1);
    gmsh::model::mesh::field::setNumber(threshField, "DistMax", 0.5);

    // 设置为背景场
    // field::setAsBackgroundMesh(tag)
    gmsh::model::mesh::field::setAsBackgroundMesh(threshField);

    // 禁用默认尺寸计算
    gmsh::option::setNumber("Mesh.MeshSizeExtendFromBoundary", 0);
    gmsh::option::setNumber("Mesh.MeshSizeFromPoints", 0);
    gmsh::option::setNumber("Mesh.MeshSizeFromCurvature", 0);

    gmsh::model::mesh::generate(3);
    gmsh::write("size_field.msh");

    gmsh::finalize();
    return 0;
}
```

## 物理组

```cpp
#include <gmsh.h>
#include <vector>
#include <iostream>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("physical_groups");

    int box = gmsh::model::occ::addBox(0, 0, 0, 1, 1, 1);
    gmsh::model::occ::synchronize();

    // 获取边界
    std::vector<std::pair<int, int>> boundary;
    gmsh::model::getBoundary({{3, box}}, boundary, false, false, false);

    // 分类面
    std::vector<int> bottomFaces, topFaces, sideFaces;

    for (const auto& [dim, tag] : boundary) {
        // 获取质心
        double x, y, z;
        gmsh::model::occ::getCenterOfMass(dim, tag, x, y, z);

        if (z < 0.01) {
            bottomFaces.push_back(tag);
        } else if (z > 0.99) {
            topFaces.push_back(tag);
        } else {
            sideFaces.push_back(tag);
        }
    }

    // 添加物理组
    // addPhysicalGroup(dim, tags, tag=-1, name="") -> int
    gmsh::model::addPhysicalGroup(3, {box}, 1, "Volume");
    gmsh::model::addPhysicalGroup(2, bottomFaces, 10, "Bottom");
    gmsh::model::addPhysicalGroup(2, topFaces, 20, "Top");
    gmsh::model::addPhysicalGroup(2, sideFaces, 30, "Sides");

    // 获取物理组信息
    // getPhysicalGroups(dimTags, dim=-1)
    std::vector<std::pair<int, int>> groups;
    gmsh::model::getPhysicalGroups(groups);

    std::cout << "物理组列表:" << std::endl;
    for (const auto& [dim, tag] : groups) {
        std::string name;
        gmsh::model::getPhysicalName(dim, tag, name);

        std::vector<int> entities;
        gmsh::model::getEntitiesForPhysicalGroup(dim, tag, entities);

        std::cout << "  (" << dim << ", " << tag << ") '"
                  << name << "': " << entities.size() << "个实体"
                  << std::endl;
    }

    gmsh::model::mesh::generate(3);
    gmsh::write("physical_groups.msh");

    gmsh::finalize();
    return 0;
}
```

## 后处理视图

```cpp
#include <gmsh.h>
#include <vector>
#include <cmath>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);
    gmsh::model::add("view_example");

    gmsh::model::occ::addBox(0, 0, 0, 1, 1, 1);
    gmsh::model::occ::synchronize();
    gmsh::model::mesh::generate(3);

    // 获取节点
    std::vector<std::size_t> nodeTags;
    std::vector<double> nodeCoords;
    std::vector<double> parametricCoords;
    gmsh::model::mesh::getNodes(nodeTags, nodeCoords, parametricCoords);

    // 创建节点数据
    std::vector<std::vector<double>> data;
    for (size_t i = 0; i < nodeTags.size(); i++) {
        double x = nodeCoords[i * 3];
        double y = nodeCoords[i * 3 + 1];
        double z = nodeCoords[i * 3 + 2];

        // 温度场
        double T = 100 * std::exp(-(x*x + y*y + z*z));
        data.push_back({T});
    }

    // 创建视图
    // view::add(name, tag=-1) -> int
    int viewTag = gmsh::view::add("Temperature");

    // 添加模型数据
    // view::addModelData(tag, step, modelName, dataType, tags, data,
    //                    time=0, numComponents=1, partition=0)
    gmsh::view::addModelData(
        viewTag,
        0,                           // 步骤
        "view_example",              // 模型名称
        "NodeData",                  // 数据类型
        nodeTags,                    // 节点tags
        data                         // 数据
    );

    // 写入视图
    gmsh::view::write(viewTag, "temperature.pos");

    gmsh::finalize();
    return 0;
}
```

## 选项系统

```cpp
#include <gmsh.h>
#include <iostream>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);

    // 设置数值选项
    // option::setNumber(name, value)
    gmsh::option::setNumber("Mesh.MeshSizeMax", 0.5);
    gmsh::option::setNumber("General.Verbosity", 5);

    // 获取数值选项
    // option::getNumber(name, value)
    double sizeMax;
    gmsh::option::getNumber("Mesh.MeshSizeMax", sizeMax);
    std::cout << "MeshSizeMax: " << sizeMax << std::endl;

    // 设置字符串选项
    // option::setString(name, value)
    gmsh::option::setString("General.GraphicsFont", "Helvetica");

    // 获取字符串选项
    // option::getString(name, value)
    std::string font;
    gmsh::option::getString("General.GraphicsFont", font);
    std::cout << "Font: " << font << std::endl;

    // 设置颜色选项
    // option::setColor(name, r, g, b, a=255)
    gmsh::option::setColor("Mesh.SurfaceFaces", 255, 0, 0, 255);

    // 获取颜色选项
    // option::getColor(name, r, g, b, a)
    int r, g, b, a;
    gmsh::option::getColor("Mesh.SurfaceFaces", r, g, b, a);
    std::cout << "颜色: R=" << r << ", G=" << g
              << ", B=" << b << ", A=" << a << std::endl;

    gmsh::finalize();
    return 0;
}
```

## 文件操作

```cpp
#include <gmsh.h>

int main(int argc, char **argv) {
    gmsh::initialize(argc, argv);

    // 打开文件
    // open(fileName)
    gmsh::open("model.step");

    // 合并文件
    // merge(fileName)
    gmsh::merge("additional.msh");

    // 写入文件
    // write(fileName)
    gmsh::write("output.msh");
    gmsh::write("output.vtk");
    gmsh::write("output.stl");

    // 设置文件格式版本
    gmsh::option::setNumber("Mesh.MshFileVersion", 4.1);

    // 清除模型
    // clear()
    gmsh::clear();

    gmsh::finalize();
    return 0;
}
```

## 完整工作流示例

```cpp
#include <gmsh.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>

class GmshSession {
public:
    GmshSession(int argc, char** argv) {
        gmsh::initialize(argc, argv);
    }
    ~GmshSession() {
        gmsh::finalize();
    }
};

void createGeometry() {
    // 创建带孔的板
    int plate = gmsh::model::occ::addRectangle(0, 0, 0, 10, 5);
    int hole1 = gmsh::model::occ::addDisk(2.5, 2.5, 0, 0.8, 0.8);
    int hole2 = gmsh::model::occ::addDisk(7.5, 2.5, 0, 0.8, 0.8);

    std::vector<std::pair<int, int>> outDimTags;
    std::vector<std::vector<std::pair<int, int>>> outDimTagsMap;

    gmsh::model::occ::cut(
        {{2, plate}},
        {{2, hole1}, {2, hole2}},
        outDimTags, outDimTagsMap
    );

    gmsh::model::occ::synchronize();
}

void setupPhysicalGroups() {
    std::vector<std::pair<int, int>> surfaces;
    gmsh::model::getEntities(surfaces, 2);

    std::vector<int> surfaceTags;
    for (const auto& [dim, tag] : surfaces) {
        surfaceTags.push_back(tag);
    }

    gmsh::model::addPhysicalGroup(2, surfaceTags, -1, "Plate");

    // 获取边界曲线
    std::vector<std::pair<int, int>> boundary;
    gmsh::model::getBoundary(surfaces, boundary, false, false, false);

    std::vector<int> leftCurves, rightCurves, holeCurves;

    for (const auto& [dim, tag] : boundary) {
        double x, y, z;
        gmsh::model::occ::getCenterOfMass(dim, tag, x, y, z);

        if (std::abs(x) < 0.1) {
            leftCurves.push_back(tag);
        } else if (std::abs(x - 10) < 0.1) {
            rightCurves.push_back(tag);
        } else {
            holeCurves.push_back(tag);
        }
    }

    if (!leftCurves.empty())
        gmsh::model::addPhysicalGroup(1, leftCurves, -1, "Left");
    if (!rightCurves.empty())
        gmsh::model::addPhysicalGroup(1, rightCurves, -1, "Right");
    if (!holeCurves.empty())
        gmsh::model::addPhysicalGroup(1, holeCurves, -1, "Holes");
}

void setupMeshSize() {
    gmsh::option::setNumber("Mesh.MeshSizeMin", 0.1);
    gmsh::option::setNumber("Mesh.MeshSizeMax", 0.5);

    // 获取孔的曲线
    std::vector<std::pair<int, int>> surfaces;
    gmsh::model::getEntities(surfaces, 2);

    std::vector<std::pair<int, int>> boundary;
    gmsh::model::getBoundary(surfaces, boundary, false, false, false);

    std::vector<double> holeCurves;
    for (const auto& [dim, tag] : boundary) {
        double x, y, z;
        gmsh::model::occ::getCenterOfMass(dim, tag, x, y, z);

        if ((x > 1 && x < 4 && y > 1 && y < 4) ||
            (x > 6 && x < 9 && y > 1 && y < 4)) {
            holeCurves.push_back(tag);
        }
    }

    if (!holeCurves.empty()) {
        int dist = gmsh::model::mesh::field::add("Distance");
        gmsh::model::mesh::field::setNumbers(dist, "CurvesList", holeCurves);

        int thresh = gmsh::model::mesh::field::add("Threshold");
        gmsh::model::mesh::field::setNumber(thresh, "InField", dist);
        gmsh::model::mesh::field::setNumber(thresh, "SizeMin", 0.05);
        gmsh::model::mesh::field::setNumber(thresh, "SizeMax", 0.5);
        gmsh::model::mesh::field::setNumber(thresh, "DistMin", 0.1);
        gmsh::model::mesh::field::setNumber(thresh, "DistMax", 1.0);

        gmsh::model::mesh::field::setAsBackgroundMesh(thresh);
    }
}

void generateMesh() {
    gmsh::option::setNumber("Mesh.Algorithm", 6);
    gmsh::model::mesh::generate(2);
}

void analyzeMesh() {
    std::vector<std::size_t> nodeTags;
    std::vector<double> coords;
    std::vector<double> parametricCoords;
    gmsh::model::mesh::getNodes(nodeTags, coords, parametricCoords);

    std::cout << "\n网格统计:" << std::endl;
    std::cout << "  节点数: " << nodeTags.size() << std::endl;

    std::vector<int> elementTypes;
    std::vector<std::vector<std::size_t>> elementTags;
    std::vector<std::vector<std::size_t>> elementNodeTags;
    gmsh::model::mesh::getElements(elementTypes, elementTags, elementNodeTags);

    size_t totalElems = 0;
    for (size_t i = 0; i < elementTypes.size(); i++) {
        std::string name;
        int dim, order, numNodes, numPrimaryNodes;
        std::vector<double> localNodeCoord;

        gmsh::model::mesh::getElementProperties(
            elementTypes[i], name, dim, order,
            numNodes, localNodeCoord, numPrimaryNodes
        );

        std::cout << "  " << name << ": " << elementTags[i].size() << "个" << std::endl;
        totalElems += elementTags[i].size();
    }
    std::cout << "  总元素数: " << totalElems << std::endl;

    // 网格质量
    std::vector<double> qualities;
    gmsh::model::mesh::getElementQualities({}, qualities, "minSJ");

    if (!qualities.empty()) {
        double minQ = *std::min_element(qualities.begin(), qualities.end());
        double maxQ = *std::max_element(qualities.begin(), qualities.end());
        double avgQ = 0;
        for (double q : qualities) avgQ += q;
        avgQ /= qualities.size();

        std::cout << "\n网格质量 (minSJ):" << std::endl;
        std::cout << "  最小: " << minQ << std::endl;
        std::cout << "  最大: " << maxQ << std::endl;
        std::cout << "  平均: " << avgQ << std::endl;
    }
}

int main(int argc, char **argv) {
    GmshSession session(argc, argv);
    gmsh::model::add("workflow_example");

    std::cout << "1. 创建几何..." << std::endl;
    createGeometry();

    std::cout << "2. 设置物理组..." << std::endl;
    setupPhysicalGroups();

    std::cout << "3. 设置网格尺寸..." << std::endl;
    setupMeshSize();

    std::cout << "4. 生成网格..." << std::endl;
    generateMesh();

    std::cout << "5. 分析网格..." << std::endl;
    analyzeMesh();

    std::cout << "\n6. 保存结果..." << std::endl;
    gmsh::write("workflow_result.msh");
    gmsh::write("workflow_result.vtk");

    // 检查是否显示GUI
    bool runGui = true;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "-nopopup") {
            runGui = false;
            break;
        }
    }

    if (runGui) {
        gmsh::fltk::run();
    }

    std::cout << "\n完成!" << std::endl;
    return 0;
}
```

## C++ API特点

### 与Python API的对比

| 特性 | Python API | C++ API |
|------|-----------|---------|
| 返回值 | 直接返回 | 通过引用参数返回 |
| 数组类型 | list/tuple | std::vector |
| dimTags | list of tuples | vector of pair |
| 字符串 | str | std::string |
| 异常处理 | try/except | try/catch |

### 类型映射

```cpp
// Python: [(dim, tag), ...]
// C++:
std::vector<std::pair<int, int>> dimTags;

// Python: [x1, y1, z1, x2, y2, z2, ...]
// C++:
std::vector<double> coords;

// Python: nodeTags, coords, parametricCoords = gmsh.model.mesh.getNodes()
// C++:
std::vector<std::size_t> nodeTags;
std::vector<double> coords;
std::vector<double> parametricCoords;
gmsh::model::mesh::getNodes(nodeTags, coords, parametricCoords);
```

### 错误处理

```cpp
#include <gmsh.h>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    try {
        gmsh::initialize(argc, argv);

        // ... Gmsh操作 ...

        gmsh::finalize();
    }
    catch (const std::exception& e) {
        std::cerr << "Gmsh错误: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "未知错误" << std::endl;
        return 1;
    }

    return 0;
}
```

## 下一步

- [03-常用选项参考](./03-常用选项参考.md) - 了解Gmsh选项系统
