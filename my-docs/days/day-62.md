# Day 62：动画与时间步

## 学习目标
掌握Gmsh中多时间步数据的处理和动画显示，理解时间序列数据的存储结构和播放控制机制。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 时间步数据结构与存储 |
| 10:00-11:00 | 1h | 动画控制与时间插值 |
| 11:00-12:00 | 1h | 谐波分析与时间域转换 |
| 14:00-15:00 | 1h | 实践练习与总结 |

---

## 上午学习内容

### 1. 时间步数据结构

**多时间步存储方式**：

```cpp
// PViewDataList中的时间步存储
class PViewDataList {
public:
    int NbTimeStep;                    // 时间步数量
    std::vector<double> Time;          // 时间值数组
    std::vector<double> TimeStepMin;   // 每步最小值
    std::vector<double> TimeStepMax;   // 每步最大值

    // 数据布局（以标量三角形ST为例）：
    // 坐标: x1,y1,z1, x2,y2,z2, x3,y3,z3 (9个值)
    // 值:   v1_t0,v2_t0,v3_t0, v1_t1,v2_t1,v3_t1, ... (3*NbTimeStep个值)
    std::vector<double> ST;
};

// PViewDataGModel中的时间步存储
class PViewDataGModel {
private:
    // 每个时间步独立存储
    std::vector<stepData<double>*> _steps;
};

template<class Real>
class stepData {
private:
    double _time;                      // 此时间步的时间值
    double _min, _max;                 // 此时间步的值范围
    std::vector<Real*> *_data;         // 此时间步的数据
};
```

**时间步访问接口**：

```cpp
// PViewData基类接口
virtual int getNumTimeSteps() = 0;     // 获取时间步数量
virtual double getTime(int step);       // 获取指定步的时间值
virtual double getMin(int step = -1);   // 获取最值（-1表示全局）
virtual double getMax(int step = -1);

// getValue需要指定时间步
virtual void getValue(int step, int ent, int ele, int nod, int comp, double &val);
```

### 2. 动画控制机制

**PViewOptions中的时间控制**：

```cpp
// 文件位置：src/post/PViewOptions.h
class PViewOptions {
public:
    int timeStep;           // 当前显示的时间步索引
    double currentTime;     // 当前时间值（用于插值）
    // ...
};
```

**GUI动画控制**（在FLTK界面中）：

```
动画控制按钮：
[<<]  - 跳转到第一步
[<]   - 上一步
[>]   - 下一步
[>>]  - 跳转到最后一步
[▶]   - 播放/暂停动画
[⟳]   - 循环播放

时间滑块：可直接拖动选择时间步
```

**动画播放逻辑**：

```cpp
// 伪代码：动画播放循环
void playAnimation() {
    for(int step = 0; step < numTimeSteps; step++) {
        // 设置当前时间步
        for(auto view : PView::list) {
            view->getOptions()->timeStep = step;
            view->setChanged(true);
        }

        // 重绘
        redraw();

        // 等待（控制帧率）
        sleep(animationDelay);
    }
}
```

### 3. 时间步合并与分离

**Combine Time操作**：

```cpp
// 将多个视图的时间步合并到一个视图
// 文件位置：src/post/PView.cpp

static void PView::combine(bool time, int how, bool remove, bool copyOptions) {
    if(time) {
        // 时间合并：不同视图 -> 同一视图的不同时间步
        // 例如：view_t0, view_t1, view_t2 -> view[t0, t1, t2]
    }
    else {
        // 空间合并：合并相同时间步的不同区域数据
    }
}

// API调用
gmsh::view::combine("steps", "all");  // 按时间步合并
gmsh::view::combine("space", "all"); // 按空间合并
```

**数据合并示例**：

```cpp
// 将多个单时间步视图合并为多时间步视图
#include "gmsh.h"
#include <vector>

int main() {
    gmsh::initialize();
    gmsh::model::add("combine_demo");

    // 创建网格
    gmsh::model::occ::addDisk(0, 0, 0, 1, 1);
    gmsh::model::occ::synchronize();
    gmsh::model::mesh::generate(2);

    std::vector<std::size_t> nodeTags;
    std::vector<double> coords, pc;
    gmsh::model::mesh::getNodes(nodeTags, coords, pc);

    // 创建多个时间步的视图（每个独立）
    for(int t = 0; t < 10; t++) {
        std::vector<double> data;
        double time = t * 0.1;

        for(size_t i = 0; i < nodeTags.size(); i++) {
            double x = coords[3*i];
            double y = coords[3*i + 1];
            // 时变数据：sin波传播
            data.push_back(std::sin(2*M_PI*(x - time)));
        }

        int view = gmsh::view::add("Step " + std::to_string(t));
        gmsh::view::addHomogeneousModelData(view, 0, "combine_demo",
                                            "NodeData", nodeTags, data, time);
    }

    // 合并所有视图的时间步
    gmsh::view::combine("steps", "all");

    gmsh::fltk::run();
    gmsh::finalize();
    return 0;
}
```

### 4. HarmonicToTime插件

**用途**：将频域数据转换为时域动画

**文件位置**：`src/plugin/HarmonicToTime.cpp`

**原理**：

```
频域数据：复数值 a + bi（或幅值+相位）
时域数据：a*cos(ωt) - b*sin(ωt) = |c|*cos(ωt + φ)

插件参数：
- NumSteps：生成的时间步数量
- Start：起始相位（默认0）
- End：终止相位（默认2π）
- RealPart：实部视图索引
- ImaginaryPart：虚部视图索引
```

**使用示例**：

```cpp
// 假设有实部和虚部两个视图
gmsh::plugin::setNumber("HarmonicToTime", "RealPart", 0);     // 实部视图
gmsh::plugin::setNumber("HarmonicToTime", "ImaginaryPart", 1); // 虚部视图
gmsh::plugin::setNumber("HarmonicToTime", "NumSteps", 20);     // 20帧动画
gmsh::plugin::run("HarmonicToTime");
```

---

## 下午学习内容

### 5. 多时间步数据创建

**练习1：创建简单的时间序列**

```cpp
#include "gmsh.h"
#include <cmath>
#include <vector>

int main() {
    gmsh::initialize();
    gmsh::model::add("time_series");

    // 创建2D网格
    gmsh::model::occ::addRectangle(0, 0, 0, 4, 2);
    gmsh::model::occ::synchronize();
    gmsh::option::setNumber("Mesh.CharacteristicLengthMax", 0.1);
    gmsh::model::mesh::generate(2);

    std::vector<std::size_t> nodeTags;
    std::vector<double> coords, pc;
    gmsh::model::mesh::getNodes(nodeTags, coords, pc);

    // 创建视图
    int viewTag = gmsh::view::add("Wave Animation");

    // 添加多个时间步
    int numSteps = 50;
    double dt = 0.02;

    for(int t = 0; t < numSteps; t++) {
        double time = t * dt;
        std::vector<double> data;

        for(size_t i = 0; i < nodeTags.size(); i++) {
            double x = coords[3*i];
            double y = coords[3*i + 1];

            // 波动方程解：行波
            double k = 2 * M_PI;  // 波数
            double omega = 2 * M_PI;  // 角频率
            double val = std::sin(k * x - omega * time) *
                        std::exp(-0.5 * y * y);

            data.push_back(val);
        }

        // 添加此时间步的数据
        gmsh::view::addHomogeneousModelData(
            viewTag, t, "time_series", "NodeData",
            nodeTags, data, time
        );
    }

    // 设置动画选项
    gmsh::option::setNumber("View[0].ShowTime", 1);
    gmsh::option::setNumber("View[0].RangeType", 2);  // Custom range
    gmsh::option::setNumber("View[0].CustomMin", -1);
    gmsh::option::setNumber("View[0].CustomMax", 1);

    gmsh::fltk::run();
    gmsh::finalize();
    return 0;
}
```

**练习2：热扩散动画**

```cpp
#include "gmsh.h"
#include <cmath>
#include <vector>

int main() {
    gmsh::initialize();
    gmsh::model::add("heat_diffusion");

    // 创建圆形区域
    gmsh::model::occ::addDisk(0, 0, 0, 1, 1);
    gmsh::model::occ::synchronize();
    gmsh::option::setNumber("Mesh.CharacteristicLengthMax", 0.05);
    gmsh::model::mesh::generate(2);

    std::vector<std::size_t> nodeTags;
    std::vector<double> coords, pc;
    gmsh::model::mesh::getNodes(nodeTags, coords, pc);

    int viewTag = gmsh::view::add("Heat Diffusion");

    // 热扩散：初始为中心高斯分布，随时间扩散
    int numSteps = 40;
    double alpha = 0.1;  // 热扩散系数

    for(int t = 0; t < numSteps; t++) {
        double time = t * 0.1;
        std::vector<double> data;

        for(size_t i = 0; i < nodeTags.size(); i++) {
            double x = coords[3*i];
            double y = coords[3*i + 1];
            double r2 = x*x + y*y;

            // 高斯热源的扩散解
            // T(r,t) = T0 / (4πκt) * exp(-r²/(4κt))
            // 简化为：
            double sigma = 0.1 + alpha * time;  // 标准差随时间增大
            double val = std::exp(-r2 / (2 * sigma * sigma)) / sigma;

            data.push_back(val);
        }

        gmsh::view::addHomogeneousModelData(
            viewTag, t, "heat_diffusion", "NodeData",
            nodeTags, data, time
        );
    }

    // 颜色设置
    gmsh::option::setNumber("View[0].IntervalsType", 2);  // Continuous
    gmsh::option::setString("View[0].ColorTable", "Jet");

    gmsh::fltk::run();
    gmsh::finalize();
    return 0;
}
```

**练习3：向量场动画（旋转涡旋）**

```cpp
#include "gmsh.h"
#include <cmath>
#include <vector>

int main() {
    gmsh::initialize();
    gmsh::model::add("rotating_vortex");

    gmsh::model::occ::addRectangle(-2, -2, 0, 4, 4);
    gmsh::model::occ::synchronize();
    gmsh::option::setNumber("Mesh.CharacteristicLengthMax", 0.15);
    gmsh::model::mesh::generate(2);

    std::vector<std::size_t> nodeTags;
    std::vector<double> coords, pc;
    gmsh::model::mesh::getNodes(nodeTags, coords, pc);

    int viewTag = gmsh::view::add("Rotating Vortex");

    int numSteps = 60;

    for(int t = 0; t < numSteps; t++) {
        double time = t * 0.05;
        double theta = time * 2 * M_PI;  // 旋转角度
        std::vector<double> data;

        for(size_t i = 0; i < nodeTags.size(); i++) {
            double x = coords[3*i];
            double y = coords[3*i + 1];

            // 旋转涡旋：速度场随时间旋转
            double cosT = std::cos(theta);
            double sinT = std::sin(theta);

            // 基础旋转场
            double vx0 = -y;
            double vy0 = x;

            // 应用旋转变换
            double vx = vx0 * cosT - vy0 * sinT;
            double vy = vx0 * sinT + vy0 * cosT;

            // 衰减因子
            double r = std::sqrt(x*x + y*y);
            double decay = std::exp(-0.3 * r);

            data.push_back(vx * decay);
            data.push_back(vy * decay);
            data.push_back(0);
        }

        gmsh::view::addHomogeneousModelData(
            viewTag, t, "rotating_vortex", "NodeData",
            nodeTags, data, time, 3
        );
    }

    gmsh::option::setNumber("View[0].VectorType", 4);  // Arrow3D

    gmsh::fltk::run();
    gmsh::finalize();
    return 0;
}
```

### 6. 动画导出

**导出为图像序列**：

```cpp
// 使用API导出动画帧
#include "gmsh.h"

void exportAnimation(int viewTag, const std::string &prefix) {
    int numSteps;
    std::vector<std::string> types;
    gmsh::view::getType(viewTag, types, numSteps);

    for(int step = 0; step < numSteps; step++) {
        // 设置当前时间步
        gmsh::option::setNumber("View[0].TimeStep", step);

        // 刷新显示
        gmsh::graphics::draw();

        // 导出PNG
        char filename[256];
        snprintf(filename, sizeof(filename), "%s_%04d.png", prefix.c_str(), step);
        gmsh::write(filename);
    }
}
```

**GEO脚本中的动画**：

```geo
// 在GEO脚本中创建动画
For t In {0:2*Pi:0.1}
    View[0].TimeStep = t / (2*Pi) * 50;  // 假设50个时间步
    Draw;
    Sleep 0.1;  // 暂停0.1秒
    Print Sprintf("frame_%03g.png", t);
EndFor
```

---

## 检查点

### 理论理解

- [ ] 理解多时间步数据的存储结构
- [ ] 区分PViewDataList和PViewDataGModel中时间步的不同组织方式
- [ ] 理解时间步合并（Combine Time）的概念
- [ ] 了解HarmonicToTime插件的用途

### 代码阅读

- [ ] 已阅读PViewDataList中时间步相关代码
- [ ] 已阅读stepData模板类的结构
- [ ] 已阅读PView::combine()函数
- [ ] 了解HarmonicToTime插件实现

### 实践技能

- [ ] 能创建包含多个时间步的视图数据
- [ ] 能使用API控制动画播放
- [ ] 能将频域数据转换为时域动画
- [ ] 能导出动画帧序列

---

## 关键源码索引

| 主题 | 文件 | 重点内容 |
|------|------|----------|
| 时间步数据 | PViewDataList.h:21-25 | NbTimeStep, Time |
| stepData类 | PViewDataGModel.h:13 | 单时间步数据容器 |
| 时间合并 | PView.cpp | combine(true, ...) |
| 谐波转换 | HarmonicToTime.cpp | execute() |
| 动画选项 | PViewOptions.h:62 | timeStep |

---

## 扩展阅读

### 必读
1. Gmsh参考手册 - Combining views章节
2. `tutorials/t8.geo` - 后处理视图操作

### 选读
1. VTK动画输出
2. ParaView时间序列数据处理

---

## 导航

- 上一天：[Day 61 - 向量场可视化](day-61.md)
- 下一天：[Day 63 - 第九周复习](day-63.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
