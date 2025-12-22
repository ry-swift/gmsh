# Day 67：有限元装配

## 学习目标

深入理解有限元装配过程，掌握femTerm类的设计、dofManager的装配机制，以及从局部矩阵到全局系统的映射原理。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 有限元装配理论基础 |
| 10:00-11:00 | 1h | femTerm类体系分析 |
| 11:00-12:00 | 1h | dofManager装配机制 |
| 14:00-15:00 | 1h | 综合练习 |

---

## 上午学习内容

### 1. 有限元装配理论基础

**有限元方程的形成**：

```text
变分形式：
∫_Ω (∇u·∇v + c·u·v) dΩ = ∫_Ω f·v dΩ

离散化：u ≈ Σ uᵢ·Nᵢ(x)  （Nᵢ是基函数）

代入得：
Σⱼ uⱼ ∫_Ω (∇Nⱼ·∇Nᵢ + c·Nⱼ·Nᵢ) dΩ = ∫_Ω f·Nᵢ dΩ

矩阵形式：
Ku = F

其中：
Kᵢⱼ = ∫_Ω (∇Nⱼ·∇Nᵢ + c·Nⱼ·Nᵢ) dΩ
Fᵢ = ∫_Ω f·Nᵢ dΩ
```

**元素装配过程**：

```text
全局系统 = Σ 局部元素贡献

对于元素e：
  局部刚度矩阵 Kᵉ (n×n, n=元素节点数)
  局部载荷向量 Fᵉ (n×1)

装配规则：
  K[global_i][global_j] += Kᵉ[local_i][local_j]
  F[global_i] += Fᵉ[local_i]

其中：global_i = 节点i的全局自由度编号
```

**装配示意图**：

```text
元素1 (节点1,2,3)           元素2 (节点2,3,4)
┌─────────────┐            ┌─────────────┐
│ k₁₁ k₁₂ k₁₃ │            │ k₂₂ k₂₃ k₂₄ │
│ k₂₁ k₂₂ k₂₃ │            │ k₃₂ k₃₃ k₃₄ │
│ k₃₁ k₃₂ k₃₃ │            │ k₄₂ k₄₃ k₄₄ │
└─────────────┘            └─────────────┘

装配到全局：
┌──────────────────────┐
│ K₁₁  K₁₂  K₁₃   0   │  ← 只有元素1贡献
│ K₂₁  K₂₂  K₂₃  K₂₄  │  ← 元素1和2都贡献
│ K₃₁  K₃₂  K₃₃  K₃₄  │  ← 元素1和2都贡献
│  0   K₄₂  K₄₃  K₄₄  │  ← 只有元素2贡献
└──────────────────────┘

注意：K₂₂, K₂₃, K₃₂, K₃₃ 由两个元素累加
```

### 2. femTerm类体系

**文件位置**：`src/solver/femTerm.h`

```cpp
// 有限元项基类模板
template<class T>
class femTerm {
protected:
    GModel *_gm;                  // 几何模型
    FunctionSpace<T> *_fs;        // 函数空间
    T _K;                         // 材料参数（可选）

public:
    femTerm(GModel *gm, FunctionSpace<T> *fs, T K = T(1))
        : _gm(gm), _fs(fs), _K(K) {}

    virtual ~femTerm() {}

    // ========== 尺寸信息 ==========

    // 行自由度数（测试函数空间）
    virtual int sizeOfR(SElement *se) const = 0;

    // 列自由度数（试探函数空间）
    virtual int sizeOfC(SElement *se) const = 0;

    // ========== 自由度映射 ==========

    // 获取第iRow行对应的全局自由度
    virtual Dof getLocalDofR(SElement *se, int iRow) const = 0;

    // 获取第iCol列对应的全局自由度
    virtual Dof getLocalDofC(SElement *se, int iCol) const = 0;

    // ========== 元素计算（核心虚函数） ==========

    // 计算元素刚度矩阵
    virtual void elementMatrix(SElement *se,
                               fullMatrix<T> &m) const = 0;

    // 计算元素载荷向量（可选）
    virtual void elementVector(SElement *se,
                               fullVector<T> &v) const {
        // 默认为零向量
    }

    // ========== 装配接口 ==========

    // 装配到矩阵
    void addToMatrix(dofManager<T> &dm,
                     groupOfElements &L,
                     groupOfElements &C) const {
        // 预分配稀疏模式
        for(auto e : L.elements()) {
            for(auto f : C.elements()) {
                insertSparsity(dm, e, f);
            }
        }

        // 装配元素矩阵
        for(auto e : L.elements()) {
            SElement se(e);

            // 计算局部矩阵
            fullMatrix<T> localMat(sizeOfR(&se), sizeOfC(&se));
            elementMatrix(&se, localMat);

            // 获取自由度列表
            std::vector<Dof> dofsR, dofsC;
            for(int i = 0; i < sizeOfR(&se); i++) {
                dofsR.push_back(getLocalDofR(&se, i));
            }
            for(int j = 0; j < sizeOfC(&se); j++) {
                dofsC.push_back(getLocalDofC(&se, j));
            }

            // 装配到全局
            dm.assemble(localMat, dofsR, dofsC);
        }
    }

    // 装配到右端向量
    void addToRHS(dofManager<T> &dm, groupOfElements &L) const {
        for(auto e : L.elements()) {
            SElement se(e);

            fullVector<T> localVec(sizeOfR(&se));
            elementVector(&se, localVec);

            std::vector<Dof> dofsR;
            for(int i = 0; i < sizeOfR(&se); i++) {
                dofsR.push_back(getLocalDofR(&se, i));
            }

            dm.assemble(localVec, dofsR);
        }
    }
};
```

### 3. 具体有限元项实现

**helmholtzTerm：Helmholtz/扩散方程**

```cpp
// ∇·(k∇u) - a·u = f
// 弱形式：∫(k∇u·∇v + a·u·v)dΩ = ∫f·v dΩ

template<class T>
class helmholtzTerm : public femTerm<T> {
    T _k;  // 扩散系数
    T _a;  // 反应系数

public:
    helmholtzTerm(GModel *gm, FunctionSpace<T> *fs, T k, T a)
        : femTerm<T>(gm, fs), _k(k), _a(a) {}

    int sizeOfR(SElement *se) const override {
        return se->getNumVertices();
    }

    int sizeOfC(SElement *se) const override {
        return se->getNumVertices();
    }

    Dof getLocalDofR(SElement *se, int i) const override {
        return Dof(se->getVertex(i)->getNum(), 0);
    }

    Dof getLocalDofC(SElement *se, int j) const override {
        return Dof(se->getVertex(j)->getNum(), 0);
    }

    void elementMatrix(SElement *se, fullMatrix<T> &m) const override {
        MElement *e = se->getElement();
        int n = e->getNumVertices();
        m.resize(n, n);
        m.setAll(0.0);

        // 获取高斯积分点
        int order = 2 * e->getPolynomialOrder();
        IntPt *GP;
        int nGP = e->getIntegrationPoints(order, &GP);

        for(int g = 0; g < nGP; g++) {
            double u = GP[g].pt[0];
            double v = GP[g].pt[1];
            double w = GP[g].pt[2];
            double weight = GP[g].weight;

            // 形函数和梯度
            double s[256];
            double grads[256][3];
            e->getShapeFunctions(u, v, w, s);
            e->getGradShapeFunctions(u, v, w, grads);

            // Jacobian
            double jac[3][3];
            double detJ = e->getJacobian(u, v, w, jac);

            // 逆Jacobian
            double invJac[3][3];
            inv3x3(jac, invJac);

            // 物理坐标中的梯度
            double gradPhys[256][3];
            for(int i = 0; i < n; i++) {
                gradPhys[i][0] = invJac[0][0]*grads[i][0] +
                                 invJac[0][1]*grads[i][1] +
                                 invJac[0][2]*grads[i][2];
                gradPhys[i][1] = invJac[1][0]*grads[i][0] +
                                 invJac[1][1]*grads[i][1] +
                                 invJac[1][2]*grads[i][2];
                gradPhys[i][2] = invJac[2][0]*grads[i][0] +
                                 invJac[2][1]*grads[i][1] +
                                 invJac[2][2]*grads[i][2];
            }

            // 组装元素矩阵
            double dV = weight * std::abs(detJ);
            for(int i = 0; i < n; i++) {
                for(int j = 0; j < n; j++) {
                    // 扩散项：k * ∇Nᵢ·∇Nⱼ
                    double diffusion = _k * (
                        gradPhys[i][0]*gradPhys[j][0] +
                        gradPhys[i][1]*gradPhys[j][1] +
                        gradPhys[i][2]*gradPhys[j][2]
                    );

                    // 反应项：a * Nᵢ * Nⱼ
                    double reaction = _a * s[i] * s[j];

                    m(i, j) += (diffusion + reaction) * dV;
                }
            }
        }
    }
};
```

**elasticityTerm：线性弹性**

```cpp
// 弹性力学弱形式
// ∫ σ:ε(v) dΩ = ∫ f·v dΩ + ∫ t·v dΓ

template<class T>
class elasticityTerm : public femTerm<SVector3> {
    double _E;   // 杨氏模量
    double _nu;  // 泊松比

public:
    elasticityTerm(GModel *gm, FunctionSpace<SVector3> *fs,
                   double E, double nu)
        : femTerm<SVector3>(gm, fs), _E(E), _nu(nu) {}

    // 每个节点3个自由度（ux, uy, uz）
    int sizeOfR(SElement *se) const override {
        return 3 * se->getNumVertices();
    }

    int sizeOfC(SElement *se) const override {
        return 3 * se->getNumVertices();
    }

    // 自由度编号：节点i的x/y/z分量
    Dof getLocalDofR(SElement *se, int i) const override {
        int node = i / 3;
        int comp = i % 3;
        return Dof(se->getVertex(node)->getNum(), comp);
    }

    Dof getLocalDofC(SElement *se, int j) const override {
        int node = j / 3;
        int comp = j % 3;
        return Dof(se->getVertex(node)->getNum(), comp);
    }

    void elementMatrix(SElement *se, fullMatrix<double> &K) const override {
        // 弹性刚度矩阵计算
        // K = ∫ B^T · D · B dV
        // B: 应变-位移矩阵
        // D: 弹性本构矩阵

        MElement *e = se->getElement();
        int n = e->getNumVertices();
        int ndof = 3 * n;
        K.resize(ndof, ndof);
        K.setAll(0.0);

        // 计算弹性矩阵D（各向同性）
        double D[6][6] = {0};
        double lambda = _E * _nu / ((1+_nu)*(1-2*_nu));
        double mu = _E / (2*(1+_nu));

        D[0][0] = D[1][1] = D[2][2] = lambda + 2*mu;
        D[0][1] = D[0][2] = D[1][0] = D[1][2] = D[2][0] = D[2][1] = lambda;
        D[3][3] = D[4][4] = D[5][5] = mu;

        // 高斯积分
        int order = 2 * e->getPolynomialOrder();
        IntPt *GP;
        int nGP = e->getIntegrationPoints(order, &GP);

        for(int g = 0; g < nGP; g++) {
            double u = GP[g].pt[0], v = GP[g].pt[1], w = GP[g].pt[2];
            double weight = GP[g].weight;

            // 计算B矩阵和Jacobian
            double B[6][24];  // 假设最多8节点
            double detJ = computeBMatrix(e, u, v, w, B);

            // K += B^T * D * B * detJ * weight
            for(int i = 0; i < ndof; i++) {
                for(int j = 0; j < ndof; j++) {
                    double sum = 0.0;
                    for(int p = 0; p < 6; p++) {
                        for(int q = 0; q < 6; q++) {
                            sum += B[p][i] * D[p][q] * B[q][j];
                        }
                    }
                    K(i, j) += sum * std::abs(detJ) * weight;
                }
            }
        }
    }
};
```

---

## 下午学习内容

### 4. dofManager装配机制

**装配函数详解**：

```cpp
template<class T>
void dofManager<T>::assemble(const fullMatrix<T> &localMatrix,
                              const std::vector<Dof> &R,
                              const std::vector<Dof> &C) {
    int nR = R.size();
    int nC = C.size();

    for(int i = 0; i < nR; i++) {
        const Dof &dofR = R[i];

        // 跳过固定自由度（Dirichlet BC）
        if(fixed.find(dofR) != fixed.end()) continue;

        // 检查约束
        if(constraints.find(dofR) != constraints.end()) {
            // 处理约束自由度
            assembleWithConstraint(localMatrix, R, C, i);
            continue;
        }

        // 获取全局行号
        auto itR = unknown.find(dofR);
        if(itR == unknown.end()) continue;
        int globalR = itR->second;

        for(int j = 0; j < nC; j++) {
            const Dof &dofC = C[j];

            // 固定自由度处理
            if(fixed.find(dofC) != fixed.end()) {
                // 移到右端项
                T val = fixed[dofC];
                _lsys->addToRightHandSide(globalR, -localMatrix(i, j) * val);
                continue;
            }

            // 约束自由度
            if(constraints.find(dofC) != constraints.end()) {
                // 展开约束
                assembleColumnConstraint(localMatrix, R, C, i, j);
                continue;
            }

            // 普通装配
            auto itC = unknown.find(dofC);
            if(itC == unknown.end()) continue;
            int globalC = itC->second;

            _lsys->addToMatrix(globalR, globalC, localMatrix(i, j));
        }
    }
}
```

**约束处理示例**：

```text
约束：DOF₃ = 0.5 * DOF₁ + 0.5 * DOF₂

原始局部矩阵装配：
K[i][3] * u₃ → K[i][3] * (0.5*u₁ + 0.5*u₂)
             = 0.5*K[i][3]*u₁ + 0.5*K[i][3]*u₂

转换：
K[i][1] += 0.5 * K[i][3]
K[i][2] += 0.5 * K[i][3]
K[i][3] = 0  （约束自由度的列归零）
```

### 5. 装配优化技术

**稀疏模式预计算**：

```cpp
// 在装配前预计算非零模式，避免运行时重分配
void precomputeSparsity(dofManager<T> &dm, groupOfElements &elems) {
    for(auto e : elems.elements()) {
        SElement se(e);
        int n = sizeOfR(&se);

        for(int i = 0; i < n; i++) {
            Dof dofI = getLocalDofR(&se, i);
            int rowI = dm.getRow(dofI);
            if(rowI < 0) continue;

            for(int j = 0; j < n; j++) {
                Dof dofJ = getLocalDofC(&se, j);
                int colJ = dm.getRow(dofJ);
                if(colJ < 0) continue;

                dm.insertInSparsityPattern(dofI, dofJ);
            }
        }
    }
}
```

**并行装配（OpenMP）**：

```cpp
// 并行元素矩阵计算
void parallelAssemble(dofManager<T> &dm, groupOfElements &elems) {
    std::vector<MElement*> elemVec(elems.elements().begin(),
                                    elems.elements().end());
    int nElems = elemVec.size();

    // 线程局部存储
    std::vector<std::vector<std::tuple<int, int, T>>> threadContribs(
        omp_get_max_threads()
    );

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        auto &contrib = threadContribs[tid];

        #pragma omp for schedule(dynamic, 100)
        for(int ie = 0; ie < nElems; ie++) {
            SElement se(elemVec[ie]);

            // 计算局部矩阵
            fullMatrix<T> localMat(sizeOfR(&se), sizeOfC(&se));
            elementMatrix(&se, localMat);

            // 收集贡献
            for(int i = 0; i < sizeOfR(&se); i++) {
                int rowI = dm.getRow(getLocalDofR(&se, i));
                if(rowI < 0) continue;

                for(int j = 0; j < sizeOfC(&se); j++) {
                    int colJ = dm.getRow(getLocalDofC(&se, j));
                    if(colJ < 0) continue;

                    contrib.push_back({rowI, colJ, localMat(i, j)});
                }
            }
        }
    }

    // 串行装配（线程安全）
    for(auto &contrib : threadContribs) {
        for(auto &[i, j, val] : contrib) {
            dm.linearSystem()->addToMatrix(i, j, val);
        }
    }
}
```

### 6. 完整装配流程

```cpp
// 完整的有限元装配示例
void assembleSystem(GModel *model, dofManager<double> &dm,
                    linearSystem<double> *lsys) {
    // 1. 创建函数空间
    ScalarLagrangeFunctionSpace *fs =
        new ScalarLagrangeFunctionSpace(model);

    // 2. 创建有限元项
    helmholtzTerm<double> helm(model, fs, 1.0, 0.0);  // -Δu = f

    // 3. 获取元素组
    groupOfElements domain;
    domain.addPhysical(3, 1);  // 3D, physical tag 1

    // 4. 编号自由度
    for(auto e : domain.elements()) {
        for(int i = 0; i < e->getNumVertices(); i++) {
            Dof d(e->getVertex(i)->getNum(), 0);
            dm.numberDof(d);
        }
    }

    // 5. 应用边界条件
    groupOfElements boundary;
    boundary.addPhysical(2, 2);  // 2D, physical tag 2
    for(auto e : boundary.elements()) {
        for(int i = 0; i < e->getNumVertices(); i++) {
            Dof d(e->getVertex(i)->getNum(), 0);
            dm.fixDof(d, 0.0);  // Dirichlet BC: u = 0
        }
    }

    // 6. 预分配稀疏模式
    helm.insertSparsity(dm, domain, domain);

    // 7. 分配线性系统
    lsys->allocate(dm.sizeOfR());

    // 8. 装配刚度矩阵
    helm.addToMatrix(dm, domain, domain);

    // 9. 装配载荷向量
    // ... 体力和边界力

    std::cout << "装配完成: " << dm.sizeOfR() << " 个自由度\n";
}
```

---

## 练习作业

### 基础练习

**练习1**：手工装配

```text
给定网格：两个三角形单元共享一条边

节点编号：1, 2, 3, 4
元素1：节点 (1, 2, 3)
元素2：节点 (2, 4, 3)

元素刚度矩阵（假设）：
K₁ = [4  -1  -1]    K₂ = [4  -1  -1]
     [-1  4  -1]         [-1  4  -1]
     [-1 -1   4]         [-1 -1   4]

任务：
1. 确定全局自由度编号
2. 装配全局刚度矩阵 K (4×4)
3. 验证共享节点(2,3)的贡献被正确累加
```

**练习2**：理解femTerm接口

```cpp
// 回答以下问题：

// 1. sizeOfR 和 sizeOfC 何时不相等？
// 答：_______

// 2. getLocalDofR 和 getLocalDofC 的区别？
// 答：_______

// 3. 为什么elementMatrix是虚函数而addToMatrix不是？
// 答：_______
```

### 进阶练习

**练习3**：实现简单的装配框架

```cpp
#include <iostream>
#include <vector>
#include <map>
#include <cmath>

// 简化的有限元装配器
class SimpleAssembler {
public:
    // 简化的元素（三角形）
    struct Triangle {
        int nodes[3];  // 节点编号
    };

    // 简化的节点
    struct Node {
        double x, y;
    };

private:
    std::vector<Node> _nodes;
    std::vector<Triangle> _elements;

    // 全局系统
    int _nDofs;
    std::vector<std::vector<double>> _K;
    std::vector<double> _F;

    // 边界条件
    std::map<int, double> _fixedDofs;  // 节点 → 值

public:
    void addNode(double x, double y) {
        _nodes.push_back({x, y});
    }

    void addElement(int n1, int n2, int n3) {
        _elements.push_back({{n1, n2, n3}});
    }

    void setDirichletBC(int node, double value) {
        _fixedDofs[node] = value;
    }

    // 计算三角形面积
    double triangleArea(const Triangle &e) const {
        const Node &p1 = _nodes[e.nodes[0]];
        const Node &p2 = _nodes[e.nodes[1]];
        const Node &p3 = _nodes[e.nodes[2]];

        return 0.5 * std::abs(
            (p2.x - p1.x) * (p3.y - p1.y) -
            (p3.x - p1.x) * (p2.y - p1.y)
        );
    }

    // 计算元素刚度矩阵（Laplace方程，线性三角形）
    void computeElementStiffness(const Triangle &e,
                                 double Ke[3][3]) const {
        const Node &p1 = _nodes[e.nodes[0]];
        const Node &p2 = _nodes[e.nodes[1]];
        const Node &p3 = _nodes[e.nodes[2]];

        double A = triangleArea(e);

        // 形函数梯度（常数）
        double b[3], c[3];
        b[0] = (p2.y - p3.y) / (2*A);
        b[1] = (p3.y - p1.y) / (2*A);
        b[2] = (p1.y - p2.y) / (2*A);

        c[0] = (p3.x - p2.x) / (2*A);
        c[1] = (p1.x - p3.x) / (2*A);
        c[2] = (p2.x - p1.x) / (2*A);

        // Ke = A * (b*b^T + c*c^T)
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                Ke[i][j] = A * (b[i]*b[j] + c[i]*c[j]);
            }
        }
    }

    // 装配全局系统
    void assemble() {
        _nDofs = _nodes.size();
        _K.assign(_nDofs, std::vector<double>(_nDofs, 0.0));
        _F.assign(_nDofs, 0.0);

        // 遍历元素
        for(const auto &e : _elements) {
            double Ke[3][3];
            computeElementStiffness(e, Ke);

            // 装配
            for(int i = 0; i < 3; i++) {
                int gi = e.nodes[i];  // 全局节点
                for(int j = 0; j < 3; j++) {
                    int gj = e.nodes[j];
                    _K[gi][gj] += Ke[i][j];
                }
            }
        }

        // 应用边界条件（惩罚法）
        double penalty = 1e30;
        for(auto &[node, val] : _fixedDofs) {
            _K[node][node] += penalty;
            _F[node] += penalty * val;
        }
    }

    // 设置源项
    void setSource(int node, double value) {
        _F[node] += value;
    }

    // 求解（高斯消元）
    std::vector<double> solve() {
        std::vector<double> x(_nDofs, 0.0);

        // 复制系统（避免修改原始数据）
        auto A = _K;
        auto b = _F;

        // 前向消元
        for(int k = 0; k < _nDofs; k++) {
            for(int i = k + 1; i < _nDofs; i++) {
                double factor = A[i][k] / A[k][k];
                for(int j = k; j < _nDofs; j++) {
                    A[i][j] -= factor * A[k][j];
                }
                b[i] -= factor * b[k];
            }
        }

        // 回代
        for(int i = _nDofs - 1; i >= 0; i--) {
            x[i] = b[i];
            for(int j = i + 1; j < _nDofs; j++) {
                x[i] -= A[i][j] * x[j];
            }
            x[i] /= A[i][i];
        }

        return x;
    }

    void printSystem() const {
        std::cout << "刚度矩阵 K:\n";
        for(int i = 0; i < _nDofs; i++) {
            for(int j = 0; j < _nDofs; j++) {
                std::cout << _K[i][j] << "\t";
            }
            std::cout << "| " << _F[i] << "\n";
        }
    }
};

int main() {
    SimpleAssembler fem;

    // 创建简单网格：单位正方形，2个三角形
    //  3---2
    //  |\ |
    //  | \|
    //  0---1

    fem.addNode(0, 0);  // 0
    fem.addNode(1, 0);  // 1
    fem.addNode(1, 1);  // 2
    fem.addNode(0, 1);  // 3

    fem.addElement(0, 1, 2);  // 下三角
    fem.addElement(0, 2, 3);  // 上三角

    // 边界条件：u=0 在左边界
    fem.setDirichletBC(0, 0.0);
    fem.setDirichletBC(3, 0.0);

    // 边界条件：u=1 在右边界
    fem.setDirichletBC(1, 1.0);
    fem.setDirichletBC(2, 1.0);

    // 装配
    fem.assemble();
    fem.printSystem();

    // 求解
    auto u = fem.solve();

    std::cout << "\n解:\n";
    for(int i = 0; i < u.size(); i++) {
        std::cout << "u[" << i << "] = " << u[i] << "\n";
    }

    return 0;
}
```

### 挑战练习

**练习4**：实现弹性力学装配

```cpp
// 扩展上面的SimpleAssembler，实现2D弹性力学

// 提示：
// 1. 每个节点2个自由度(ux, uy)
// 2. 元素刚度矩阵变为 6×6
// 3. 需要实现B矩阵和D矩阵

class ElasticAssembler {
    // TODO: 实现2D平面应力弹性问题
    // B矩阵 (3×6): 应变-位移关系
    // D矩阵 (3×3): 本构关系
    // Ke = ∫ B^T * D * B dA
};
```

**练习5**：分析Gmsh装配源码

```cpp
// 阅读 src/solver/elasticitySolver.cpp 的 assemble() 方法

// 问题：
// 1. 刚度项和边界项如何分别装配？
// 2. Neumann边界条件如何处理？
// 3. 如何处理多材料问题？
```

---

## 知识图谱

```text
有限元装配
│
├── 理论基础
│   ├── 变分形式 → 离散化
│   ├── 局部刚度矩阵
│   └── 全局装配规则
│
├── femTerm体系
│   ├── 基类模板设计
│   ├── 虚函数：elementMatrix/Vector
│   ├── helmholtzTerm
│   └── elasticityTerm
│
├── dofManager装配
│   ├── assemble()流程
│   ├── 边界条件处理
│   └── 约束自由度展开
│
├── 优化技术
│   ├── 稀疏模式预计算
│   ├── 并行装配
│   └── 元素着色
│
└── 高斯积分
    ├── 积分点和权重
    ├── 形函数评估
    └── Jacobian计算
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 | 优先级 |
|------|----------|--------|--------|
| femTerm.h | 有限元项基类 | ~130行 | ★★★★★ |
| helmholtzTerm.h | Helmholtz方程实现 | ~200行 | ★★★★★ |
| elasticityTerm.h/cpp | 弹性力学实现 | ~350行 | ★★★★☆ |
| dofManager.h | assemble()方法 | ~732行 | ★★★★★ |
| laplaceTerm.h | Laplace/质量项 | ~100行 | ★★★☆☆ |

---

## 今日检查点

- [ ] 理解有限元装配的理论基础
- [ ] 掌握femTerm类的接口设计
- [ ] 理解helmholtzTerm的实现
- [ ] 掌握dofManager的装配机制
- [ ] 能解释约束自由度的处理方式
- [ ] 完成简单装配框架练习

---

## 导航

- 上一天：[Day 66 - 特征值问题](day-66.md)
- 下一天：[Day 68 - 边界条件处理](day-68.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
