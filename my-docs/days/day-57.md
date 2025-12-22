# Day 57：后处理数据结构

## 学习目标
理解Gmsh后处理模块的核心数据结构，掌握PView和PViewData的设计理念与使用方法。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习后处理模块概览 |
| 10:00-11:00 | 1h | 阅读PView类实现 |
| 11:00-12:00 | 1h | 分析PViewData接口 |
| 14:00-15:00 | 1h | 完成练习并总结 |

---

## 上午学习内容

### 1. 后处理模块概览

**目录结构**：`src/post/`

```
src/post/
├── 核心类
│   ├── PView.h/cpp           # 后处理视图主类
│   ├── PViewData.h/cpp       # 数据抽象接口
│   ├── PViewOptions.h/cpp    # 显示选项
│   └── PViewFactory.h/cpp    # 视图工厂
│
├── 数据容器
│   ├── PViewDataGModel.h/cpp # 网格关联数据
│   ├── PViewDataList.h/cpp   # 列表形式数据
│   └── PViewDataRemote.h     # 远程数据代理
│
├── 空间计算
│   ├── OctreePost.h/cpp      # 八叉树空间索引
│   ├── shapeFunctions.h/cpp  # 形状函数
│   └── adaptiveData.h/cpp    # 自适应数据
│
├── 可视化
│   ├── ColorTable.h/cpp      # 颜色映射
│   └── PViewVertexArrays.cpp # 顶点数组渲染
│
└── 输入输出
    ├── PViewIO.cpp           # 视图I/O
    └── PViewDataGModelIO*.cpp # 各格式支持
```

**后处理在Gmsh中的作用**：
```
几何建模 → 网格生成 → 求解计算 → 【后处理可视化】
                                    │
                                    ├── 数据加载（MSH/CGNS/MED）
                                    ├── 数据插值与查询
                                    ├── 等值线/等值面
                                    ├── 向量场/张量场
                                    └── 动画与导出
```

### 2. PView类

**文件位置**：`src/post/PView.h`

```cpp
class PView {
private:
    // 唯一标识
    static int _globalTag;
    int _tag;              // 唯一标签
    int _index;            // 在全局列表中的索引

    // 核心数据
    PViewData *_data;      // 后处理数据
    PViewOptions *_options; // 显示选项

    // 渲染数据
    VertexArray *va_points;     // 点顶点数组
    VertexArray *va_lines;      // 线顶点数组
    VertexArray *va_triangles;  // 三角形顶点数组
    VertexArray *va_vectors;    // 向量顶点数组
    VertexArray *va_ellipses;   // 椭圆顶点数组

    // 视图状态
    bool _changed;         // 数据是否改变
    bool _aliasOf;         // 是否为别名视图
    std::string _name;     // 视图名称

public:
    // 静态视图管理
    static std::vector<PView *> list;  // 全局视图列表
    static PView *getByTag(int tag);
    static PView *getByIndex(int index);

    // 构造函数
    PView(PViewData *data = nullptr, int tag = -1);
    PView(const std::string &name, ...);  // 多种重载

    // 数据访问
    PViewData *getData() { return _data; }
    PViewOptions *getOptions() { return _options; }
    void setData(PViewData *data);

    // 渲染
    void fillVertexArrays();
    void deleteVertexArrays();

    // I/O
    bool write(const std::string &fileName, int format, bool append = false);
    static bool readMSH(const std::string &fileName);

    // 操作
    static void combine(bool time, int how, bool remove);
};
```

**PView的职责**：
```
PView（视图容器）
│
├── 数据管理
│   ├── 持有PViewData指针
│   ├── 数据生命周期管理
│   └── 支持视图别名（轻量复制）
│
├── 显示控制
│   ├── 持有PViewOptions
│   ├── 管理顶点数组
│   └── 响应配置变化
│
├── 全局管理
│   ├── 静态视图列表
│   ├── 按tag/index查找
│   └── 视图合并操作
│
└── 文件I/O
    ├── 读取各种格式
    └── 导出视图数据
```

### 3. PViewData抽象接口

**文件位置**：`src/post/PViewData.h`

```cpp
class PViewData {
protected:
    // 基本属性
    std::string _name;           // 数据名称
    std::string _fileName;       // 来源文件
    int _fileIndex;              // 文件中的索引

    // 数据状态
    bool _dirty;                 // 是否需要更新
    bool _finalized;             // 是否已完成初始化

    // 自适应可视化
    adaptiveData *_adaptive;

public:
    // 基本信息
    virtual bool finalize(bool computeMinMax = true);
    virtual std::string getName() { return _name; }
    virtual bool hasModel(GModel *model, int step = -1) { return false; }

    // 时间步相关
    virtual int getNumTimeSteps() = 0;
    virtual double getTime(int step) { return 0.; }
    virtual double getMin(int step = -1, bool onlyVisible = false,
                          int tensorRep = 0, int forceNumComponents = 0,
                          int componentMap[9] = nullptr) = 0;
    virtual double getMax(int step = -1, ...) = 0;

    // 空间信息
    virtual SBoundingBox3d getBoundingBox(int step = -1) = 0;
    virtual int getNumElements(int step = -1, int ent = -1) { return 0; }
    virtual int getNumNodes(int step = -1, int ent = -1) { return 0; }

    // 数据查询与插值
    virtual bool searchScalar(double x, double y, double z,
                              double *values, int step = -1,
                              double *size = nullptr,
                              int qn = 0, double *qx = nullptr,
                              double *qy = nullptr, double *qz = nullptr,
                              bool grad = false, int dim = -1) = 0;
    virtual bool searchVector(double x, double y, double z,
                              double *values, int step = -1, ...) = 0;
    virtual bool searchTensor(double x, double y, double z,
                              double *values, int step = -1, ...) = 0;

    // 元素遍历（用于可视化）
    virtual int getNumScalars(int step = -1) { return 0; }
    virtual int getNumVectors(int step = -1) { return 0; }
    virtual int getNumTensors(int step = -1) { return 0; }

    // 数据导出
    virtual bool writePOS(const std::string &fileName, bool binary = false,
                          bool parsed = true, bool append = false);
    virtual bool writeMSH(const std::string &fileName, double version = 2.2,
                          bool binary = false, bool saveMesh = true,
                          bool multipleView = false, int partitionNum = -1,
                          bool saveInterpolationMatrices = true,
                          bool forceNodeData = false);
};
```

**数据类型分类**：

```
PViewData
│
├── 按空间维度
│   ├── 点数据（0D）
│   ├── 线数据（1D）
│   ├── 面数据（2D）
│   └── 体数据（3D）
│
├── 按数据类型
│   ├── 标量（Scalar）：温度、压力、密度
│   ├── 矢量（Vector）：速度、位移、力
│   └── 张量（Tensor）：应力、应变
│
└── 按存储位置
    ├── NodeData：节点值
    ├── ElementData：单元值
    ├── ElementNodeData：单元-节点值
    └── GaussPointData：高斯点值
```

---

## 下午学习内容

### 4. PViewDataGModel

**文件位置**：`src/post/PViewDataGModel.h`

```cpp
// 模板类：存储单个时间步的数据
template <class Real>
class stepData {
public:
    GModel *_model;                          // 关联的几何模型
    std::vector<GEntity *> _entities;        // 包含数据的实体
    std::vector<Real *> *_data;              // 数据数组（按id索引）
    double _min, _max;                       // 最值
    double _time;                            // 时间值
    int _numComp;                            // 分量数
    std::set<int> _partitions;               // 并行分区
    std::vector<std::vector<double>> _gaussPoints;  // 高斯点坐标
};

class PViewDataGModel : public PViewData {
public:
    // 数据类型枚举
    enum DataType {
        NodeData,           // 节点数据
        ElementData,        // 单元数据
        ElementNodeData,    // 单元-节点数据
        GaussPointData,     // 高斯点数据
        BeamData            // 梁单元数据
    };

private:
    DataType _type;
    std::vector<stepData<double> *> _steps;  // 时间步数据
    OctreePost *_octree;                     // 空间索引

public:
    // 构造函数
    PViewDataGModel(DataType type = NodeData);

    // 数据添加
    bool addData(GModel *model, const std::map<int, std::vector<double>> &data,
                 int step, double time, int partition, int numComp);

    // 时间步访问
    int getNumTimeSteps() override { return _steps.size(); }
    stepData<double> *getStepData(int step) { return _steps[step]; }

    // 元素数据访问
    bool getValue(int step, int ent, int ele, int nod, int comp, double &val);
    bool setValue(int step, int ent, int ele, int nod, int comp, double val);

    // 空间查询
    bool searchScalar(...) override;
    bool searchVector(...) override;
    bool searchTensor(...) override;
};
```

**数据存储示意**：

```
PViewDataGModel
│
├── _steps[0]（时间步0）
│   ├── _model → GModel*
│   ├── _data → std::vector<double*>
│   │   ├── [0] → nullptr（顶点0无数据）
│   │   ├── [1] → [v1_comp0, v1_comp1, ...]
│   │   ├── [2] → [v2_comp0, v2_comp1, ...]
│   │   └── ...
│   ├── _min, _max
│   └── _time = 0.0
│
├── _steps[1]（时间步1）
│   └── ...
│
└── _octree → OctreePost*（用于空间查询）
```

### 5. PViewDataList

**文件位置**：`src/post/PViewDataList.h`

```cpp
class PViewDataList : public PViewData {
private:
    // 针对每种元素类型的数据列表
    // 格式：[x1,y1,z1, x2,y2,z2, ..., v1,v2,...]

    // 点数据
    int NbSP, NbVP, NbTP;              // 标量/矢量/张量点的数量
    std::vector<double> SP, VP, TP;    // 点数据

    // 线数据
    int NbSL, NbVL, NbTL;
    std::vector<double> SL, VL, TL;

    // 三角形数据
    int NbST, NbVT, NbTT;
    std::vector<double> ST, VT, TT;

    // 四边形数据
    int NbSQ, NbVQ, NbTQ;
    std::vector<double> SQ, VQ, TQ;

    // 四面体数据
    int NbSS, NbVS, NbTS;
    std::vector<double> SS, VS, TS;

    // ... 其他元素类型

    // 2D图表数据
    int NbT2;
    std::vector<double> T2D;           // 2D文本
    std::vector<char> T2C;             // 2D字符

    // 时间步
    std::vector<double> Time;
    int NbTimeStep;

public:
    PViewDataList();

    // 数据导入（从.pos文件格式）
    bool importLists(int N[24], std::vector<double> *V[24]);

    // 元素访问
    int getNumScalars(int step = -1) override;
    int getNumVectors(int step = -1) override;
    int getNumTensors(int step = -1) override;

    // 空间查询
    bool searchScalar(...) override;
    bool searchVector(...) override;
    bool searchTensor(...) override;
};
```

**PViewDataList vs PViewDataGModel对比**：

| 特性 | PViewDataGModel | PViewDataList |
|------|-----------------|---------------|
| 网格关联 | 需要GModel | 独立存储 |
| 存储效率 | 按ID稀疏存储 | 连续列表存储 |
| 适用场景 | 有限元结果 | 粒子轨迹、采样点 |
| 拓扑信息 | 保留完整拓扑 | 仅几何位置 |
| 内存占用 | 大网格高效 | 小数据简单 |

---

## 练习作业

### 基础练习

**练习1**：理解PView的生命周期

```cpp
// 追踪以下代码中PView的创建和管理

// 1. 从文件加载
PView::readMSH("results.msh");  // 创建新PView并添加到list

// 2. 访问视图
PView *v = PView::list[0];      // 通过索引访问
PView *v2 = PView::getByTag(1); // 通过tag访问

// 3. 获取数据
PViewData *data = v->getData();
int nSteps = data->getNumTimeSteps();
double minVal = data->getMin(0);  // 第0时间步的最小值
double maxVal = data->getMax(0);

// 4. 导出
v->write("output.pos", 0);  // 导出为.pos格式

// 问题：
// - PView::list是如何管理内存的？
// - tag和index有什么区别？
// - 如何安全地删除一个PView？
```

### 进阶练习

**练习2**：实现简单的后处理数据容器

```cpp
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

// 简化版的后处理数据类
class SimplePostData {
public:
    struct Node {
        double x, y, z;
        int id;
    };

    struct Element {
        std::vector<int> nodeIds;
        int id;
    };

private:
    std::vector<Node> nodes;
    std::vector<Element> elements;
    std::map<int, std::vector<double>> nodeData;  // nodeId -> values
    int numComponents;
    std::string name;

public:
    SimplePostData(const std::string &n, int nComp)
        : name(n), numComponents(nComp) {}

    // 添加节点
    void addNode(int id, double x, double y, double z) {
        nodes.push_back({x, y, z, id});
    }

    // 添加单元
    void addElement(int id, const std::vector<int> &nids) {
        elements.push_back({nids, id});
    }

    // 设置节点数据
    void setNodeValue(int nodeId, const std::vector<double> &values) {
        if(values.size() == numComponents) {
            nodeData[nodeId] = values;
        }
    }

    // 获取数据范围
    void getMinMax(double &minVal, double &maxVal) const {
        minVal = 1e30;
        maxVal = -1e30;

        for(const auto &pair : nodeData) {
            for(double v : pair.second) {
                minVal = std::min(minVal, v);
                maxVal = std::max(maxVal, v);
            }
        }
    }

    // 在指定点插值（仅支持三角形）
    bool interpolate(double x, double y, double z,
                     std::vector<double> &result) const {
        // 遍历所有单元，找到包含该点的单元
        for(const auto &elem : elements) {
            if(elem.nodeIds.size() != 3) continue;  // 仅处理三角形

            // 获取三角形顶点
            const Node *n[3];
            for(int i = 0; i < 3; i++) {
                for(const auto &node : nodes) {
                    if(node.id == elem.nodeIds[i]) {
                        n[i] = &node;
                        break;
                    }
                }
            }

            // 计算重心坐标
            double detT = (n[1]->y - n[2]->y)*(n[0]->x - n[2]->x) +
                          (n[2]->x - n[1]->x)*(n[0]->y - n[2]->y);

            double l1 = ((n[1]->y - n[2]->y)*(x - n[2]->x) +
                         (n[2]->x - n[1]->x)*(y - n[2]->y)) / detT;
            double l2 = ((n[2]->y - n[0]->y)*(x - n[2]->x) +
                         (n[0]->x - n[2]->x)*(y - n[2]->y)) / detT;
            double l3 = 1 - l1 - l2;

            // 检查点是否在三角形内
            if(l1 >= -1e-10 && l2 >= -1e-10 && l3 >= -1e-10) {
                // 插值
                result.resize(numComponents, 0.0);

                for(int i = 0; i < 3; i++) {
                    auto it = nodeData.find(elem.nodeIds[i]);
                    if(it != nodeData.end()) {
                        double w = (i == 0) ? l1 : ((i == 1) ? l2 : l3);
                        for(int c = 0; c < numComponents; c++) {
                            result[c] += w * it->second[c];
                        }
                    }
                }
                return true;
            }
        }
        return false;  // 点不在任何单元内
    }

    // 打印信息
    void print() const {
        std::cout << "PostData: " << name << "\n";
        std::cout << "  Nodes: " << nodes.size() << "\n";
        std::cout << "  Elements: " << elements.size() << "\n";
        std::cout << "  Components: " << numComponents << "\n";

        double minV, maxV;
        getMinMax(minV, maxV);
        std::cout << "  Range: [" << minV << ", " << maxV << "]\n";
    }
};

int main() {
    // 创建简单的三角形网格后处理数据
    SimplePostData data("Temperature", 1);

    // 添加节点
    data.addNode(0, 0.0, 0.0, 0.0);
    data.addNode(1, 1.0, 0.0, 0.0);
    data.addNode(2, 0.5, 1.0, 0.0);

    // 添加三角形单元
    data.addElement(0, {0, 1, 2});

    // 设置节点温度值
    data.setNodeValue(0, {100.0});  // 节点0: 100度
    data.setNodeValue(1, {200.0});  // 节点1: 200度
    data.setNodeValue(2, {150.0});  // 节点2: 150度

    data.print();

    // 在重心处插值
    std::vector<double> result;
    if(data.interpolate(0.5, 0.33, 0.0, result)) {
        std::cout << "Temperature at centroid: " << result[0] << "\n";
        // 期望值: (100+200+150)/3 ≈ 150
    }

    // 在中点处插值
    if(data.interpolate(0.5, 0.0, 0.0, result)) {
        std::cout << "Temperature at midpoint (0-1): " << result[0] << "\n";
        // 期望值: (100+200)/2 = 150
    }

    return 0;
}
```

**练习3**：阅读PView数据加载流程

```cpp
// 追踪从MSH文件加载后处理数据的流程

// 1. PView::readMSH() 入口
//    文件：src/post/PViewIO.cpp

// 2. 根据文件版本选择读取方式
//    - MSH2: readMSH2()
//    - MSH4: readMSH4()

// 3. 解析$NodeData/$ElementData等块
//    创建PViewDataGModel并填充数据

// 4. 调用finalize()计算最值和边界

// 问题：
// - $NodeData和$ElementData的区别是什么？
// - 时间步是如何组织的？
// - 多分量数据如何存储？
```

### 挑战练习

**练习4**：实现多时间步数据管理

```cpp
#include <iostream>
#include <vector>
#include <map>

// 多时间步后处理数据
class TimeSeriesData {
public:
    struct TimeStep {
        double time;
        std::map<int, std::vector<double>> nodeData;
        double minVal, maxVal;

        void computeMinMax() {
            minVal = 1e30;
            maxVal = -1e30;
            for(const auto &p : nodeData) {
                for(double v : p.second) {
                    minVal = std::min(minVal, v);
                    maxVal = std::max(maxVal, v);
                }
            }
        }
    };

private:
    std::vector<TimeStep> steps;
    std::string name;
    int numComponents;

public:
    TimeSeriesData(const std::string &n, int nComp)
        : name(n), numComponents(nComp) {}

    // 添加时间步
    int addTimeStep(double time) {
        TimeStep step;
        step.time = time;
        step.minVal = 1e30;
        step.maxVal = -1e30;
        steps.push_back(step);
        return steps.size() - 1;
    }

    // 设置指定时间步的节点数据
    void setNodeData(int stepIndex, int nodeId,
                     const std::vector<double> &values) {
        if(stepIndex < steps.size() && values.size() == numComponents) {
            steps[stepIndex].nodeData[nodeId] = values;
        }
    }

    // 完成数据添加
    void finalize() {
        for(auto &step : steps) {
            step.computeMinMax();
        }
    }

    // 获取时间步数
    int getNumTimeSteps() const { return steps.size(); }

    // 获取指定时间步的时间值
    double getTime(int step) const {
        return (step < steps.size()) ? steps[step].time : 0.0;
    }

    // 获取全局最值
    void getGlobalMinMax(double &minVal, double &maxVal) const {
        minVal = 1e30;
        maxVal = -1e30;
        for(const auto &step : steps) {
            minVal = std::min(minVal, step.minVal);
            maxVal = std::max(maxVal, step.maxVal);
        }
    }

    // 时间插值
    bool interpolateTime(int nodeId, double time,
                         std::vector<double> &result) const {
        if(steps.empty()) return false;

        // 找到时间范围
        int i1 = 0, i2 = 0;
        for(int i = 0; i < steps.size() - 1; i++) {
            if(time >= steps[i].time && time <= steps[i+1].time) {
                i1 = i;
                i2 = i + 1;
                break;
            }
        }

        // 边界情况
        if(time <= steps[0].time) {
            auto it = steps[0].nodeData.find(nodeId);
            if(it != steps[0].nodeData.end()) {
                result = it->second;
                return true;
            }
            return false;
        }
        if(time >= steps.back().time) {
            auto it = steps.back().nodeData.find(nodeId);
            if(it != steps.back().nodeData.end()) {
                result = it->second;
                return true;
            }
            return false;
        }

        // 线性插值
        auto it1 = steps[i1].nodeData.find(nodeId);
        auto it2 = steps[i2].nodeData.find(nodeId);

        if(it1 == steps[i1].nodeData.end() ||
           it2 == steps[i2].nodeData.end()) {
            return false;
        }

        double t = (time - steps[i1].time) / (steps[i2].time - steps[i1].time);
        result.resize(numComponents);

        for(int c = 0; c < numComponents; c++) {
            result[c] = (1-t) * it1->second[c] + t * it2->second[c];
        }

        return true;
    }

    void print() const {
        std::cout << "TimeSeriesData: " << name << "\n";
        std::cout << "  Time steps: " << steps.size() << "\n";
        for(int i = 0; i < steps.size(); i++) {
            std::cout << "  Step " << i << ": t=" << steps[i].time
                      << ", range=[" << steps[i].minVal << ", "
                      << steps[i].maxVal << "]\n";
        }
    }
};

int main() {
    TimeSeriesData data("Displacement", 3);  // 3分量位移

    // 添加3个时间步
    int s0 = data.addTimeStep(0.0);
    int s1 = data.addTimeStep(0.5);
    int s2 = data.addTimeStep(1.0);

    // 节点0的位移随时间变化
    data.setNodeData(s0, 0, {0.0, 0.0, 0.0});
    data.setNodeData(s1, 0, {0.5, 0.1, 0.0});
    data.setNodeData(s2, 0, {1.0, 0.2, 0.0});

    data.finalize();
    data.print();

    // 时间插值：t=0.25
    std::vector<double> disp;
    if(data.interpolateTime(0, 0.25, disp)) {
        std::cout << "Displacement at t=0.25: ("
                  << disp[0] << ", " << disp[1] << ", " << disp[2] << ")\n";
        // 期望：(0.25, 0.05, 0.0)
    }

    return 0;
}
```

---

## 知识图谱

```
后处理数据结构
│
├── PView（视图容器）
│   ├── 唯一标识（tag, index）
│   ├── PViewData（数据）
│   ├── PViewOptions（选项）
│   └── VertexArray（渲染）
│
├── PViewData（抽象接口）
│   ├── getNumTimeSteps()
│   ├── getMin()/getMax()
│   ├── getBoundingBox()
│   └── searchScalar/Vector/Tensor()
│
├── PViewDataGModel（网格数据）
│   ├── DataType枚举
│   ├── stepData<double>模板
│   └── OctreePost空间索引
│
├── PViewDataList（列表数据）
│   ├── SP/VP/TP（点）
│   ├── SL/VL/TL（线）
│   └── ST/VT/TT（三角形）
│
└── 全局管理
    ├── PView::list（视图列表）
    └── PView::getByTag/Index()
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 |
|------|----------|--------|
| PView.h | 视图类定义 | ~200行 |
| PView.cpp | 视图实现 | ~800行 |
| PViewData.h | 数据接口 | ~300行 |
| PViewData.cpp | 数据基类实现 | ~500行 |
| PViewDataGModel.h | 网格数据类 | ~400行 |
| PViewDataGModel.cpp | 网格数据实现 | ~1500行 |
| PViewDataList.h | 列表数据类 | ~200行 |
| PViewDataList.cpp | 列表数据实现 | ~800行 |

---

## 今日检查点

- [ ] 理解PView作为视图容器的职责
- [ ] 能解释PViewData的抽象接口设计
- [ ] 理解PViewDataGModel和PViewDataList的区别
- [ ] 能描述后处理数据的存储结构
- [ ] 理解时间步在数据组织中的作用

---

## 导航

- 上一天：[Day 56 - 第八周复习](day-56.md)
- 下一天：[Day 58 - PView视图系统](day-58.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
