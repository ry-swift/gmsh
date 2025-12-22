# Day 56：第八周复习

## 学习目标
巩固第八周所学的数值计算模块知识，完成综合练习项目，为下一阶段学习做准备。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 知识点回顾与梳理 |
| 10:00-11:00 | 1h | 综合练习：有限元基础计算器 |
| 11:00-12:00 | 1h | 代码阅读复习 |
| 14:00-15:00 | 1h | 整理学习笔记，完成周检查点 |

---

## 上午学习内容

### 1. 第八周知识图谱

```
数值计算模块
│
├── Day 50: 基函数系统
│   ├── BasisFactory工厂模式
│   ├── nodalBasis节点基函数
│   ├── polynomialBasis多项式基
│   └── 形函数的数学性质
│
├── Day 51: 数值积分
│   ├── Gauss-Legendre积分
│   ├── 各单元类型积分点
│   ├── 积分精度与阶数
│   └── 参考→物理单元积分
│
├── Day 52: 层次基函数
│   ├── 层次基vs节点基
│   ├── HierarchicalBasis类体系
│   ├── H1空间（标量连续）
│   └── Hcurl空间（切向连续）
│
├── Day 53: Jacobian与条件数
│   ├── 几何映射与Jacobian矩阵
│   ├── JacobianBasis实现
│   ├── CondNumBasis条件数
│   └── SICN质量指标
│
├── Day 54: Bezier基
│   ├── Bernstein多项式
│   ├── de Casteljau算法
│   ├── Lagrange-Bezier转换
│   └── 凸包性质应用
│
└── Day 55: 矩阵与数值工具
    ├── fullMatrix/fullVector
    ├── BLAS/LAPACK集成
    ├── Numeric几何函数
    └── 正交多项式
```

### 2. 核心概念回顾

**形函数（Shape Functions）**：
```
定义：将参考单元映射到物理单元的插值函数

线性三角形：
N₁ = 1 - u - v
N₂ = u
N₃ = v

性质：
- Σ Nᵢ = 1（配分性）
- Nᵢ(xⱼ) = δᵢⱼ（插值性，对节点基）
- 0 ≤ Nᵢ ≤ 1（凸包性，对正单元）
```

**高斯积分**：
```
∫ f(x)dx ≈ Σ wᵢ·f(xᵢ)

关键点：
- n个积分点精确积分2n-1阶多项式
- 积分点是Legendre多项式的根
- 高阶元素需要更多积分点
```

**Jacobian与质量**：
```
J = [∂x/∂ξ  ∂x/∂η]
    [∂y/∂ξ  ∂y/∂η]

det(J) > 0 → 有效单元
det(J) < 0 → 翻转单元
det(J) = 0 → 退化单元

SICN = ICN / ICN_ideal ∈ [0,1]
```

### 3. 模块关系图

```
BasisFactory
     │
     ├──→ nodalBasis ────→ 形函数值
     │         │
     │         └──→ polynomialBasis
     │
     ├──→ JacobianBasis ─→ Jacobian计算
     │         │
     │         └──→ bezierBasis ─→ 有效性检查
     │
     └──→ CondNumBasis ──→ 质量评估

GaussIntegration
     │
     ├──→ GaussLegendre1D ─→ 1D积分点
     ├──→ GaussQuadratureTri ─→ 三角形积分
     └──→ GaussQuadratureTet ─→ 四面体积分

fullMatrix/fullVector
     │
     └──→ BLAS/LAPACK ─→ 高效数值计算
```

---

## 下午学习内容

### 4. 综合练习：有限元基础计算器

**目标**：整合本周所学，实现一个简单的有限元计算器。

```cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <array>

// ========== 基础类型 ==========
using Vec3 = std::array<double, 3>;
using Matrix = std::vector<std::vector<double>>;

// ========== 三角形形函数 ==========
class TriangleShapeFunctions {
public:
    // 线性形函数值
    void evaluate(double u, double v, double N[3]) const {
        N[0] = 1.0 - u - v;
        N[1] = u;
        N[2] = v;
    }

    // 形函数梯度（参考坐标系）
    void gradient(double dNdxi[3], double dNdeta[3]) const {
        // 对线性三角形，梯度是常数
        dNdxi[0] = -1.0;  dNdeta[0] = -1.0;
        dNdxi[1] = 1.0;   dNdeta[1] = 0.0;
        dNdxi[2] = 0.0;   dNdeta[2] = 1.0;
    }
};

// ========== 高斯积分 ==========
class TriangleQuadrature {
public:
    // 3点公式（精确到2阶）
    static const int npts = 3;

    double pts[3][2] = {
        {1.0/6.0, 1.0/6.0},
        {2.0/3.0, 1.0/6.0},
        {1.0/6.0, 2.0/3.0}
    };
    double weights[3] = {1.0/6.0, 1.0/6.0, 1.0/6.0};
};

// ========== Jacobian计算 ==========
class TriangleJacobian {
    double nodes[3][2];  // 物理节点坐标

public:
    void setNodes(const double coords[3][2]) {
        for(int i = 0; i < 3; i++) {
            nodes[i][0] = coords[i][0];
            nodes[i][1] = coords[i][1];
        }
    }

    // 计算Jacobian矩阵
    void compute(double J[2][2]) const {
        J[0][0] = nodes[1][0] - nodes[0][0];  // dx/dxi
        J[0][1] = nodes[2][0] - nodes[0][0];  // dx/deta
        J[1][0] = nodes[1][1] - nodes[0][1];  // dy/dxi
        J[1][1] = nodes[2][1] - nodes[0][1];  // dy/deta
    }

    // Jacobian行列式
    double det() const {
        double J[2][2];
        compute(J);
        return J[0][0]*J[1][1] - J[0][1]*J[1][0];
    }

    // 逆Jacobian
    void inverse(double Jinv[2][2]) const {
        double J[2][2];
        compute(J);
        double d = det();
        Jinv[0][0] = J[1][1] / d;
        Jinv[0][1] = -J[0][1] / d;
        Jinv[1][0] = -J[1][0] / d;
        Jinv[1][1] = J[0][0] / d;
    }

    // 条件数
    double conditionNumber() const {
        double J[2][2], Jinv[2][2];
        compute(J);
        inverse(Jinv);

        double normJ = std::sqrt(J[0][0]*J[0][0] + J[0][1]*J[0][1] +
                                  J[1][0]*J[1][0] + J[1][1]*J[1][1]);
        double normJinv = std::sqrt(Jinv[0][0]*Jinv[0][0] + Jinv[0][1]*Jinv[0][1] +
                                     Jinv[1][0]*Jinv[1][0] + Jinv[1][1]*Jinv[1][1]);
        return normJ * normJinv;
    }

    // SICN
    double SICN() const {
        double idealCond = 2.0 / std::sqrt(3.0);
        return idealCond / conditionNumber();
    }
};

// ========== 单元刚度矩阵 ==========
class TriangleStiffness {
    TriangleShapeFunctions shape;
    TriangleQuadrature quad;
    TriangleJacobian jacobian;

public:
    // 计算单元刚度矩阵（拉普拉斯算子）
    void compute(const double coords[3][2], double K[3][3]) {
        jacobian.setNodes(coords);

        // 初始化
        for(int i = 0; i < 3; i++)
            for(int j = 0; j < 3; j++)
                K[i][j] = 0.0;

        // 形函数梯度（参考坐标）
        double dNdxi[3], dNdeta[3];
        shape.gradient(dNdxi, dNdeta);

        // Jacobian逆和行列式
        double Jinv[2][2];
        jacobian.inverse(Jinv);
        double detJ = jacobian.det();

        // 物理坐标系中的梯度
        // dN/dx = dN/dxi * dxi/dx + dN/deta * deta/dx
        //       = dN/dxi * Jinv[0][0] + dN/deta * Jinv[1][0]
        double dNdx[3], dNdy[3];
        for(int i = 0; i < 3; i++) {
            dNdx[i] = dNdxi[i] * Jinv[0][0] + dNdeta[i] * Jinv[1][0];
            dNdy[i] = dNdxi[i] * Jinv[0][1] + dNdeta[i] * Jinv[1][1];
        }

        // 刚度矩阵（对线性三角形，积分是解析的）
        // K_ij = ∫ ∇Ni · ∇Nj dΩ = (dNi/dx * dNj/dx + dNi/dy * dNj/dy) * Area
        double area = 0.5 * std::abs(detJ);

        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                K[i][j] = (dNdx[i]*dNdx[j] + dNdy[i]*dNdy[j]) * area;
            }
        }
    }

    // 计算质量矩阵
    void computeMass(const double coords[3][2], double M[3][3]) {
        jacobian.setNodes(coords);
        double detJ = jacobian.det();
        double area = 0.5 * std::abs(detJ);

        // 对于线性三角形：
        // M_ii = area/6, M_ij = area/12 (i≠j)
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                if(i == j) {
                    M[i][j] = area / 6.0;
                } else {
                    M[i][j] = area / 12.0;
                }
            }
        }
    }
};

// ========== 测试函数 ==========
void printMatrix(const char *name, double M[3][3]) {
    std::cout << name << ":\n";
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            printf("%10.6f ", M[i][j]);
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    std::cout << "=== 有限元基础计算器 ===\n\n";

    // 测试三角形1：直角三角形
    double tri1[3][2] = {{0, 0}, {1, 0}, {0, 1}};

    std::cout << "--- 直角三角形 ---\n";
    std::cout << "顶点: (0,0), (1,0), (0,1)\n\n";

    TriangleJacobian jac1;
    jac1.setNodes(tri1);
    std::cout << "Jacobian行列式: " << jac1.det() << "\n";
    std::cout << "面积: " << 0.5 * std::abs(jac1.det()) << "\n";
    std::cout << "条件数: " << jac1.conditionNumber() << "\n";
    std::cout << "SICN: " << jac1.SICN() << "\n\n";

    TriangleStiffness stiff;
    double K1[3][3], M1[3][3];
    stiff.compute(tri1, K1);
    stiff.computeMass(tri1, M1);

    printMatrix("刚度矩阵 K", K1);
    printMatrix("质量矩阵 M", M1);

    // 验证刚度矩阵的性质
    double sumK = 0;
    for(int i = 0; i < 3; i++) {
        double rowSum = 0;
        for(int j = 0; j < 3; j++) {
            rowSum += K1[i][j];
            sumK += K1[i][j];
        }
        std::cout << "K 第" << i << "行之和: " << rowSum << " (应为0)\n";
    }
    std::cout << "\n";

    // 测试三角形2：等边三角形
    double h = std::sqrt(3.0) / 2.0;
    double tri2[3][2] = {{0, 0}, {1, 0}, {0.5, h}};

    std::cout << "--- 等边三角形 ---\n";
    std::cout << "顶点: (0,0), (1,0), (0.5, " << h << ")\n\n";

    TriangleJacobian jac2;
    jac2.setNodes(tri2);
    std::cout << "Jacobian行列式: " << jac2.det() << "\n";
    std::cout << "面积: " << 0.5 * std::abs(jac2.det()) << "\n";
    std::cout << "条件数: " << jac2.conditionNumber() << "\n";
    std::cout << "SICN: " << jac2.SICN() << " (应接近1.0)\n\n";

    double K2[3][3], M2[3][3];
    stiff.compute(tri2, K2);
    stiff.computeMass(tri2, M2);

    printMatrix("刚度矩阵 K", K2);

    return 0;
}
```

### 5. 周复习练习

**练习1**：形函数验证

```cpp
// 验证形函数的配分性和插值性

void testShapeFunctions() {
    TriangleShapeFunctions shape;

    // 测试点
    double testPts[4][2] = {
        {0.0, 0.0},   // 顶点0
        {1.0, 0.0},   // 顶点1
        {0.0, 1.0},   // 顶点2
        {1.0/3.0, 1.0/3.0}  // 重心
    };

    for(int p = 0; p < 4; p++) {
        double N[3];
        shape.evaluate(testPts[p][0], testPts[p][1], N);

        double sum = N[0] + N[1] + N[2];
        printf("点(%.2f, %.2f): N = [%.4f, %.4f, %.4f], 和 = %.4f\n",
               testPts[p][0], testPts[p][1], N[0], N[1], N[2], sum);
    }
}
```

**练习2**：高斯积分验证

```cpp
// 使用高斯积分计算三角形面积

double integrateOne(const double coords[3][2]) {
    TriangleQuadrature quad;
    TriangleJacobian jac;
    jac.setNodes(coords);

    double result = 0;
    for(int i = 0; i < quad.npts; i++) {
        // 积分常数1，乘以|J|
        result += quad.weights[i] * std::abs(jac.det());
    }
    return result;  // 应该等于面积
}
```

---

## 周检查点

### 理论知识

- [ ] 能解释节点基函数的定义和性质
- [ ] 理解高斯积分点数与精度的关系
- [ ] 能区分层次基函数和节点基函数
- [ ] 理解Jacobian矩阵的几何意义
- [ ] 能解释SICN质量指标的含义
- [ ] 理解Bezier凸包性质的应用

### 实践技能

- [ ] 能手工计算三角形形函数值
- [ ] 能选择合适的积分阶数
- [ ] 能计算简单单元的Jacobian
- [ ] 能使用de Casteljau算法求Bezier曲线点
- [ ] 能使用fullMatrix进行基本运算

### 代码阅读

- [ ] 已阅读BasisFactory.cpp的工厂逻辑
- [ ] 已阅读nodalBasis.cpp的形函数实现
- [ ] 已阅读GaussIntegration.cpp的积分点获取
- [ ] 已阅读JacobianBasis.cpp的Jacobian计算
- [ ] 已阅读bezierBasis.cpp的转换和子分

---

## 关键源码索引

| 主题 | 文件 | 核心函数/类 |
|------|------|------------|
| 基函数工厂 | BasisFactory.cpp | `getNodalBasis()` |
| 节点基函数 | nodalBasis.cpp | `f()`, `df()` |
| 数值积分 | GaussIntegration.cpp | `getGaussPointsTri()` |
| 层次基函数 | HierarchicalBasisH1Tria.cpp | `generateBasis()` |
| Jacobian | JacobianBasis.cpp | `getJacobians()` |
| 条件数 | CondNumBasis.cpp | `getInvCondNum()` |
| Bezier基 | bezierBasis.cpp | `lag2Bez()`, `subDivide()` |
| 矩阵运算 | fullMatrix.cpp | `mult()`, `invert()` |

---

## 第八周总结

### 核心收获

1. **基函数体系**：理解了Gmsh中多种基函数的设计和用途
2. **数值积分**：掌握了高斯积分在有限元中的应用
3. **层次基函数**：了解了p自适应有限元的数学基础
4. **几何质量**：学会了使用Jacobian和条件数评估网格质量
5. **Bezier技术**：理解了Bezier基在几何计算中的优势
6. **矩阵工具**：熟悉了Gmsh的数值计算基础设施

### 关键洞察

- **工厂模式**：BasisFactory实现了基函数的统一创建接口
- **精度平衡**：积分点数需要与元素阶数匹配
- **层次化设计**：层次基函数便于自适应细化
- **Bezier凸包**：提供了判断Jacobian符号的有效方法
- **BLAS集成**：fullMatrix利用BLAS实现高效计算

### 下周预告

第9周：后处理与可视化
- Day 57：后处理数据结构
- Day 58：PView视图系统
- Day 59：数据插值
- Day 60：等值线/等值面
- Day 61：向量场可视化
- Day 62：动画与时间步
- Day 63：第九周复习

将深入Gmsh的后处理模块。

---

## 扩展阅读建议

### 必读
1. 完成本周所有练习
2. 用debugger追踪一次形函数求值

### 选读
1. Zienkiewicz & Taylor《有限元方法》第4章
2. Hughes《有限元方法：线性静力和动力有限元分析》
3. Szabó & Babuška《hp有限元方法》

---

## 导航

- 上一天：[Day 55 - 矩阵运算与数值工具](day-55.md)
- 下一天：[Day 57 - 后处理数据结构](day-57.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
