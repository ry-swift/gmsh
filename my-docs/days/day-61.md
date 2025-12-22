# Day 61：向量场可视化

## 学习目标
掌握Gmsh中向量场的可视化方法，理解箭头、流线、粒子追踪等可视化技术的实现原理和使用方式。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 向量场可视化概述与箭头表示 |
| 10:00-11:00 | 1h | 流线（StreamLines）算法 |
| 11:00-12:00 | 1h | 粒子追踪（Particles）与动画 |
| 14:00-15:00 | 1h | 实践练习与源码分析 |

---

## 上午学习内容

### 1. 向量场可视化概述

**向量数据类型**：
```
向量场数据包含3个分量：
- 速度场 v = (vx, vy, vz)
- 位移场 u = (ux, uy, uz)
- 力场 F = (Fx, Fy, Fz)
```

**PViewOptions中的向量类型**：

```cpp
// 文件位置：src/post/PViewOptions.h:20-26
enum VectorType {
    Segment = 1,      // 线段（简单快速）
    Arrow = 2,        // 2D箭头（常用）
    Pyramid = 3,      // 金字塔形（3D效果）
    Arrow3D = 4,      // 3D箭头（最真实）
    Displacement = 5  // 位移变形（网格变形显示）
};
```

**向量绘制流程**：

```
PViewData (向量数据)
    │
    ▼
fillVertexArrays()
    │
    ├── 计算向量方向和大小
    ├── 根据VectorType选择几何形状
    ├── 应用缩放因子 (arrowSizeMin/Max)
    └── 生成顶点数组
    │
    ▼
va_vectors (VertexArray)
    │
    ▼
OpenGL渲染
```

### 2. 箭头几何生成

**向量箭头的几何构造**：

```cpp
// 在PViewVertexArrays.cpp中的向量绘制
// 箭头由两部分组成：轴（线段）和头（三角形或金字塔）

void addArrow(double x, double y, double z,    // 起点
              double dx, double dy, double dz,  // 方向向量
              double scale)                     // 缩放
{
    // 1. 计算箭头终点
    double ex = x + dx * scale;
    double ey = y + dy * scale;
    double ez = z + dz * scale;

    // 2. 计算箭头头部
    double headLen = scale * 0.2;  // 头部长度
    double headRad = scale * 0.1;  // 头部半径

    // 3. 构造头部三角形（需要垂直方向）
    SVector3 dir(dx, dy, dz);
    dir.normalize();
    SVector3 perp = dir.perpendicular();
    // ...
}
```

**箭头大小计算**：

```cpp
// 箭头大小可以基于：
// 1. 固定大小：所有箭头相同
// 2. 按模长缩放：|v| -> arrow_size
// 3. 对数缩放：log(|v|) -> arrow_size

double getArrowSize(PViewOptions *opt, double norm) {
    double smin = opt->arrowSizeMin;
    double smax = opt->arrowSizeMax;
    double vmin = opt->tmpMin;
    double vmax = opt->tmpMax;

    // 线性插值
    if(vmax - vmin > 1e-10)
        return smin + (smax - smin) * (norm - vmin) / (vmax - vmin);
    return smin;
}
```

### 3. 流线（StreamLines）算法

**文件位置**：`src/plugin/StreamLines.cpp`

**流线定义**：
```
流线是向量场中处处与速度向量相切的曲线。
数学定义：dr/ds = v(r)，其中s是弧长参数
```

**数值积分方法**：

```cpp
// Runge-Kutta 4阶方法（RK4）
// 用于求解 dr/dt = v(r)

void RK4Step(double *pos, double dt, PViewData *data) {
    double k1[3], k2[3], k3[3], k4[3];
    double tmp[3];

    // k1 = v(pos)
    data->searchVector(pos[0], pos[1], pos[2], k1);

    // k2 = v(pos + dt/2 * k1)
    for(int i = 0; i < 3; i++) tmp[i] = pos[i] + 0.5 * dt * k1[i];
    data->searchVector(tmp[0], tmp[1], tmp[2], k2);

    // k3 = v(pos + dt/2 * k2)
    for(int i = 0; i < 3; i++) tmp[i] = pos[i] + 0.5 * dt * k2[i];
    data->searchVector(tmp[0], tmp[1], tmp[2], k3);

    // k4 = v(pos + dt * k3)
    for(int i = 0; i < 3; i++) tmp[i] = pos[i] + dt * k3[i];
    data->searchVector(tmp[0], tmp[1], tmp[2], k4);

    // 更新位置
    for(int i = 0; i < 3; i++)
        pos[i] += dt * (k1[i] + 2*k2[i] + 2*k3[i] + k4[i]) / 6.0;
}
```

**StreamLines插件参数**：

```cpp
// 关键选项
StringXNumber StreamLinesOptions[] = {
    {GMSH_FULLRC, "X0", ..., 0.},       // 种子点起始X
    {GMSH_FULLRC, "Y0", ..., 0.},       // 种子点起始Y
    {GMSH_FULLRC, "Z0", ..., 0.},       // 种子点起始Z
    {GMSH_FULLRC, "X1", ..., 1.},       // 种子平面终点X
    {GMSH_FULLRC, "Y1", ..., 0.},
    {GMSH_FULLRC, "Z1", ..., 0.},
    {GMSH_FULLRC, "X2", ..., 0.},
    {GMSH_FULLRC, "Y2", ..., 1.},
    {GMSH_FULLRC, "Z2", ..., 0.},
    {GMSH_FULLRC, "nPointsU", ..., 10}, // U方向种子点数
    {GMSH_FULLRC, "nPointsV", ..., 1},  // V方向种子点数
    {GMSH_FULLRC, "MaxIter", ..., 100}, // 最大迭代步数
    {GMSH_FULLRC, "DT", ..., 0.1},      // 时间步长
};
```

### 4. 粒子追踪（Particles）

**与StreamLines的区别**：
```
StreamLines：稳态流场，流线 = 迹线 = 路径线
Particles：  非稳态流场，考虑粒子质量和力场
```

**粒子动力学方程**：

```cpp
// 牛顿第二定律：F = ma
// 对于速度场：dv/dt = F/m
// 对于位置：  dx/dt = v

struct Particle {
    double pos[3];   // 位置
    double vel[3];   // 速度
    double mass;     // 质量
};

void updateParticle(Particle &p, double *force, double dt) {
    // 速度更新（Euler方法）
    for(int i = 0; i < 3; i++)
        p.vel[i] += (force[i] / p.mass) * dt;

    // 位置更新
    for(int i = 0; i < 3; i++)
        p.pos[i] += p.vel[i] * dt;
}
```

---

## 下午学习内容

### 5. 向量场可视化实践

**练习1：基本向量场显示**

```cpp
#include "gmsh.h"
#include <cmath>
#include <vector>

int main() {
    gmsh::initialize();
    gmsh::model::add("vector_field");

    // 创建简单网格
    gmsh::model::occ::addRectangle(0, 0, 0, 2, 2);
    gmsh::model::occ::synchronize();
    gmsh::option::setNumber("Mesh.CharacteristicLengthMax", 0.1);
    gmsh::model::mesh::generate(2);

    // 获取节点
    std::vector<std::size_t> nodeTags;
    std::vector<double> coords, pc;
    gmsh::model::mesh::getNodes(nodeTags, coords, pc);

    // 创建旋转向量场 v = (-y, x, 0)
    std::vector<double> vectorData;
    for(size_t i = 0; i < nodeTags.size(); i++) {
        double x = coords[3*i] - 1.0;     // 平移到中心
        double y = coords[3*i + 1] - 1.0;
        vectorData.push_back(-y);  // vx
        vectorData.push_back(x);   // vy
        vectorData.push_back(0);   // vz
    }

    // 创建向量视图
    int viewTag = gmsh::view::add("Rotation Field");
    gmsh::view::addHomogeneousModelData(
        viewTag, 0, "vector_field", "NodeData",
        nodeTags, vectorData, 0.0, 3  // 3分量
    );

    // 设置向量显示选项
    gmsh::option::setNumber("View[0].VectorType", 4);    // Arrow3D
    gmsh::option::setNumber("View[0].ArrowSizeMax", 30);
    gmsh::option::setNumber("View[0].GlyphLocation", 1); // COG

    gmsh::fltk::run();
    gmsh::finalize();
    return 0;
}
```

**练习2：使用StreamLines插件**

```cpp
#include "gmsh.h"
#include <cmath>
#include <vector>

int main() {
    gmsh::initialize();
    gmsh::model::add("streamlines");

    // 创建3D区域
    gmsh::model::occ::addBox(0, 0, 0, 2, 2, 1);
    gmsh::model::occ::synchronize();
    gmsh::option::setNumber("Mesh.CharacteristicLengthMax", 0.2);
    gmsh::model::mesh::generate(3);

    // 获取节点
    std::vector<std::size_t> nodeTags;
    std::vector<double> coords, pc;
    gmsh::model::mesh::getNodes(nodeTags, coords, pc);

    // 创建源汇流场
    // v = (x-1, y-1, 0) 从中心向外辐射
    std::vector<double> vectorData;
    for(size_t i = 0; i < nodeTags.size(); i++) {
        double x = coords[3*i] - 1.0;
        double y = coords[3*i + 1] - 1.0;
        double r = std::sqrt(x*x + y*y);
        if(r > 0.01) {
            vectorData.push_back(x/r);
            vectorData.push_back(y/r);
            vectorData.push_back(0);
        } else {
            vectorData.push_back(0);
            vectorData.push_back(0);
            vectorData.push_back(0);
        }
    }

    int viewTag = gmsh::view::add("Radial Field");
    gmsh::view::addHomogeneousModelData(
        viewTag, 0, "streamlines", "NodeData",
        nodeTags, vectorData, 0.0, 3
    );

    // 运行StreamLines插件
    gmsh::plugin::setNumber("StreamLines", "X0", 0.1);
    gmsh::plugin::setNumber("StreamLines", "Y0", 0.1);
    gmsh::plugin::setNumber("StreamLines", "Z0", 0.5);
    gmsh::plugin::setNumber("StreamLines", "X1", 0.1);
    gmsh::plugin::setNumber("StreamLines", "Y1", 1.9);
    gmsh::plugin::setNumber("StreamLines", "Z1", 0.5);
    gmsh::plugin::setNumber("StreamLines", "X2", 0.1);
    gmsh::plugin::setNumber("StreamLines", "Y2", 0.1);
    gmsh::plugin::setNumber("StreamLines", "Z2", 0.5);
    gmsh::plugin::setNumber("StreamLines", "nPointsU", 10);
    gmsh::plugin::setNumber("StreamLines", "nPointsV", 1);
    gmsh::plugin::setNumber("StreamLines", "MaxIter", 200);
    gmsh::plugin::setNumber("StreamLines", "DT", 0.05);
    gmsh::plugin::setNumber("StreamLines", "View", 0);
    gmsh::plugin::run("StreamLines");

    gmsh::fltk::run();
    gmsh::finalize();
    return 0;
}
```

**练习3：位移变形显示**

```cpp
#include "gmsh.h"
#include <cmath>
#include <vector>

int main() {
    gmsh::initialize();
    gmsh::model::add("displacement");

    // 创建悬臂梁
    gmsh::model::occ::addBox(0, 0, 0, 5, 1, 0.5);
    gmsh::model::occ::synchronize();
    gmsh::option::setNumber("Mesh.CharacteristicLengthMax", 0.3);
    gmsh::model::mesh::generate(3);

    std::vector<std::size_t> nodeTags;
    std::vector<double> coords, pc;
    gmsh::model::mesh::getNodes(nodeTags, coords, pc);

    // 模拟弯曲位移场
    // 悬臂梁挠度：w = P*x^2*(3L-x)/(6EI)
    // 简化为 w ~ x^2
    std::vector<double> dispData;
    double L = 5.0;
    for(size_t i = 0; i < nodeTags.size(); i++) {
        double x = coords[3*i];
        double y = coords[3*i + 1];
        double z = coords[3*i + 2];

        // 简化的弯曲位移
        double factor = 0.1;
        double ux = 0;
        double uy = 0;
        double uz = -factor * x * x / (L * L);  // 向下弯曲

        dispData.push_back(ux);
        dispData.push_back(uy);
        dispData.push_back(uz);
    }

    int viewTag = gmsh::view::add("Displacement");
    gmsh::view::addHomogeneousModelData(
        viewTag, 0, "displacement", "NodeData",
        nodeTags, dispData, 0.0, 3
    );

    // 设置为位移显示模式
    gmsh::option::setNumber("View[0].VectorType", 5);  // Displacement
    gmsh::option::setNumber("View[0].DisplacementFactor", 5); // 放大系数

    gmsh::fltk::run();
    gmsh::finalize();
    return 0;
}
```

### 6. 源码分析任务

**任务1：追踪向量绘制流程**

```cpp
// 在src/post/PViewVertexArrays.cpp中找到向量绘制代码
// 关键函数：addOutlineVector()

// 追踪调用链：
// PView::fillVertexArrays()
//   -> addOutlineVector()
//      -> addArrow() / addPyramid() / ...
//         -> VertexArray::add()
```

**任务2：分析StreamLines插件实现**

```cpp
// 在src/plugin/StreamLines.cpp中
// 找到主要执行函数execute()

// 关键步骤：
// 1. 读取种子点参数
// 2. 生成种子点网格
// 3. 对每个种子点进行积分
// 4. 使用searchVector查询速度
// 5. 累积轨迹点
// 6. 输出为线元素
```

---

## 检查点

### 理论理解

- [ ] 理解向量场可视化的不同方法（箭头、流线、粒子）
- [ ] 能解释VectorType枚举的各种类型
- [ ] 理解流线的数学定义和数值计算方法
- [ ] 区分流线、迹线、路径线的概念

### 代码阅读

- [ ] 已阅读PViewOptions.h中的VectorType定义
- [ ] 已阅读StreamLines.h/cpp的插件结构
- [ ] 已阅读PViewVertexArrays.cpp中的向量绘制代码
- [ ] 理解RK4积分在流线计算中的应用

### 实践技能

- [ ] 能创建和显示向量场数据
- [ ] 能使用StreamLines插件生成流线
- [ ] 能使用Displacement模式显示变形
- [ ] 能调整向量显示的各种参数

---

## 关键源码索引

| 主题 | 文件 | 重点内容 |
|------|------|----------|
| 向量类型枚举 | PViewOptions.h:20 | VectorType定义 |
| 向量绘制 | PViewVertexArrays.cpp | addOutlineVector() |
| 流线插件 | StreamLines.cpp | execute(), RK4积分 |
| 粒子插件 | Particles.cpp | 带质量的粒子追踪 |
| 空间查询 | PViewData.cpp | searchVector() |
| 顶点数组 | VertexArray.h/cpp | 顶点数据存储 |

---

## 扩展阅读

### 必读
1. Gmsh参考手册 - Post-processing options章节
2. `tutorials/t21.geo` - 后处理可视化示例

### 选读
1. 科学可视化基础 - 向量场可视化技术
2. VTK向量可视化文档

---

## 导航

- 上一天：[Day 60 - 可视化渲染](day-60.md)
- 下一天：[Day 62 - 动画与时间步](day-62.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
