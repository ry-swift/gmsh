# Day 70：第十周复习

## 学习目标

巩固第十周所学的求解器与数值计算知识，完成综合练习项目，为下一阶段API高级应用做准备。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 知识点回顾与梳理 |
| 10:00-11:00 | 1h | 综合练习：迷你有限元求解器 |
| 11:00-12:00 | 1h | 代码阅读复习 |
| 14:00-15:00 | 1h | 整理学习笔记，完成周检查点 |

---

## 上午学习内容

### 1. 第十周知识图谱

```text
求解器与数值计算
│
├── Day 64: 求解器模块概述
│   ├── 模块架构（56个文件，4300行）
│   ├── 核心类层次（linearSystem → dofManager → femTerm）
│   ├── 设计模式（模板方法、策略、适配器）
│   └── 有限元求解完整流程
│
├── Day 65: 线性系统求解
│   ├── linearSystem<T>抽象接口
│   ├── CSR稀疏矩阵格式
│   ├── 稀疏模式预分配
│   └── Gmm++迭代求解器集成
│
├── Day 66: 特征值问题
│   ├── 标准/广义特征值问题
│   ├── 幂迭代、Krylov方法
│   ├── eigenSolver类（SLEPc集成）
│   └── 模态分析应用
│
├── Day 67: 有限元装配
│   ├── 局部到全局装配理论
│   ├── femTerm类体系
│   ├── helmholtzTerm/elasticityTerm
│   └── dofManager::assemble机制
│
├── Day 68: 边界条件处理
│   ├── Dirichlet/Neumann/Robin条件
│   ├── 惩罚法、置换法、Lagrange乘子
│   ├── DofAffineConstraint约束
│   └── elasticitySolver中的BC处理
│
└── Day 69: 求解器后端
    ├── PETSc（迭代求解、并行）
    ├── MUMPS（直接求解、多波前）
    ├── Eigen（轻量级、头文件库）
    └── 求解器选择策略
```

### 2. 核心概念回顾

**求解器模块架构层次**：

```text
应用层：elasticitySolver, thermicSolver, eigenSolver
           │
           ├── 管理物理问题
           ├── 设置边界条件
           └── 协调求解流程
           │
           ▼
装配层：dofManager<T>
           │
           ├── 自由度编号（unknown）
           ├── 固定值处理（fixed）
           ├── 约束展开（constraints）
           └── 元素装配（assemble）
           │
           ▼
有限元层：femTerm<T>
           │
           ├── 元素矩阵计算（elementMatrix）
           ├── 元素向量计算（elementVector）
           └── 全局装配接口（addToMatrix）
           │
           ▼
求解层：linearSystem<T>
           │
           ├── 矩阵/向量操作接口
           └── systemSolve()
           │
           ▼
后端层：CSR, PETSc, MUMPS, Eigen
```

**自由度管理核心流程**：

```cpp
// 典型的DOF管理流程
dofManager<double> dm(linearSystem);

// 1. 编号自由度
for(element e : elements) {
    for(node n : e.nodes()) {
        dm.numberDof(Dof(n.id, component));
    }
}

// 2. 应用Dirichlet条件
for(bc : dirichletBCs) {
    dm.fixDof(Dof(bc.node, bc.comp), bc.value);
}

// 3. 设置约束
for(constraint : constraints) {
    dm.setLinearConstraint(slaveDof, masterRelation);
}

// 4. 分配系统
linearSystem.allocate(dm.sizeOfR());

// 5. 装配
for(element e : elements) {
    fullMatrix<double> Ke = computeElementMatrix(e);
    std::vector<Dof> dofs = getElementDofs(e);
    dm.assemble(Ke, dofs, dofs);
}

// 6. 求解
linearSystem.systemSolve();

// 7. 提取解
for(dof d : allDofs) {
    dm.getDofValue(d, value);
}
```

**边界条件处理对比**：

```text
┌─────────────┬────────────────────────────────────────────┐
│ 方法        │ 处理方式                                   │
├─────────────┼────────────────────────────────────────────┤
│ 惩罚法      │ K[i][i] += P, F[i] += P*g                  │
│             │ 简单但有数值误差                           │
├─────────────┼────────────────────────────────────────────┤
│ 置换法      │ 固定DOF不参与编号                          │
│             │ F[free] -= K[free][fixed] * g              │
│             │ 精确，保持系统规模                         │
├─────────────┼────────────────────────────────────────────┤
│ Lagrange    │ [K  C^T][u]   [F]                          │
│ 乘子        │ [C  0  ][λ] = [g]                          │
│             │ 通用，增加系统规模                         │
└─────────────┴────────────────────────────────────────────┘
```

### 3. 求解器后端选择指南

```text
决策流程：

1. 问题规模 < 10K?
   └─YES→ Eigen（简单，无依赖）

2. 需要机器精度?
   └─YES→ 直接法（MUMPS或Eigen LU）

3. 矩阵对称正定?
   └─YES→ CG + Cholesky/ICC预处理

4. 问题规模 > 100K?
   └─YES→ PETSc + 迭代（需要调参）

5. 多右端向量?
   └─YES→ 直接法（分解可复用）

6. 需要并行?
   └─YES→ PETSc（MPI）或MUMPS（OpenMP）
```

---

## 综合练习

### 4. 综合项目：迷你有限元求解器

**目标**：实现一个完整的2D Poisson方程求解器，整合本周所有知识点。

```cpp
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <fstream>
#include <iomanip>

// =====================================================
// 第一部分：基础数据结构
// =====================================================

// 自由度标识符
struct Dof {
    int entity;
    int type;

    bool operator<(const Dof &o) const {
        if(entity != o.entity) return entity < o.entity;
        return type < o.type;
    }
};

// 节点
struct Node {
    double x, y;
    bool fixed;
    double fixedValue;
};

// 三角形元素
struct Triangle {
    int nodes[3];
};

// =====================================================
// 第二部分：线性系统（简化版CSR）
// =====================================================

class SimpleLinearSystem {
    int _n;
    std::vector<std::vector<double>> _K;  // 使用稠密存储简化
    std::vector<double> _F, _x;

public:
    void allocate(int n) {
        _n = n;
        _K.assign(n, std::vector<double>(n, 0.0));
        _F.assign(n, 0.0);
        _x.assign(n, 0.0);
    }

    void addToMatrix(int i, int j, double val) {
        if(i >= 0 && j >= 0 && i < _n && j < _n) {
            _K[i][j] += val;
        }
    }

    void addToRHS(int i, double val) {
        if(i >= 0 && i < _n) {
            _F[i] += val;
        }
    }

    // CG求解（假设对称正定）
    bool solve(double tol = 1e-10, int maxIter = 1000) {
        std::vector<double> r(_n), p(_n), Ap(_n);

        // r = F - K*x
        for(int i = 0; i < _n; i++) {
            r[i] = _F[i];
            for(int j = 0; j < _n; j++) {
                r[i] -= _K[i][j] * _x[j];
            }
            p[i] = r[i];
        }

        double rsold = dot(r, r);

        for(int iter = 0; iter < maxIter; iter++) {
            // Ap = K * p
            for(int i = 0; i < _n; i++) {
                Ap[i] = 0;
                for(int j = 0; j < _n; j++) {
                    Ap[i] += _K[i][j] * p[j];
                }
            }

            double alpha = rsold / dot(p, Ap);

            for(int i = 0; i < _n; i++) {
                _x[i] += alpha * p[i];
                r[i] -= alpha * Ap[i];
            }

            double rsnew = dot(r, r);
            if(std::sqrt(rsnew) < tol) {
                std::cout << "CG收敛，迭代" << iter + 1 << "次\n";
                return true;
            }

            double beta = rsnew / rsold;
            for(int i = 0; i < _n; i++) {
                p[i] = r[i] + beta * p[i];
            }

            rsold = rsnew;
        }

        std::cout << "CG未收敛\n";
        return false;
    }

    double getSolution(int i) const { return _x[i]; }

private:
    double dot(const std::vector<double> &a, const std::vector<double> &b) {
        double s = 0;
        for(int i = 0; i < _n; i++) s += a[i] * b[i];
        return s;
    }
};

// =====================================================
// 第三部分：自由度管理器
// =====================================================

class DofManager {
    SimpleLinearSystem *_sys;
    std::map<Dof, int> _unknown;
    std::map<Dof, double> _fixed;
    int _nextRow;

public:
    DofManager(SimpleLinearSystem *sys)
        : _sys(sys), _nextRow(0) {}

    void numberDof(const Dof &d) {
        if(_unknown.find(d) == _unknown.end() &&
           _fixed.find(d) == _fixed.end()) {
            _unknown[d] = _nextRow++;
        }
    }

    void fixDof(const Dof &d, double value) {
        _fixed[d] = value;
    }

    int getRow(const Dof &d) const {
        auto it = _unknown.find(d);
        return (it != _unknown.end()) ? it->second : -1;
    }

    bool isFixed(const Dof &d) const {
        return _fixed.find(d) != _fixed.end();
    }

    double getFixedValue(const Dof &d) const {
        return _fixed.at(d);
    }

    int size() const { return _nextRow; }

    // 装配元素矩阵
    void assemble(const std::vector<std::vector<double>> &Ke,
                  const std::vector<Dof> &dofs) {
        int n = dofs.size();
        for(int i = 0; i < n; i++) {
            int gi = getRow(dofs[i]);
            if(gi < 0) continue;  // 固定DOF

            for(int j = 0; j < n; j++) {
                if(isFixed(dofs[j])) {
                    // 移到右端
                    _sys->addToRHS(gi, -Ke[i][j] * getFixedValue(dofs[j]));
                } else {
                    int gj = getRow(dofs[j]);
                    _sys->addToMatrix(gi, gj, Ke[i][j]);
                }
            }
        }
    }

    // 装配元素载荷
    void assembleRHS(const std::vector<double> &Fe,
                     const std::vector<Dof> &dofs) {
        int n = dofs.size();
        for(int i = 0; i < n; i++) {
            int gi = getRow(dofs[i]);
            if(gi >= 0) {
                _sys->addToRHS(gi, Fe[i]);
            }
        }
    }

    double getValue(const Dof &d) const {
        if(isFixed(d)) return getFixedValue(d);
        int row = getRow(d);
        return (row >= 0) ? _sys->getSolution(row) : 0.0;
    }
};

// =====================================================
// 第四部分：有限元项（Laplace方程）
// =====================================================

class LaplaceTerm {
    std::vector<Node> &_nodes;

public:
    LaplaceTerm(std::vector<Node> &nodes) : _nodes(nodes) {}

    // 计算三角形面积
    double area(const Triangle &e) const {
        const Node &p1 = _nodes[e.nodes[0]];
        const Node &p2 = _nodes[e.nodes[1]];
        const Node &p3 = _nodes[e.nodes[2]];
        return 0.5 * std::abs(
            (p2.x - p1.x) * (p3.y - p1.y) -
            (p3.x - p1.x) * (p2.y - p1.y)
        );
    }

    // 计算元素刚度矩阵
    void elementMatrix(const Triangle &e, std::vector<std::vector<double>> &Ke) {
        const Node &p1 = _nodes[e.nodes[0]];
        const Node &p2 = _nodes[e.nodes[1]];
        const Node &p3 = _nodes[e.nodes[2]];

        double A = area(e);

        // 形函数梯度（线性三角形为常数）
        double b[3], c[3];
        b[0] = (p2.y - p3.y) / (2*A);
        b[1] = (p3.y - p1.y) / (2*A);
        b[2] = (p1.y - p2.y) / (2*A);

        c[0] = (p3.x - p2.x) / (2*A);
        c[1] = (p1.x - p3.x) / (2*A);
        c[2] = (p2.x - p1.x) / (2*A);

        // Ke = A * (b*b^T + c*c^T)
        Ke.assign(3, std::vector<double>(3));
        for(int i = 0; i < 3; i++) {
            for(int j = 0; j < 3; j++) {
                Ke[i][j] = A * (b[i]*b[j] + c[i]*c[j]);
            }
        }
    }

    // 计算元素载荷向量（源项f=1）
    void elementVector(const Triangle &e, std::vector<double> &Fe, double f = 1.0) {
        double A = area(e);
        Fe.assign(3, f * A / 3.0);  // 均匀分配
    }

    // 装配所有元素
    void addToSystem(DofManager &dm, const std::vector<Triangle> &elements) {
        for(const auto &e : elements) {
            std::vector<std::vector<double>> Ke;
            elementMatrix(e, Ke);

            std::vector<Dof> dofs(3);
            for(int i = 0; i < 3; i++) {
                dofs[i] = {e.nodes[i], 0};
            }

            dm.assemble(Ke, dofs);
        }
    }

    void addSourceToRHS(DofManager &dm, const std::vector<Triangle> &elements,
                        double f = 1.0) {
        for(const auto &e : elements) {
            std::vector<double> Fe;
            elementVector(e, Fe, f);

            std::vector<Dof> dofs(3);
            for(int i = 0; i < 3; i++) {
                dofs[i] = {e.nodes[i], 0};
            }

            dm.assembleRHS(Fe, dofs);
        }
    }
};

// =====================================================
// 第五部分：网格生成（简单的正方形网格）
// =====================================================

void generateSquareMesh(int nx, int ny,
                        std::vector<Node> &nodes,
                        std::vector<Triangle> &elements) {
    double dx = 1.0 / nx;
    double dy = 1.0 / ny;

    // 生成节点
    for(int j = 0; j <= ny; j++) {
        for(int i = 0; i <= nx; i++) {
            Node n;
            n.x = i * dx;
            n.y = j * dy;
            n.fixed = false;
            n.fixedValue = 0.0;

            // 边界节点标记
            if(i == 0 || i == nx || j == 0 || j == ny) {
                n.fixed = true;
                n.fixedValue = 0.0;  // u = 0 在边界
            }

            nodes.push_back(n);
        }
    }

    // 生成三角形元素
    for(int j = 0; j < ny; j++) {
        for(int i = 0; i < nx; i++) {
            int n00 = j * (nx + 1) + i;
            int n10 = n00 + 1;
            int n01 = n00 + (nx + 1);
            int n11 = n01 + 1;

            // 两个三角形覆盖一个正方形
            elements.push_back({{n00, n10, n11}});
            elements.push_back({{n00, n11, n01}});
        }
    }
}

// =====================================================
// 第六部分：后处理（输出VTK文件）
// =====================================================

void exportVTK(const std::string &filename,
               const std::vector<Node> &nodes,
               const std::vector<Triangle> &elements,
               const std::vector<double> &solution) {
    std::ofstream file(filename);
    file << std::fixed << std::setprecision(6);

    file << "# vtk DataFile Version 3.0\n";
    file << "FEM Solution\n";
    file << "ASCII\n";
    file << "DATASET UNSTRUCTURED_GRID\n";

    // 点
    file << "POINTS " << nodes.size() << " double\n";
    for(const auto &n : nodes) {
        file << n.x << " " << n.y << " 0\n";
    }

    // 单元
    file << "CELLS " << elements.size() << " " << elements.size() * 4 << "\n";
    for(const auto &e : elements) {
        file << "3 " << e.nodes[0] << " " << e.nodes[1] << " " << e.nodes[2] << "\n";
    }

    // 单元类型
    file << "CELL_TYPES " << elements.size() << "\n";
    for(size_t i = 0; i < elements.size(); i++) {
        file << "5\n";  // VTK_TRIANGLE
    }

    // 解数据
    file << "POINT_DATA " << nodes.size() << "\n";
    file << "SCALARS solution double 1\n";
    file << "LOOKUP_TABLE default\n";
    for(double v : solution) {
        file << v << "\n";
    }

    file.close();
    std::cout << "结果已导出到 " << filename << "\n";
}

// =====================================================
// 主程序
// =====================================================

int main() {
    std::cout << "=== 迷你有限元求解器 ===" << std::endl;
    std::cout << "求解 Poisson 方程: -Δu = 1, u|∂Ω = 0\n\n";

    // 1. 生成网格
    std::cout << "1. 生成网格..." << std::endl;
    std::vector<Node> nodes;
    std::vector<Triangle> elements;
    int nx = 20, ny = 20;
    generateSquareMesh(nx, ny, nodes, elements);
    std::cout << "   节点数: " << nodes.size() << std::endl;
    std::cout << "   元素数: " << elements.size() << std::endl;

    // 2. 创建求解器组件
    std::cout << "\n2. 初始化求解器..." << std::endl;
    SimpleLinearSystem sys;
    DofManager dm(&sys);
    LaplaceTerm laplace(nodes);

    // 3. 编号自由度
    for(size_t i = 0; i < nodes.size(); i++) {
        Dof d = {(int)i, 0};
        if(nodes[i].fixed) {
            dm.fixDof(d, nodes[i].fixedValue);
        } else {
            dm.numberDof(d);
        }
    }
    std::cout << "   自由度: " << dm.size() << std::endl;

    // 4. 分配系统
    sys.allocate(dm.size());

    // 5. 装配
    std::cout << "\n3. 装配系统..." << std::endl;
    laplace.addToSystem(dm, elements);
    laplace.addSourceToRHS(dm, elements, 1.0);

    // 6. 求解
    std::cout << "\n4. 求解..." << std::endl;
    bool success = sys.solve();

    if(success) {
        // 7. 提取解
        std::vector<double> solution(nodes.size());
        for(size_t i = 0; i < nodes.size(); i++) {
            Dof d = {(int)i, 0};
            solution[i] = dm.getValue(d);
        }

        // 找到最大值
        double maxVal = *std::max_element(solution.begin(), solution.end());
        std::cout << "   最大值: " << maxVal << std::endl;
        std::cout << "   理论值: ~0.0737 (对于单位正方形)\n";

        // 8. 导出结果
        std::cout << "\n5. 导出结果..." << std::endl;
        exportVTK("solution.vtk", nodes, elements, solution);

        std::cout << "\n=== 求解完成 ===" << std::endl;
    } else {
        std::cout << "求解失败！" << std::endl;
        return 1;
    }

    return 0;
}
```

### 5. 练习任务

**任务1**：扩展综合项目

- 添加非均匀源项 f(x,y) = sin(πx)·sin(πy)
- 实现非均匀Dirichlet边界条件
- 添加网格细化和收敛性分析

**任务2**：添加特征值求解

```cpp
// 扩展求解器，计算Laplace特征值问题
// -Δφ = λφ, φ|∂Ω = 0

// 提示：
// 1. 装配刚度矩阵K和质量矩阵M
// 2. 使用幂迭代求最小特征值
// 3. 验证第一特征值 λ₁ ≈ 2π² ≈ 19.74（单位正方形）
```

**任务3**：实现不同求解器

```cpp
// 为SimpleLinearSystem添加多种求解器选项
enum SolverType { GAUSS, JACOBI, GAUSS_SEIDEL, CG };

class SimpleLinearSystem {
    SolverType _solver;
public:
    void setSolver(SolverType s) { _solver = s; }
    bool solve();  // 根据_solver选择算法
};
```

---

## 周检查点

### 理论知识

- [ ] 能描述求解器模块的整体架构
- [ ] 理解linearSystem抽象接口的设计
- [ ] 掌握CSR稀疏矩阵格式
- [ ] 理解特征值问题的数学背景
- [ ] 能解释femTerm的模板方法模式
- [ ] 理解dofManager的装配机制
- [ ] 掌握三种边界条件的处理方法
- [ ] 能比较直接法和迭代法的优缺点

### 实践技能

- [ ] 能手工装配简单的有限元系统
- [ ] 能使用Gmsh API配置求解器
- [ ] 能实现基本的迭代求解器（Jacobi, CG）
- [ ] 能处理Dirichlet和Neumann边界条件
- [ ] 能根据问题特性选择合适的求解器后端

### 代码阅读

- [ ] 已阅读linearSystem.h的抽象接口
- [ ] 已阅读dofManager.h的核心实现
- [ ] 已阅读femTerm.h的模板方法
- [ ] 已阅读elasticitySolver的边界条件处理
- [ ] 已阅读至少一个求解器后端的实现

---

## 关键源码索引

| 主题 | 文件 | 核心函数/类 |
|------|------|------------|
| 线性系统 | linearSystem.h | linearSystemBase, linearSystem<T> |
| CSR格式 | linearSystemCSR.h | addToMatrix, systemSolve |
| 自由度管理 | dofManager.h | Dof, dofManager<T>, assemble |
| 有限元项 | femTerm.h | elementMatrix, addToMatrix |
| 边界条件 | elasticitySolver.h | dirichletBC, neumannBC |
| PETSc后端 | linearSystemPETSc.h | KSP, PC设置 |
| MUMPS后端 | linearSystemMUMPS.h | dmumps_c调用 |
| 特征值 | eigenSolver.h | EPS, solve |

---

## 第十周总结

### 核心收获

1. **模块架构**：理解了Gmsh求解器的分层设计
2. **抽象接口**：掌握了模板方法和策略模式的应用
3. **稀疏存储**：学会了CSR格式及其优化
4. **特征值问题**：了解了结构振动分析的数值方法
5. **有限元装配**：掌握了从局部到全局的装配流程
6. **边界条件**：学会了多种BC处理技术
7. **求解器选择**：能根据问题特性选择合适后端

### 关键洞察

- **抽象与实现分离**：linearSystem接口允许灵活切换后端
- **模板方法模式**：femTerm定义算法骨架，子类实现具体物理
- **约束统一处理**：dofManager的fixed和constraints统一管理
- **预分配重要性**：稀疏模式预计算避免运行时重分配
- **后端权衡**：直接法精度高但内存大，迭代法需要调参

### 下周预告

第11周：API高级应用
- Day 71：高级几何操作
- Day 72：网格操作API
- Day 73：后处理API
- Day 74：脚本自动化
- Day 75：批处理与参数扫描
- Day 76：与外部程序集成
- Day 77：第十一周复习

将深入Gmsh API的高级用法和实际应用。

---

## 扩展阅读建议

### 必读

1. 完成本周所有练习
2. 用debugger追踪完整的求解流程

### 选读

1. 《有限元方法的数学理论》
2. PETSc用户手册
3. Eigen文档
4. MUMPS用户指南

---

## 导航

- 上一天：[Day 69 - 求解器后端](day-69.md)
- 下一天：[Day 71 - 高级几何操作](day-71.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
