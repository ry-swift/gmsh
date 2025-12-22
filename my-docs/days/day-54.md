# Day 54：Bezier基与几何表示

## 学习目标
掌握Gmsh中Bezier基函数的实现，理解de Casteljau算法和Bezier曲线/曲面在几何表示中的应用。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习Bezier曲线理论 |
| 10:00-11:00 | 1h | 阅读bezierBasis实现 |
| 11:00-12:00 | 1h | 学习de Casteljau算法 |
| 14:00-15:00 | 1h | 完成练习并总结 |

---

## 上午学习内容

### 1. Bezier曲线基础

**Bezier曲线定义**：
```
给定n+1个控制点P₀, P₁, ..., Pₙ，
n次Bezier曲线定义为：

C(t) = Σᵢ₌₀ⁿ Bᵢ,ⁿ(t) · Pᵢ,  t ∈ [0,1]

其中 Bᵢ,ⁿ(t) 是Bernstein多项式：
Bᵢ,ⁿ(t) = C(n,i) · tⁱ · (1-t)ⁿ⁻ⁱ
```

**Bernstein多项式性质**：
```
1. 非负性：Bᵢ,ⁿ(t) ≥ 0 对所有 t ∈ [0,1]
2. 归一性：Σᵢ Bᵢ,ⁿ(t) = 1
3. 对称性：Bᵢ,ⁿ(t) = Bₙ₋ᵢ,ⁿ(1-t)
4. 端点插值：C(0) = P₀, C(1) = Pₙ
5. 凸包性：曲线在控制点的凸包内
```

**常用Bernstein多项式**：

```
n=1（线性）：
B₀,₁(t) = 1-t
B₁,₁(t) = t

n=2（二次）：
B₀,₂(t) = (1-t)²
B₁,₂(t) = 2t(1-t)
B₂,₂(t) = t²

n=3（三次）：
B₀,₃(t) = (1-t)³
B₁,₃(t) = 3t(1-t)²
B₂,₃(t) = 3t²(1-t)
B₃,₃(t) = t³
```

### 2. bezierBasis类

**文件位置**：`src/numeric/bezierBasis.h`

```cpp
class bezierBasis {
public:
    // 元素类型和阶数
    int _type;
    int _order;
    int _numCoeff;      // 控制点数量
    int _dim;           // 空间维度

    // 转换矩阵
    fullMatrix<double> _matLag2Bez;  // Lagrange → Bezier
    fullMatrix<double> _matBez2Lag;  // Bezier → Lagrange

    // 子分矩阵
    std::vector<fullMatrix<double>> _subDivisor;

public:
    bezierBasis(int tag);

    // Lagrange系数转Bezier系数
    void lag2Bez(const fullVector<double> &lag,
                 fullVector<double> &bez) const;

    // Bezier系数转Lagrange系数
    void bez2Lag(const fullVector<double> &bez,
                 fullVector<double> &lag) const;

    // 子分（细分）Bezier曲线/曲面
    void subDivide(const fullVector<double> &bez,
                   std::vector<fullVector<double>> &subBez) const;

    // 计算Bezier基函数值
    void f(double u, double v, double w, double *val) const;

    // 获取提升系数（升阶）
    void getElevateMatrix(fullMatrix<double> &elevate) const;
};
```

### 3. Lagrange-Bezier转换

**为什么需要转换**：
```
Lagrange形式：
- 节点值直接可见
- 常用于有限元计算

Bezier形式：
- 凸包性质（便于边界检测）
- 子分算法稳定
- 便于Jacobian正负性检测
```

**转换矩阵**：
```cpp
// 对于P2线段（3个点）：
//
// Lagrange节点值：[f(0), f(0.5), f(1)]
// Bezier控制点：  [B₀, B₁, B₂]
//
// 关系：
// f(t) = B₀(1-t)² + B₁·2t(1-t) + B₂t²
//
// 在Lagrange节点处：
// f(0)   = B₀
// f(0.5) = 0.25B₀ + 0.5B₁ + 0.25B₂
// f(1)   = B₂
//
// 转换矩阵（Lagrange → Bezier）：
// [B₀]   [ 1    0    0  ] [f(0)  ]
// [B₁] = [-0.5  2   -0.5] [f(0.5)]
// [B₂]   [ 0    0    1  ] [f(1)  ]
```

### 4. 凸包性质的应用

```cpp
// Bezier曲线/曲面的凸包性质：
// 曲线/曲面上的所有点都在控制点的凸包内
//
// 对于Jacobian检测：
// 1. 将Jacobian多项式转为Bezier形式
// 2. 检查所有Bezier系数的符号
// 3. 如果所有系数 > 0，则Jacobian全域 > 0
// 4. 如果有系数 < 0，可能需要子分进一步检查

bool checkPositiveJacobian(const fullVector<double> &jacLag,
                            const bezierBasis *bezier) {
    fullVector<double> jacBez;
    bezier->lag2Bez(jacLag, jacBez);

    // 检查所有Bezier系数
    for(int i = 0; i < jacBez.size(); i++) {
        if(jacBez(i) < 0) {
            return false;  // 可能有负Jacobian
        }
    }
    return true;  // 确定全正
}
```

---

## 下午学习内容

### 5. de Casteljau算法

**文件位置**：`src/numeric/decasteljau.h/cpp`

**算法原理**：
```
递归细分计算Bezier曲线上的点

输入：控制点P₀, P₁, ..., Pₙ，参数t
输出：曲线点C(t)

算法：
P⁽⁰⁾ᵢ = Pᵢ
P⁽ʲ⁾ᵢ = (1-t)P⁽ʲ⁻¹⁾ᵢ + t·P⁽ʲ⁻¹⁾ᵢ₊₁

最终结果：C(t) = P⁽ⁿ⁾₀
```

**几何解释（三次）**：

```
P0 ----P01---- P1
        \      /
         P012-P12
         /    \
      P0123   P2
        |
        |
       P3

t=0.5时：
P01 = (P0+P1)/2
P12 = (P1+P2)/2
P23 = (P2+P3)/2
P012 = (P01+P12)/2
P123 = (P12+P23)/2
P0123 = (P012+P123)/2  ← 曲线上的点
```

**代码实现**：

```cpp
// decasteljau.cpp
void decasteljau(double t,
                 const std::vector<SPoint3> &controlPts,
                 SPoint3 &result)
{
    int n = controlPts.size();
    std::vector<SPoint3> pts = controlPts;

    // 递归计算
    for(int j = 1; j < n; j++) {
        for(int i = 0; i < n - j; i++) {
            pts[i] = pts[i] * (1-t) + pts[i+1] * t;
        }
    }

    result = pts[0];
}

// 同时获取子分结果
void decasteljauSplit(double t,
                      const std::vector<SPoint3> &controlPts,
                      std::vector<SPoint3> &left,
                      std::vector<SPoint3> &right)
{
    int n = controlPts.size();
    std::vector<std::vector<SPoint3>> pyramid(n);

    pyramid[0] = controlPts;

    for(int j = 1; j < n; j++) {
        pyramid[j].resize(n - j);
        for(int i = 0; i < n - j; i++) {
            pyramid[j][i] = pyramid[j-1][i] * (1-t)
                          + pyramid[j-1][i+1] * t;
        }
    }

    // 左半部分：金字塔左边
    left.resize(n);
    for(int j = 0; j < n; j++) {
        left[j] = pyramid[j][0];
    }

    // 右半部分：金字塔右边
    right.resize(n);
    for(int j = 0; j < n; j++) {
        right[j] = pyramid[n-1-j][j];
    }
}
```

### 6. Bezier曲面

**三角形Bezier面片**：
```
控制点分布（P2三角形，6个控制点）：

       P002
       / \
      /   \
   P011   P101
    /       \
   /         \
P020---P110---P200

索引约定：Pᵢⱼₖ 其中 i+j+k = n
```

**Bezier三角形基函数**：
```
B^n_ijk(u,v,w) = n!/(i!j!k!) · uⁱ · vʲ · wᵏ

其中 w = 1-u-v（重心坐标）
i+j+k = n

曲面方程：
S(u,v) = Σ B^n_ijk(u,v,1-u-v) · Pijk
```

### 7. 子分算法

**文件位置**：`src/numeric/bezierBasis.cpp`

```cpp
void bezierBasis::subDivide(const fullVector<double> &bez,
                             std::vector<fullVector<double>> &subBez) const
{
    // 根据元素类型进行子分
    // 线段：分成2段
    // 三角形：分成4个子三角形
    // 四面体：分成8个子四面体

    int numSub = _subDivisor.size();
    subBez.resize(numSub);

    for(int i = 0; i < numSub; i++) {
        // 使用预计算的子分矩阵
        _subDivisor[i].mult(bez, subBez[i]);
    }
}
```

**子分在Jacobian检测中的应用**：

```cpp
bool checkJacobianPositive(const fullVector<double> &jacBez,
                            const bezierBasis *bezier,
                            int maxLevel = 5)
{
    // 检查当前Bezier系数
    double minCoeff = jacBez.min();
    double maxCoeff = jacBez.max();

    // 情况1：所有系数为正 → 确定有效
    if(minCoeff > 0) return true;

    // 情况2：所有系数为负 → 确定无效
    if(maxCoeff < 0) return false;

    // 情况3：系数有正有负 → 需要子分进一步检查
    if(maxLevel == 0) {
        // 达到最大深度，使用保守估计
        return minCoeff >= -1e-10 * maxCoeff;
    }

    // 子分
    std::vector<fullVector<double>> subBez;
    bezier->subDivide(jacBez, subBez);

    // 递归检查每个子单元
    for(auto &sub : subBez) {
        if(!checkJacobianPositive(sub, bezier, maxLevel - 1)) {
            return false;
        }
    }
    return true;
}
```

---

## 练习作业

### 基础练习

**练习1**：手工计算Bezier曲线

```cpp
// 三次Bezier曲线
// 控制点：P0=(0,0), P1=(1,2), P2=(3,2), P3=(4,0)
// 计算 t=0.5 时的曲线点

// 使用de Casteljau算法：
// P01 = (P0+P1)/2 = (0.5, 1)
// P12 = (P1+P2)/2 = (2, 2)
// P23 = (P2+P3)/2 = (3.5, 1)
// P012 = (P01+P12)/2 = (1.25, 1.5)
// P123 = (P12+P23)/2 = (2.75, 1.5)
// P0123 = (P012+P123)/2 = (2, 1.5) ← 答案

// 验证：使用Bernstein公式
// C(0.5) = 0.125P0 + 0.375P1 + 0.375P2 + 0.125P3
//        = 0.125(0,0) + 0.375(1,2) + 0.375(3,2) + 0.125(4,0)
//        = (0, 0) + (0.375, 0.75) + (1.125, 0.75) + (0.5, 0)
//        = (2, 1.5) ✓
```

### 进阶练习

**练习2**：实现Bezier曲线类

```cpp
#include <iostream>
#include <vector>
#include <cmath>

struct Point2D {
    double x, y;
    Point2D(double _x = 0, double _y = 0) : x(_x), y(_y) {}
    Point2D operator+(const Point2D &p) const {
        return Point2D(x + p.x, y + p.y);
    }
    Point2D operator*(double s) const {
        return Point2D(x * s, y * s);
    }
};

class BezierCurve {
    std::vector<Point2D> controlPts;
    int degree;

public:
    BezierCurve(const std::vector<Point2D> &pts)
        : controlPts(pts), degree(pts.size() - 1) {}

    // Bernstein多项式
    double bernstein(int i, int n, double t) const {
        // 组合数
        int c = 1;
        for(int j = 0; j < i; j++) {
            c = c * (n - j) / (j + 1);
        }
        return c * std::pow(t, i) * std::pow(1-t, n-i);
    }

    // 直接求值
    Point2D evaluate(double t) const {
        Point2D result(0, 0);
        for(int i = 0; i <= degree; i++) {
            double b = bernstein(i, degree, t);
            result = result + controlPts[i] * b;
        }
        return result;
    }

    // de Casteljau算法
    Point2D deCasteljau(double t) const {
        std::vector<Point2D> pts = controlPts;
        int n = pts.size();

        for(int j = 1; j < n; j++) {
            for(int i = 0; i < n - j; i++) {
                pts[i] = pts[i] * (1-t) + pts[i+1] * t;
            }
        }
        return pts[0];
    }

    // 子分曲线
    void split(double t,
               BezierCurve &left,
               BezierCurve &right) const {
        int n = controlPts.size();
        std::vector<std::vector<Point2D>> pyramid(n);
        pyramid[0] = controlPts;

        for(int j = 1; j < n; j++) {
            pyramid[j].resize(n - j);
            for(int i = 0; i < n - j; i++) {
                pyramid[j][i] = pyramid[j-1][i] * (1-t)
                              + pyramid[j-1][i+1] * t;
            }
        }

        std::vector<Point2D> leftPts(n), rightPts(n);
        for(int j = 0; j < n; j++) {
            leftPts[j] = pyramid[j][0];
            rightPts[j] = pyramid[n-1-j][j];
        }

        left = BezierCurve(leftPts);
        right = BezierCurve(rightPts);
    }

    // 计算曲线导数在t处的值
    Point2D derivative(double t) const {
        if(degree == 0) return Point2D(0, 0);

        // 导数的控制点
        std::vector<Point2D> derivPts(degree);
        for(int i = 0; i < degree; i++) {
            derivPts[i] = (controlPts[i+1] + controlPts[i] * (-1)) * degree;
        }

        BezierCurve derivCurve(derivPts);
        return derivCurve.evaluate(t);
    }

    // 计算曲线长度（数值积分）
    double length(int segments = 100) const {
        double len = 0;
        Point2D prev = controlPts[0];

        for(int i = 1; i <= segments; i++) {
            double t = double(i) / segments;
            Point2D curr = evaluate(t);
            double dx = curr.x - prev.x;
            double dy = curr.y - prev.y;
            len += std::sqrt(dx*dx + dy*dy);
            prev = curr;
        }
        return len;
    }
};

int main() {
    // 三次Bezier曲线
    std::vector<Point2D> pts = {
        Point2D(0, 0),
        Point2D(1, 2),
        Point2D(3, 2),
        Point2D(4, 0)
    };
    BezierCurve curve(pts);

    // 测试求值
    std::cout << "Bezier curve evaluation:\n";
    for(double t = 0; t <= 1.0; t += 0.25) {
        Point2D p1 = curve.evaluate(t);
        Point2D p2 = curve.deCasteljau(t);
        std::cout << "t=" << t << ": (" << p1.x << ", " << p1.y << ")";
        std::cout << " [deCasteljau: (" << p2.x << ", " << p2.y << ")]\n";
    }

    // 测试子分
    std::cout << "\nSplit at t=0.5:\n";
    BezierCurve left(pts), right(pts);
    curve.split(0.5, left, right);

    Point2D midLeft = left.evaluate(1.0);
    Point2D midRight = right.evaluate(0.0);
    Point2D mid = curve.evaluate(0.5);
    std::cout << "Left end: (" << midLeft.x << ", " << midLeft.y << ")\n";
    std::cout << "Right start: (" << midRight.x << ", " << midRight.y << ")\n";
    std::cout << "Original at 0.5: (" << mid.x << ", " << mid.y << ")\n";

    // 曲线长度
    std::cout << "\nCurve length: " << curve.length() << "\n";

    return 0;
}
```

**练习3**：实现Bezier三角形

```cpp
#include <iostream>
#include <vector>
#include <cmath>

struct Point3D {
    double x, y, z;
    Point3D(double _x=0, double _y=0, double _z=0) : x(_x), y(_y), z(_z) {}
    Point3D operator+(const Point3D &p) const {
        return Point3D(x+p.x, y+p.y, z+p.z);
    }
    Point3D operator*(double s) const {
        return Point3D(x*s, y*s, z*s);
    }
};

// P2 Bezier三角形（6个控制点）
class BezierTriangle {
    Point3D P[6];  // P002, P011, P101, P020, P110, P200

public:
    // 设置控制点
    void setControlPoint(int idx, const Point3D &p) {
        P[idx] = p;
    }

    // 计算三角形Bernstein基函数
    // B^2_ijk(u,v) = 2!/(i!j!k!) * u^i * v^j * w^k
    // 其中 w = 1-u-v, i+j+k = 2

    double B002(double u, double v) const {
        double w = 1-u-v;
        return w*w;
    }
    double B011(double u, double v) const {
        double w = 1-u-v;
        return 2*v*w;
    }
    double B101(double u, double v) const {
        double w = 1-u-v;
        return 2*u*w;
    }
    double B020(double u, double v) const {
        return v*v;
    }
    double B110(double u, double v) const {
        return 2*u*v;
    }
    double B200(double u, double v) const {
        return u*u;
    }

    // 求值
    Point3D evaluate(double u, double v) const {
        return P[0] * B002(u,v)   // P002
             + P[1] * B011(u,v)   // P011
             + P[2] * B101(u,v)   // P101
             + P[3] * B020(u,v)   // P020
             + P[4] * B110(u,v)   // P110
             + P[5] * B200(u,v);  // P200
    }

    // de Casteljau算法（三角形版本）
    Point3D deCasteljau(double u, double v) const {
        double w = 1-u-v;

        // 第一层：3个点
        Point3D Q00 = P[0]*w + P[1]*v + P[2]*u;  // 边002-011-101
        Point3D Q01 = P[1]*w + P[3]*v + P[4]*u;  // 边011-020-110
        Point3D Q10 = P[2]*w + P[4]*v + P[5]*u;  // 边101-110-200

        // 第二层：1个点
        Point3D R = Q00*w + Q01*v + Q10*u;

        return R;
    }
};

int main() {
    BezierTriangle tri;

    // 设置一个弯曲的三角形面片
    // 顶点在z=0平面，中点抬高
    tri.setControlPoint(0, Point3D(0, 0, 0));       // P002: 顶点0
    tri.setControlPoint(3, Point3D(0, 1, 0));       // P020: 顶点1
    tri.setControlPoint(5, Point3D(1, 0, 0));       // P200: 顶点2
    tri.setControlPoint(1, Point3D(0, 0.5, 0.2));   // P011: 边01中点，抬高
    tri.setControlPoint(4, Point3D(0.5, 0.5, 0.2)); // P110: 边12中点，抬高
    tri.setControlPoint(2, Point3D(0.5, 0, 0.2));   // P101: 边20中点，抬高

    // 测试
    std::cout << "Bezier triangle evaluation:\n";

    // 顶点
    Point3D p0 = tri.evaluate(0, 0);
    Point3D p1 = tri.evaluate(0, 1);
    Point3D p2 = tri.evaluate(1, 0);
    std::cout << "Vertex 0: (" << p0.x << ", " << p0.y << ", " << p0.z << ")\n";
    std::cout << "Vertex 1: (" << p1.x << ", " << p1.y << ", " << p1.z << ")\n";
    std::cout << "Vertex 2: (" << p2.x << ", " << p2.y << ", " << p2.z << ")\n";

    // 重心
    Point3D center1 = tri.evaluate(1.0/3, 1.0/3);
    Point3D center2 = tri.deCasteljau(1.0/3, 1.0/3);
    std::cout << "Centroid (evaluate): ("
              << center1.x << ", " << center1.y << ", " << center1.z << ")\n";
    std::cout << "Centroid (deCasteljau): ("
              << center2.x << ", " << center2.y << ", " << center2.z << ")\n";

    return 0;
}
```

### 挑战练习

**练习4**：实现Lagrange-Bezier转换

```cpp
#include <iostream>
#include <vector>
#include <cmath>

// 1D P2元素的Lagrange-Bezier转换
class LagrangeBezierConvert1D {
    // 3个点的转换矩阵
    // Lagrange节点：[0, 0.5, 1]
    // Bezier控制点索引：[0, 1, 2]

    double matL2B[3][3] = {
        {1.0,  0.0, 0.0},
        {-0.5, 2.0, -0.5},
        {0.0,  0.0, 1.0}
    };

    double matB2L[3][3] = {
        {1.0,  0.0,  0.0},
        {0.25, 0.5,  0.25},
        {0.0,  0.0,  1.0}
    };

public:
    // Lagrange值转Bezier控制点
    void lagrangeToBezier(const double lag[3], double bez[3]) const {
        for(int i = 0; i < 3; i++) {
            bez[i] = 0;
            for(int j = 0; j < 3; j++) {
                bez[i] += matL2B[i][j] * lag[j];
            }
        }
    }

    // Bezier控制点转Lagrange值
    void bezierToLagrange(const double bez[3], double lag[3]) const {
        for(int i = 0; i < 3; i++) {
            lag[i] = 0;
            for(int j = 0; j < 3; j++) {
                lag[i] += matB2L[i][j] * bez[j];
            }
        }
    }

    // 验证：检查凸包性质
    void checkConvexHull(const double bez[3]) const {
        double minBez = std::min({bez[0], bez[1], bez[2]});
        double maxBez = std::max({bez[0], bez[1], bez[2]});

        std::cout << "Bezier coefficients: ["
                  << bez[0] << ", " << bez[1] << ", " << bez[2] << "]\n";
        std::cout << "Value bounds: [" << minBez << ", " << maxBez << "]\n";

        // Bezier曲线的值一定在控制点的凸包内
        // 即 min(bez) <= f(t) <= max(bez) 对所有 t ∈ [0,1]
    }
};

int main() {
    LagrangeBezierConvert1D converter;

    // 测试1：线性函数 f(x) = 2x
    // Lagrange值：[f(0), f(0.5), f(1)] = [0, 1, 2]
    double lag1[3] = {0, 1, 2};
    double bez1[3];
    converter.lagrangeToBezier(lag1, bez1);

    std::cout << "Linear function f(x) = 2x:\n";
    std::cout << "Lagrange: [0, 1, 2]\n";
    converter.checkConvexHull(bez1);
    std::cout << "\n";

    // 测试2：二次函数 f(x) = x²
    // Lagrange值：[0, 0.25, 1]
    double lag2[3] = {0, 0.25, 1};
    double bez2[3];
    converter.lagrangeToBezier(lag2, bez2);

    std::cout << "Quadratic function f(x) = x^2:\n";
    std::cout << "Lagrange: [0, 0.25, 1]\n";
    converter.checkConvexHull(bez2);
    std::cout << "\n";

    // 测试3：验证往返转换
    double lag3[3];
    converter.bezierToLagrange(bez2, lag3);
    std::cout << "Round-trip test:\n";
    std::cout << "Original Lagrange: [" << lag2[0] << ", " << lag2[1] << ", " << lag2[2] << "]\n";
    std::cout << "After L→B→L: [" << lag3[0] << ", " << lag3[1] << ", " << lag3[2] << "]\n";

    return 0;
}
```

---

## 知识图谱

```
Bezier基与几何表示
│
├── 理论基础
│   ├── Bernstein多项式
│   ├── Bezier曲线/曲面
│   └── 凸包性质
│
├── bezierBasis类
│   ├── lag2Bez()（转换）
│   ├── bez2Lag()（逆转换）
│   ├── subDivide()（子分）
│   └── 各元素类型支持
│
├── de Casteljau算法
│   ├── 递归求值
│   ├── 几何解释
│   └── 子分应用
│
└── 应用场景
    ├── Jacobian正负检测
    ├── 高阶元素质量评估
    ├── 曲线/曲面求值
    └── 子分细化分析
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 |
|------|----------|--------|
| bezierBasis.h | Bezier基类定义 | ~250行 |
| bezierBasis.cpp | Bezier基实现 | ~1500行 |
| decasteljau.h | de Casteljau接口 | ~30行 |
| decasteljau.cpp | de Casteljau实现 | ~150行 |

---

## 今日检查点

- [ ] 理解Bernstein多项式的性质
- [ ] 能手工执行de Casteljau算法
- [ ] 理解Lagrange-Bezier转换的目的
- [ ] 理解凸包性质在Jacobian检测中的应用
- [ ] 理解子分算法的原理

---

## 导航

- 上一天：[Day 53 - Jacobian基与条件数](day-53.md)
- 下一天：[Day 55 - 矩阵运算与数值工具](day-55.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
