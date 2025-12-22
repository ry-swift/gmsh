# Day 53：Jacobian基与条件数

## 学习目标
理解Gmsh中Jacobian基函数的实现，掌握网格元素质量评估中的条件数计算方法。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习Jacobian矩阵的概念 |
| 10:00-11:00 | 1h | 阅读JacobianBasis实现 |
| 11:00-12:00 | 1h | 学习CondNumBasis条件数计算 |
| 14:00-15:00 | 1h | 完成练习并总结 |

---

## 上午学习内容

### 1. Jacobian矩阵基础

**几何映射**：
```
参考单元 → 物理单元的映射

x(ξ,η,ζ) = Σ Nᵢ(ξ,η,ζ) · xᵢ
y(ξ,η,ζ) = Σ Nᵢ(ξ,η,ζ) · yᵢ
z(ξ,η,ζ) = Σ Nᵢ(ξ,η,ζ) · zᵢ

其中：
- (ξ,η,ζ)：参考坐标
- (x,y,z)：物理坐标
- Nᵢ：形函数
- (xᵢ,yᵢ,zᵢ)：节点坐标
```

**Jacobian矩阵**：
```
       ⎡ ∂x/∂ξ  ∂x/∂η  ∂x/∂ζ ⎤
J =    ⎢ ∂y/∂ξ  ∂y/∂η  ∂y/∂ζ ⎥
       ⎣ ∂z/∂ξ  ∂z/∂η  ∂z/∂ζ ⎦

Jacobian行列式：det(J)

物理意义：
- det(J) > 0：正向映射，元素有效
- det(J) < 0：元素翻转，无效
- det(J) = 0：元素退化
```

### 2. JacobianBasis类

**文件位置**：`src/numeric/JacobianBasis.h`

```cpp
class JacobianBasis {
protected:
    // 基函数相关
    const bezierBasis *_bezier;  // Bezier基函数
    int _numLagCoeff;            // Lagrange系数数量
    int _numJacNodes;            // Jacobian采样节点数

    // 映射矩阵
    fullMatrix<double> _gradShapeMatX;  // x方向梯度
    fullMatrix<double> _gradShapeMatY;  // y方向梯度
    fullMatrix<double> _gradShapeMatZ;  // z方向梯度

public:
    JacobianBasis(int tag, FuncSpaceData);

    // 计算Jacobian行列式
    double getPrimNormals1D(
        const fullMatrix<double> &nodesXYZ,
        fullMatrix<double> &result) const;

    double getPrimNormal2D(
        const fullMatrix<double> &nodesXYZ,
        fullMatrix<double> &result,
        bool normalize = false) const;

    double getPrimJac3D(
        const fullMatrix<double> &nodesXYZ) const;

    // 在多个点计算Jacobian
    void getJacobians(
        const fullMatrix<double> &nodesXYZ,
        fullVector<double> &jacobians) const;

    // 获取最小/最大Jacobian
    void getMinMaxJac(
        const fullMatrix<double> &nodesXYZ,
        double &minJ, double &maxJ) const;

    // 获取签名Jacobian范围（归一化）
    void getSignedJacAndNormalization(
        const fullMatrix<double> &nodesXYZ,
        fullVector<double> &jac,
        double &minJ, double &maxJ,
        double &normalization) const;
};
```

### 3. Jacobian计算原理

**线性三角形**：
```cpp
// 对于线性三角形，Jacobian是常数
//
// 节点：(x₀,y₀), (x₁,y₁), (x₂,y₂)
// 形函数：N₁ = 1-ξ-η, N₂ = ξ, N₃ = η
//
// ∂x/∂ξ = x₁ - x₀
// ∂x/∂η = x₂ - x₀
// ∂y/∂ξ = y₁ - y₀
// ∂y/∂η = y₂ - y₀
//
// det(J) = (x₁-x₀)(y₂-y₀) - (x₂-x₀)(y₁-y₀)
//        = 2 × 面积
```

**高阶元素**：
```cpp
// 对于高阶元素，Jacobian不再是常数
// 需要在多个点采样

// Jacobian的多项式阶数：
// - 几何阶数为p的元素
// - 形函数导数是p-1阶
// - Jacobian是(p-1)×dim阶多项式（2D）

// 例如P2三角形：
// - 几何阶数p=2
// - Jacobian是1×2=2阶多项式
// - 需要检查多个点确保全域有效
```

### 4. Bezier基在Jacobian计算中的应用

**文件位置**：`src/numeric/bezierBasis.h`

```cpp
// 使用Bezier基表示Jacobian的优势：
// 1. Bezier多项式具有凸包性质
// 2. 控制点给出函数值的界
// 3. 便于检测Jacobian的正负性

class bezierBasis {
public:
    // 将Lagrange系数转换为Bezier系数
    void lag2Bez(
        const fullVector<double> &lagCoeff,
        fullVector<double> &bezCoeff) const;

    // Bezier系数转回Lagrange系数
    void bez2Lag(
        const fullVector<double> &bezCoeff,
        fullVector<double> &lagCoeff) const;

    // 子分（用于细化检查）
    void subDivide(
        const fullVector<double> &bezCoeff,
        std::vector<fullVector<double>> &subCoeff) const;
};
```

---

## 下午学习内容

### 5. CondNumBasis条件数基函数

**文件位置**：`src/numeric/CondNumBasis.h`

**条件数定义**：
```
κ(J) = ||J|| · ||J⁻¹||

其中 ||·|| 是矩阵范数（通常用Frobenius范数）

意义：
- κ = 1：完美单元（等边三角形/正四面体）
- κ → ∞：退化单元
```

```cpp
class CondNumBasis {
protected:
    // 评估点和权重
    fullMatrix<double> _pts;
    fullVector<double> _weights;

    // Jacobian计算所需的梯度矩阵
    fullMatrix<double> _gradXdXi;
    fullMatrix<double> _gradXdEta;
    fullMatrix<double> _gradXdZeta;

public:
    CondNumBasis(int tag, int order = -1);

    // 计算条件数
    void getCondNum(
        const fullMatrix<double> &nodesXYZ,
        fullVector<double> &condNum) const;

    // 计算反条件数（倒数，便于比较）
    void getInvCondNum(
        const fullMatrix<double> &nodesXYZ,
        fullVector<double> &invCondNum) const;

    // 获取最小反条件数（质量指标）
    double getMinInvCondNum(
        const fullMatrix<double> &nodesXYZ) const;
};
```

### 6. SICN（Scaled Inverse Condition Number）

**归一化反条件数**：
```
SICN = ICN / ICN_ideal

其中：
- ICN = 1/κ（反条件数）
- ICN_ideal 是理想单元的反条件数

范围：
- SICN ∈ [0, 1]（对于有效单元）
- SICN = 1：完美单元
- SICN = 0：退化单元
- SICN < 0：翻转单元（无效）
```

**各类型元素的理想单元**：

| 元素类型 | 理想形状 | ICN_ideal |
|----------|----------|-----------|
| 三角形 | 等边三角形 | 1.0 |
| 四边形 | 正方形 | 1.0 |
| 四面体 | 正四面体 | 1.0 |
| 六面体 | 立方体 | 1.0 |

### 7. 代码实现分析

**JacobianBasis计算流程**：

```cpp
void JacobianBasis::getJacobians(
    const fullMatrix<double> &nodesXYZ,
    fullVector<double> &jacobians) const
{
    // 1. 计算各方向的梯度
    fullVector<double> gradX(_numJacNodes);
    fullVector<double> gradY(_numJacNodes);
    fullVector<double> gradZ(_numJacNodes);

    // 在每个采样点计算
    for(int i = 0; i < _numJacNodes; i++) {
        // ∂x/∂ξ, ∂x/∂η, ∂x/∂ζ
        double dxdxi = 0, dxdeta = 0, dxdzeta = 0;
        double dydxi = 0, dydeta = 0, dydzeta = 0;
        double dzdxi = 0, dzdeta = 0, dzdzeta = 0;

        for(int j = 0; j < _numLagCoeff; j++) {
            dxdxi += _gradShapeMatX(i, j) * nodesXYZ(j, 0);
            // ... 其他分量
        }

        // 2. 计算行列式
        if(_dim == 2) {
            // 2D: det = dxdxi*dydeta - dxdeta*dydxi
            jacobians(i) = dxdxi*dydeta - dxdeta*dydxi;
        } else {
            // 3D: 完整的3×3行列式
            jacobians(i) = dxdxi*(dydeta*dzdzeta - dydzeta*dzdeta)
                         - dxdeta*(dydxi*dzdzeta - dydzeta*dzdxi)
                         + dxdzeta*(dydxi*dzdeta - dydeta*dzdxi);
        }
    }
}
```

**条件数计算流程**：

```cpp
void CondNumBasis::getInvCondNum(
    const fullMatrix<double> &nodesXYZ,
    fullVector<double> &invCondNum) const
{
    for(int i = 0; i < _numPts; i++) {
        // 1. 计算Jacobian矩阵的分量
        double J[3][3] = {0};
        for(int j = 0; j < _numCoeff; j++) {
            J[0][0] += _gradXdXi(i,j) * nodesXYZ(j,0);
            J[0][1] += _gradXdEta(i,j) * nodesXYZ(j,0);
            // ... 其他分量
        }

        // 2. 计算Frobenius范数
        double normJ = sqrt(J[0][0]*J[0][0] + J[0][1]*J[0][1] + ...);

        // 3. 计算逆矩阵范数
        double detJ = det3x3(J);
        double normJinv = sqrt(...) / detJ;

        // 4. 条件数 = normJ * normJinv
        // 反条件数 = 1 / 条件数
        invCondNum(i) = 1.0 / (normJ * normJinv);
    }
}
```

---

## 练习作业

### 基础练习

**练习1**：手工计算三角形的Jacobian

```cpp
// 给定三角形顶点：
// P0 = (0, 0)
// P1 = (2, 0)
// P2 = (1, 1.5)
//
// 计算Jacobian矩阵和行列式

// 解：
// J = | x1-x0  x2-x0 |   | 2  1  |
//     | y1-y0  y2-y0 | = | 0  1.5 |
//
// det(J) = 2*1.5 - 1*0 = 3.0
// 三角形面积 = det(J)/2 = 1.5

// 对于等边三角形（边长a=2）：
// P0 = (0, 0)
// P1 = (2, 0)
// P2 = (1, √3)
//
// det(J) = 2*√3 ≈ 3.46
// 面积 = √3 ≈ 1.73
```

### 进阶练习

**练习2**：实现简单的Jacobian计算器

```cpp
#include <iostream>
#include <cmath>
#include <vector>

// 2D三角形Jacobian计算
class TriangleJacobian {
    // 线性三角形的3个顶点
    double x[3], y[3];

public:
    TriangleJacobian(double x0, double y0,
                     double x1, double y1,
                     double x2, double y2) {
        x[0] = x0; y[0] = y0;
        x[1] = x1; y[1] = y1;
        x[2] = x2; y[2] = y2;
    }

    // 计算Jacobian矩阵元素
    void getJacobianMatrix(double J[2][2]) const {
        J[0][0] = x[1] - x[0];  // dx/dxi
        J[0][1] = x[2] - x[0];  // dx/deta
        J[1][0] = y[1] - y[0];  // dy/dxi
        J[1][1] = y[2] - y[0];  // dy/deta
    }

    // 计算Jacobian行列式
    double determinant() const {
        return (x[1]-x[0])*(y[2]-y[0]) - (x[2]-x[0])*(y[1]-y[0]);
    }

    // 计算面积
    double area() const {
        return std::abs(determinant()) / 2.0;
    }

    // 计算条件数
    double conditionNumber() const {
        double J[2][2];
        getJacobianMatrix(J);

        // Frobenius范数
        double normJ = std::sqrt(J[0][0]*J[0][0] + J[0][1]*J[0][1] +
                                  J[1][0]*J[1][0] + J[1][1]*J[1][1]);

        // 逆矩阵
        double det = determinant();
        double Jinv[2][2];
        Jinv[0][0] = J[1][1] / det;
        Jinv[0][1] = -J[0][1] / det;
        Jinv[1][0] = -J[1][0] / det;
        Jinv[1][1] = J[0][0] / det;

        double normJinv = std::sqrt(Jinv[0][0]*Jinv[0][0] + Jinv[0][1]*Jinv[0][1] +
                                     Jinv[1][0]*Jinv[1][0] + Jinv[1][1]*Jinv[1][1]);

        return normJ * normJinv;
    }

    // 计算SICN（归一化反条件数）
    double SICN() const {
        // 理想三角形（等边）的条件数 = 2/√3 ≈ 1.1547
        double idealCond = 2.0 / std::sqrt(3.0);
        double cond = conditionNumber();
        return idealCond / cond;
    }
};

int main() {
    // 测试1：直角三角形
    TriangleJacobian t1(0, 0, 1, 0, 0, 1);
    std::cout << "Right triangle:\n";
    std::cout << "  det(J) = " << t1.determinant() << "\n";
    std::cout << "  area = " << t1.area() << "\n";
    std::cout << "  cond = " << t1.conditionNumber() << "\n";
    std::cout << "  SICN = " << t1.SICN() << "\n\n";

    // 测试2：等边三角形
    double h = std::sqrt(3.0) / 2.0;
    TriangleJacobian t2(0, 0, 1, 0, 0.5, h);
    std::cout << "Equilateral triangle:\n";
    std::cout << "  det(J) = " << t2.determinant() << "\n";
    std::cout << "  area = " << t2.area() << "\n";
    std::cout << "  cond = " << t2.conditionNumber() << "\n";
    std::cout << "  SICN = " << t2.SICN() << " (should be ~1.0)\n\n";

    // 测试3：扁平三角形（质量差）
    TriangleJacobian t3(0, 0, 1, 0, 0.5, 0.1);
    std::cout << "Flat triangle:\n";
    std::cout << "  det(J) = " << t3.determinant() << "\n";
    std::cout << "  area = " << t3.area() << "\n";
    std::cout << "  cond = " << t3.conditionNumber() << "\n";
    std::cout << "  SICN = " << t3.SICN() << " (should be small)\n";

    return 0;
}
```

**练习3**：高阶元素的Jacobian变化

```cpp
#include <iostream>
#include <cmath>
#include <vector>

// P2三角形（6节点）的Jacobian计算
class QuadraticTriangleJacobian {
    double x[6], y[6];

public:
    QuadraticTriangleJacobian(const double coords[6][2]) {
        for(int i = 0; i < 6; i++) {
            x[i] = coords[i][0];
            y[i] = coords[i][1];
        }
    }

    // P2形函数
    void shapeFunctions(double xi, double eta, double N[6]) const {
        double L1 = 1.0 - xi - eta;
        double L2 = xi;
        double L3 = eta;

        N[0] = L1 * (2*L1 - 1);
        N[1] = L2 * (2*L2 - 1);
        N[2] = L3 * (2*L3 - 1);
        N[3] = 4 * L1 * L2;
        N[4] = 4 * L2 * L3;
        N[5] = 4 * L3 * L1;
    }

    // P2形函数梯度
    void shapeFunctionDerivatives(double xi, double eta,
                                  double dNdxi[6], double dNdeta[6]) const {
        double L1 = 1.0 - xi - eta;
        double L2 = xi;
        double L3 = eta;

        // dN/dL1, dN/dL2, dN/dL3
        // 然后使用链式法则：dN/dxi = dN/dL1*dL1/dxi + dN/dL2*dL2/dxi + ...

        dNdxi[0] = 1 - 4*L1;
        dNdxi[1] = 4*L2 - 1;
        dNdxi[2] = 0;
        dNdxi[3] = 4*(L1 - L2);
        dNdxi[4] = 4*L3;
        dNdxi[5] = -4*L3;

        dNdeta[0] = 1 - 4*L1;
        dNdeta[1] = 0;
        dNdeta[2] = 4*L3 - 1;
        dNdeta[3] = -4*L2;
        dNdeta[4] = 4*L2;
        dNdeta[5] = 4*(L1 - L3);
    }

    // 计算指定点的Jacobian行列式
    double jacobianAt(double xi, double eta) const {
        double dNdxi[6], dNdeta[6];
        shapeFunctionDerivatives(xi, eta, dNdxi, dNdeta);

        double dxdxi = 0, dxdeta = 0;
        double dydxi = 0, dydeta = 0;

        for(int i = 0; i < 6; i++) {
            dxdxi += dNdxi[i] * x[i];
            dxdeta += dNdeta[i] * x[i];
            dydxi += dNdxi[i] * y[i];
            dydeta += dNdeta[i] * y[i];
        }

        return dxdxi * dydeta - dxdeta * dydxi;
    }

    // 获取Jacobian的最小和最大值
    void getMinMaxJacobian(double &minJ, double &maxJ) const {
        minJ = 1e30;
        maxJ = -1e30;

        // 在多个点采样
        const int n = 5;
        for(int i = 0; i <= n; i++) {
            for(int j = 0; j <= n - i; j++) {
                double xi = double(i) / n;
                double eta = double(j) / n;
                double J = jacobianAt(xi, eta);
                minJ = std::min(minJ, J);
                maxJ = std::max(maxJ, J);
            }
        }
    }
};

int main() {
    // 直线边P2三角形（等同于P1）
    double coords1[6][2] = {
        {0, 0}, {1, 0}, {0.5, 0.866},  // 顶点
        {0.5, 0}, {0.75, 0.433}, {0.25, 0.433}  // 边中点
    };
    QuadraticTriangleJacobian t1(coords1);

    double minJ, maxJ;
    t1.getMinMaxJacobian(minJ, maxJ);
    std::cout << "Linear edges (straight P2):\n";
    std::cout << "  Jacobian range: [" << minJ << ", " << maxJ << "]\n";
    std::cout << "  Ratio max/min: " << maxJ/minJ << " (should be 1)\n\n";

    // 弯曲边P2三角形
    double coords2[6][2] = {
        {0, 0}, {1, 0}, {0.5, 0.866},
        {0.5, 0.1},  // 边中点向上弯曲
        {0.75, 0.433}, {0.25, 0.433}
    };
    QuadraticTriangleJacobian t2(coords2);

    t2.getMinMaxJacobian(minJ, maxJ);
    std::cout << "Curved edge P2:\n";
    std::cout << "  Jacobian range: [" << minJ << ", " << maxJ << "]\n";
    std::cout << "  Ratio max/min: " << maxJ/minJ << " (should be > 1)\n";

    return 0;
}
```

### 挑战练习

**练习4**：实现四面体的Jacobian检查

```cpp
#include <iostream>
#include <cmath>

// 线性四面体Jacobian
class TetrahedronJacobian {
    double x[4], y[4], z[4];

public:
    TetrahedronJacobian(const double coords[4][3]) {
        for(int i = 0; i < 4; i++) {
            x[i] = coords[i][0];
            y[i] = coords[i][1];
            z[i] = coords[i][2];
        }
    }

    // 计算3x3行列式
    double det3x3(double a[3][3]) const {
        return a[0][0]*(a[1][1]*a[2][2] - a[1][2]*a[2][1])
             - a[0][1]*(a[1][0]*a[2][2] - a[1][2]*a[2][0])
             + a[0][2]*(a[1][0]*a[2][1] - a[1][1]*a[2][0]);
    }

    // 计算Jacobian行列式
    double determinant() const {
        double J[3][3];
        // dx/dxi = x1 - x0, dx/deta = x2 - x0, dx/dzeta = x3 - x0
        J[0][0] = x[1] - x[0]; J[0][1] = x[2] - x[0]; J[0][2] = x[3] - x[0];
        J[1][0] = y[1] - y[0]; J[1][1] = y[2] - y[0]; J[1][2] = y[3] - y[0];
        J[2][0] = z[1] - z[0]; J[2][1] = z[2] - z[0]; J[2][2] = z[3] - z[0];

        return det3x3(J);
    }

    // 计算体积
    double volume() const {
        return std::abs(determinant()) / 6.0;
    }

    // 检查元素有效性
    bool isValid() const {
        return determinant() > 0;
    }

    // 计算条件数
    double conditionNumber() const {
        double J[3][3];
        J[0][0] = x[1] - x[0]; J[0][1] = x[2] - x[0]; J[0][2] = x[3] - x[0];
        J[1][0] = y[1] - y[0]; J[1][1] = y[2] - y[0]; J[1][2] = y[3] - y[0];
        J[2][0] = z[1] - z[0]; J[2][1] = z[2] - z[0]; J[2][2] = z[3] - z[0];

        // Frobenius范数
        double normJ = 0;
        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
                normJ += J[i][j] * J[i][j];
        normJ = std::sqrt(normJ);

        // 简化的逆矩阵范数估计
        double det = determinant();
        // 使用辅因子矩阵计算
        // ... (完整实现需要计算逆矩阵)

        // 这里用简化估计
        return normJ * normJ * normJ / std::abs(det);
    }
};

int main() {
    // 正四面体
    double h = std::sqrt(2.0/3.0);
    double coords1[4][3] = {
        {0, 0, 0},
        {1, 0, 0},
        {0.5, std::sqrt(3.0)/2.0, 0},
        {0.5, std::sqrt(3.0)/6.0, h}
    };
    TetrahedronJacobian t1(coords1);

    std::cout << "Regular tetrahedron:\n";
    std::cout << "  det(J) = " << t1.determinant() << "\n";
    std::cout << "  volume = " << t1.volume() << "\n";
    std::cout << "  valid = " << (t1.isValid() ? "yes" : "no") << "\n\n";

    // 扁平四面体
    double coords2[4][3] = {
        {0, 0, 0},
        {1, 0, 0},
        {0.5, 1, 0},
        {0.5, 0.5, 0.1}  // 很小的高度
    };
    TetrahedronJacobian t2(coords2);

    std::cout << "Flat tetrahedron:\n";
    std::cout << "  det(J) = " << t2.determinant() << "\n";
    std::cout << "  volume = " << t2.volume() << "\n";
    std::cout << "  valid = " << (t2.isValid() ? "yes" : "no") << "\n";

    return 0;
}
```

---

## 知识图谱

```
Jacobian与条件数系统
│
├── JacobianBasis
│   ├── 几何映射计算
│   ├── Jacobian行列式
│   ├── Bezier表示（检测有效性）
│   └── 高阶元素支持
│
├── CondNumBasis
│   ├── 条件数计算
│   ├── 反条件数（质量指标）
│   └── SICN（归一化指标）
│
├── bezierBasis
│   ├── Lagrange-Bezier转换
│   ├── 凸包性质
│   └── 子分（细化检查）
│
└── 应用场景
    ├── 网格有效性检查
    ├── 网格质量评估
    ├── 高阶元素验证
    └── 有限元积分
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 |
|------|----------|--------|
| JacobianBasis.h | Jacobian计算接口 | ~400行 |
| JacobianBasis.cpp | Jacobian实现 | ~1100行 |
| CondNumBasis.h | 条件数计算接口 | ~200行 |
| CondNumBasis.cpp | 条件数实现 | ~800行 |
| bezierBasis.h | Bezier基定义 | ~250行 |
| bezierBasis.cpp | Bezier基实现 | ~1500行 |

---

## 今日检查点

- [ ] 理解Jacobian矩阵的几何意义
- [ ] 能手工计算简单三角形的Jacobian
- [ ] 理解条件数与元素质量的关系
- [ ] 理解Bezier基在有效性检查中的应用
- [ ] 能区分线性和高阶元素的Jacobian特点

---

## 导航

- 上一天：[Day 52 - 层次基函数](day-52.md)
- 下一天：[Day 54 - Bezier基与几何表示](day-54.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
