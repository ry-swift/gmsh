# Day 68：边界条件处理

## 学习目标

掌握有限元中边界条件的分类与处理方法，深入理解Dirichlet条件、Neumann条件及Lagrange乘子法在Gmsh中的实现。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 边界条件理论基础 |
| 10:00-11:00 | 1h | Dirichlet边界条件处理 |
| 11:00-12:00 | 1h | Neumann边界条件与Lagrange乘子 |
| 14:00-15:00 | 1h | 源码分析与练习 |

---

## 上午学习内容

### 1. 边界条件理论基础

**边界条件分类**：

```text
偏微分方程：-∇·(k∇u) = f  在Ω内
边界条件：
  u = g            在Γ_D上 (Dirichlet)
  k∂u/∂n = h       在Γ_N上 (Neumann)
  k∂u/∂n + αu = β  在Γ_R上 (Robin/混合)

边界划分：∂Ω = Γ_D ∪ Γ_N ∪ Γ_R
```

**物理意义对照**：

| 边界类型 | 数学形式 | 热传导 | 弹性力学 | 流体力学 |
|----------|----------|--------|----------|----------|
| Dirichlet | u = g | 固定温度 | 固定位移 | 固定速度 |
| Neumann | ∂u/∂n = h | 热通量 | 表面力 | 压力/应力 |
| Robin | ∂u/∂n + αu = β | 对流换热 | 弹性支撑 | 滑移边界 |

**弱形式中的边界条件**：

```text
强形式：
-∇·(k∇u) = f    在Ω
u = g           在Γ_D
k∂u/∂n = h      在Γ_N

弱形式（乘测试函数v并积分）：
∫_Ω k∇u·∇v dΩ = ∫_Ω f·v dΩ + ∫_{Γ_N} h·v dΓ

注意：
• Dirichlet条件：需要额外处理（不自然满足）
• Neumann条件：自然出现在边界积分中（自然边界条件）
```

### 2. Dirichlet边界条件处理

**方法对比**：

| 方法 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| 惩罚法 | 简单，不改变矩阵结构 | 数值误差，条件数恶化 | 快速原型 |
| 置换法 | 精确，无额外误差 | 改变矩阵结构 | 一般问题 |
| 消元法 | 减少系统规模 | 破坏对称性 | 小规模问题 |
| Lagrange乘子 | 通用，支持复杂约束 | 增加系统规模，鞍点问题 | 接触问题 |

**惩罚法实现**：

```cpp
// 惩罚法：K[i][i] += P, F[i] += P * g
// P是很大的数（惩罚因子）
void applyDirichletPenalty(linearSystem<double> *sys,
                           int dofIndex, double value,
                           double penalty = 1e30) {
    sys->addToMatrix(dofIndex, dofIndex, penalty);
    sys->addToRightHandSide(dofIndex, penalty * value);
}

// 原理：
// 原方程：Σⱼ Kᵢⱼ uⱼ = Fᵢ
// 添加惩罚：Σⱼ Kᵢⱼ uⱼ + P(uᵢ - g) ≈ Fᵢ
// 当P很大时：uᵢ ≈ g
```

**置换法实现**：

```cpp
// 置换法：将已知值移到右端
void applyDirichletSubstitution(dofManager<double> &dm,
                                const Dof &dof, double value) {
    // 1. 标记为固定
    dm.fixDof(dof, value);

    // 2. 在装配时处理
    // 对于装配 K[i][j] * u[j]：
    // 如果 dof[j] 是固定的：F[i] -= K[i][j] * value
    // K[i][j] = 0 和 K[j][i] = 0 (保持对称)
}

// dofManager::assemble中的处理
template<class T>
void dofManager<T>::assemble(const fullMatrix<T> &localMatrix,
                              const std::vector<Dof> &R,
                              const std::vector<Dof> &C) {
    for(int i = 0; i < R.size(); i++) {
        if(fixed.find(R[i]) != fixed.end()) continue;  // 跳过固定行

        int globalR = unknown[R[i]];

        for(int j = 0; j < C.size(); j++) {
            if(fixed.find(C[j]) != fixed.end()) {
                // 移到右端
                T val = fixed[C[j]];
                _lsys->addToRightHandSide(globalR, -localMatrix(i, j) * val);
            } else {
                int globalC = unknown[C[j]];
                _lsys->addToMatrix(globalR, globalC, localMatrix(i, j));
            }
        }
    }
}
```

**Gmsh中的Dirichlet边界条件**：

```cpp
// elasticitySolver.h 中的边界条件结构
struct BoundaryCondition {
    enum location {
        ON_VERTEX,   // 点约束
        ON_EDGE,     // 边约束
        ON_FACE,     // 面约束
        ON_VOLUME    // 体约束（少用）
    };

    location onWhat;
    groupOfElements *g;  // 约束作用的元素组
};

struct dirichletBC : public BoundaryCondition {
    int _comp;                      // 位移分量 (0=x, 1=y, 2=z, -1=all)
    simpleFunction<double> *_f;     // 位移函数

    // 应用到dofManager
    void apply(dofManager<double> &dm) {
        for(auto e : g->elements()) {
            for(int i = 0; i < e->getNumVertices(); i++) {
                MVertex *v = e->getVertex(i);
                double x = v->x(), y = v->y(), z = v->z();

                if(_comp < 0) {  // 所有分量
                    for(int c = 0; c < 3; c++) {
                        Dof d(v->getNum(), c);
                        dm.fixDof(d, (*_f)(x, y, z));
                    }
                } else {  // 指定分量
                    Dof d(v->getNum(), _comp);
                    dm.fixDof(d, (*_f)(x, y, z));
                }
            }
        }
    }
};
```

### 3. Neumann边界条件处理

**Neumann条件的自然性**：

```text
弱形式推导：
∫_Ω k∇u·∇v dΩ - ∫_∂Ω k(∂u/∂n)v dΓ = ∫_Ω fv dΩ

Neumann边界：k∂u/∂n = h

代入得：
∫_Ω k∇u·∇v dΩ = ∫_Ω fv dΩ + ∫_{Γ_N} hv dΓ

关键点：Neumann条件作为右端向量的边界积分项出现
```

**Neumann边界条件实现**：

```cpp
// elasticitySolver.h 中的Neumann边界条件
struct neumannBC : public BoundaryCondition {
    SVector3 _force;  // 表面力向量

    // 或者使用函数形式
    simpleFunction<double> *_fX, *_fY, *_fZ;
};

// 装配Neumann边界贡献
void assembleNeumannBC(dofManager<double> &dm,
                       const neumannBC &bc,
                       FunctionSpace<SVector3> *fs) {
    for(auto e : bc.g->elements()) {
        // 边界元素（2D面元素或1D线元素）
        int nNodes = e->getNumVertices();

        // 获取高斯积分点
        int order = e->getPolynomialOrder() + 1;
        IntPt *GP;
        int nGP = e->getIntegrationPoints(order, &GP);

        for(int g = 0; g < nGP; g++) {
            double u = GP[g].pt[0], v = GP[g].pt[1];
            double weight = GP[g].weight;

            // 形函数
            double s[256];
            e->getShapeFunctions(u, v, 0, s);

            // 边界元素的Jacobian（度量）
            double jac[3][3];
            double detJ = e->getJacobianDeterminant(u, v, 0);

            // 获取表面力
            double x, y, z;
            e->pnt(u, v, 0, x, y, z);
            SVector3 force = bc._force;
            // 或 force = SVector3(bc._fX(x,y,z), bc._fY(x,y,z), bc._fZ(x,y,z));

            // 装配到右端向量
            for(int i = 0; i < nNodes; i++) {
                MVertex *vi = e->getVertex(i);
                for(int comp = 0; comp < 3; comp++) {
                    Dof d(vi->getNum(), comp);
                    double contribution = s[i] * force[comp] * weight * detJ;
                    dm.addToRHS(d, contribution);
                }
            }
        }
    }
}
```

---

## 下午学习内容

### 4. Lagrange乘子法

**原理**：

```text
约束优化问题：
min ½u^T K u - u^T F
subject to: Cu = g

引入Lagrange乘子λ：
L(u, λ) = ½u^T K u - u^T F + λ^T(Cu - g)

鞍点条件：
∂L/∂u = 0  →  Ku + C^T λ = F
∂L/∂λ = 0  →  Cu = g

矩阵形式：
[K   C^T] [u]   [F]
[C   0  ] [λ] = [g]
```

**鞍点系统特点**：

```text
优点：
• 精确满足约束
• 直接得到约束反力（λ）
• 支持复杂约束（如接触、滑移）

缺点：
• 增加系统规模
• 矩阵不正定（需要特殊求解器）
• 可能有零对角元
```

**Gmsh中的Lagrange乘子实现**：

```cpp
// elasticitySolver.h
struct LagrangeMultiplierField {
    int _tag;
    groupOfElements *g;           // 约束作用域
    SVector3 _direction;          // 约束方向
    simpleFunction<double> *_f;   // 约束值函数

    // Lagrange乘子的自由度编号
    // 使用特殊的实体编号（负数或大数）区分
};

// 装配Lagrange乘子
void assembleLagrangeMultipliers(dofManager<double> &dm,
                                 const LagrangeMultiplierField &lm) {
    // 1. 为Lagrange乘子创建自由度
    int lmDofBase = 1000000 + lm._tag * 10000;

    int lmIndex = 0;
    for(auto e : lm.g->elements()) {
        for(int i = 0; i < e->getNumVertices(); i++) {
            MVertex *v = e->getVertex(i);

            // Lagrange乘子自由度
            Dof lmDof(lmDofBase + lmIndex, 0);
            dm.numberDof(lmDof);
            lmIndex++;

            // 位移自由度
            for(int comp = 0; comp < 3; comp++) {
                Dof dispDof(v->getNum(), comp);

                // 约束方程：n·u = g
                // 添加到系统：
                // C矩阵：第lm行，第disp列 = n[comp]
                // C^T矩阵：第disp行，第lm列 = n[comp]

                double coeff = lm._direction[comp];
                if(std::abs(coeff) > 1e-15) {
                    // 离散化约束
                    // TODO: 实际实现需要配合边界积分
                }
            }

            // 右端项：g值
            double x = v->x(), y = v->y(), z = v->z();
            dm.addToRHS(lmDof, (*lm._f)(x, y, z));
        }
    }
}
```

### 5. 线性约束处理

**DofAffineConstraint结构**：

```cpp
// dofManager.h
template<class T>
struct DofAffineConstraint {
    // 约束形式：DOF = Σ(cⱼ * DOFⱼ) + shift
    std::vector<std::pair<Dof, T>> linear;  // 线性项
    T shift;                                 // 常数项
};

// 设置约束
template<class T>
void dofManager<T>::setLinearConstraint(const Dof &d,
                                         const DofAffineConstraint<T> &c) {
    constraints[d] = c;
    // 约束自由度不再编号为独立未知量
}
```

**约束应用场景**：

```text
1. 周期边界条件
   u(x=0) = u(x=L)
   约束：DOF[x=L] = 1.0 * DOF[x=0] + 0

2. 刚性连接
   u_B = u_A + θ × (r_B - r_A)
   约束：DOF_B[i] = DOF_A[i] + rotation_contribution

3. 对称边界
   u_n = 0 (法向位移为零)
   约束：n·u = 0
```

**约束展开在装配中的处理**：

```cpp
// 装配时展开约束
template<class T>
void dofManager<T>::assembleWithConstraint(
    const fullMatrix<T> &localMatrix,
    const std::vector<Dof> &R,
    const std::vector<Dof> &C,
    int constrainedRow) {

    Dof constDof = R[constrainedRow];
    const DofAffineConstraint<T> &constr = constraints[constDof];

    // 将约束自由度的贡献分配给主自由度
    for(auto &[masterDof, coeff] : constr.linear) {
        int masterRow = unknown[masterDof];
        if(masterRow < 0) continue;

        for(int j = 0; j < C.size(); j++) {
            int globalC = getGlobalCol(C[j]);
            if(globalC < 0) continue;

            // K[master][j] += coeff * K[constrained][j]
            _lsys->addToMatrix(masterRow, globalC,
                               coeff * localMatrix(constrainedRow, j));
        }
    }

    // 处理shift（常数项）
    if(std::abs(constr.shift) > 1e-15) {
        for(int j = 0; j < C.size(); j++) {
            int globalC = getGlobalCol(C[j]);
            // F[j] -= K[j][constrained] * shift
            // 类似Dirichlet条件处理
        }
    }
}
```

### 6. elasticitySolver边界条件完整流程

```cpp
// elasticitySolver.cpp 中的边界条件处理
void elasticitySolver::applyBoundaryConditions() {
    // 1. 应用Dirichlet边界条件
    for(auto &bc : allDirichlet) {
        for(auto e : bc.g->elements()) {
            for(int i = 0; i < e->getNumVertices(); i++) {
                MVertex *v = e->getVertex(i);
                double val = (*bc._f)(v->x(), v->y(), v->z());

                if(bc._comp < 0) {
                    // 所有分量
                    for(int c = 0; c < 3; c++) {
                        pAssembler->fixDof(Dof(v->getNum(), c), val);
                    }
                } else {
                    pAssembler->fixDof(Dof(v->getNum(), bc._comp), val);
                }
            }
        }
    }

    // 2. 应用Lagrange乘子约束
    for(auto &lm : LMFields) {
        // 特殊处理...
    }
}

void elasticitySolver::assembleNeumannBCs() {
    // 在装配阶段处理Neumann条件
    for(auto &bc : allNeumann) {
        for(auto e : bc.g->elements()) {
            // 边界积分
            integrateSurfaceTraction(e, bc._force);
        }
    }
}
```

---

## 练习作业

### 基础练习

**练习1**：边界条件分类

```text
给定问题：
-Δu = 1  在Ω = [0,1]×[0,1]
u = 0    在x=0和x=1
∂u/∂n = 0 在y=0和y=1

1. 识别边界条件类型
2. 画出边界划分示意图
3. 写出对应的弱形式
```

**练习2**：惩罚法数值实验

```cpp
// 使用不同惩罚因子求解简单问题
// 观察解的精度和条件数

void testPenaltyMethod() {
    double penalties[] = {1e3, 1e6, 1e9, 1e12, 1e15};

    for(double P : penalties) {
        // 构建系统
        // 应用边界条件
        // 求解
        // 计算误差
        // 估计条件数
    }
}

// 问题：
// 1. 惩罚因子太小有什么问题？
// 2. 惩罚因子太大有什么问题？
// 3. 如何选择合适的惩罚因子？
```

### 进阶练习

**练习3**：实现完整的边界条件处理

```cpp
#include <iostream>
#include <vector>
#include <map>
#include <cmath>

class FEMWithBC {
public:
    struct Node {
        double x, y;
    };

    struct Triangle {
        int nodes[3];
    };

    enum BCType { DIRICHLET, NEUMANN };

    struct BoundaryCondition {
        BCType type;
        int node;       // 对于Dirichlet
        int edge[2];    // 对于Neumann（边的两个端点）
        double value;
    };

private:
    std::vector<Node> _nodes;
    std::vector<Triangle> _elements;
    std::vector<BoundaryCondition> _bcs;

    int _nDofs;
    std::vector<std::vector<double>> _K;
    std::vector<double> _F;
    std::map<int, double> _fixedNodes;

public:
    void addNode(double x, double y) {
        _nodes.push_back({x, y});
    }

    void addElement(int n1, int n2, int n3) {
        _elements.push_back({{n1, n2, n3}});
    }

    void addDirichletBC(int node, double value) {
        _bcs.push_back({DIRICHLET, node, {-1, -1}, value});
        _fixedNodes[node] = value;
    }

    void addNeumannBC(int n1, int n2, double flux) {
        _bcs.push_back({NEUMANN, -1, {n1, n2}, flux});
    }

    // 计算元素刚度矩阵
    void computeElementStiffness(const Triangle &e, double Ke[3][3]) {
        // ... (同Day 67)
    }

    // 计算边界积分（Neumann）
    void computeEdgeLoad(int n1, int n2, double flux, double Fe[2]) {
        const Node &p1 = _nodes[n1];
        const Node &p2 = _nodes[n2];

        // 边长
        double L = std::sqrt(std::pow(p2.x - p1.x, 2) +
                            std::pow(p2.y - p1.y, 2));

        // 线性形函数在边上积分
        // ∫₀¹ N₁ ds = L/2,  ∫₀¹ N₂ ds = L/2
        Fe[0] = flux * L / 2.0;
        Fe[1] = flux * L / 2.0;
    }

    void assemble() {
        _nDofs = _nodes.size();
        _K.assign(_nDofs, std::vector<double>(_nDofs, 0.0));
        _F.assign(_nDofs, 0.0);

        // 1. 装配刚度矩阵
        for(const auto &e : _elements) {
            double Ke[3][3];
            computeElementStiffness(e, Ke);

            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 3; j++) {
                    _K[e.nodes[i]][e.nodes[j]] += Ke[i][j];
                }
            }
        }

        // 2. 应用Neumann边界条件
        for(const auto &bc : _bcs) {
            if(bc.type == NEUMANN) {
                double Fe[2];
                computeEdgeLoad(bc.edge[0], bc.edge[1], bc.value, Fe);
                _F[bc.edge[0]] += Fe[0];
                _F[bc.edge[1]] += Fe[1];
            }
        }

        // 3. 应用Dirichlet边界条件（置换法）
        for(const auto &[node, val] : _fixedNodes) {
            // 修改右端向量
            for(int i = 0; i < _nDofs; i++) {
                if(_fixedNodes.find(i) == _fixedNodes.end()) {
                    _F[i] -= _K[i][node] * val;
                }
            }

            // 修改矩阵（保持对称性）
            for(int i = 0; i < _nDofs; i++) {
                _K[i][node] = 0.0;
                _K[node][i] = 0.0;
            }
            _K[node][node] = 1.0;
            _F[node] = val;
        }
    }

    std::vector<double> solve() {
        // 高斯消元求解
        // ... (同Day 67)
    }
};

int main() {
    FEMWithBC fem;

    // 创建网格
    fem.addNode(0, 0);  // 0
    fem.addNode(1, 0);  // 1
    fem.addNode(1, 1);  // 2
    fem.addNode(0, 1);  // 3

    fem.addElement(0, 1, 2);
    fem.addElement(0, 2, 3);

    // 边界条件
    fem.addDirichletBC(0, 0.0);  // 左下角u=0
    fem.addDirichletBC(3, 0.0);  // 左上角u=0
    fem.addNeumannBC(1, 2, 1.0); // 右边界热通量

    fem.assemble();
    auto u = fem.solve();

    // 输出结果
    for(int i = 0; i < u.size(); i++) {
        std::cout << "u[" << i << "] = " << u[i] << "\n";
    }

    return 0;
}
```

### 挑战练习

**练习4**：实现周期边界条件

```cpp
// 周期边界条件：u(x=0) = u(x=L)
// 使用线性约束实现

class PeriodicBC {
public:
    // 找到左右边界上对应的节点对
    void findMatchingNodes(const std::vector<Node> &nodes,
                           double L, double tol = 1e-10) {
        for(int i = 0; i < nodes.size(); i++) {
            if(std::abs(nodes[i].x) < tol) {  // 左边界
                // 找右边界对应节点
                for(int j = 0; j < nodes.size(); j++) {
                    if(std::abs(nodes[j].x - L) < tol &&
                       std::abs(nodes[i].y - nodes[j].y) < tol) {
                        // 创建约束：u[j] = u[i]
                        _pairs.push_back({i, j});
                    }
                }
            }
        }
    }

    void apply(dofManager<double> &dm) {
        for(auto &[master, slave] : _pairs) {
            // 设置约束：DOF[slave] = DOF[master]
            DofAffineConstraint<double> c;
            c.linear.push_back({Dof(master, 0), 1.0});
            c.shift = 0.0;
            dm.setLinearConstraint(Dof(slave, 0), c);
        }
    }

private:
    std::vector<std::pair<int, int>> _pairs;
};
```

**练习5**：分析Gmsh边界条件源码

```cpp
// 阅读 src/solver/elasticitySolver.cpp

// 问题：
// 1. addDirichletBC如何处理不同维度的约束？
// 2. Neumann边界条件如何进行边界积分？
// 3. LagrangeMultiplierField如何创建和装配？
```

---

## 知识图谱

```text
边界条件处理
│
├── 边界条件类型
│   ├── Dirichlet（本质边界条件）
│   ├── Neumann（自然边界条件）
│   └── Robin（混合边界条件）
│
├── Dirichlet处理方法
│   ├── 惩罚法
│   ├── 置换法
│   ├── 消元法
│   └── Lagrange乘子法
│
├── Neumann处理
│   ├── 弱形式边界积分
│   ├── 边界元素积分
│   └── 右端向量贡献
│
├── Lagrange乘子
│   ├── 鞍点系统
│   ├── 约束矩阵
│   └── 接触/滑移应用
│
├── 线性约束
│   ├── DofAffineConstraint
│   ├── 周期边界
│   └── 刚性连接
│
└── Gmsh实现
    ├── BoundaryCondition结构
    ├── dirichletBC/neumannBC
    └── elasticitySolver中的处理
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 | 优先级 |
|------|----------|--------|--------|
| elasticitySolver.h | 边界条件结构定义 | ~200行 | ★★★★★ |
| elasticitySolver.cpp | 边界条件应用 | ~1000行 | ★★★★★ |
| dofManager.h | fixDof, setLinearConstraint | ~732行 | ★★★★☆ |
| thermicSolver.h | 热学边界条件 | ~100行 | ★★★☆☆ |

---

## 今日检查点

- [ ] 理解三种边界条件的物理意义
- [ ] 掌握Dirichlet条件的多种处理方法
- [ ] 理解Neumann条件在弱形式中的自然性
- [ ] 了解Lagrange乘子法的原理
- [ ] 掌握线性约束的设置方法
- [ ] 完成边界条件练习程序

---

## 导航

- 上一天：[Day 67 - 有限元装配](day-67.md)
- 下一天：[Day 69 - 求解器后端](day-69.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
