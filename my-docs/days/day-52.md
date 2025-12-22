# Day 52：层次基函数

## 学习目标
理解Gmsh中层次基函数（Hierarchical Basis）的设计理念，掌握H1和Hcurl空间的基函数实现。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习层次基函数理论 |
| 10:00-11:00 | 1h | 阅读HierarchicalBasis基类 |
| 11:00-12:00 | 1h | 分析H1空间三角形基函数 |
| 14:00-15:00 | 1h | 了解Hcurl基函数并完成练习 |

---

## 上午学习内容

### 1. 层次基函数vs节点基函数

**节点基函数（Lagrange基）**：
```
特点：
- 每个基函数对应一个几何节点
- 满足Kronecker性质：Nᵢ(xⱼ) = δᵢⱼ
- p细化需要重新计算所有基函数

问题：
- 阶数增加时，条件数恶化
- 不便于p自适应和hp自适应
```

**层次基函数（Hierarchical基）**：
```
特点：
- 基函数按多项式阶数分层
- 高阶基函数是低阶的"补充"
- p细化只需添加新基函数，保留原有基函数

优势：
- 更好的条件数
- 便于p自适应
- 便于多级预处理
```

**对比示意（1D）**：

```
Lagrange基（3节点）：      层次基（3阶）：

L₁   L₂   L₃              H₁   H₂   H₃
 \   |   /                 |   ∩    ∩
  \  |  /                  |  /\   /\
   \_|_/                   | /  \ /  \
  __/|\__                  |/____\____\
                         顶点  边1  边2
```

### 2. HierarchicalBasis基类

**文件位置**：`src/numeric/HierarchicalBasis.h`

```cpp
class HierarchicalBasis {
protected:
    int _nvertex;          // 顶点函数数量
    int _nedge;            // 每条边的内部函数数量
    int _nface;            // 每个面的内部函数数量
    int _nvolume;          // 体内部函数数量
    int _nfaceTri;         // 三角形面函数数
    int _nfaceQuad;        // 四边形面函数数
    int _type;             // 元素类型
    int _order;            // 多项式阶数

public:
    // 获取函数空间信息
    int getNumVertexFunctions() const { return _nvertex; }
    int getNumEdgeFunctions() const { return _nedge; }
    int getNumFaceFunctions() const { return _nface; }
    int getNumVolumeFunctions() const { return _nvolume; }

    // 总函数数量
    int getNumShapeFunctions() const {
        return _nvertex + _nedge + _nface + _nvolume;
    }

    // 核心虚函数：计算基函数值
    virtual void generateBasis(
        double const &u, double const &v, double const &w,
        std::vector<double> &vertexBasis,
        std::vector<double> &edgeBasis,
        std::vector<double> &faceBasis,
        std::vector<double> &bubbleBasis) = 0;

    // 计算基函数梯度
    virtual void generateGradBasis(
        double const &u, double const &v, double const &w,
        std::vector<std::vector<double>> &gradVertexBasis,
        std::vector<std::vector<double>> &gradEdgeBasis,
        std::vector<std::vector<double>> &gradFaceBasis,
        std::vector<std::vector<double>> &gradBubbleBasis) = 0;

    // 获取方向信息（用于边和面函数）
    virtual void getKeysInfo(
        std::vector<int> &functionTypeInfo,
        std::vector<int> &orderInfo) = 0;
};
```

### 3. H1空间三角形基函数

**文件位置**：`src/numeric/HierarchicalBasisH1Tria.h/cpp`

**H1空间定义**：
```
H¹(Ω) = {v ∈ L²(Ω) : ∇v ∈ L²(Ω)}

即函数本身和它的梯度都是平方可积的。
这是最常用的有限元空间，适用于标量问题（热传导、势场等）。
```

**三角形H1层次基函数结构**：

```cpp
class HierarchicalBasisH1Tria : public HierarchicalBasisH1 {
    // p阶三角形：
    // - 3个顶点函数
    // - 3条边，每条边有(p-1)个内部函数
    // - 1个面，有(p-1)(p-2)/2个内部函数

public:
    HierarchicalBasisH1Tria(int order);

    void generateBasis(double const &u, double const &v, double const &w,
                       std::vector<double> &vertexBasis,
                       std::vector<double> &edgeBasis,
                       std::vector<double> &faceBasis,
                       std::vector<double> &bubbleBasis) override;
};
```

**基函数层次结构**：

```
P1（1阶）：3个顶点函数
    2
    |\
    | \    顶点函数：
    |  \   φ₁ = L₁ = 1-u-v
    |___\  φ₂ = L₂ = u
    0    1 φ₃ = L₃ = v

P2（2阶）：3个顶点 + 3条边×1个 = 6个函数
    2
    |\
    5 4    边函数（每边1个）：
    |  \   ψ₃₄ = L₁L₂（边0-1）
    |___\  ψ₄₅ = L₂L₃（边1-2）
    0  3 1 ψ₅₆ = L₃L₁（边2-0）

P3（3阶）：3 + 6 + 1 = 10个函数
    2
    |\
    | \    新增边函数（每边2个）
    |  \   新增面函数（1个泡函数）
    |___\  面泡函数：L₁L₂L₃
    0    1
```

### 4. Legendre多项式在边函数中的应用

```cpp
// 边函数使用移位Legendre多项式

// 1D Legendre多项式（在[-1,1]上正交）
// P₀(x) = 1
// P₁(x) = x
// P₂(x) = (3x² - 1)/2
// P₃(x) = (5x³ - 3x)/2

// 移位到[0,1]区间的Legendre多项式
// L̃ₙ(t) = Pₙ(2t - 1)

// 边函数定义：
// φ_edge,i = (端点基乘积) × (Legendre多项式)
// 例如边0-1上的第i个内部函数：
// ψᵢ = L₁ · L₂ · L̃ᵢ₋₂(L₂)  其中i ≥ 2
```

---

## 下午学习内容

### 5. Hcurl空间基函数

**文件位置**：`src/numeric/HierarchicalBasisHcurl.h`

**Hcurl空间定义**：
```
H(curl; Ω) = {v ∈ L²(Ω)³ : ∇×v ∈ L²(Ω)³}

向量场v的旋度也是平方可积的。
适用于电磁场问题（Maxwell方程）。
```

**Nédélec元素特点**：
```
- 基函数是向量值的
- 边界上保证切向连续性（非完全连续）
- 自然表示电场/磁场
```

```cpp
class HierarchicalBasisHcurl : public HierarchicalBasis {
public:
    // 生成向量基函数
    virtual void generateCurlBasis(
        double const &u, double const &v, double const &w,
        std::vector<std::vector<double>> &edgeBasis,
        std::vector<std::vector<double>> &faceBasis,
        std::vector<std::vector<double>> &bubbleBasis) = 0;

    // 生成旋度
    virtual void generateCurlOfCurlBasis(
        double const &u, double const &v, double const &w,
        std::vector<std::vector<double>> &curlEdgeBasis,
        std::vector<std::vector<double>> &curlFaceBasis,
        std::vector<std::vector<double>> &curlBubbleBasis) = 0;
};
```

### 6. 三角形Hcurl基函数

**文件位置**：`src/numeric/HierarchicalBasisHcurlTria.h/cpp`

```cpp
class HierarchicalBasisHcurlTria : public HierarchicalBasisHcurl {
    // Nédélec元素（1阶）：
    // - 3条边，每边1个基函数
    // - 没有顶点函数（旋度空间的特点）

    // 边基函数形式：
    // w_e = L₁∇L₂ - L₂∇L₁
    // 其中L₁, L₂是边的两个端点的重心坐标
};
```

**1阶Nédélec边函数**：

```
边0-1的基函数：
w₁ = L₁∇L₂ - L₂∇L₁
   = (1-u-v)·(1,0) - u·(-1,-1)
   = (1-v, v)

边1-2的基函数：
w₂ = L₂∇L₃ - L₃∇L₂
   = u·(0,1) - v·(1,0)
   = (-v, u)

边2-0的基函数：
w₃ = L₃∇L₁ - L₁∇L₃
   = v·(-1,-1) - (1-u-v)·(0,1)
   = (-v, u+v-1)
```

### 7. FuncSpaceData：函数空间数据

**文件位置**：`src/numeric/FuncSpaceData.h`

```cpp
// 描述有限元函数空间的配置数据
class FuncSpaceData {
public:
    int _type;           // 元素类型
    bool _serendipity;   // 是否为Serendipity元素
    int _spaceOrder;     // 空间阶数
    int _nij;            // 形函数数量

    // 获取基函数
    const HierarchicalBasis *getHierarchicalBasis() const;
    const nodalBasis *getNodalBasis() const;

    // 空间类型
    enum SpaceType {
        H1,      // 标量，连续
        Hcurl,   // 向量，切向连续
        Hdiv,    // 向量，法向连续
        L2       // 间断
    };
};
```

---

## 练习作业

### 基础练习

**练习1**：计算P2三角形的基函数数量

```cpp
// P阶三角形层次基函数数量计算
//
// 顶点函数：3（固定）
// 边函数：3 × (p-1)
// 面函数：(p-1)(p-2)/2
//
// 总数 = 3 + 3(p-1) + (p-1)(p-2)/2
//      = (p+1)(p+2)/2
//
// 验证：
// P1: 3 + 0 + 0 = 3 ✓
// P2: 3 + 3 + 0 = 6 ✓
// P3: 3 + 6 + 1 = 10 ✓
// P4: 3 + 9 + 3 = 15 ✓
```

### 进阶练习

**练习2**：实现简化的层次基函数

```cpp
#include <iostream>
#include <vector>
#include <cmath>

// 简化的P2三角形层次基函数
class HierarchicalTriP2 {
public:
    // 3个顶点函数 + 3个边函数 = 6个基函数

    void generateBasis(double u, double v,
                       std::vector<double> &vertexBasis,
                       std::vector<double> &edgeBasis) {
        // 重心坐标
        double L1 = 1.0 - u - v;
        double L2 = u;
        double L3 = v;

        // 顶点函数（线性部分）
        vertexBasis.resize(3);
        vertexBasis[0] = L1;
        vertexBasis[1] = L2;
        vertexBasis[2] = L3;

        // 边函数（二次修正）
        // 这些函数在顶点处为0，在边中点处非零
        edgeBasis.resize(3);
        edgeBasis[0] = 4.0 * L1 * L2;  // 边0-1
        edgeBasis[1] = 4.0 * L2 * L3;  // 边1-2
        edgeBasis[2] = 4.0 * L3 * L1;  // 边2-0
    }

    void generateGradBasis(double u, double v,
                           std::vector<std::vector<double>> &gradVertex,
                           std::vector<std::vector<double>> &gradEdge) {
        double L1 = 1.0 - u - v;
        double L2 = u;
        double L3 = v;

        // ∇L1 = (-1, -1), ∇L2 = (1, 0), ∇L3 = (0, 1)

        // 顶点函数梯度
        gradVertex.resize(3, std::vector<double>(2));
        gradVertex[0] = {-1.0, -1.0};  // ∇L1
        gradVertex[1] = {1.0, 0.0};    // ∇L2
        gradVertex[2] = {0.0, 1.0};    // ∇L3

        // 边函数梯度（使用乘积法则）
        // ∇(L1·L2) = L2·∇L1 + L1·∇L2
        gradEdge.resize(3, std::vector<double>(2));
        gradEdge[0] = {4.0*(L2*(-1) + L1*1), 4.0*(L2*(-1) + L1*0)};
        gradEdge[1] = {4.0*(L3*1 + L2*0), 4.0*(L3*0 + L2*1)};
        gradEdge[2] = {4.0*(L1*0 + L3*(-1)), 4.0*(L1*1 + L3*(-1))};
    }
};

int main() {
    HierarchicalTriP2 basis;

    // 测试点
    double u = 0.5, v = 0.0;  // 边0-1的中点

    std::vector<double> vb, eb;
    basis.generateBasis(u, v, vb, eb);

    std::cout << "At edge midpoint (0.5, 0):\n";
    std::cout << "Vertex basis: " << vb[0] << " " << vb[1] << " " << vb[2] << "\n";
    std::cout << "Edge basis: " << eb[0] << " " << eb[1] << " " << eb[2] << "\n";

    // 验证：在边中点，对应的边函数应该取最大值
    // 边0-1中点：L1=L2=0.5, L3=0
    // eb[0] = 4*0.5*0.5 = 1.0 ✓
    // eb[1] = 4*0.5*0 = 0 ✓
    // eb[2] = 4*0*0.5 = 0 ✓

    return 0;
}
```

**练习3**：比较层次基和节点基的性质

```cpp
#include <iostream>
#include <cmath>

// 节点基（Lagrange P2）
class LagrangeTriP2 {
public:
    void f(double u, double v, double sf[6]) {
        double L1 = 1.0 - u - v;
        double L2 = u;
        double L3 = v;

        // 顶点节点（使用二次Lagrange公式）
        sf[0] = L1 * (2*L1 - 1);  // 节点0
        sf[1] = L2 * (2*L2 - 1);  // 节点1
        sf[2] = L3 * (2*L3 - 1);  // 节点2

        // 边中点节点
        sf[3] = 4 * L1 * L2;  // 节点3（边0-1中点）
        sf[4] = 4 * L2 * L3;  // 节点4（边1-2中点）
        sf[5] = 4 * L3 * L1;  // 节点5（边2-0中点）
    }
};

// 层次基（已在练习2实现）
// ...

// 对比：
// 1. 节点基：在节点i处，φᵢ=1，其他=0
// 2. 层次基：顶点函数是线性的，不满足Kronecker性质
//
// 但两者张成相同的多项式空间！
// 只是基的选择不同，可以相互转换
```

### 挑战练习

**练习4**：实现简单的Nédélec边函数

```cpp
#include <iostream>
#include <vector>
#include <cmath>

// 1阶Nédélec三角形元素
class NedelecTriP1 {
public:
    // 3条边，每边1个向量基函数
    // 返回：每个基函数在(u,v)点的(wx, wy)分量

    void generateBasis(double u, double v,
                       std::vector<std::vector<double>> &edgeBasis) {
        // 重心坐标
        double L1 = 1.0 - u - v;
        double L2 = u;
        double L3 = v;

        // 重心坐标梯度
        // ∇L1 = (-1, -1)
        // ∇L2 = (1, 0)
        // ∇L3 = (0, 1)

        edgeBasis.resize(3, std::vector<double>(2));

        // 边0-1 (从节点0到节点1)
        // w₁ = L1·∇L2 - L2·∇L1
        edgeBasis[0][0] = L1*1.0 - L2*(-1.0);   // wx
        edgeBasis[0][1] = L1*0.0 - L2*(-1.0);   // wy

        // 边1-2 (从节点1到节点2)
        // w₂ = L2·∇L3 - L3·∇L2
        edgeBasis[1][0] = L2*0.0 - L3*1.0;
        edgeBasis[1][1] = L2*1.0 - L3*0.0;

        // 边2-0 (从节点2到节点0)
        // w₃ = L3·∇L1 - L1·∇L3
        edgeBasis[2][0] = L3*(-1.0) - L1*0.0;
        edgeBasis[2][1] = L3*(-1.0) - L1*1.0;
    }

    // 计算旋度（2D中旋度是标量）
    void generateCurl(double u, double v,
                      std::vector<double> &curlBasis) {
        // 对于Nédélec边函数，旋度是常数
        // curl(w_e) = 2 (对于标准参考三角形)

        curlBasis.resize(3);
        curlBasis[0] = 2.0;  // 边0-1
        curlBasis[1] = 2.0;  // 边1-2
        curlBasis[2] = 2.0;  // 边2-0
    }
};

int main() {
    NedelecTriP1 nedelec;

    // 测试点：三角形重心
    double u = 1.0/3.0, v = 1.0/3.0;

    std::vector<std::vector<double>> wb;
    nedelec.generateBasis(u, v, wb);

    std::cout << "Nedelec basis at centroid (" << u << ", " << v << "):\n";
    for(int i = 0; i < 3; i++) {
        std::cout << "w" << i << " = (" << wb[i][0] << ", " << wb[i][1] << ")\n";
    }

    // 验证切向连续性
    // 在边0-1上（v=0），w₁的切向分量应该只依赖于u
    std::cout << "\nAlong edge 0-1 (v=0):\n";
    for(double t = 0; t <= 1; t += 0.25) {
        std::vector<std::vector<double>> w;
        nedelec.generateBasis(t, 0, w);
        // 边0-1的切向是(1,0)
        std::cout << "u=" << t << ": w1·t = " << w[0][0] << "\n";
    }

    return 0;
}
```

---

## 知识图谱

```
层次基函数系统
│
├── HierarchicalBasis（基类）
│   ├── _nvertex, _nedge, _nface, _nvolume
│   ├── generateBasis()
│   └── generateGradBasis()
│
├── H1空间（标量，连续）
│   ├── HierarchicalBasisH1（H1基类）
│   ├── HierarchicalBasisH1Tria（三角形）
│   ├── HierarchicalBasisH1Tetra（四面体）
│   ├── HierarchicalBasisH1Quad（四边形）
│   └── HierarchicalBasisH1Brick（六面体）
│
├── Hcurl空间（向量，切向连续）
│   ├── HierarchicalBasisHcurl（Hcurl基类）
│   ├── HierarchicalBasisHcurlTria（三角形）
│   ├── HierarchicalBasisHcurlTetra（四面体）
│   └── ...
│
└── 应用场景
    ├── p自适应有限元
    ├── hp自适应有限元
    ├── 多级预处理
    └── 电磁场问题（Hcurl）
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 |
|------|----------|--------|
| HierarchicalBasis.h | 基类定义 | ~200行 |
| HierarchicalBasisH1.h | H1空间基类 | ~150行 |
| HierarchicalBasisH1Tria.cpp | 三角形H1实现 | ~700行 |
| HierarchicalBasisH1Tetra.cpp | 四面体H1实现 | ~900行 |
| HierarchicalBasisHcurl.h | Hcurl空间基类 | ~150行 |
| HierarchicalBasisHcurlTria.cpp | 三角形Hcurl | ~900行 |

---

## 今日检查点

- [ ] 理解层次基函数与节点基函数的区别
- [ ] 能解释层次基的"层次"含义
- [ ] 理解H1空间和Hcurl空间的物理意义
- [ ] 能计算P阶三角形的基函数数量
- [ ] 理解Nédélec元素的切向连续性

---

## 扩展阅读

1. **Szabó & Babuška**：《Finite Element Analysis》- 层次基的经典教材
2. **Demkowicz**：《Computing with hp-Adaptive Finite Elements》
3. **Nédélec边元素**：用于Maxwell方程的有限元

---

## 导航

- 上一天：[Day 51 - 数值积分](day-51.md)
- 下一天：[Day 53 - Jacobian基与条件数](day-53.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
