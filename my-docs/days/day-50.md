# Day 50：基函数系统概览

## 学习目标
理解Gmsh数值计算模块的基函数体系，掌握节点基函数和多项式基函数的概念与实现。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 阅读BasisFactory工厂类 |
| 10:00-11:00 | 1h | 学习nodalBasis节点基函数 |
| 11:00-12:00 | 1h | 分析polynomialBasis多项式基 |
| 14:00-15:00 | 1h | 完成练习并总结 |

---

## 上午学习内容

### 1. 数值计算模块概览

**目录结构**：`src/numeric/`

```
src/numeric/
├── 基函数系统
│   ├── BasisFactory.h/cpp      # 基函数工厂
│   ├── nodalBasis.h/cpp        # 节点基函数
│   ├── polynomialBasis.h/cpp   # 多项式基函数
│   ├── orthogonalBasis.h/cpp   # 正交基函数
│   └── HierarchicalBasis*.h/cpp # 层次基函数
│
├── 数值积分
│   ├── GaussIntegration.h/cpp  # 高斯积分主接口
│   ├── GaussLegendre1D.h       # 1D Legendre积分点
│   ├── GaussQuadratureTri.cpp  # 三角形积分
│   ├── GaussQuadratureTet.cpp  # 四面体积分
│   └── ...
│
├── 几何基函数
│   ├── bezierBasis.h/cpp       # Bezier基函数
│   ├── JacobianBasis.h/cpp     # Jacobian基函数
│   └── CondNumBasis.h/cpp      # 条件数基函数
│
└── 数学工具
    ├── fullMatrix.h/cpp        # 矩阵运算
    ├── Numeric.h/cpp           # 通用数值函数
    └── robustPredicates.h/cpp  # 精确几何谓词
```

### 2. BasisFactory工厂类

**文件位置**：`src/numeric/BasisFactory.h`

```cpp
// BasisFactory的核心职责：根据元素类型创建对应的基函数
class BasisFactory {
public:
    // 获取节点基函数
    static const nodalBasis *getNodalBasis(int tag);

    // 获取Jacobian基函数（用于几何映射）
    static const JacobianBasis *getJacobianBasis(int tag, int order = -1);

    // 获取条件数基函数（用于质量评估）
    static const CondNumBasis *getCondNumBasis(int tag, int order = -1);

    // 获取Bezier基函数
    static const bezierBasis *getBezierBasis(int parentTag, int order);
};
```

**设计模式分析**：
- **工厂模式**：根据元素tag动态创建基函数对象
- **缓存机制**：内部使用map缓存已创建的基函数
- **延迟初始化**：按需创建，避免预先创建所有类型

### 3. nodalBasis节点基函数

**文件位置**：`src/numeric/nodalBasis.h`

```cpp
class nodalBasis {
public:
    // 基本属性
    int type;           // 元素类型（TYPE_TRI, TYPE_TET等）
    int parentType;     // 父类型
    int order;          // 多项式阶数
    int dimension;      // 空间维度
    int numFaces;       // 面的数量

    // 节点信息
    fullMatrix<double> points;  // 节点坐标（参考坐标系）

    // 形函数相关
    fullMatrix<double> monomials;    // 单项式指数
    fullMatrix<double> coefficients; // 系数矩阵

    // 闭包信息（边界关系）
    std::vector<std::vector<int>> closures;
    std::vector<std::vector<int>> fullClosures;

    // 核心方法
    virtual void f(double u, double v, double w, double *sf) const;
    virtual void df(double u, double v, double w,
                    double *dsfdu, double *dsfdv, double *dsdfw) const;
    virtual void ddf(double u, double v, double w, double *ddf) const;
};
```

**形函数计算示意**：

```
参考单元（三角形）        形函数值
    2                   N₁ = 1-u-v
    |\                  N₂ = u
    | \                 N₃ = v
    |  \
    |___\
    0    1              (u,v) ∈ [0,1]×[0,1-u]
```

### 4. 多项式基函数

**文件位置**：`src/numeric/polynomialBasis.h`

```cpp
class polynomialBasis : public nodalBasis {
protected:
    // 单项式系数矩阵
    // 形函数 = Σ coefficients[i][j] * u^a * v^b * w^c
    fullMatrix<double> coefficients;
    fullMatrix<double> monomials;  // [a, b, c] 指数

public:
    // 构造函数
    polynomialBasis(int tag);

    // 形函数求值
    void f(double u, double v, double w, double *sf) const override;

    // 形函数梯度
    void df(double u, double v, double w,
            double *dsfdu, double *dsfdv, double *dsdfw) const override;

    // 形函数Hessian
    void ddf(double u, double v, double w, double *ddf) const override;
};
```

---

## 下午学习内容

### 5. 形函数的数学基础

**Lagrange插值多项式**：

对于n+1个节点的一阶元素：
```
       n
L(x) = Σ yᵢ·Lᵢ(x)
      i=0

其中 Lᵢ(x) = ∏(j≠i) (x-xⱼ)/(xᵢ-xⱼ)
```

**二维三角形形函数（线性）**：
```
N₁(u,v) = 1 - u - v    （节点0：原点）
N₂(u,v) = u            （节点1：u方向）
N₃(u,v) = v            （节点2：v方向）

性质：
- Nᵢ(xⱼ) = δᵢⱼ（节点处取值0或1）
- ΣNᵢ = 1（配分性）
- 0 ≤ Nᵢ ≤ 1（凸包性）
```

### 6. 闭包(Closure)概念

```cpp
// 闭包定义了元素边界上的节点对应关系
// 这对于高阶元素的节点编号和连续性至关重要

/*
  高阶三角形（P2）节点编号：

        2
        |\
        | \
        5  4
        |   \
        |____\
        0  3  1

  边闭包：
  - 边0（0-1）：节点 [0, 3, 1]
  - 边1（1-2）：节点 [1, 4, 2]
  - 边2（2-0）：节点 [2, 5, 0]
*/

// 闭包用于：
// 1. 确保相邻单元共享边上的节点一致
// 2. 施加边界条件
// 3. 高阶元素的连续性
```

### 7. 代码阅读练习

**练习1：追踪三角形形函数创建**

```cpp
// 在BasisFactory.cpp中查找
const nodalBasis *BasisFactory::getNodalBasis(int tag)
{
    // 检查缓存
    auto it = fs.find(tag);
    if(it != fs.end()) {
        return it->second;
    }

    // 创建新基函数
    nodalBasis *F = nullptr;
    int parentType = ElementType::getParentType(tag);
    int order = ElementType::getOrder(tag);

    switch(parentType) {
        case TYPE_TRI:
            F = new polynomialBasis(tag);
            break;
        // ...其他类型
    }

    fs[tag] = F;
    return F;
}
```

**练习2：理解形函数求值**

```cpp
// polynomialBasis::f() 的实现逻辑
void polynomialBasis::f(double u, double v, double w, double *sf) const
{
    // 对每个形函数
    for(int i = 0; i < numFunctions; i++) {
        sf[i] = 0.0;
        // 对每个单项式
        for(int j = 0; j < numMonomials; j++) {
            // sf[i] += coeff[i][j] * u^a * v^b * w^c
            double monomial = coefficients(i, j);
            monomial *= pow(u, monomials(j, 0));
            monomial *= pow(v, monomials(j, 1));
            monomial *= pow(w, monomials(j, 2));
            sf[i] += monomial;
        }
    }
}
```

---

## 练习作业

### 基础练习

**练习1**：手工计算线性三角形形函数

```cpp
// 给定点P(0.3, 0.2)在参考三角形中
// 计算三个形函数值

// 提示：
// N1 = 1 - u - v = ?
// N2 = u = ?
// N3 = v = ?
// 验证：N1 + N2 + N3 = ?
```

### 进阶练习

**练习2**：阅读并注释nodalBasis代码

```cpp
// 打开 src/numeric/nodalBasis.cpp
// 找到并注释以下函数：

// 1. 构造函数中如何初始化节点坐标
// 2. f()函数如何计算形函数值
// 3. df()函数如何计算形函数导数
```

**练习3**：实现简单的形函数求值器

```cpp
#include <iostream>
#include <cmath>

// 线性三角形形函数
class LinearTriangleBasis {
public:
    // 计算形函数值
    void f(double u, double v, double sf[3]) const {
        sf[0] = 1.0 - u - v;  // N1
        sf[1] = u;             // N2
        sf[2] = v;             // N3
    }

    // 计算形函数梯度
    void df(double u, double v,
            double dsfdu[3], double dsfdv[3]) const {
        // dN1/du, dN2/du, dN3/du
        dsfdu[0] = -1.0;
        dsfdu[1] = 1.0;
        dsfdu[2] = 0.0;

        // dN1/dv, dN2/dv, dN3/dv
        dsfdv[0] = -1.0;
        dsfdv[1] = 0.0;
        dsfdv[2] = 1.0;
    }

    // 通过形函数插值坐标
    void interpolate(double u, double v,
                     double x[3], double y[3],
                     double &px, double &py) const {
        double sf[3];
        f(u, v, sf);
        px = sf[0]*x[0] + sf[1]*x[1] + sf[2]*x[2];
        py = sf[0]*y[0] + sf[1]*y[1] + sf[2]*y[2];
    }
};

int main() {
    LinearTriangleBasis basis;

    // 物理三角形顶点
    double x[3] = {0.0, 1.0, 0.5};
    double y[3] = {0.0, 0.0, 1.0};

    // 参考坐标
    double u = 0.25, v = 0.25;

    // 形函数值
    double sf[3];
    basis.f(u, v, sf);
    std::cout << "Shape functions at (" << u << "," << v << "):\n";
    std::cout << "N1=" << sf[0] << " N2=" << sf[1] << " N3=" << sf[2] << "\n";
    std::cout << "Sum = " << sf[0]+sf[1]+sf[2] << "\n";

    // 插值坐标
    double px, py;
    basis.interpolate(u, v, x, y, px, py);
    std::cout << "Physical point: (" << px << ", " << py << ")\n";

    return 0;
}
```

### 挑战练习

**练习4**：扩展到二次三角形

```cpp
// 二次三角形（P2）有6个节点
// 节点：3个顶点 + 3个边中点
// 形函数使用完整的二次多项式

class QuadraticTriangleBasis {
public:
    // 6个节点坐标（参考空间）
    // (0,0), (1,0), (0,1), (0.5,0), (0.5,0.5), (0,0.5)

    void f(double u, double v, double sf[6]) const {
        double L1 = 1.0 - u - v;  // 重心坐标
        double L2 = u;
        double L3 = v;

        // 顶点形函数（角节点）
        sf[0] = L1 * (2*L1 - 1);
        sf[1] = L2 * (2*L2 - 1);
        sf[2] = L3 * (2*L3 - 1);

        // 边中点形函数
        sf[3] = 4 * L1 * L2;  // 边0-1的中点
        sf[4] = 4 * L2 * L3;  // 边1-2的中点
        sf[5] = 4 * L3 * L1;  // 边2-0的中点
    }

    // TODO: 实现df()计算梯度
};
```

---

## 知识图谱

```
基函数系统
│
├── BasisFactory（工厂类）
│   ├── getNodalBasis() ─── 节点基
│   ├── getJacobianBasis() ─ 雅可比基
│   └── getBezierBasis() ─── Bezier基
│
├── nodalBasis（节点基函数基类）
│   ├── type, order, dimension
│   ├── points（节点坐标）
│   ├── closures（边界闭包）
│   └── f(), df(), ddf()
│
├── polynomialBasis（多项式基函数）
│   ├── monomials（单项式指数）
│   ├── coefficients（系数）
│   └── Lagrange插值实现
│
└── 应用场景
    ├── 有限元形函数
    ├── 几何映射
    └── 场量插值
```

---

## 关键源码索引

| 文件 | 核心内容 | 行数参考 |
|------|----------|----------|
| BasisFactory.h | 工厂接口定义 | ~50行 |
| BasisFactory.cpp | 基函数创建逻辑 | ~200行 |
| nodalBasis.h | 节点基类定义 | ~100行 |
| nodalBasis.cpp | 节点基实现 | ~900行 |
| polynomialBasis.h | 多项式基定义 | ~50行 |
| polynomialBasis.cpp | 多项式基实现 | ~600行 |

---

## 今日检查点

- [ ] 理解BasisFactory的工厂模式设计
- [ ] 能解释节点基函数的核心属性
- [ ] 理解形函数的数学定义和性质
- [ ] 能手工计算线性三角形的形函数值
- [ ] 理解闭包(closure)在高阶元素中的作用

---

## 导航

- 上一天：[Day 49 - 第七周复习](day-49.md)
- 下一天：[Day 51 - 数值积分](day-51.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
