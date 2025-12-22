# Day 51：数值积分

## 学习目标
掌握Gmsh中高斯数值积分的实现原理，理解不同单元类型的积分点分布和权重计算。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习高斯积分基本原理 |
| 10:00-11:00 | 1h | 阅读GaussIntegration接口 |
| 11:00-12:00 | 1h | 分析三角形和四面体积分点 |
| 14:00-15:00 | 1h | 完成练习并总结 |

---

## 上午学习内容

### 1. 高斯积分基本原理

**数值积分问题**：
```
       b
∫ f(x)dx ≈ Σ wᵢ·f(xᵢ)
a          i=1..n

其中：
- xᵢ 是积分点（高斯点）
- wᵢ 是积分权重
- n 是积分点数
```

**高斯-勒让德积分（1D）**：
```
       1
∫ f(x)dx ≈ Σ wᵢ·f(xᵢ)
-1         i=1..n

- n个积分点可以精确积分2n-1阶多项式
- 积分点是Legendre多项式的根
- 权重由正交性条件确定
```

**常用积分点（标准区间[-1,1]）**：

| n | 积分点 xᵢ | 权重 wᵢ | 精度阶 |
|---|-----------|---------|--------|
| 1 | 0 | 2 | 1 |
| 2 | ±1/√3 | 1, 1 | 3 |
| 3 | 0, ±√(3/5) | 8/9, 5/9, 5/9 | 5 |

### 2. GaussIntegration接口

**文件位置**：`src/numeric/GaussIntegration.h`

```cpp
// 核心类：提供各类单元的高斯积分点和权重
class GaussIntegration {
public:
    // 获取线段积分（1D）
    static int getGaussPointsLine(int order, fullMatrix<double> &pts,
                                  fullVector<double> &weights);

    // 获取三角形积分（2D）
    static int getGaussPointsTri(int order, fullMatrix<double> &pts,
                                 fullVector<double> &weights);

    // 获取四边形积分（2D）
    static int getGaussPointsQuad(int order, fullMatrix<double> &pts,
                                  fullVector<double> &weights);

    // 获取四面体积分（3D）
    static int getGaussPointsTet(int order, fullMatrix<double> &pts,
                                 fullVector<double> &weights);

    // 获取六面体积分（3D）
    static int getGaussPointsHex(int order, fullMatrix<double> &pts,
                                 fullVector<double> &weights);

    // 获取棱柱积分（3D）
    static int getGaussPointsPri(int order, fullMatrix<double> &pts,
                                 fullVector<double> &weights);

    // 获取金字塔积分（3D）
    static int getGaussPointsPyr(int order, fullMatrix<double> &pts,
                                 fullVector<double> &weights);
};
```

### 3. 1D Gauss-Legendre积分点

**文件位置**：`src/numeric/GaussLegendre1D.h`

```cpp
// 预计算的Gauss-Legendre积分点和权重
// 范围：[-1, 1]

// 1点公式（精确到1阶）
static double GaussLegendre1D_1[1][2] = {
    {0.0, 2.0}  // {点, 权重}
};

// 2点公式（精确到3阶）
static double GaussLegendre1D_2[2][2] = {
    {-0.577350269189626, 1.0},  // -1/√3
    { 0.577350269189626, 1.0}   //  1/√3
};

// 3点公式（精确到5阶）
static double GaussLegendre1D_3[3][2] = {
    {-0.774596669241483, 0.555555555555556},  // -√(3/5)
    { 0.0,               0.888888888888889},  // 0
    { 0.774596669241483, 0.555555555555556}   //  √(3/5)
};

// ... 更高阶公式
```

### 4. 三角形积分点

**文件位置**：`src/numeric/GaussQuadratureTri.cpp`

**参考三角形**：
```
     (0,1)
       2
       |\
       | \
       |  \
       |___\
       0    1
    (0,0)  (1,0)

面积 = 0.5
```

**常用积分公式**：

```cpp
// 1点公式（重心规则，精确到1阶）
// 积分点：重心 (1/3, 1/3)
// 权重：0.5（面积）

// 3点公式（精确到2阶）
// 积分点：三边中点
static double TriGauss3[3][3] = {
    {0.5, 0.0, 1.0/6.0},   // (u, v, w)
    {0.5, 0.5, 1.0/6.0},
    {0.0, 0.5, 1.0/6.0}
};

// 4点公式（精确到3阶）
static double TriGauss4[4][3] = {
    {1.0/3.0, 1.0/3.0, -27.0/96.0},  // 重心（负权重！）
    {0.6, 0.2, 25.0/96.0},
    {0.2, 0.6, 25.0/96.0},
    {0.2, 0.2, 25.0/96.0}
};

// 7点公式（精确到5阶）- 常用选择
// ... 见源码
```

**积分点可视化**：

```
3点公式：               7点公式：
    2                      2
    |\                     |\
    | *                    |*\
    |  \                   * * *
    *___*                  |___*\
    0    1                 0  *  1
```

---

## 下午学习内容

### 5. 四面体积分点

**文件位置**：`src/numeric/GaussQuadratureTet.cpp`

**参考四面体**：
```
顶点：
0: (0, 0, 0)
1: (1, 0, 0)
2: (0, 1, 0)
3: (0, 0, 1)

体积 = 1/6
```

**常用积分公式**：

```cpp
// 1点公式（重心规则，精确到1阶）
// 积分点：(1/4, 1/4, 1/4)
// 权重：1/6

// 4点公式（精确到2阶）
// 积分点分布在四个子四面体的重心
static double TetGauss4[4][4] = {
    // u, v, w, weight
    {0.138196..., 0.138196..., 0.138196..., 1.0/24.0},
    {0.585410..., 0.138196..., 0.138196..., 1.0/24.0},
    {0.138196..., 0.585410..., 0.138196..., 1.0/24.0},
    {0.138196..., 0.138196..., 0.585410..., 1.0/24.0}
};

// 5点公式（精确到3阶）
// 包含重心点
static double TetGauss5[5][4] = {
    {0.25, 0.25, 0.25, -4.0/30.0},  // 重心（负权重）
    {0.5,  1.0/6.0, 1.0/6.0, 9.0/120.0},
    // ...
};

// 15点公式（精确到5阶）- 常用选择
// ... 见源码
```

### 6. 积分精度要求

**有限元中的积分需求**：

```
刚度矩阵计算：
K = ∫ BᵀDB dΩ

其中：
- B：应变矩阵（含形函数导数）
- D：材料矩阵
- dΩ：体积微元

对于p阶元素：
- B中含p-1阶多项式
- BᵀDB含2(p-1)阶多项式
- 需要能精确积分2(p-1)阶的积分公式
```

**积分阶数选择指南**：

| 元素阶数 | 被积函数阶数 | 最小积分点数(三角形) |
|----------|--------------|---------------------|
| P1 | 0 | 1 |
| P2 | 2 | 3-4 |
| P3 | 4 | 6-7 |
| P4 | 6 | 12-13 |

### 7. 代码实现分析

**GaussIntegration.cpp关键代码**：

```cpp
int GaussIntegration::getGaussPointsTri(int order,
                                         fullMatrix<double> &pts,
                                         fullVector<double> &weights)
{
    // 根据所需精度选择积分公式
    int npts;
    double *data;

    if(order <= 1) {
        npts = 1;
        data = TriGauss1;
    }
    else if(order <= 2) {
        npts = 3;
        data = TriGauss3;
    }
    else if(order <= 3) {
        npts = 4;
        data = TriGauss4;
    }
    // ... 更高阶

    // 填充输出矩阵
    pts.resize(npts, 2);
    weights.resize(npts);

    for(int i = 0; i < npts; i++) {
        pts(i, 0) = data[i][0];     // u坐标
        pts(i, 1) = data[i][1];     // v坐标
        weights(i) = data[i][2];    // 权重
    }

    return npts;
}
```

---

## 练习作业

### 基础练习

**练习1**：手工验证1D高斯积分

```cpp
// 使用2点高斯积分计算 ∫₋₁¹ x² dx
// 精确值 = 2/3

// 积分点：x₁ = -1/√3, x₂ = 1/√3
// 权重：w₁ = w₂ = 1

// I ≈ w₁·f(x₁) + w₂·f(x₂)
//   = 1·(-1/√3)² + 1·(1/√3)²
//   = 1/3 + 1/3 = 2/3 ✓

// 验证：∫₋₁¹ x³ dx = 0
// I ≈ 1·(-1/√3)³ + 1·(1/√3)³
//   = -1/(3√3) + 1/(3√3) = 0 ✓
```

### 进阶练习

**练习2**：实现简单的三角形积分

```cpp
#include <iostream>
#include <cmath>

// 三角形高斯积分类
class TriangleQuadrature {
public:
    // 3点公式
    static const int npts = 3;
    double pts[3][2] = {
        {0.5, 0.0},
        {0.5, 0.5},
        {0.0, 0.5}
    };
    double weights[3] = {1.0/6.0, 1.0/6.0, 1.0/6.0};

    // 积分 f(u,v) 在参考三角形上
    template<typename Func>
    double integrate(Func f) {
        double result = 0.0;
        for(int i = 0; i < npts; i++) {
            result += weights[i] * f(pts[i][0], pts[i][1]);
        }
        return result;
    }
};

// 测试：积分常数函数（应得面积0.5）
double constant(double u, double v) { return 1.0; }

// 测试：积分线性函数 f(u,v) = u
double linear_u(double u, double v) { return u; }

// 测试：积分二次函数 f(u,v) = u*v
double quadratic(double u, double v) { return u * v; }

int main() {
    TriangleQuadrature quad;

    std::cout << "∫1 dA = " << quad.integrate(constant)
              << " (exact: 0.5)\n";

    std::cout << "∫u dA = " << quad.integrate(linear_u)
              << " (exact: 1/6 ≈ 0.1667)\n";

    std::cout << "∫uv dA = " << quad.integrate(quadratic)
              << " (exact: 1/24 ≈ 0.0417)\n";

    return 0;
}
```

**练习3**：使用Gmsh API验证积分

```cpp
// 使用Gmsh C++ API获取积分点
#include "gmsh.h"
#include <iostream>

int main(int argc, char **argv)
{
    gmsh::initialize();

    // 获取三角形的5阶积分点
    std::vector<double> pts, weights;
    gmsh::model::mesh::getIntegrationPoints(
        2,      // 元素类型（三角形）
        "Gauss5",  // 积分规则
        pts, weights);

    std::cout << "Integration points for triangle (order 5):\n";
    for(size_t i = 0; i < weights.size(); i++) {
        std::cout << "Point " << i << ": ("
                  << pts[3*i] << ", " << pts[3*i+1] << ", " << pts[3*i+2]
                  << ") weight = " << weights[i] << "\n";
    }

    gmsh::finalize();
    return 0;
}
```

### 挑战练习

**练习4**：实现物理单元上的积分

```cpp
#include <iostream>
#include <cmath>

// 在物理三角形上积分
class PhysicalTriangleIntegrator {
    // 参考三角形积分点（7点公式）
    static const int npts = 7;
    double refPts[7][2];
    double refWeights[7];

    // 物理三角形顶点
    double x[3], y[3];

public:
    PhysicalTriangleIntegrator(double x0, double y0,
                                double x1, double y1,
                                double x2, double y2) {
        x[0] = x0; y[0] = y0;
        x[1] = x1; y[1] = y1;
        x[2] = x2; y[2] = y2;

        // 初始化7点积分公式（精确到5阶）
        // 简化：使用3点公式
        refPts[0][0] = 0.5; refPts[0][1] = 0.0;
        refPts[1][0] = 0.5; refPts[1][1] = 0.5;
        refPts[2][0] = 0.0; refPts[2][1] = 0.5;
        refWeights[0] = refWeights[1] = refWeights[2] = 1.0/6.0;
    }

    // 计算Jacobian行列式
    double jacobian() {
        // J = |dx/du  dx/dv|
        //     |dy/du  dy/dv|
        //
        // 对于线性三角形：
        // x = (1-u-v)*x0 + u*x1 + v*x2
        // dx/du = x1 - x0
        // dx/dv = x2 - x0
        double dxdu = x[1] - x[0];
        double dxdv = x[2] - x[0];
        double dydu = y[1] - y[0];
        double dydv = y[2] - y[0];
        return std::abs(dxdu*dydv - dxdv*dydu);
    }

    // 参考坐标到物理坐标
    void refToPhysical(double u, double v, double &px, double &py) {
        double N0 = 1.0 - u - v;
        double N1 = u;
        double N2 = v;
        px = N0*x[0] + N1*x[1] + N2*x[2];
        py = N0*y[0] + N1*y[1] + N2*y[2];
    }

    // 积分物理空间中的函数 f(x,y)
    template<typename Func>
    double integrate(Func f) {
        double jac = jacobian();
        double result = 0.0;

        for(int i = 0; i < 3; i++) {  // 使用3点
            double px, py;
            refToPhysical(refPts[i][0], refPts[i][1], px, py);
            result += refWeights[i] * f(px, py) * jac;
        }
        return result;
    }
};

// 测试
int main() {
    // 物理三角形：(0,0), (2,0), (0,2)
    // 面积 = 2
    PhysicalTriangleIntegrator integrator(0, 0, 2, 0, 0, 2);

    // 积分常数1（应得面积）
    auto constant = [](double x, double y) { return 1.0; };
    std::cout << "Area = " << integrator.integrate(constant)
              << " (exact: 2.0)\n";

    // 积分 x（重心x坐标 * 面积 = 2/3 * 2 = 4/3）
    auto fx = [](double x, double y) { return x; };
    std::cout << "∫x dA = " << integrator.integrate(fx)
              << " (exact: 4/3 ≈ 1.333)\n";

    return 0;
}
```

---

## 知识图谱

```
数值积分系统
│
├── 理论基础
│   ├── Gauss-Legendre正交
│   ├── 积分点和权重
│   └── 精度与点数关系
│
├── GaussIntegration（主接口）
│   ├── getGaussPointsLine()
│   ├── getGaussPointsTri()
│   ├── getGaussPointsQuad()
│   ├── getGaussPointsTet()
│   └── ...
│
├── 数据存储
│   ├── GaussLegendre1D.h（1D预计算表）
│   ├── GaussQuadratureTri.cpp（三角形）
│   ├── GaussQuadratureTet.cpp（四面体）
│   └── ...
│
└── 应用场景
    ├── 有限元刚度矩阵
    ├── 质量矩阵
    ├── 载荷向量
    └── 后处理积分量
```

---

## 关键源码索引

| 文件 | 核心内容 | 特点 |
|------|----------|------|
| GaussIntegration.h | 统一接口 | 所有单元类型入口 |
| GaussIntegration.cpp | 积分点选择逻辑 | 根据阶数分发 |
| GaussLegendre1D.h | 1D积分点表 | 预计算，高精度 |
| GaussQuadratureTri.cpp | 三角形积分 | ~50KB，多种阶数 |
| GaussQuadratureTet.cpp | 四面体积分 | ~240KB，极详细 |

---

## 今日检查点

- [ ] 理解高斯积分的基本原理（点、权重、精度）
- [ ] 能解释为什么n个点能精确积分2n-1阶多项式
- [ ] 理解参考单元和物理单元积分的关系
- [ ] 能选择合适的积分阶数用于有限元计算
- [ ] 完成三角形积分的手工验证

---

## 扩展阅读

1. **Hammer积分**：另一种三角形积分方法
2. **Newton-Cotes积分**：等距点积分（对比Gauss积分）
3. **自适应积分**：根据误差自动细化

---

## 导航

- 上一天：[Day 50 - 基函数系统概览](day-50.md)
- 下一天：[Day 52 - 层次基函数](day-52.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
