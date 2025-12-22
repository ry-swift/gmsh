# Day 63：第九周复习

## 学习目标
巩固第九周所学的后处理与可视化知识，完成综合练习项目，为下一阶段学习做准备。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 知识点回顾与梳理 |
| 10:00-11:00 | 1h | 综合练习：后处理可视化工具 |
| 11:00-12:00 | 1h | 代码阅读复习 |
| 14:00-15:00 | 1h | 整理学习笔记，完成周检查点 |

---

## 上午学习内容

### 1. 第九周知识图谱

```
后处理与可视化
│
├── Day 57: 后处理数据结构
│   ├── PViewData抽象接口
│   ├── PViewDataList（列表型）
│   ├── PViewDataGModel（模型型）
│   └── 数据类型：NodeData, ElementData, ElementNodeData
│
├── Day 58: PView视图系统
│   ├── PView = PViewData + PViewOptions
│   ├── 视图创建与管理
│   ├── 视图操作（合并、别名）
│   └── IO格式（POS, MSH, MED）
│
├── Day 59: 数据插值
│   ├── OctreePost空间查询
│   ├── searchScalar/Vector/Tensor
│   ├── 形函数插值
│   └── 高阶元素插值
│
├── Day 60: 可视化渲染
│   ├── VertexArray顶点缓存
│   ├── 等值线/等值面（Marching算法）
│   ├── 颜色映射（ColorTable）
│   └── OpenGL渲染管线
│
├── Day 61: 向量场可视化
│   ├── VectorType枚举
│   ├── 箭头绘制
│   ├── StreamLines流线
│   └── Particles粒子追踪
│
└── Day 62: 动画与时间步
    ├── 多时间步存储
    ├── stepData模板类
    ├── 时间合并Combine
    └── HarmonicToTime频域转时域
```

### 2. 核心概念回顾

**后处理数据层次结构**：

```
PView (视图)
├── PViewData (数据容器)
│   ├── PViewDataList
│   │   └── SP, VP, TP, SL, VL, ... (按类型存储)
│   │
│   └── PViewDataGModel
│       └── stepData<double>* (按时间步存储)
│           ├── _model (GModel引用)
│           └── _data[id] (按ID索引)
│
└── PViewOptions (显示选项)
    ├── 类型选项 (type, intervalsType, vectorType)
    ├── 范围选项 (min, max, rangeType)
    └── 渲染选项 (light, smooth, colorTable)
```

**数据访问统一接口**：

```cpp
// 五个核心查询维度
int step;    // 时间步
int ent;     // 实体（Entity）
int ele;     // 元素（Element）
int nod;     // 节点（Node）
int comp;    // 分量（Component）

// 统一的getValue接口
void PViewData::getValue(step, ent, ele, nod, comp, double &val);
```

**向量场可视化方法**：

```
方法          | 适用场景           | 优点              | 缺点
-------------|-------------------|------------------|------------------
箭头(Arrow)   | 局部流动方向       | 直观，量化        | 稀疏，遮挡
流线          | 稳态流场全局结构    | 展示整体流动      | 不适合非稳态
粒子追踪      | 非稳态流动，力场    | 支持物理模拟      | 计算量大
位移变形      | 结构位移           | 直观展示变形      | 可能放大/缩小
```

### 3. 模块关系图

```
                    ┌─────────────────┐
                    │     GModel      │
                    │  (几何+网格)     │
                    └────────┬────────┘
                             │ 数据来源
                             ▼
┌──────────────┐    ┌─────────────────┐    ┌──────────────┐
│ PViewDataList │◄──│    PViewData    │──►│PViewDataGModel│
│  (列表存储)   │    │   (抽象接口)    │    │  (模型绑定)   │
└──────────────┘    └────────┬────────┘    └──────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │      PView      │
                    │  (数据+选项)     │
                    └────────┬────────┘
                             │
              ┌──────────────┼──────────────┐
              ▼              ▼              ▼
      ┌────────────┐ ┌────────────┐ ┌────────────┐
      │ OctreePost │ │PViewOptions│ │VertexArray │
      │ (空间查询) │ │ (显示选项) │ │(渲染数据)  │
      └────────────┘ └────────────┘ └────────────┘
```

---

## 综合练习

### 4. 综合项目：后处理数据浏览器

**目标**：创建一个展示后处理模块各种功能的综合程序。

```cpp
#include "gmsh.h"
#include <cmath>
#include <vector>
#include <iostream>

// ========== 辅助函数 ==========

// 创建测试网格
void createMesh() {
    gmsh::model::occ::addBox(0, 0, 0, 2, 1, 0.5);
    gmsh::model::occ::synchronize();
    gmsh::option::setNumber("Mesh.CharacteristicLengthMax", 0.1);
    gmsh::model::mesh::generate(3);
}

// 获取节点信息
void getNodes(std::vector<std::size_t> &tags,
              std::vector<double> &coords) {
    std::vector<double> pc;
    gmsh::model::mesh::getNodes(tags, coords, pc);
}

// ========== 标量场生成 ==========

// 生成距离场
std::vector<double> createDistanceField(
    const std::vector<double> &coords,
    double cx, double cy, double cz)
{
    std::vector<double> data;
    size_t n = coords.size() / 3;
    for(size_t i = 0; i < n; i++) {
        double x = coords[3*i] - cx;
        double y = coords[3*i + 1] - cy;
        double z = coords[3*i + 2] - cz;
        data.push_back(std::sqrt(x*x + y*y + z*z));
    }
    return data;
}

// 生成应力场（模拟）
std::vector<double> createStressField(const std::vector<double> &coords) {
    std::vector<double> data;
    size_t n = coords.size() / 3;
    for(size_t i = 0; i < n; i++) {
        double x = coords[3*i];
        double y = coords[3*i + 1];
        // 简单的应力分布：梁的弯曲应力
        double stress = x * (0.5 - y);
        data.push_back(stress);
    }
    return data;
}

// ========== 向量场生成 ==========

// 创建流动场
std::vector<double> createFlowField(const std::vector<double> &coords) {
    std::vector<double> data;
    size_t n = coords.size() / 3;
    for(size_t i = 0; i < n; i++) {
        double x = coords[3*i] - 1.0;
        double y = coords[3*i + 1] - 0.5;
        // 涡旋场
        double r = std::sqrt(x*x + y*y) + 0.01;
        double vx = -y / r;
        double vy = x / r;
        double vz = 0;
        data.push_back(vx);
        data.push_back(vy);
        data.push_back(vz);
    }
    return data;
}

// 创建位移场
std::vector<double> createDisplacementField(
    const std::vector<double> &coords)
{
    std::vector<double> data;
    size_t n = coords.size() / 3;
    double L = 2.0;
    for(size_t i = 0; i < n; i++) {
        double x = coords[3*i];
        double y = coords[3*i + 1];
        // 悬臂梁挠度
        double ux = 0;
        double uy = -0.05 * x * x;  // 向下弯曲
        double uz = 0;
        data.push_back(ux);
        data.push_back(uy);
        data.push_back(uz);
    }
    return data;
}

// ========== 时间序列生成 ==========

void createTimeAnimation(int viewTag,
                         const std::vector<std::size_t> &nodeTags,
                         const std::vector<double> &coords,
                         int numSteps)
{
    for(int t = 0; t < numSteps; t++) {
        double time = t * 0.1;
        std::vector<double> data;

        size_t n = nodeTags.size();
        for(size_t i = 0; i < n; i++) {
            double x = coords[3*i];
            double y = coords[3*i + 1];
            // 波动
            double val = std::sin(2*M_PI*(x - time)) *
                        std::exp(-2*y*y);
            data.push_back(val);
        }

        gmsh::view::addHomogeneousModelData(
            viewTag, t, "PostDemo", "NodeData",
            nodeTags, data, time
        );
    }
}

// ========== 主程序 ==========

int main() {
    gmsh::initialize();
    gmsh::model::add("PostDemo");

    std::cout << "=== Gmsh后处理模块演示 ===" << std::endl;

    // 1. 创建网格
    std::cout << "1. 创建测试网格..." << std::endl;
    createMesh();

    std::vector<std::size_t> nodeTags;
    std::vector<double> coords;
    getNodes(nodeTags, coords);
    std::cout << "   节点数: " << nodeTags.size() << std::endl;

    // 2. 创建标量场视图
    std::cout << "2. 创建标量场视图..." << std::endl;

    int view1 = gmsh::view::add("Distance Field");
    auto distData = createDistanceField(coords, 1.0, 0.5, 0.25);
    gmsh::view::addHomogeneousModelData(view1, 0, "PostDemo", "NodeData",
                                        nodeTags, distData);
    gmsh::option::setNumber("View[0].IntervalsType", 2);  // Continuous

    int view2 = gmsh::view::add("Stress Field");
    auto stressData = createStressField(coords);
    gmsh::view::addHomogeneousModelData(view2, 0, "PostDemo", "NodeData",
                                        nodeTags, stressData);
    gmsh::option::setNumber("View[1].IntervalsType", 1);  // Iso
    gmsh::option::setNumber("View[1].NbIso", 15);

    // 3. 创建向量场视图
    std::cout << "3. 创建向量场视图..." << std::endl;

    int view3 = gmsh::view::add("Flow Field");
    auto flowData = createFlowField(coords);
    gmsh::view::addHomogeneousModelData(view3, 0, "PostDemo", "NodeData",
                                        nodeTags, flowData, 0.0, 3);
    gmsh::option::setNumber("View[2].VectorType", 4);  // Arrow3D
    gmsh::option::setNumber("View[2].ArrowSizeMax", 20);

    int view4 = gmsh::view::add("Displacement");
    auto dispData = createDisplacementField(coords);
    gmsh::view::addHomogeneousModelData(view4, 0, "PostDemo", "NodeData",
                                        nodeTags, dispData, 0.0, 3);
    gmsh::option::setNumber("View[3].VectorType", 5);  // Displacement
    gmsh::option::setNumber("View[3].DisplacementFactor", 3);

    // 4. 创建时间动画
    std::cout << "4. 创建时间序列动画..." << std::endl;

    int view5 = gmsh::view::add("Wave Animation");
    createTimeAnimation(view5, nodeTags, coords, 30);
    gmsh::option::setNumber("View[4].RangeType", 2);  // Custom
    gmsh::option::setNumber("View[4].CustomMin", -1);
    gmsh::option::setNumber("View[4].CustomMax", 1);
    gmsh::option::setNumber("View[4].ShowTime", 1);

    // 5. 默认只显示第一个视图
    for(int i = 1; i < 5; i++) {
        gmsh::option::setNumber("View[" + std::to_string(i) + "].Visible", 0);
    }

    std::cout << "\n=== 操作指南 ===" << std::endl;
    std::cout << "- 使用 Tools > Options > View 切换不同视图" << std::endl;
    std::cout << "- View 0: 距离场（连续色带）" << std::endl;
    std::cout << "- View 1: 应力场（等值线）" << std::endl;
    std::cout << "- View 2: 流动场（3D箭头）" << std::endl;
    std::cout << "- View 3: 位移场（变形显示）" << std::endl;
    std::cout << "- View 4: 波动动画（按空格播放）" << std::endl;

    // 6. 运行GUI
    gmsh::fltk::run();

    gmsh::finalize();
    return 0;
}
```

### 5. 练习任务

**任务1**：扩展综合项目

- 添加张量场（应力张量）的显示
- 实现等值面提取
- 添加StreamLines流线显示

**任务2**：数据探测器

```cpp
// 实现一个点击查询功能
// 1. 从用户输入获取坐标
// 2. 使用searchScalar查询值
// 3. 显示结果

void probeValue(int viewTag, double x, double y, double z) {
    double value;
    // 使用Probe插件或直接调用searchScalar
    gmsh::plugin::setNumber("Probe", "X", x);
    gmsh::plugin::setNumber("Probe", "Y", y);
    gmsh::plugin::setNumber("Probe", "Z", z);
    gmsh::plugin::setNumber("Probe", "View", viewTag);
    gmsh::plugin::run("Probe");
}
```

**任务3**：动画导出

```cpp
// 将动画导出为图像序列
void exportFrames(int viewTag, const std::string &prefix) {
    // 获取时间步数
    // 遍历每个时间步
    // 设置timeStep选项
    // 导出PNG
}
```

---

## 周检查点

### 理论知识

- [ ] 能解释PViewData、PViewDataList、PViewDataGModel的设计差异
- [ ] 理解后处理数据的Entity-Element-Node三层结构
- [ ] 能说明不同VectorType的适用场景
- [ ] 理解时间步数据的存储和访问方式
- [ ] 了解等值线/等值面的生成原理（Marching算法）
- [ ] 理解流线的数学定义和数值计算方法

### 实践技能

- [ ] 能使用API创建标量/向量/张量视图
- [ ] 能配置各种显示选项（颜色、间隔类型等）
- [ ] 能创建多时间步动画
- [ ] 能使用StreamLines等后处理插件
- [ ] 能导出可视化结果

### 代码阅读

- [ ] 已阅读PViewData.h的抽象接口
- [ ] 已阅读PViewDataList的数据组织
- [ ] 已阅读PViewDataGModel的stepData结构
- [ ] 已阅读PViewVertexArrays.cpp的渲染逻辑
- [ ] 已阅读至少一个后处理插件的实现

---

## 关键源码索引

| 主题 | 文件 | 核心函数/类 |
|------|------|------------|
| 数据基类 | PViewData.h | getValue(), searchScalar() |
| 列表数据 | PViewDataList.h | SP, VP, ST, VT, ... |
| 模型数据 | PViewDataGModel.h | stepData<>, DataType |
| 视图管理 | PView.h | PView, combine() |
| 显示选项 | PViewOptions.h | VectorType, IntervalsType |
| 空间查询 | OctreePost.h/cpp | searchScalar() |
| 渲染数据 | PViewVertexArrays.cpp | fillVertexArrays() |
| 颜色映射 | ColorTable.h/cpp | getColor() |
| 流线插件 | StreamLines.cpp | execute() |
| 时域转换 | HarmonicToTime.cpp | execute() |

---

## 第九周总结

### 核心收获

1. **数据结构体系**：理解了Gmsh后处理模块的两种数据存储方式
2. **视图系统**：掌握了PView的创建、管理和配置
3. **空间查询**：学会了使用OctreePost进行高效的数据查询
4. **渲染管线**：了解了从数据到OpenGL渲染的完整流程
5. **向量可视化**：掌握了箭头、流线等向量场可视化技术
6. **时间动画**：学会了创建和控制多时间步动画

### 关键洞察

- **抽象与实现分离**：PViewData提供统一接口，具体实现可选
- **灵活的数据组织**：支持连续和非连续数据的不同需求
- **高效的空间查询**：八叉树/KD树保证了大数据量下的性能
- **插件化设计**：后处理操作通过插件机制扩展
- **时间步独立存储**：便于大规模时序数据的管理

### 下周预告

第10周：求解器与数值计算
- Day 64：求解器模块概述
- Day 65：线性系统求解
- Day 66：特征值问题
- Day 67：有限元装配
- Day 68：边界条件处理
- Day 69：求解器后端（PETSc/MUMPS）
- Day 70：第十周复习

将深入Gmsh的求解器框架。

---

## 扩展阅读建议

### 必读
1. 完成本周所有练习
2. 用debugger追踪一次完整的后处理数据创建和显示流程

### 选读
1. VTK可视化管线设计
2. ParaView后处理技术
3. 科学可视化原理与方法

---

## 导航

- 上一天：[Day 62 - 动画与时间步](day-62.md)
- 下一天：[Day 64 - 求解器模块概述](day-64.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
