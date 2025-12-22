# Day 60：可视化渲染

## 学习目标

掌握Gmsh后处理模块的渲染系统，理解VertexArrays、等值线/等值面生成算法和OpenGL渲染流程。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习渲染管线概述 |
| 10:00-11:00 | 1h | 阅读VertexArray实现 |
| 11:00-12:00 | 1h | 分析等值线/等值面算法 |
| 14:00-15:00 | 1h | 完成练习并总结 |

---

## 上午学习内容

### 1. 渲染管线概述

**Gmsh渲染架构**：

```text
PViewData (数据)
    │
    ▼
PViewOptions (配置)
    │
    ▼
fillVertexArrays() (数据处理)
    │
    ├── 标量场 → 颜色映射
    ├── 向量场 → 箭头几何
    └── 张量场 → 椭球/框架
    │
    ▼
VertexArray (顶点缓存)
    │
    ├── va_points    (点)
    ├── va_lines     (线)
    ├── va_triangles (面)
    └── va_vectors   (向量)
    │
    ▼
OpenGL渲染
```

**渲染数据类型**：

```text
可视化元素分类：
│
├── 点元素（Points）
│   ├── 采样点显示
│   ├── 节点标记
│   └── 粒子位置
│
├── 线元素（Lines）
│   ├── 等值线
│   ├── 网格边界
│   ├── 流线
│   └── 向量箭头轮廓
│
├── 面元素（Triangles）
│   ├── 等值面
│   ├── 连续着色面
│   └── 向量箭头填充
│
└── 向量元素（Vectors）
    ├── 速度场
    ├── 位移场
    └── 力场
```

### 2. VertexArray顶点数组

**文件位置**：`src/graphics/VertexArray.h`

```cpp
class VertexArray {
public:
    // 顶点数据类型
    enum DataType {
        POSITION,      // 位置 (x,y,z)
        NORMAL,        // 法线 (nx,ny,nz)
        COLOR,         // 颜色 (r,g,b,a)
        TEXCOORD       // 纹理坐标 (u,v)
    };

private:
    // 顶点数据存储
    std::vector<float> _vertices;    // 位置
    std::vector<float> _normals;     // 法线
    std::vector<unsigned char> _colors;  // 颜色

    // 元素数量
    int _numVertices;
    int _numElements;

    // 数据排列方式
    int _numVerticesPerElement;      // 每个元素的顶点数

public:
    VertexArray(int numVerticesPerElement);
    ~VertexArray();

    // 添加顶点
    void add(float x, float y, float z,
             float nx, float ny, float nz,
             unsigned char r, unsigned char g,
             unsigned char b, unsigned char a);

    // 添加带颜色的顶点
    void add(float x, float y, float z,
             unsigned int color);

    // 获取数据指针（用于OpenGL）
    float *getVertexArray() { return _vertices.data(); }
    float *getNormalArray() { return _normals.data(); }
    unsigned char *getColorArray() { return _colors.data(); }

    // 获取数量
    int getNumVertices() { return _numVertices; }
    int getNumElements() { return _numElements; }

    // 清空
    void clear();

    // 排序（用于透明度渲染）
    void sort(double eye[3]);
};
```

### 3. PView顶点数组填充

**文件位置**：`src/post/PViewVertexArrays.cpp`

```cpp
void PView::fillVertexArrays() {
    // 清空现有数组
    deleteVertexArrays();

    PViewOptions *opt = getOptions();
    PViewData *data = getData();

    // 根据显示类型创建顶点数组
    if(opt->drawPoints)
        va_points = new VertexArray(1);      // 点
    if(opt->drawLines)
        va_lines = new VertexArray(2);       // 线段
    if(opt->drawTriangles || opt->drawQuadrangles)
        va_triangles = new VertexArray(3);   // 三角形
    if(opt->vectorType)
        va_vectors = new VertexArray(3);     // 向量箭头

    // 遍历数据填充顶点数组
    for(int step = 0; step < data->getNumTimeSteps(); step++) {
        if(step != opt->timeStep) continue;

        // 遍历所有实体
        for(int ent = 0; ent < data->getNumEntities(step); ent++) {
            // 遍历所有元素
            for(int ele = 0; ele < data->getNumElements(step, ent); ele++) {
                fillOneElement(step, ent, ele, opt, data);
            }
        }
    }
}

void PView::fillOneElement(int step, int ent, int ele,
                           PViewOptions *opt, PViewData *data) {
    // 获取元素类型和节点数
    int type = data->getType(step, ent, ele);
    int numNodes = data->getNumNodes(step, ent, ele);
    int numComp = data->getNumComponents(step, ent, ele);

    // 获取节点坐标
    std::vector<double> xyz(3 * numNodes);
    for(int i = 0; i < numNodes; i++) {
        data->getNode(step, ent, ele, i,
                      xyz[3*i], xyz[3*i+1], xyz[3*i+2]);
    }

    // 获取节点值
    std::vector<double> values(numComp * numNodes);
    for(int i = 0; i < numNodes; i++) {
        for(int c = 0; c < numComp; c++) {
            data->getValue(step, ent, ele, i, c,
                          values[numComp*i + c]);
        }
    }

    // 根据区间类型处理
    switch(opt->intervalsType) {
    case PViewOptions::Iso:
        // 等值线/等值面
        addIsoElement(type, numNodes, xyz, values, numComp, opt);
        break;
    case PViewOptions::Continuous:
        // 连续着色
        addContinuousElement(type, numNodes, xyz, values, numComp, opt);
        break;
    case PViewOptions::Discrete:
        // 离散着色
        addDiscreteElement(type, numNodes, xyz, values, numComp, opt);
        break;
    }

    // 如果是向量场，添加箭头
    if(numComp == 3 && opt->vectorType) {
        addVectorElement(type, numNodes, xyz, values, opt);
    }
}
```

---

## 下午学习内容

### 4. 等值线算法（Marching Squares）

**三角形中的等值线提取**：

```cpp
// 在三角形中提取等值线
void extractIsolineInTriangle(
    double x[3], double y[3], double z[3],  // 三角形顶点坐标
    double v[3],                             // 顶点值
    double isoValue,                         // 等值线值
    std::vector<double> &linePoints)         // 输出线段端点
{
    // 统计在等值线两侧的顶点
    int above = 0, below = 0;
    for(int i = 0; i < 3; i++) {
        if(v[i] > isoValue) above++;
        else if(v[i] < isoValue) below++;
    }

    // 如果所有点都在同侧，无交线
    if(above == 0 || below == 0) return;

    // 找到与等值线相交的边
    std::vector<double> crossPoints;

    for(int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;

        // 检查边(i,j)是否与等值线相交
        if((v[i] - isoValue) * (v[j] - isoValue) < 0) {
            // 计算交点参数
            double t = (isoValue - v[i]) / (v[j] - v[i]);

            // 计算交点坐标
            double px = x[i] + t * (x[j] - x[i]);
            double py = y[i] + t * (y[j] - y[i]);
            double pz = z[i] + t * (z[j] - z[i]);

            crossPoints.push_back(px);
            crossPoints.push_back(py);
            crossPoints.push_back(pz);
        }
    }

    // 应该恰好有2个交点
    if(crossPoints.size() == 6) {
        linePoints.insert(linePoints.end(),
                          crossPoints.begin(), crossPoints.end());
    }
}
```

**等值线示意**：

```text
三角形等值线情况分类：

情况1: 一个顶点在上，两个在下
       *
      /|\
     / | \
    *--+--*
    等值线穿过两条边

情况2: 两个顶点在上，一个在下
    *-----*
     \   /
      \ /
       *
    等值线穿过两条边

情况3: 所有顶点在同侧
    *-----*
     \   /
      \ /
       *
    无等值线
```

### 5. 等值面算法（Marching Tetrahedra）

**四面体中的等值面提取**：

```cpp
// Marching Tetrahedra查找表
// 每个顶点可能在等值面上方或下方，共16种情况
static const int tetCases[16][7] = {
    {0},                    // 0000: 无交面
    {1, 0,3, 0,2, 0,1},    // 0001: 顶点0在内
    {1, 1,0, 1,2, 1,3},    // 0010: 顶点1在内
    {2, 0,2, 0,3, 1,3, 1,2}, // 0011: 顶点0,1在内
    // ... 更多情况
};

void extractIsosurfaceInTetrahedron(
    double xyz[4][3],      // 四面体顶点坐标
    double v[4],           // 顶点值
    double isoValue,       // 等值面值
    VertexArray *va)       // 输出顶点数组
{
    // 计算每个顶点的符号
    int sign = 0;
    for(int i = 0; i < 4; i++) {
        if(v[i] >= isoValue) sign |= (1 << i);
    }

    // 查表获取三角形配置
    const int *caseEntry = tetCases[sign];
    int numTriangles = caseEntry[0];

    // 生成三角形
    for(int t = 0; t < numTriangles; t++) {
        double triXYZ[3][3];

        for(int i = 0; i < 3; i++) {
            // 获取边的两个端点
            int e1 = caseEntry[1 + t*6 + i*2];
            int e2 = caseEntry[1 + t*6 + i*2 + 1];

            // 计算交点
            double t = (isoValue - v[e1]) / (v[e2] - v[e1]);
            for(int j = 0; j < 3; j++) {
                triXYZ[i][j] = xyz[e1][j] + t * (xyz[e2][j] - xyz[e1][j]);
            }
        }

        // 计算法线
        double n[3];
        computeTriangleNormal(triXYZ, n);

        // 添加到顶点数组
        unsigned int color = getColorForValue(isoValue);
        for(int i = 0; i < 3; i++) {
            va->add(triXYZ[i][0], triXYZ[i][1], triXYZ[i][2],
                    n[0], n[1], n[2], color);
        }
    }
}
```

### 6. 向量场渲染

```cpp
// 生成箭头几何
void createArrowGeometry(
    double base[3],        // 箭头起点
    double dir[3],         // 方向向量
    double length,         // 长度
    int arrowType,         // 箭头类型
    VertexArray *va)       // 输出
{
    // 归一化方向
    double mag = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
    if(mag < 1e-15) return;

    double d[3] = {dir[0]/mag, dir[1]/mag, dir[2]/mag};

    // 计算端点
    double tip[3] = {
        base[0] + length * d[0],
        base[1] + length * d[1],
        base[2] + length * d[2]
    };

    switch(arrowType) {
    case VectorType::Segment:
        // 简单线段
        va->add(base[0], base[1], base[2], /*color*/);
        va->add(tip[0], tip[1], tip[2], /*color*/);
        break;

    case VectorType::Arrow3D:
        // 3D箭头（圆锥+圆柱）
        createCone(tip, d, length * 0.2, length * 0.1, va);
        createCylinder(base, tip, length * 0.03, va);
        break;

    case VectorType::Pyramid:
        // 金字塔形箭头
        createPyramid(base, tip, length * 0.1, va);
        break;
    }
}

// 创建圆锥（箭头头部）
void createCone(double apex[3], double axis[3],
                double height, double radius,
                VertexArray *va) {
    // 计算正交基
    double u[3], v[3];
    computeOrthogonalBasis(axis, u, v);

    // 圆锥底面圆心
    double center[3] = {
        apex[0] - height * axis[0],
        apex[1] - height * axis[1],
        apex[2] - height * axis[2]
    };

    // 生成三角形扇形
    int nSeg = 12;
    for(int i = 0; i < nSeg; i++) {
        double a1 = 2 * M_PI * i / nSeg;
        double a2 = 2 * M_PI * (i + 1) / nSeg;

        double p1[3], p2[3];
        for(int j = 0; j < 3; j++) {
            p1[j] = center[j] + radius * (std::cos(a1)*u[j] + std::sin(a1)*v[j]);
            p2[j] = center[j] + radius * (std::cos(a2)*u[j] + std::sin(a2)*v[j]);
        }

        // 添加三角形
        va->add(apex[0], apex[1], apex[2], /*normal, color*/);
        va->add(p1[0], p1[1], p1[2], /*normal, color*/);
        va->add(p2[0], p2[1], p2[2], /*normal, color*/);
    }
}
```

---

## 练习作业

### 基础练习

**练习1**：理解渲染流程

```cpp
// 追踪以下代码的执行流程
PView *view = PView::list[0];

// 当选项改变时触发重新渲染
view->setChanged(true);

// 填充顶点数组
view->fillVertexArrays();

// 绘制
drawView(view);

// 问题：
// 1. setChanged(true)触发了什么操作？
// 2. fillVertexArrays中如何处理不同的元素类型？
// 3. drawView如何使用顶点数组？
```

### 进阶练习

**练习2**：实现简单的等值线提取

```cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>

// 2D等值线提取器
class IsolineExtractor {
public:
    struct Point {
        double x, y;
    };

    struct Line {
        Point p1, p2;
    };

private:
    // 网格数据
    int nx, ny;              // 网格尺寸
    double xmin, xmax;       // x范围
    double ymin, ymax;       // y范围
    std::vector<double> values;  // 节点值

public:
    IsolineExtractor(int _nx, int _ny,
                     double _xmin, double _xmax,
                     double _ymin, double _ymax)
        : nx(_nx), ny(_ny),
          xmin(_xmin), xmax(_xmax),
          ymin(_ymin), ymax(_ymax),
          values(_nx * _ny, 0.0) {}

    // 设置节点值
    void setValue(int i, int j, double v) {
        if(i >= 0 && i < nx && j >= 0 && j < ny) {
            values[j * nx + i] = v;
        }
    }

    // 获取节点值
    double getValue(int i, int j) const {
        return values[j * nx + i];
    }

    // 获取节点坐标
    void getCoord(int i, int j, double &x, double &y) const {
        x = xmin + i * (xmax - xmin) / (nx - 1);
        y = ymin + j * (ymax - ymin) / (ny - 1);
    }

    // 生成测试数据
    void generateTestData() {
        for(int j = 0; j < ny; j++) {
            for(int i = 0; i < nx; i++) {
                double x, y;
                getCoord(i, j, x, y);
                // 使用sin函数作为测试场
                setValue(i, j, std::sin(2*M_PI*x) * std::sin(2*M_PI*y));
            }
        }
    }

    // 提取等值线（Marching Squares）
    std::vector<Line> extractIsoline(double isoValue) {
        std::vector<Line> lines;

        // 遍历每个网格单元
        for(int j = 0; j < ny - 1; j++) {
            for(int i = 0; i < nx - 1; i++) {
                // 获取四个角点的值
                double v[4] = {
                    getValue(i, j),
                    getValue(i+1, j),
                    getValue(i+1, j+1),
                    getValue(i, j+1)
                };

                // 获取四个角点的坐标
                double x[4], y[4];
                getCoord(i, j, x[0], y[0]);
                getCoord(i+1, j, x[1], y[1]);
                getCoord(i+1, j+1, x[2], y[2]);
                getCoord(i, j+1, x[3], y[3]);

                // 计算case编号
                int caseNum = 0;
                for(int k = 0; k < 4; k++) {
                    if(v[k] >= isoValue) caseNum |= (1 << k);
                }

                // 处理各种情况
                std::vector<Point> edgePoints;
                extractEdgePoints(x, y, v, isoValue, caseNum, edgePoints);

                // 将边缘点配对成线段
                for(size_t k = 0; k < edgePoints.size(); k += 2) {
                    if(k + 1 < edgePoints.size()) {
                        Line l = {edgePoints[k], edgePoints[k+1]};
                        lines.push_back(l);
                    }
                }
            }
        }

        return lines;
    }

    // 导出为SVG
    void exportSVG(const std::string &filename,
                   const std::vector<std::vector<Line>> &isoLines,
                   const std::vector<double> &isoValues) {
        std::ofstream file(filename);

        int width = 500, height = 500;
        file << "<?xml version=\"1.0\"?>\n";
        file << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
             << "width=\"" << width << "\" height=\"" << height << "\">\n";

        // 背景
        file << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";

        // 颜色表
        const char *colors[] = {
            "#0000FF", "#0080FF", "#00FFFF", "#00FF80",
            "#00FF00", "#80FF00", "#FFFF00", "#FF8000",
            "#FF0000", "#FF0080"
        };

        // 绘制等值线
        for(size_t i = 0; i < isoLines.size(); i++) {
            const char *color = colors[i % 10];

            for(const auto &line : isoLines[i]) {
                double sx = (line.p1.x - xmin) / (xmax - xmin) * width;
                double sy = height - (line.p1.y - ymin) / (ymax - ymin) * height;
                double ex = (line.p2.x - xmin) / (xmax - xmin) * width;
                double ey = height - (line.p2.y - ymin) / (ymax - ymin) * height;

                file << "<line x1=\"" << sx << "\" y1=\"" << sy
                     << "\" x2=\"" << ex << "\" y2=\"" << ey
                     << "\" stroke=\"" << color << "\" stroke-width=\"1\"/>\n";
            }
        }

        file << "</svg>\n";
        file.close();

        std::cout << "Exported to " << filename << "\n";
    }

private:
    // 边界交点提取
    void extractEdgePoints(double x[4], double y[4], double v[4],
                          double isoValue, int caseNum,
                          std::vector<Point> &points) {
        // 边的定义：(顶点i, 顶点j)
        static const int edges[4][2] = {{0,1}, {1,2}, {2,3}, {3,0}};

        // Marching Squares查找表
        // 定义每种情况下需要连接的边
        static const int edgeTable[16][5] = {
            {0},                    // 0: 无
            {2, 0, 3},              // 1
            {2, 0, 1},              // 2
            {2, 1, 3},              // 3
            {2, 1, 2},              // 4
            {4, 0, 1, 2, 3},        // 5 (歧义情况)
            {2, 0, 2},              // 6
            {2, 2, 3},              // 7
            {2, 2, 3},              // 8
            {2, 0, 2},              // 9
            {4, 0, 3, 1, 2},        // 10 (歧义情况)
            {2, 1, 2},              // 11
            {2, 1, 3},              // 12
            {2, 0, 1},              // 13
            {2, 0, 3},              // 14
            {0}                     // 15: 无
        };

        int numEdges = edgeTable[caseNum][0];

        for(int i = 0; i < numEdges; i++) {
            int edgeIdx = edgeTable[caseNum][1 + i];
            int i1 = edges[edgeIdx][0];
            int i2 = edges[edgeIdx][1];

            // 线性插值计算交点
            double t = (isoValue - v[i1]) / (v[i2] - v[i1]);
            Point p;
            p.x = x[i1] + t * (x[i2] - x[i1]);
            p.y = y[i1] + t * (y[i2] - y[i1]);

            points.push_back(p);
        }
    }
};

int main() {
    // 创建50x50的网格
    IsolineExtractor extractor(50, 50, 0.0, 1.0, 0.0, 1.0);

    // 生成测试数据
    extractor.generateTestData();

    // 提取多条等值线
    std::vector<double> isoValues;
    std::vector<std::vector<IsolineExtractor::Line>> allLines;

    for(int i = -9; i <= 9; i += 2) {
        double v = i * 0.1;
        isoValues.push_back(v);
        allLines.push_back(extractor.extractIsoline(v));

        std::cout << "Isoline at " << v << ": "
                  << allLines.back().size() << " segments\n";
    }

    // 导出SVG
    extractor.exportSVG("isolines.svg", allLines, isoValues);

    return 0;
}
```

**练习3**：实现简单的颜色映射渲染

```cpp
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>

// 简单的PPM图像渲染器
class ScalarFieldRenderer {
    int width, height;
    std::vector<unsigned char> pixels;

    // 数据范围
    double dataMin, dataMax;

public:
    ScalarFieldRenderer(int w, int h)
        : width(w), height(h), pixels(w * h * 3, 0) {}

    // 渲染标量场
    void renderScalarField(
        std::function<double(double, double)> field,
        double xmin, double xmax,
        double ymin, double ymax)
    {
        // 第一遍：计算数据范围
        dataMin = 1e30;
        dataMax = -1e30;

        for(int j = 0; j < height; j++) {
            for(int i = 0; i < width; i++) {
                double x = xmin + (xmax - xmin) * i / (width - 1);
                double y = ymin + (ymax - ymin) * j / (height - 1);
                double v = field(x, y);
                dataMin = std::min(dataMin, v);
                dataMax = std::max(dataMax, v);
            }
        }

        std::cout << "Data range: [" << dataMin << ", " << dataMax << "]\n";

        // 第二遍：渲染
        for(int j = 0; j < height; j++) {
            for(int i = 0; i < width; i++) {
                double x = xmin + (xmax - xmin) * i / (width - 1);
                double y = ymin + (ymax - ymin) * j / (height - 1);
                double v = field(x, y);

                // 归一化
                double t = (v - dataMin) / (dataMax - dataMin);

                // 颜色映射
                unsigned char r, g, b;
                jetColormap(t, r, g, b);

                // 翻转y轴
                int idx = ((height - 1 - j) * width + i) * 3;
                pixels[idx] = r;
                pixels[idx + 1] = g;
                pixels[idx + 2] = b;
            }
        }
    }

    // 叠加等值线
    void overlayIsolines(
        std::function<double(double, double)> field,
        double xmin, double xmax,
        double ymin, double ymax,
        int numIso)
    {
        double dv = (dataMax - dataMin) / (numIso + 1);

        for(int j = 0; j < height - 1; j++) {
            for(int i = 0; i < width - 1; i++) {
                double x0 = xmin + (xmax - xmin) * i / (width - 1);
                double y0 = ymin + (ymax - ymin) * j / (height - 1);
                double x1 = xmin + (xmax - xmin) * (i + 1) / (width - 1);
                double y1 = ymin + (ymax - ymin) * (j + 1) / (height - 1);

                double v00 = field(x0, y0);
                double v10 = field(x1, y0);
                double v01 = field(x0, y1);
                double v11 = field(x1, y1);

                // 检查每条等值线
                for(int k = 1; k <= numIso; k++) {
                    double isoVal = dataMin + k * dv;

                    // 检查是否有等值线穿过此单元
                    bool hasIso = false;
                    if((v00 - isoVal) * (v10 - isoVal) < 0) hasIso = true;
                    if((v10 - isoVal) * (v11 - isoVal) < 0) hasIso = true;
                    if((v11 - isoVal) * (v01 - isoVal) < 0) hasIso = true;
                    if((v01 - isoVal) * (v00 - isoVal) < 0) hasIso = true;

                    if(hasIso) {
                        // 用黑色标记等值线
                        int idx = ((height - 1 - j) * width + i) * 3;
                        pixels[idx] = 0;
                        pixels[idx + 1] = 0;
                        pixels[idx + 2] = 0;
                    }
                }
            }
        }
    }

    // 保存为PPM
    void savePPM(const std::string &filename) {
        std::ofstream file(filename, std::ios::binary);
        file << "P6\n" << width << " " << height << "\n255\n";
        file.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
        file.close();
        std::cout << "Saved to " << filename << "\n";
    }

private:
    // Jet颜色映射
    void jetColormap(double t, unsigned char &r, unsigned char &g, unsigned char &b) {
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
    ScalarFieldRenderer renderer(400, 400);

    // 定义测试场
    auto field = [](double x, double y) -> double {
        return std::sin(3*M_PI*x) * std::cos(3*M_PI*y) +
               0.5*std::sin(M_PI*x*2) * std::sin(M_PI*y*2);
    };

    // 渲染标量场
    renderer.renderScalarField(field, 0, 1, 0, 1);

    // 叠加等值线
    renderer.overlayIsolines(field, 0, 1, 0, 1, 10);

    // 保存
    renderer.savePPM("scalar_with_isolines.ppm");

    return 0;
}
```

### 挑战练习

**练习4**：实现向量场可视化

```cpp
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>

// 向量场渲染器
class VectorFieldRenderer {
    int width, height;
    std::vector<unsigned char> pixels;

public:
    VectorFieldRenderer(int w, int h)
        : width(w), height(h), pixels(w * h * 3, 255) {}

    // 绘制箭头
    void drawArrow(double x0, double y0, double x1, double y1,
                   unsigned char r, unsigned char g, unsigned char b) {
        // Bresenham直线算法
        int ix0 = (int)x0, iy0 = (int)y0;
        int ix1 = (int)x1, iy1 = (int)y1;

        int dx = std::abs(ix1 - ix0);
        int dy = std::abs(iy1 - iy0);
        int sx = (ix0 < ix1) ? 1 : -1;
        int sy = (iy0 < iy1) ? 1 : -1;
        int err = dx - dy;

        while(true) {
            setPixel(ix0, iy0, r, g, b);

            if(ix0 == ix1 && iy0 == iy1) break;

            int e2 = 2 * err;
            if(e2 > -dy) { err -= dy; ix0 += sx; }
            if(e2 < dx) { err += dx; iy0 += sy; }
        }

        // 绘制箭头头部
        double angle = std::atan2(y1 - y0, x1 - x0);
        double headLen = 5;
        double headAngle = M_PI / 6;

        double ax1 = x1 - headLen * std::cos(angle - headAngle);
        double ay1 = y1 - headLen * std::sin(angle - headAngle);
        double ax2 = x1 - headLen * std::cos(angle + headAngle);
        double ay2 = y1 - headLen * std::sin(angle + headAngle);

        drawLine(x1, y1, ax1, ay1, r, g, b);
        drawLine(x1, y1, ax2, ay2, r, g, b);
    }

    // 渲染向量场
    void renderVectorField(
        std::function<void(double, double, double&, double&)> field,
        double xmin, double xmax,
        double ymin, double ymax,
        int nx, int ny,
        double scale = 1.0)
    {
        // 计算最大速度用于归一化
        double maxMag = 0;
        for(int j = 0; j < ny; j++) {
            for(int i = 0; i < nx; i++) {
                double x = xmin + (xmax - xmin) * i / (nx - 1);
                double y = ymin + (ymax - ymin) * j / (ny - 1);
                double vx, vy;
                field(x, y, vx, vy);
                double mag = std::sqrt(vx*vx + vy*vy);
                maxMag = std::max(maxMag, mag);
            }
        }

        if(maxMag < 1e-10) return;

        // 网格间距
        double dx = (xmax - xmin) / (nx - 1);
        double dy = (ymax - ymin) / (ny - 1);
        double cellSize = std::min(dx, dy);

        // 绘制向量
        for(int j = 0; j < ny; j++) {
            for(int i = 0; i < nx; i++) {
                double x = xmin + (xmax - xmin) * i / (nx - 1);
                double y = ymin + (ymax - ymin) * j / (ny - 1);

                double vx, vy;
                field(x, y, vx, vy);

                double mag = std::sqrt(vx*vx + vy*vy);
                if(mag < 1e-10) continue;

                // 归一化后缩放
                double arrowLen = scale * cellSize * 0.8 * (mag / maxMag);
                double endX = x + arrowLen * vx / mag;
                double endY = y + arrowLen * vy / mag;

                // 转换为像素坐标
                double px0 = (x - xmin) / (xmax - xmin) * (width - 1);
                double py0 = (1 - (y - ymin) / (ymax - ymin)) * (height - 1);
                double px1 = (endX - xmin) / (xmax - xmin) * (width - 1);
                double py1 = (1 - (endY - ymin) / (ymax - ymin)) * (height - 1);

                // 根据速度大小着色
                unsigned char r, g, b;
                magToColor(mag / maxMag, r, g, b);

                drawArrow(px0, py0, px1, py1, r, g, b);
            }
        }
    }

    // 保存为PPM
    void savePPM(const std::string &filename) {
        std::ofstream file(filename, std::ios::binary);
        file << "P6\n" << width << " " << height << "\n255\n";
        file.write(reinterpret_cast<char*>(pixels.data()), pixels.size());
        file.close();
        std::cout << "Saved to " << filename << "\n";
    }

private:
    void setPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
        if(x < 0 || x >= width || y < 0 || y >= height) return;
        int idx = (y * width + x) * 3;
        pixels[idx] = r;
        pixels[idx + 1] = g;
        pixels[idx + 2] = b;
    }

    void drawLine(double x0, double y0, double x1, double y1,
                  unsigned char r, unsigned char g, unsigned char b) {
        int ix0 = (int)x0, iy0 = (int)y0;
        int ix1 = (int)x1, iy1 = (int)y1;

        int dx = std::abs(ix1 - ix0);
        int dy = std::abs(iy1 - iy0);
        int sx = (ix0 < ix1) ? 1 : -1;
        int sy = (iy0 < iy1) ? 1 : -1;
        int err = dx - dy;

        while(true) {
            setPixel(ix0, iy0, r, g, b);
            if(ix0 == ix1 && iy0 == iy1) break;
            int e2 = 2 * err;
            if(e2 > -dy) { err -= dy; ix0 += sx; }
            if(e2 < dx) { err += dx; iy0 += sy; }
        }
    }

    void magToColor(double t, unsigned char &r, unsigned char &g, unsigned char &b) {
        // 蓝→青→绿→黄→红
        if(t < 0.25) {
            r = 0; g = (unsigned char)(t*4*255); b = 255;
        } else if(t < 0.5) {
            r = 0; g = 255; b = (unsigned char)((0.5-t)*4*255);
        } else if(t < 0.75) {
            r = (unsigned char)((t-0.5)*4*255); g = 255; b = 0;
        } else {
            r = 255; g = (unsigned char)((1-t)*4*255); b = 0;
        }
    }
};

int main() {
    VectorFieldRenderer renderer(500, 500);

    // 定义旋涡向量场
    auto vortexField = [](double x, double y, double &vx, double &vy) {
        // 中心在(0.5, 0.5)的旋涡
        double cx = 0.5, cy = 0.5;
        double dx = x - cx, dy = y - cy;
        double r = std::sqrt(dx*dx + dy*dy);

        if(r < 0.01) {
            vx = vy = 0;
            return;
        }

        // 旋转场
        double strength = std::exp(-r*r*10);
        vx = -dy * strength / r;
        vy = dx * strength / r;
    };

    // 渲染
    renderer.renderVectorField(vortexField, 0, 1, 0, 1, 20, 20, 1.5);
    renderer.savePPM("vector_field.ppm");

    return 0;
}
```

---

## 知识图谱

```text
可视化渲染
│
├── 渲染管线
│   ├── PViewData → 数据源
│   ├── PViewOptions → 配置
│   ├── fillVertexArrays() → 处理
│   └── OpenGL → 绘制
│
├── VertexArray
│   ├── 顶点位置
│   ├── 法线向量
│   ├── 颜色数据
│   └── 纹理坐标
│
├── 等值线/面算法
│   ├── Marching Squares (2D)
│   ├── Marching Tetrahedra (3D)
│   └── 查找表优化
│
├── 向量场渲染
│   ├── 线段/箭头
│   ├── 流线追踪
│   └── LIC纹理
│
└── 颜色映射
    ├── 连续着色
    ├── 离散着色
    └── 等值线叠加
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 |
|------|----------|--------|
| VertexArray.h | 顶点数组类定义 | ~150行 |
| VertexArray.cpp | 顶点数组实现 | ~300行 |
| PViewVertexArrays.cpp | 视图渲染实现 | ~2000行 |
| drawPost.cpp | OpenGL绘制 | ~800行 |

---

## 今日检查点

- [ ] 理解Gmsh渲染管线的整体架构
- [ ] 能解释VertexArray的数据组织方式
- [ ] 理解Marching Squares等值线算法
- [ ] 能描述向量场的箭头几何生成
- [ ] 理解颜色映射在渲染中的应用

---

## 导航

- 上一天：[Day 59 - 数据插值与空间查询](day-59.md)
- 下一天：[Day 61 - 后处理插件系统](day-61.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
