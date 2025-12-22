# Day 64：求解器模块概述

## 学习目标

全面了解Gmsh求解器模块的整体架构，掌握核心类层次结构和设计模式，为深入学习各子系统打下基础。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 求解器模块架构全览 |
| 10:00-11:00 | 1h | 核心类层次分析 |
| 11:00-12:00 | 1h | 有限元求解流程 |
| 14:00-15:00 | 1h | 代码阅读与练习 |

---

## 上午学习内容

### 1. 求解器模块架构全览

**模块位置**：`src/solver/`

**模块规模**：
- 56个文件（23个.cpp + 33个.h）
- 约4,300行C++代码
- 支持多种物理问题和求解器后端

**目录结构**：

```text
src/solver/
│
├── 线性系统抽象层
│   ├── linearSystem.h           # 基础抽象接口
│   ├── linearSystemCSR.h/cpp    # 压缩稀疏行实现
│   ├── linearSystemFull.h       # 稠密矩阵实现
│   └── linearSystemGmm.h        # Gmm++库接口
│
├── 求解器后端
│   ├── linearSystemPETSc.h/hpp/cpp  # PETSc分布式求解
│   ├── linearSystemMUMPS.h/cpp      # MUMPS直接求解
│   └── linearSystemEigen.h/cpp      # Eigen库求解
│
├── 自由度管理
│   └── dofManager.h/cpp         # DOF编号与装配
│
├── 函数空间
│   └── functionSpace.h/cpp      # 基函数与梯度
│
├── 有限元项
│   ├── femTerm.h                # FEM项基类
│   ├── helmholtzTerm.h          # Helmholtz/扩散
│   ├── laplaceTerm.h            # Laplace方程
│   └── elasticityTerm.h/cpp     # 弹性力学
│
├── 物理求解器
│   ├── elasticitySolver.h/cpp   # 弹性求解器
│   ├── thermicSolver.h/cpp      # 热传导求解器
│   ├── eigenSolver.h/cpp        # 特征值求解器
│   └── frameSolver.h/cpp        # 框架结构求解
│
├── 支撑组件
│   ├── SElement.h/cpp           # 求解元素封装
│   ├── groupOfElements.h/cpp    # 元素分组
│   └── sparsityPattern.h/cpp    # 稀疏模式管理
│
└── 张量表示
    ├── STensor3.h/cpp           # 3×3张量
    ├── STensor33.h/cpp          # 3阶张量
    └── STensor43.h/cpp          # 4阶张量
```

### 2. 核心类层次结构

**整体架构图**：

```text
                     ┌─────────────────────────┐
                     │       GModel            │
                     │    (几何+网格模型)       │
                     └───────────┬─────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              │                  │                  │
              ▼                  ▼                  ▼
    ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
    │  elasticitySolver│ │  thermicSolver  │ │   eigenSolver   │
    │   (弹性力学)     │ │   (热传导)      │ │   (特征值)      │
    └────────┬────────┘ └────────┬────────┘ └────────┬────────┘
             │                   │                   │
             └───────────────────┼───────────────────┘
                                 │
                                 ▼
                     ┌─────────────────────────┐
                     │      dofManager<T>      │
                     │    (自由度管理器)        │
                     └───────────┬─────────────┘
                                 │
              ┌──────────────────┼──────────────────┐
              │                  │                  │
              ▼                  ▼                  ▼
    ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
    │ FunctionSpace<T>│ │   femTerm<T>    │ │  linearSystem   │
    │   (函数空间)     │ │  (有限元项)     │ │  (线性系统)      │
    └─────────────────┘ └─────────────────┘ └────────┬────────┘
                                                     │
                          ┌──────────────────────────┼──────────┐
                          │              │           │          │
                          ▼              ▼           ▼          ▼
                    ┌──────────┐  ┌──────────┐ ┌──────────┐ ┌──────────┐
                    │   CSR    │  │  PETSc   │ │  MUMPS   │ │  Eigen   │
                    └──────────┘  └──────────┘ └──────────┘ └──────────┘
```

### 3. 线性系统抽象层

**文件位置**：`src/solver/linearSystem.h`

```cpp
// 线性系统基类（非模板）
class linearSystemBase {
public:
    virtual ~linearSystemBase() {}

    // 参数化支持
    virtual void setParameter(const std::string &key,
                              const std::string &value) {}

    // 核心操作
    virtual bool isAllocated() const = 0;
    virtual void allocate(int nbRows) = 0;
    virtual void clear() = 0;

    // 求解
    virtual int systemSolve() = 0;

    // 零空间处理（用于特殊约束）
    virtual void setPrec(double p) {}
};

// 模板化线性系统接口
template<class scalar>
class linearSystem : public linearSystemBase {
public:
    // 矩阵操作
    virtual void addToMatrix(int row, int col, const scalar &val) = 0;
    virtual scalar getFromMatrix(int row, int col) const = 0;

    // 右端向量操作
    virtual void addToRightHandSide(int row, const scalar &val, int ith = 0) = 0;
    virtual scalar getFromRightHandSide(int row) const = 0;

    // 解向量操作
    virtual void addToSolution(int row, const scalar &val) = 0;
    virtual scalar getFromSolution(int row) const = 0;

    // 归零操作
    virtual void zeroMatrix() = 0;
    virtual void zeroRightHandSide() = 0;
    virtual void zeroSolution() = 0;
};
```

**设计要点**：
- 使用模板支持多种标量类型（double, complex等）
- 抽象接口允许多种后端实现
- 分离矩阵、右端向量、解向量的操作

### 4. 自由度管理系统

**文件位置**：`src/solver/dofManager.h`

```cpp
// 自由度标识符
class Dof {
    // 实体编号（可以是顶点ID、边ID等）
    int _entity;
    // 类型编码（支持双整数表示）
    int _type;

public:
    Dof(int entity, int type) : _entity(entity), _type(type) {}

    // 比较运算符（用于map/set）
    bool operator<(const Dof &other) const;
    bool operator==(const Dof &other) const;

    int getEntity() const { return _entity; }
    int getType() const { return _type; }
};

// 仿射约束结构
template<class T>
struct DofAffineConstraint {
    // 线性项：DOF_i = Σ(c_j * DOF_j) + shift
    std::vector<std::pair<Dof, T>> linear;
    T shift;  // 常数项
};

// 自由度管理器
template<class T>
class dofManager {
    // 线性系统后端
    linearSystem<T> *_lsys;

    // 自由度映射表
    std::map<Dof, int> unknown;      // 待求自由度 → 行号
    std::map<Dof, T> fixed;          // 固定值自由度
    std::map<Dof, DofAffineConstraint<T>> constraints;  // 约束关系

    // 并行支持
    std::map<Dof, std::pair<int, int>> ghostByDof;  // 鬼魂自由度

public:
    // 编号自由度
    void numberDof(const Dof &d);
    void numberGhostDof(const Dof &d, int procId);

    // 固定自由度（Dirichlet边界条件）
    void fixDof(const Dof &d, const T &value);

    // 设置线性约束
    void setLinearConstraint(const Dof &d,
                             const DofAffineConstraint<T> &constraint);

    // 获取自由度总数
    int sizeOfF() const { return fixed.size(); }
    int sizeOfR() const { return unknown.size(); }

    // 元素装配
    void assemble(const fullMatrix<T> &localMatrix,
                  const std::vector<Dof> &R,
                  const std::vector<Dof> &C);

    void assemble(const fullVector<T> &localVector,
                  const std::vector<Dof> &R);

    // 稀疏结构预分配
    void insertInSparsityPattern(const Dof &R, const Dof &C);

    // 获取解
    void getDofValue(const Dof &d, T &val) const;
};
```

**关键概念**：
- **Dof（自由度）**：有限元方程中的未知数单位
- **unknown**：待求解的自由度
- **fixed**：Dirichlet边界条件（已知值）
- **constraints**：线性约束关系（如周期边界条件）

---

## 下午学习内容

### 5. 有限元求解完整流程

**典型求解流程图**：

```text
┌─────────────────────────────────────────────────────────────────┐
│                        1. 初始化阶段                             │
├─────────────────────────────────────────────────────────────────┤
│  • 创建GModel（几何+网格）                                        │
│  • 选择线性系统后端（PETSc/MUMPS/CSR）                            │
│  • 创建dofManager                                               │
│  • 初始化函数空间（Lagrange等）                                   │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                      2. 自由度编号阶段                            │
├─────────────────────────────────────────────────────────────────┤
│  • 遍历所有网格单元                                               │
│  • 为每个节点创建Dof并编号                                        │
│  • 应用边界条件（fixDof）                                         │
│  • 设置约束关系（如有）                                           │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                     3. 稀疏结构预分配                             │
├─────────────────────────────────────────────────────────────────┤
│  • 遍历所有元素对(L, C)                                          │
│  • 调用insertInSparsityPattern                                   │
│  • 线性系统分配内存                                               │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                       4. 元素装配阶段                             │
├─────────────────────────────────────────────────────────────────┤
│  For each element e:                                            │
│    • 创建SElement封装                                            │
│    • 调用femTerm::elementMatrix计算局部矩阵                       │
│    • 使用高斯积分求积                                             │
│    • 调用dofManager::assemble装配到全局                           │
│  处理Neumann边界条件                                             │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                        5. 求解阶段                               │
├─────────────────────────────────────────────────────────────────┤
│  • 调用linearSystem::systemSolve()                              │
│  • 后端执行矩阵分解/迭代                                          │
│  • 解向量存储在linearSystem中                                     │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                       6. 后处理阶段                              │
├─────────────────────────────────────────────────────────────────┤
│  • 从dofManager提取解                                            │
│  • 计算导出量（应力、热流等）                                      │
│  • 创建PView可视化                                               │
│  • 导出结果文件                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 6. 物理求解器示例

**弹性力学求解器结构**：

```cpp
// elasticitySolver.h 核心结构
class elasticitySolver {
    GModel *pModel;                    // 几何模型
    dofManager<double> *pAssembler;    // 装配器
    FunctionSpace<SVector3> *LagSpace; // 向量函数空间

    // 物理域
    std::vector<elasticField> elasticFields;  // 弹性材料

    // 边界条件
    std::vector<dirichletBC> allDirichlet;    // 位移约束
    std::vector<neumannBC> allNeumann;        // 力边界条件

    // Lagrange乘子（高级约束）
    std::vector<LagrangeMultiplierField> LMFields;

public:
    // 设置材料
    void addElasticDomain(int tag, double E, double nu);

    // 设置边界条件
    void addDirichletBC(int dim, int tag, int comp, double value);
    void addNeumannBC(int dim, int tag, const std::vector<double> &force);

    // 求解流程
    void assemble();   // 装配全局系统
    void solve();      // 求解线性系统

    // 后处理
    void buildDisplacementView();  // 位移视图
    void buildStressesView();      // 应力视图
    void buildVonMisesView();      // Von Mises应力
};
```

### 7. 设计模式分析

**求解器模块使用的设计模式**：

| 模式 | 应用位置 | 作用 |
|------|----------|------|
| **模板方法** | `linearSystem<T>`, `femTerm<T>` | 类型无关的算法框架 |
| **策略模式** | 多种线性求解器后端 | 运行时切换求解算法 |
| **工厂模式** | `FunctionSpace::clone()` | 创建函数空间副本 |
| **装饰器** | `elasticityMixedTerm` | 扩展基础有限元项 |
| **适配器** | PETSc/MUMPS包装层 | 统一不同库的接口 |
| **组合模式** | `groupOfElements` | 元素集合管理 |

**模板方法示例**：

```cpp
// femTerm<T> 定义了算法骨架
template<class T>
class femTerm {
public:
    // 算法步骤由子类实现
    virtual void elementMatrix(SElement *se,
                               fullMatrix<T> &m) const = 0;

    // 公共装配流程（模板方法）
    void addToMatrix(dofManager<T> &dm,
                     groupOfElements &L,
                     groupOfElements &C) const {
        // 遍历元素
        for(auto e : L.elements()) {
            SElement se(e);

            // 计算局部矩阵（调用子类实现）
            fullMatrix<T> localMat;
            elementMatrix(&se, localMat);

            // 获取自由度
            std::vector<Dof> R = getLocalDofsR(&se);
            std::vector<Dof> C = getLocalDofsC(&se);

            // 装配到全局
            dm.assemble(localMat, R, C);
        }
    }
};
```

---

## 练习作业

### 基础练习

**练习1**：理解模块结构

```bash
# 统计求解器模块的文件和代码量
cd src/solver
find . -name "*.h" | wc -l
find . -name "*.cpp" | wc -l
wc -l *.h *.cpp | tail -1

# 问题：
# 1. 头文件和源文件的数量比例说明什么？
# 2. 哪些类主要在头文件中实现？为什么？
```

**练习2**：追踪类层次

```cpp
// 画出以下类的继承关系图
// linearSystemBase
// linearSystem<double>
// linearSystemCSR<double>
// linearSystemPETSc<double>

// 回答：
// 1. 为什么需要非模板基类linearSystemBase？
// 2. linearSystem<T>增加了什么功能？
```

### 进阶练习

**练习3**：阅读核心头文件

```cpp
// 阅读 linearSystem.h 并回答：
// 1. addToMatrix 和 getFromMatrix 的语义区别
// 2. 为什么需要分别操作矩阵、右端项、解向量？
// 3. systemSolve 返回值的含义

// 阅读 dofManager.h 并回答：
// 1. unknown、fixed、constraints 三个映射表的职责
// 2. 如何表示周期边界条件？
// 3. assemble 函数如何处理约束自由度？
```

**练习4**：简单有限元程序框架

```cpp
#include <iostream>
#include <vector>
#include <map>

// 简化的Dof类
class Dof {
    int _entity, _type;
public:
    Dof(int e, int t) : _entity(e), _type(t) {}
    bool operator<(const Dof &o) const {
        if(_entity != o._entity) return _entity < o._entity;
        return _type < o._type;
    }
    int entity() const { return _entity; }
    int type() const { return _type; }
};

// 简化的线性系统（稠密矩阵）
class SimpleLinearSystem {
    int _n;
    std::vector<std::vector<double>> _A;
    std::vector<double> _b, _x;

public:
    void allocate(int n) {
        _n = n;
        _A.assign(n, std::vector<double>(n, 0.0));
        _b.assign(n, 0.0);
        _x.assign(n, 0.0);
    }

    void addToMatrix(int i, int j, double val) {
        _A[i][j] += val;
    }

    void addToRHS(int i, double val) {
        _b[i] += val;
    }

    // 简单的高斯消元求解
    void solve() {
        // 前向消元
        for(int k = 0; k < _n; k++) {
            for(int i = k + 1; i < _n; i++) {
                double factor = _A[i][k] / _A[k][k];
                for(int j = k; j < _n; j++) {
                    _A[i][j] -= factor * _A[k][j];
                }
                _b[i] -= factor * _b[k];
            }
        }

        // 回代
        for(int i = _n - 1; i >= 0; i--) {
            _x[i] = _b[i];
            for(int j = i + 1; j < _n; j++) {
                _x[i] -= _A[i][j] * _x[j];
            }
            _x[i] /= _A[i][i];
        }
    }

    double getSolution(int i) const { return _x[i]; }

    void print() const {
        std::cout << "Matrix A:\n";
        for(int i = 0; i < _n; i++) {
            for(int j = 0; j < _n; j++) {
                std::cout << _A[i][j] << "\t";
            }
            std::cout << "| " << _b[i] << "\n";
        }
    }
};

// 简化的DOF管理器
class SimpleDofManager {
    std::map<Dof, int> _dofToRow;
    std::map<Dof, double> _fixed;
    SimpleLinearSystem *_sys;
    int _nextRow;

public:
    SimpleDofManager(SimpleLinearSystem *sys)
        : _sys(sys), _nextRow(0) {}

    void numberDof(const Dof &d) {
        if(_dofToRow.find(d) == _dofToRow.end() &&
           _fixed.find(d) == _fixed.end()) {
            _dofToRow[d] = _nextRow++;
        }
    }

    void fixDof(const Dof &d, double val) {
        _fixed[d] = val;
    }

    int getRow(const Dof &d) const {
        auto it = _dofToRow.find(d);
        return (it != _dofToRow.end()) ? it->second : -1;
    }

    bool isFixed(const Dof &d) const {
        return _fixed.find(d) != _fixed.end();
    }

    double getFixedValue(const Dof &d) const {
        return _fixed.at(d);
    }

    int size() const { return _nextRow; }

    void allocate() {
        _sys->allocate(_nextRow);
    }

    // 装配局部刚度矩阵
    void assemble(const std::vector<std::vector<double>> &Ke,
                  const std::vector<Dof> &dofs) {
        int n = dofs.size();
        for(int i = 0; i < n; i++) {
            for(int j = 0; j < n; j++) {
                if(!isFixed(dofs[i]) && !isFixed(dofs[j])) {
                    _sys->addToMatrix(getRow(dofs[i]),
                                      getRow(dofs[j]),
                                      Ke[i][j]);
                }
            }
        }
    }

    // 装配载荷向量
    void assembleRHS(const std::vector<double> &Fe,
                     const std::vector<Dof> &dofs) {
        int n = dofs.size();
        for(int i = 0; i < n; i++) {
            if(!isFixed(dofs[i])) {
                _sys->addToRHS(getRow(dofs[i]), Fe[i]);
            }
        }
    }
};

// 示例：求解一维弹簧系统
// k1=100, k2=200, k3=100
// 固定节点1，节点4受力F=500
int main() {
    SimpleLinearSystem sys;
    SimpleDofManager dm(&sys);

    // 创建4个节点的自由度
    std::vector<Dof> dofs;
    for(int i = 1; i <= 4; i++) {
        dofs.push_back(Dof(i, 0));
    }

    // 固定节点1
    dm.fixDof(dofs[0], 0.0);

    // 编号其他自由度
    for(auto &d : dofs) {
        dm.numberDof(d);
    }

    std::cout << "自由度数量: " << dm.size() << std::endl;

    // 分配系统
    dm.allocate();

    // 弹簧刚度
    double k1 = 100, k2 = 200, k3 = 100;

    // 装配弹簧1 (节点1-2)
    std::vector<std::vector<double>> Ke1 = {{k1, -k1}, {-k1, k1}};
    dm.assemble(Ke1, {dofs[0], dofs[1]});

    // 装配弹簧2 (节点2-3)
    std::vector<std::vector<double>> Ke2 = {{k2, -k2}, {-k2, k2}};
    dm.assemble(Ke2, {dofs[1], dofs[2]});

    // 装配弹簧3 (节点3-4)
    std::vector<std::vector<double>> Ke3 = {{k3, -k3}, {-k3, k3}};
    dm.assemble(Ke3, {dofs[2], dofs[3]});

    // 施加外力
    std::vector<double> F = {0, 0, 500};  // 节点4受力
    dm.assembleRHS({0, 500}, {dofs[2], dofs[3]});

    // 打印系统
    sys.print();

    // 求解
    sys.solve();

    // 输出结果
    std::cout << "\n位移结果:\n";
    std::cout << "u1 = 0 (固定)\n";
    for(int i = 1; i < 4; i++) {
        std::cout << "u" << i+1 << " = "
                  << sys.getSolution(dm.getRow(dofs[i])) << "\n";
    }

    return 0;
}
```

### 挑战练习

**练习5**：分析Gmsh求解器源码

```cpp
// 阅读 elasticitySolver.cpp 的 assemble() 函数
// 1. 追踪元素刚度矩阵的计算过程
// 2. 找出高斯积分的调用位置
// 3. 理解边界条件的处理方式

// 阅读 femTerm.h 的 addToMatrix 模板方法
// 1. 该方法如何处理不同类型的有限元项？
// 2. 局部到全局的映射如何实现？
```

---

## 知识图谱

```text
求解器模块概述
│
├── 模块架构
│   ├── 线性系统层（抽象接口）
│   ├── 求解器后端层（PETSc/MUMPS/Eigen）
│   ├── 自由度管理层（dofManager）
│   ├── 函数空间层（FunctionSpace）
│   ├── 有限元项层（femTerm）
│   └── 物理求解器层（elastic/thermic/eigen）
│
├── 核心类
│   ├── linearSystem<T> - 线性系统接口
│   ├── dofManager<T> - 自由度管理
│   ├── Dof - 自由度标识
│   ├── femTerm<T> - 有限元项
│   └── SElement - 求解元素
│
├── 求解流程
│   ├── 初始化
│   ├── DOF编号
│   ├── 稀疏预分配
│   ├── 元素装配
│   ├── 系统求解
│   └── 后处理
│
└── 设计模式
    ├── 模板方法
    ├── 策略模式
    ├── 工厂模式
    └── 适配器模式
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 | 优先级 |
|------|----------|--------|--------|
| linearSystem.h | 线性系统抽象接口 | ~50行 | ★★★★★ |
| dofManager.h | 自由度管理器 | ~732行 | ★★★★★ |
| femTerm.h | 有限元项基类 | ~130行 | ★★★★★ |
| elasticitySolver.h | 弹性求解器定义 | ~200行 | ★★★★☆ |
| elasticitySolver.cpp | 弹性求解器实现 | ~1000行 | ★★★★☆ |
| functionSpace.h | 函数空间定义 | ~800行 | ★★★☆☆ |
| SElement.h | 求解元素封装 | ~65行 | ★★★☆☆ |
| groupOfElements.h | 元素分组 | ~135行 | ★★★☆☆ |

---

## 今日检查点

- [ ] 能描述求解器模块的整体架构
- [ ] 理解线性系统抽象层的设计
- [ ] 掌握Dof和dofManager的概念
- [ ] 理解有限元求解的完整流程
- [ ] 能识别模块中使用的设计模式
- [ ] 完成弹簧系统练习程序

---

## 导航

- 上一天：[Day 63 - 第九周复习](day-63.md)
- 下一天：[Day 65 - 线性系统求解](day-65.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
