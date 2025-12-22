# Day 58：PView视图系统

## 学习目标
掌握Gmsh后处理视图的显示配置系统，理解PViewOptions和ColorTable的设计与应用。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习PViewOptions配置体系 |
| 10:00-11:00 | 1h | 阅读显示类型与表示方式 |
| 11:00-12:00 | 1h | 分析ColorTable颜色映射 |
| 14:00-15:00 | 1h | 完成练习并总结 |

---

## 上午学习内容

### 1. PViewOptions概览

**文件位置**：`src/post/PViewOptions.h`

```cpp
class PViewOptions {
public:
    // 绘制类型
    enum PlotType {
        Plot3D = 1,        // 3D绘制
        Plot2DSpace = 2,   // 2D空间图
        Plot2DTime = 3     // 2D时间图
    };

    // 区间表示类型
    enum IntervalsType {
        Iso = 1,           // 等值线/等值面
        Continuous = 2,    // 连续着色
        Discrete = 3,      // 离散着色
        Numeric = 4        // 数值显示
    };

    // 向量表示类型
    enum VectorType {
        Segment = 1,       // 线段
        Arrow = 2,         // 2D箭头
        Pyramid = 3,       // 金字塔
        Arrow3D = 4,       // 3D箭头
        Displacement = 5   // 位移显示
    };

    // 张量表示类型
    enum TensorType {
        VonMises = 1,      // von Mises等效应力
        MaxEigenvalue = 2, // 最大特征值
        MinEigenvalue = 3, // 最小特征值
        Eigenvectors = 4,  // 特征向量
        Ellipse = 5,       // 椭圆
        Ellipsoid = 6,     // 椭球
        Frame = 7          // 坐标框架
    };

    // 范围类型
    enum RangeType {
        Default = 1,       // 自动范围
        Custom = 2,        // 自定义范围
        PerTimeStep = 3    // 按时间步范围
    };

    // ... 更多枚举
};
```

### 2. PViewOptions核心属性

```cpp
class PViewOptions {
public:
    // 基本显示控制
    int visible;           // 是否可见
    int type;              // PlotType
    int intervalsType;     // IntervalsType
    int nbIso;             // 等值线/面数量

    // 数据范围
    int rangeType;         // RangeType
    double customMin;      // 自定义最小值
    double customMax;      // 自定义最大值
    double externalMin;    // 外部最小值
    double externalMax;    // 外部最大值

    // 向量控制
    int vectorType;        // VectorType
    double arrowSizeMin;   // 箭头最小尺寸
    double arrowSizeMax;   // 箭头最大尺寸
    int arrowSizeProportional; // 是否按比例

    // 张量控制
    int tensorType;        // TensorType

    // 元素可见性
    int drawPoints;        // 绘制点
    int drawLines;         // 绘制线
    int drawTriangles;     // 绘制三角形
    int drawQuadrangles;   // 绘制四边形
    int drawTetrahedra;    // 绘制四面体
    int drawHexahedra;     // 绘制六面体
    int drawPrisms;        // 绘制棱柱
    int drawPyramids;      // 绘制金字塔
    int drawTrihedra;      // 绘制三面体

    // 颜色与透明度
    ColorTable colorTable; // 颜色映射表
    unsigned int color;    // 基础颜色

    // 光照与材质
    int light;             // 是否启用光照
    int lightTwoSide;      // 双面光照
    int smoothNormals;     // 平滑法线

    // 网格显示
    int showElement;       // 显示单元边界
    int showTime;          // 显示时间步
    int showScale;         // 显示色标

    // 变换
    double transform[3][3]; // 变换矩阵
    double offset[3];       // 偏移量
    double raise[3];        // 高度偏移

    // 方法
    void reset();           // 重置为默认值
    static PViewOptions *reference(); // 获取参考选项
};
```

### 3. 显示配置层次

```
PViewOptions
│
├── 绘制类型（type）
│   ├── Plot3D：标准3D可视化
│   ├── Plot2DSpace：2D平面图（XY/XZ/YZ）
│   └── Plot2DTime：时间序列图
│
├── 区间表示（intervalsType）
│   ├── Iso：等值线/等值面
│   ├── Continuous：连续平滑着色
│   ├── Discrete：分段离散着色
│   └── Numeric：直接显示数值
│
├── 元素可见性
│   ├── drawPoints/Lines/Triangles...
│   └── 按单元类型独立控制
│
├── 数据范围
│   ├── Default：自动计算
│   ├── Custom：用户指定min/max
│   └── PerTimeStep：每步独立范围
│
└── 外观控制
    ├── 颜色映射（colorTable）
    ├── 光照（light, lightTwoSide）
    └── 变换（transform, offset, raise）
```

### 4. GEO脚本中的视图选项

```geo
// 在GEO脚本中设置视图选项
View[0].Type = 1;              // 3D绘制
View[0].IntervalsType = 2;     // 连续着色
View[0].NbIso = 20;            // 20条等值线

View[0].RangeType = 2;         // 自定义范围
View[0].CustomMin = 0;
View[0].CustomMax = 100;

View[0].VectorType = 4;        // 3D箭头
View[0].ArrowSizeMax = 50;

View[0].ShowScale = 1;         // 显示色标
View[0].ShowTime = 1;          // 显示时间

View[0].Light = 1;             // 启用光照
View[0].SmoothNormals = 1;     // 平滑法线
```

---

## 下午学习内容

### 5. ColorTable颜色映射

**文件位置**：`src/post/ColorTable.h`

```cpp
class ColorTable {
public:
    // 颜色表大小
    static const int SIZE = 256;

    // RGBA颜色数组
    unsigned int table[SIZE];

    // 预定义色表类型
    enum PredefColorTable {
        GMSH = 0,          // Gmsh默认
        VIS5D = 1,         // Vis5D风格
        JET = 2,           // Jet（蓝→青→黄→红）
        HOT = 3,           // Hot（黑→红→黄→白）
        COOL = 4,          // Cool（青→品红）
        GRAYSCALE = 5,     // 灰度
        RAINBOW = 6,       // 彩虹
        // ...更多预定义
    };

    // 配置参数
    int ipar[10];   // 整数参数（预设类型、反转等）
    double dpar[10]; // 浮点参数（曲率、旋转等）

    // ipar含义：
    // [0] - 预定义色表编号
    // [1] - 是否反转
    // [2] - 是否交换
    // [3] - 颜色模式（RGB/HSV）
    // [4] - 透明度模式

    // dpar含义：
    // [0] - 曲率
    // [1] - 偏移
    // [2] - 旋转
    // [3] - alpha系数
    // [4] - beta（亮度）系数

public:
    ColorTable();

    // 重新计算颜色表
    void recompute(bool alpha = true, int numColors = 256);

    // 获取颜色
    unsigned int getColor(double value, double min, double max);

    // 颜色分解
    static void getRGB(unsigned int c, int &r, int &g, int &b);
    static void getRGBA(unsigned int c, int &r, int &g, int &b, int &a);

    // 颜色合成
    static unsigned int packRGBA(int r, int g, int b, int a);
};
```

### 6. 颜色映射原理

```
数值 → 归一化 → 颜色索引 → 颜色值

步骤：
1. 归一化：t = (value - min) / (max - min)  ∈ [0, 1]
2. 索引计算：index = (int)(t * 255)
3. 查表：color = table[index]
```

**常用色表对比**：

```
Jet（科学可视化常用）:
  0.0        0.25       0.5        0.75       1.0
  蓝色   →   青色   →   绿色   →   黄色   →   红色

Hot（热力图）:
  0.0        0.33       0.66       1.0
  黑色   →   红色   →   黄色   →   白色

Grayscale（灰度）:
  0.0                              1.0
  黑色   ────────────────────────→ 白色

Rainbow（彩虹）:
  0.0        0.17       0.33       0.5        0.67       0.83       1.0
  紫色   →   蓝色   →   青色   →   绿色   →   黄色   →   橙色   →   红色
```

### 7. 色表配置示例

```cpp
// 创建自定义色表
void setupCustomColorTable(ColorTable &ct) {
    // 使用预定义的Jet色表
    ct.ipar[0] = ColorTable::JET;
    ct.ipar[1] = 0;  // 不反转
    ct.ipar[2] = 0;  // 不交换

    ct.dpar[0] = 0.0;  // 曲率
    ct.dpar[1] = 0.0;  // 偏移
    ct.dpar[2] = 0.0;  // 旋转
    ct.dpar[3] = 1.0;  // alpha
    ct.dpar[4] = 0.0;  // beta

    ct.recompute();
}

// 获取指定值对应的颜色
unsigned int getColorForValue(ColorTable &ct,
                               double value,
                               double minVal,
                               double maxVal) {
    // 归一化
    double t = (value - minVal) / (maxVal - minVal);
    t = std::max(0.0, std::min(1.0, t));

    // 获取颜色索引
    int index = (int)(t * 255);

    return ct.table[index];
}
```

---

## 练习作业

### 基础练习

**练习1**：理解视图选项的作用

```cpp
// 分析以下选项设置的效果

PViewOptions *opt = view->getOptions();

// 设置1：显示等值面
opt->intervalsType = PViewOptions::Iso;
opt->nbIso = 10;  // 10个等值面

// 设置2：向量场显示
opt->vectorType = PViewOptions::Arrow3D;
opt->arrowSizeMax = 100;
opt->arrowSizeProportional = 1;

// 设置3：范围控制
opt->rangeType = PViewOptions::Custom;
opt->customMin = 0;
opt->customMax = 1;

// 问题：
// 1. 等值面的数量和位置如何确定？
// 2. 箭头大小与数据值有什么关系？
// 3. 超出自定义范围的值如何处理？
```

### 进阶练习

**练习2**：实现简单的颜色映射

```cpp
#include <iostream>
#include <cmath>
#include <algorithm>

// 简单的颜色结构
struct Color {
    unsigned char r, g, b, a;

    Color(unsigned char _r = 0, unsigned char _g = 0,
          unsigned char _b = 0, unsigned char _a = 255)
        : r(_r), g(_g), b(_b), a(_a) {}

    void print() const {
        printf("RGBA(%3d, %3d, %3d, %3d)\n", r, g, b, a);
    }
};

// 颜色映射表
class SimpleColorTable {
public:
    static const int SIZE = 256;
    Color table[SIZE];

    enum Type { GRAYSCALE, JET, HOT, COOL };

private:
    Type type;
    bool reversed;

public:
    SimpleColorTable(Type t = JET, bool rev = false)
        : type(t), reversed(rev) {
        recompute();
    }

    void recompute() {
        for(int i = 0; i < SIZE; i++) {
            double t = double(i) / (SIZE - 1);
            if(reversed) t = 1.0 - t;

            switch(type) {
            case GRAYSCALE:
                table[i] = grayscale(t);
                break;
            case JET:
                table[i] = jet(t);
                break;
            case HOT:
                table[i] = hot(t);
                break;
            case COOL:
                table[i] = cool(t);
                break;
            }
        }
    }

    // 灰度
    Color grayscale(double t) {
        unsigned char v = (unsigned char)(t * 255);
        return Color(v, v, v);
    }

    // Jet色表
    Color jet(double t) {
        double r, g, b;
        if(t < 0.125) {
            r = 0; g = 0; b = 0.5 + 4*t;
        } else if(t < 0.375) {
            r = 0; g = 4*(t-0.125); b = 1;
        } else if(t < 0.625) {
            r = 4*(t-0.375); g = 1; b = 1 - 4*(t-0.375);
        } else if(t < 0.875) {
            r = 1; g = 1 - 4*(t-0.625); b = 0;
        } else {
            r = 1 - 4*(t-0.875); g = 0; b = 0;
        }
        return Color((unsigned char)(r*255),
                     (unsigned char)(g*255),
                     (unsigned char)(b*255));
    }

    // Hot色表
    Color hot(double t) {
        double r, g, b;
        if(t < 0.33) {
            r = t / 0.33; g = 0; b = 0;
        } else if(t < 0.66) {
            r = 1; g = (t - 0.33) / 0.33; b = 0;
        } else {
            r = 1; g = 1; b = (t - 0.66) / 0.34;
        }
        return Color((unsigned char)(r*255),
                     (unsigned char)(g*255),
                     (unsigned char)(b*255));
    }

    // Cool色表
    Color cool(double t) {
        return Color((unsigned char)(t * 255),
                     (unsigned char)((1-t) * 255),
                     255);
    }

    // 获取值对应的颜色
    Color getColor(double value, double minVal, double maxVal) {
        double t = (value - minVal) / (maxVal - minVal);
        t = std::max(0.0, std::min(1.0, t));
        int index = (int)(t * (SIZE - 1));
        return table[index];
    }

    // 打印色表采样
    void printSamples(int n = 10) {
        std::cout << "Color table samples (" << n << " points):\n";
        for(int i = 0; i < n; i++) {
            double t = double(i) / (n - 1);
            int index = (int)(t * (SIZE - 1));
            std::cout << "  t=" << t << ": ";
            table[index].print();
        }
    }
};

int main() {
    // 测试不同色表
    std::cout << "=== Grayscale ===\n";
    SimpleColorTable gray(SimpleColorTable::GRAYSCALE);
    gray.printSamples(5);

    std::cout << "\n=== Jet ===\n";
    SimpleColorTable jet(SimpleColorTable::JET);
    jet.printSamples(5);

    std::cout << "\n=== Hot ===\n";
    SimpleColorTable hot(SimpleColorTable::HOT);
    hot.printSamples(5);

    // 数据映射示例
    std::cout << "\n=== Data mapping example ===\n";
    double data[] = {0, 25, 50, 75, 100};
    for(double v : data) {
        Color c = jet.getColor(v, 0, 100);
        std::cout << "Value " << v << ": ";
        c.print();
    }

    return 0;
}
```

**练习3**：视图选项配置器

```cpp
#include <iostream>
#include <string>
#include <map>

// 简化的视图选项类
class SimpleViewOptions {
public:
    // 绘制类型
    enum PlotType { PLOT_3D = 1, PLOT_2D_SPACE, PLOT_2D_TIME };

    // 区间类型
    enum IntervalType { ISO = 1, CONTINUOUS, DISCRETE, NUMERIC };

    // 向量类型
    enum VectorType { SEGMENT = 1, ARROW, PYRAMID, ARROW3D, DISPLACEMENT };

private:
    // 基本属性
    bool visible;
    PlotType plotType;
    IntervalType intervalType;
    int numIso;

    // 范围
    bool customRange;
    double minValue, maxValue;

    // 向量
    VectorType vectorType;
    double arrowSize;
    bool arrowProportional;

    // 显示元素
    bool showPoints, showLines, showTriangles;
    bool showScale, showTime;

    // 光照
    bool lighting;

public:
    SimpleViewOptions() {
        reset();
    }

    void reset() {
        visible = true;
        plotType = PLOT_3D;
        intervalType = CONTINUOUS;
        numIso = 10;

        customRange = false;
        minValue = 0;
        maxValue = 1;

        vectorType = ARROW3D;
        arrowSize = 50;
        arrowProportional = true;

        showPoints = true;
        showLines = true;
        showTriangles = true;
        showScale = true;
        showTime = true;

        lighting = true;
    }

    // 设置方法
    void setVisible(bool v) { visible = v; }
    void setPlotType(PlotType t) { plotType = t; }
    void setIntervalType(IntervalType t) { intervalType = t; }
    void setNumIso(int n) { numIso = n; }

    void setCustomRange(double min, double max) {
        customRange = true;
        minValue = min;
        maxValue = max;
    }

    void setAutoRange() { customRange = false; }

    void setVectorType(VectorType t) { vectorType = t; }
    void setArrowSize(double s) { arrowSize = s; }

    void setShowElements(bool pts, bool lines, bool tris) {
        showPoints = pts;
        showLines = lines;
        showTriangles = tris;
    }

    void setLighting(bool l) { lighting = l; }

    // 获取方法
    bool isVisible() const { return visible; }
    double getMin() const { return minValue; }
    double getMax() const { return maxValue; }
    bool hasCustomRange() const { return customRange; }

    // 打印配置
    void print() const {
        std::cout << "View Options:\n";
        std::cout << "  Visible: " << (visible ? "yes" : "no") << "\n";
        std::cout << "  Plot type: " << plotTypeStr() << "\n";
        std::cout << "  Interval type: " << intervalTypeStr() << "\n";
        std::cout << "  Num iso: " << numIso << "\n";

        if(customRange) {
            std::cout << "  Range: [" << minValue << ", " << maxValue << "]\n";
        } else {
            std::cout << "  Range: auto\n";
        }

        std::cout << "  Vector type: " << vectorTypeStr() << "\n";
        std::cout << "  Arrow size: " << arrowSize << "\n";

        std::cout << "  Show: points=" << showPoints
                  << ", lines=" << showLines
                  << ", triangles=" << showTriangles << "\n";

        std::cout << "  Lighting: " << (lighting ? "on" : "off") << "\n";
    }

private:
    std::string plotTypeStr() const {
        switch(plotType) {
        case PLOT_3D: return "3D";
        case PLOT_2D_SPACE: return "2D Space";
        case PLOT_2D_TIME: return "2D Time";
        }
        return "unknown";
    }

    std::string intervalTypeStr() const {
        switch(intervalType) {
        case ISO: return "Iso";
        case CONTINUOUS: return "Continuous";
        case DISCRETE: return "Discrete";
        case NUMERIC: return "Numeric";
        }
        return "unknown";
    }

    std::string vectorTypeStr() const {
        switch(vectorType) {
        case SEGMENT: return "Segment";
        case ARROW: return "Arrow";
        case PYRAMID: return "Pyramid";
        case ARROW3D: return "Arrow3D";
        case DISPLACEMENT: return "Displacement";
        }
        return "unknown";
    }
};

int main() {
    SimpleViewOptions opt;

    std::cout << "=== Default options ===\n";
    opt.print();

    std::cout << "\n=== Modified options ===\n";
    opt.setIntervalType(SimpleViewOptions::ISO);
    opt.setNumIso(20);
    opt.setCustomRange(0, 100);
    opt.setVectorType(SimpleViewOptions::DISPLACEMENT);
    opt.setLighting(false);
    opt.print();

    return 0;
}
```

### 挑战练习

**练习4**：实现带色标的可视化输出

```cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

// 生成PPM格式的色标图像
class ColorBarGenerator {
    int width, height;
    std::vector<unsigned char> pixels;

public:
    ColorBarGenerator(int w = 50, int h = 256)
        : width(w), height(h), pixels(w * h * 3, 0) {}

    // 使用Jet色表填充
    void fillJet(bool vertical = true) {
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                double t = vertical ? double(height - 1 - y) / (height - 1)
                                    : double(x) / (width - 1);

                unsigned char r, g, b;
                jetColor(t, r, g, b);

                int idx = (y * width + x) * 3;
                pixels[idx] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
            }
        }
    }

    // 添加刻度线
    void addTicks(int numTicks = 5) {
        for(int i = 0; i < numTicks; i++) {
            int y = i * (height - 1) / (numTicks - 1);
            // 在左边绘制白色刻度线
            for(int x = 0; x < 5; x++) {
                int idx = (y * width + x) * 3;
                pixels[idx] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
            }
        }
    }

    // 保存为PPM文件
    void savePPM(const std::string &filename) {
        std::ofstream file(filename, std::ios::binary);
        file << "P6\n" << width << " " << height << "\n255\n";
        file.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
        file.close();
        std::cout << "Saved color bar to " << filename << "\n";
    }

private:
    void jetColor(double t, unsigned char &r, unsigned char &g, unsigned char &b) {
        double dr, dg, db;
        if(t < 0.125) {
            dr = 0; dg = 0; db = 0.5 + 4*t;
        } else if(t < 0.375) {
            dr = 0; dg = 4*(t-0.125); db = 1;
        } else if(t < 0.625) {
            dr = 4*(t-0.375); dg = 1; db = 1 - 4*(t-0.375);
        } else if(t < 0.875) {
            dr = 1; dg = 1 - 4*(t-0.625); db = 0;
        } else {
            dr = 1 - 4*(t-0.875); dg = 0; db = 0;
        }
        r = (unsigned char)(dr * 255);
        g = (unsigned char)(dg * 255);
        b = (unsigned char)(db * 255);
    }
};

// 生成简单的2D标量场可视化
class ScalarFieldVisualizer {
    int width, height;
    std::vector<double> data;
    std::vector<unsigned char> pixels;
    double minVal, maxVal;

public:
    ScalarFieldVisualizer(int w, int h)
        : width(w), height(h), data(w * h, 0), pixels(w * h * 3, 0) {}

    // 设置数据
    void setData(int x, int y, double value) {
        if(x >= 0 && x < width && y >= 0 && y < height) {
            data[y * width + x] = value;
        }
    }

    // 计算范围
    void computeRange() {
        minVal = data[0];
        maxVal = data[0];
        for(double v : data) {
            minVal = std::min(minVal, v);
            maxVal = std::max(maxVal, v);
        }
    }

    // 生成测试数据（正弦波）
    void generateTestData() {
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                double fx = double(x) / width * 4 * M_PI;
                double fy = double(y) / height * 4 * M_PI;
                setData(x, y, std::sin(fx) * std::cos(fy));
            }
        }
        computeRange();
    }

    // 渲染为图像
    void render() {
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                double v = data[y * width + x];
                double t = (v - minVal) / (maxVal - minVal);

                unsigned char r, g, b;
                jetColor(t, r, g, b);

                int idx = (y * width + x) * 3;
                pixels[idx] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
            }
        }
    }

    // 保存为PPM
    void savePPM(const std::string &filename) {
        std::ofstream file(filename, std::ios::binary);
        file << "P6\n" << width << " " << height << "\n255\n";
        file.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
        file.close();
        std::cout << "Saved field to " << filename << "\n";
        std::cout << "  Range: [" << minVal << ", " << maxVal << "]\n";
    }

private:
    void jetColor(double t, unsigned char &r, unsigned char &g, unsigned char &b) {
        // 同上
        double dr, dg, db;
        if(t < 0.125) {
            dr = 0; dg = 0; db = 0.5 + 4*t;
        } else if(t < 0.375) {
            dr = 0; dg = 4*(t-0.125); db = 1;
        } else if(t < 0.625) {
            dr = 4*(t-0.375); dg = 1; db = 1 - 4*(t-0.375);
        } else if(t < 0.875) {
            dr = 1; dg = 1 - 4*(t-0.625); db = 0;
        } else {
            dr = 1 - 4*(t-0.875); dg = 0; db = 0;
        }
        r = (unsigned char)(dr * 255);
        g = (unsigned char)(dg * 255);
        b = (unsigned char)(db * 255);
    }
};

int main() {
    // 生成色标
    ColorBarGenerator colorbar(50, 256);
    colorbar.fillJet();
    colorbar.addTicks(11);
    colorbar.savePPM("colorbar.ppm");

    // 生成标量场可视化
    ScalarFieldVisualizer field(256, 256);
    field.generateTestData();
    field.render();
    field.savePPM("scalar_field.ppm");

    return 0;
}
```

---

## 知识图谱

```
PView视图系统
│
├── PViewOptions（配置核心）
│   ├── PlotType（绘制类型）
│   ├── IntervalsType（区间表示）
│   ├── VectorType（向量显示）
│   ├── TensorType（张量显示）
│   └── RangeType（范围控制）
│
├── ColorTable（颜色映射）
│   ├── 预定义色表（Jet/Hot/Cool...）
│   ├── 颜色插值
│   └── 透明度控制
│
├── 显示控制
│   ├── 元素可见性
│   ├── 光照与材质
│   └── 变换与偏移
│
└── GEO脚本接口
    └── View[i].Option = value
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 |
|------|----------|--------|
| PViewOptions.h | 选项类定义 | ~400行 |
| PViewOptions.cpp | 选项实现 | ~600行 |
| ColorTable.h | 颜色表定义 | ~100行 |
| ColorTable.cpp | 颜色计算实现 | ~400行 |

---

## 今日检查点

- [ ] 理解PViewOptions的配置层次
- [ ] 能解释不同绘制类型的区别
- [ ] 理解ColorTable的颜色映射原理
- [ ] 能在GEO脚本中设置视图选项
- [ ] 理解向量和张量的不同显示方式

---

## 导航

- 上一天：[Day 57 - 后处理数据结构](day-57.md)
- 下一天：[Day 59 - 数据插值与空间查询](day-59.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
