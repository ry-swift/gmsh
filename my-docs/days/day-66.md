# Day 66：特征值问题

## 学习目标

掌握Gmsh中特征值问题的数学背景、eigenSolver的实现原理，理解SLEPc库的集成方式及在结构动力学中的应用。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 特征值问题数学基础 |
| 10:00-11:00 | 1h | eigenSolver类分析 |
| 11:00-12:00 | 1h | SLEPc库集成 |
| 14:00-15:00 | 1h | 模态分析实践 |

---

## 上午学习内容

### 1. 特征值问题数学基础

**标准特征值问题**：

```text
求解 Ax = λx

其中：
• A: n×n矩阵
• λ: 特征值（标量）
• x: 特征向量（非零）

几何意义：
特征向量在A变换下只改变长度（缩放λ倍），不改变方向
```

**广义特征值问题**：

```text
求解 Ax = λBx

其中：
• A: 刚度矩阵（对称正定）
• B: 质量矩阵（对称正定）
• λ: 广义特征值
• x: 广义特征向量

物理意义：
在结构动力学中，λ = ω² 代表振动频率的平方
```

**有限元中的特征值问题**：

```text
结构振动方程：
Mü + Ku = 0

假设谐波响应：u(t) = φ·sin(ωt)

代入得：
-ω²Mφ + Kφ = 0
Kφ = ω²Mφ

这是广义特征值问题：
• K: 刚度矩阵
• M: 质量矩阵
• ω²: 特征值（固有频率的平方）
• φ: 模态振型（特征向量）
```

**特征值问题分类**：

| 类型 | 方程 | 应用 |
|------|------|------|
| 标准 | Ax = λx | Laplace特征值 |
| 广义 | Ax = λBx | 结构振动 |
| 多项式 | (λ²A₂ + λA₁ + A₀)x = 0 | 阻尼振动 |
| 非线性 | A(λ)x = 0 | 特殊物理问题 |

### 2. 特征值求解算法

**幂迭代法（Power Iteration）**：

```cpp
// 求最大特征值和对应特征向量
void powerIteration(const Matrix &A, Vector &x, double &lambda,
                    int maxIter = 1000, double tol = 1e-8) {
    int n = A.size();
    x.normalize();

    for(int iter = 0; iter < maxIter; iter++) {
        // y = A * x
        Vector y = A.multiply(x);

        // 新特征值估计（Rayleigh商）
        double lambda_new = dot(x, y) / dot(x, x);

        // 归一化
        y.normalize();

        // 收敛检查
        if(std::abs(lambda_new - lambda) < tol) {
            lambda = lambda_new;
            x = y;
            return;
        }

        lambda = lambda_new;
        x = y;
    }
}
```

**优缺点**：
- 优点：简单，只需矩阵-向量乘法
- 缺点：只能求最大特征值，收敛慢

**Krylov子空间方法**：

```text
Krylov子空间：
K_m(A, v) = span{v, Av, A²v, ..., A^(m-1)v}

Arnoldi过程：构建K_m的正交基
Lanczos过程：对称矩阵的简化版本

Krylov-Schur算法（SLEPc默认）：
1. 构建Arnoldi分解
2. 对投影矩阵求特征值
3. Schur分解提取Ritz值
4. 隐式重启加速收敛
```

**移位-逆迭代（Shift-Invert）**：

```text
变换：(A - σI)⁻¹x = μx

其中：μ = 1/(λ - σ)

优势：
• 将接近σ的特征值变换为最大
• 快速找到指定区域的特征值
• 用于求内部特征值

代价：
• 需要分解(A - σI)
• 每次迭代需要线性求解
```

### 3. eigenSolver类分析

**文件位置**：`src/solver/eigenSolver.h`

```cpp
#ifndef EIGENSOLVER_H
#define EIGENSOLVER_H

#include <vector>
#include <complex>
#include <string>

#if defined(HAVE_SLEPC)
#include <slepceps.h>
#endif

class eigenSolver {
private:
#if defined(HAVE_SLEPC)
    EPS _eps;                    // SLEPc特征值求解器上下文
    Mat _A, _B;                  // PETSc矩阵对象
    int _nev;                    // 请求的特征值数
#endif

    // 结果存储
    std::vector<std::complex<double>> _eigenValues;
    std::vector<std::vector<std::complex<double>>> _eigenVectors;

    bool _hermitian;             // 是否Hermite问题

public:
    eigenSolver(linearSystemPETSc<double> *A,
                linearSystemPETSc<double> *B = nullptr,
                bool hermitian = true);

    ~eigenSolver();

    // 求解接口
    bool solve(int numEigenValues,           // 请求的特征值数
               std::string which = "",        // 选择准则
               std::string method = "krylovschur",  // 算法
               double tolVal = 1e-7,          // 容差
               int iterMax = 20);             // 最大迭代

    // 结果访问
    int getNumConvergedEigenValues() const;
    std::complex<double> getEigenValue(int num) const;
    std::vector<std::complex<double>>& getEigenVector(int num);

    // 模态归一化
    void normalize_mode(std::vector<int> modeView, double scale);
};

#endif
```

**关键参数说明**：

| 参数 | 值 | 说明 |
|------|-----|------|
| which | "LM" | Largest Magnitude（最大模） |
| | "SM" | Smallest Magnitude（最小模） |
| | "LR" | Largest Real（最大实部） |
| | "SR" | Smallest Real（最小实部） |
| | "TARGET_MAGNITUDE" | 接近目标值 |
| method | "krylovschur" | Krylov-Schur（默认） |
| | "arnoldi" | Arnoldi |
| | "lanczos" | Lanczos（对称问题） |
| | "power" | 幂迭代 |

---

## 下午学习内容

### 4. SLEPc库集成

**SLEPc简介**：

```text
SLEPc (Scalable Library for Eigenvalue Problem Computations)
• 构建在PETSc之上
• 支持大规模稀疏特征值问题
• 提供多种算法和预处理

层次结构：
MPI → PETSc → SLEPc
      (线性系统)  (特征值)
```

**eigenSolver实现详解**：

```cpp
// eigenSolver.cpp 核心实现
#include "eigenSolver.h"

#if defined(HAVE_SLEPC)

eigenSolver::eigenSolver(linearSystemPETSc<double> *A,
                         linearSystemPETSc<double> *B,
                         bool hermitian)
    : _hermitian(hermitian) {
    // 获取PETSc矩阵
    _A = A->getMatrix();
    _B = B ? B->getMatrix() : nullptr;

    // 创建EPS上下文
    EPSCreate(PETSC_COMM_WORLD, &_eps);

    // 设置算子
    if(_B) {
        EPSSetOperators(_eps, _A, _B);  // 广义问题
        EPSSetProblemType(_eps, EPS_GHEP);  // 广义Hermite
    } else {
        EPSSetOperators(_eps, _A, nullptr);  // 标准问题
        EPSSetProblemType(_eps, _hermitian ? EPS_HEP : EPS_NHEP);
    }
}

eigenSolver::~eigenSolver() {
    EPSDestroy(&_eps);
}

bool eigenSolver::solve(int numEigenValues, std::string which,
                        std::string method, double tolVal, int iterMax) {
    _nev = numEigenValues;

    // 设置求解参数
    EPSSetDimensions(_eps, _nev, PETSC_DEFAULT, PETSC_DEFAULT);
    EPSSetTolerances(_eps, tolVal, iterMax);

    // 设置特征值选择准则
    if(which == "LM") EPSSetWhichEigenpairs(_eps, EPS_LARGEST_MAGNITUDE);
    else if(which == "SM") EPSSetWhichEigenpairs(_eps, EPS_SMALLEST_MAGNITUDE);
    else if(which == "LR") EPSSetWhichEigenpairs(_eps, EPS_LARGEST_REAL);
    else if(which == "SR") EPSSetWhichEigenpairs(_eps, EPS_SMALLEST_REAL);
    // ... 更多选项

    // 设置求解方法
    if(method == "krylovschur") EPSSetType(_eps, EPSKRYLOVSCHUR);
    else if(method == "arnoldi") EPSSetType(_eps, EPSARNOLDI);
    else if(method == "lanczos") EPSSetType(_eps, EPSLANCZOS);
    else if(method == "power") EPSSetType(_eps, EPSPOWER);

    // 从命令行/选项覆盖
    EPSSetFromOptions(_eps);

    // 求解
    EPSSolve(_eps);

    // 获取收敛的特征对数量
    int nconv;
    EPSGetConverged(_eps, &nconv);

    // 提取结果
    _eigenValues.resize(nconv);
    _eigenVectors.resize(nconv);

    Vec xr, xi;  // 实部和虚部向量
    MatCreateVecs(_A, nullptr, &xr);
    MatCreateVecs(_A, nullptr, &xi);

    for(int i = 0; i < nconv; i++) {
        PetscScalar kr, ki;
        EPSGetEigenpair(_eps, i, &kr, &ki, xr, xi);

        _eigenValues[i] = std::complex<double>(PetscRealPart(kr),
                                                PetscImaginaryPart(kr));

        // 提取特征向量
        const PetscScalar *xr_array;
        VecGetArrayRead(xr, &xr_array);
        int n;
        VecGetSize(xr, &n);
        _eigenVectors[i].resize(n);
        for(int j = 0; j < n; j++) {
            _eigenVectors[i][j] = xr_array[j];
        }
        VecRestoreArrayRead(xr, &xr_array);
    }

    VecDestroy(&xr);
    VecDestroy(&xi);

    return nconv > 0;
}

#else  // 无SLEPc时的空实现

eigenSolver::eigenSolver(linearSystemPETSc<double> *A,
                         linearSystemPETSc<double> *B,
                         bool hermitian) {
    Msg::Error("eigenSolver需要SLEPc库支持");
}

bool eigenSolver::solve(int, std::string, std::string, double, int) {
    return false;
}

#endif
```

### 5. 模态分析应用

**结构振动模态分析流程**：

```text
1. 建立有限元模型
   ↓
2. 组装刚度矩阵K和质量矩阵M
   ↓
3. 求解广义特征值问题 Kφ = ω²Mφ
   ↓
4. 提取特征值（频率）和特征向量（振型）
   ↓
5. 后处理和可视化
```

**使用eigenSolver进行模态分析**：

```cpp
// 结构模态分析示例
void performModalAnalysis(GModel *model, int numModes) {
    // 1. 创建线性系统（刚度和质量）
    linearSystemPETSc<double> *K = new linearSystemPETSc<double>();
    linearSystemPETSc<double> *M = new linearSystemPETSc<double>();

    // 2. 创建装配器
    dofManager<double> *dmK = new dofManager<double>(K);
    dofManager<double> *dmM = new dofManager<double>(M);

    // 3. 编号自由度
    for(auto v : model->vertices()) {
        for(int comp = 0; comp < 3; comp++) {
            Dof d(v->tag(), comp);
            dmK->numberDof(d);
            dmM->numberDof(d);
        }
    }

    // 4. 分配系统
    K->allocate(dmK->sizeOfR());
    M->allocate(dmM->sizeOfR());

    // 5. 装配刚度矩阵和质量矩阵
    // ... (使用elasticityTerm和massTerm)

    // 6. 创建特征值求解器
    eigenSolver solver(K, M, true);  // true = Hermite问题

    // 7. 求解
    bool success = solver.solve(
        numModes,           // 求numModes个模态
        "SM",               // 最小特征值（低频模态）
        "krylovschur",      // Krylov-Schur算法
        1e-8,               // 容差
        100                 // 最大迭代
    );

    if(success) {
        int nconv = solver.getNumConvergedEigenValues();
        std::cout << "收敛的模态数: " << nconv << "\n";

        for(int i = 0; i < nconv; i++) {
            auto lambda = solver.getEigenValue(i);
            double freq = std::sqrt(lambda.real()) / (2 * M_PI);
            std::cout << "模态" << i+1 << ": 频率 = " << freq << " Hz\n";
        }
    }
}
```

### 6. 命令行配置

**SLEPc选项（通过PETSc选项系统）**：

```bash
# 基本特征值求解选项
-eps_type krylovschur          # 求解器类型
-eps_nev 10                    # 请求的特征值数
-eps_tol 1e-8                  # 收敛容差
-eps_max_it 100                # 最大迭代

# 特征值选择
-eps_which smallest_magnitude  # 最小模
-eps_target 0.5                # 目标值（用于shift-invert）

# 频谱变换（Spectral Transformation）
-st_type sinvert               # shift-invert
-st_shift 1.0                  # 移位值

# 预处理（用于shift-invert中的线性求解）
-st_ksp_type preonly
-st_pc_type lu
-st_pc_factor_mat_solver_type mumps

# 调试输出
-eps_view                      # 显示求解器配置
-eps_monitor                   # 监控收敛过程
```

---

## 练习作业

### 基础练习

**练习1**：手算特征值

```text
给定矩阵：
A = [3  1]
    [1  3]

1. 计算特征多项式 det(A - λI) = 0
2. 求解特征值 λ₁, λ₂
3. 求解对应的特征向量

验证：
• 检验 Ax = λx
• 检验特征向量正交性（对称矩阵）
```

**练习2**：理解广义特征值

```text
给定：
K = [2  -1]    M = [2  0]
    [-1  2]        [0  1]

求解 Kx = λMx

1. 转化为标准形式：M⁻¹Kx = λx
2. 计算M⁻¹K
3. 求特征值和特征向量
4. 验证解的正确性
```

### 进阶练习

**练习3**：实现幂迭代法

```cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <random>

// 简单的稠密矩阵类
class Matrix {
    int _n;
    std::vector<std::vector<double>> _data;

public:
    Matrix(int n) : _n(n), _data(n, std::vector<double>(n, 0.0)) {}

    double& operator()(int i, int j) { return _data[i][j]; }
    double operator()(int i, int j) const { return _data[i][j]; }

    int size() const { return _n; }

    // 矩阵-向量乘法
    std::vector<double> multiply(const std::vector<double> &x) const {
        std::vector<double> y(_n, 0.0);
        for(int i = 0; i < _n; i++) {
            for(int j = 0; j < _n; j++) {
                y[i] += _data[i][j] * x[j];
            }
        }
        return y;
    }
};

// 向量归一化
double normalize(std::vector<double> &v) {
    double norm = 0.0;
    for(auto x : v) norm += x * x;
    norm = std::sqrt(norm);
    for(auto &x : v) x /= norm;
    return norm;
}

// 向量点积
double dot(const std::vector<double> &a, const std::vector<double> &b) {
    double s = 0.0;
    for(size_t i = 0; i < a.size(); i++) s += a[i] * b[i];
    return s;
}

// 幂迭代法
// 返回最大特征值，x存储对应特征向量
double powerIteration(const Matrix &A, std::vector<double> &x,
                      int maxIter = 1000, double tol = 1e-10) {
    int n = A.size();

    // 随机初始化
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);
    for(int i = 0; i < n; i++) x[i] = dis(gen);
    normalize(x);

    double lambda = 0.0;

    for(int iter = 0; iter < maxIter; iter++) {
        // y = A * x
        std::vector<double> y = A.multiply(x);

        // Rayleigh商估计特征值
        double lambda_new = dot(x, y);

        // 归一化
        normalize(y);

        // 收敛检查
        if(std::abs(lambda_new - lambda) < tol) {
            x = y;
            std::cout << "幂迭代收敛，迭代" << iter + 1 << "次\n";
            return lambda_new;
        }

        lambda = lambda_new;
        x = y;
    }

    std::cout << "幂迭代未收敛\n";
    return lambda;
}

// 反幂迭代法（求最小特征值）
// 需要求解 (A - σI)y = x
double inversePowerIteration(const Matrix &A, std::vector<double> &x,
                             double sigma = 0.0,
                             int maxIter = 1000, double tol = 1e-10) {
    // TODO: 实现反幂迭代
    // 提示：
    // 1. 构造 B = A - σI
    // 2. 每次迭代求解 By = x
    // 3. 可以使用简单的高斯消元
    return 0.0;
}

int main() {
    // 创建对称正定矩阵
    Matrix A(3);
    A(0, 0) = 4; A(0, 1) = 1; A(0, 2) = 1;
    A(1, 0) = 1; A(1, 1) = 3; A(1, 2) = 0;
    A(2, 0) = 1; A(2, 1) = 0; A(2, 2) = 2;

    std::vector<double> x(3);

    // 求最大特征值
    double lambda_max = powerIteration(A, x);
    std::cout << "最大特征值: " << lambda_max << "\n";
    std::cout << "特征向量: [" << x[0] << ", " << x[1] << ", " << x[2] << "]\n";

    // 验证 Ax = λx
    auto Ax = A.multiply(x);
    std::cout << "验证 Ax: [" << Ax[0] << ", " << Ax[1] << ", " << Ax[2] << "]\n";
    std::cout << "λx:     [" << lambda_max*x[0] << ", "
              << lambda_max*x[1] << ", " << lambda_max*x[2] << "]\n";

    return 0;
}
```

### 挑战练习

**练习4**：实现QR算法

```cpp
// QR算法可以求所有特征值
// 基本思想：A = QR → A' = RQ → 重复

// TODO: 实现以下功能
// 1. Householder QR分解
// 2. QR迭代
// 3. 带移位的QR算法（加速收敛）

void qrDecomposition(const Matrix &A, Matrix &Q, Matrix &R) {
    // Householder反射实现
    // 你的代码
}

std::vector<double> qrAlgorithm(Matrix A, int maxIter = 1000) {
    int n = A.size();
    std::vector<double> eigenvalues(n);

    for(int iter = 0; iter < maxIter; iter++) {
        Matrix Q(n), R(n);
        qrDecomposition(A, Q, R);

        // A' = R * Q
        // 你的代码

        // 检查收敛（下三角接近零）
    }

    // 提取对角线作为特征值
    for(int i = 0; i < n; i++) {
        eigenvalues[i] = A(i, i);
    }

    return eigenvalues;
}
```

**练习5**：分析Gmsh模态分析输出

```cpp
// 阅读Gmsh弹性求解器的模态分析功能
// 文件：src/solver/elasticitySolver.cpp

// 问题：
// 1. 质量矩阵如何装配？使用什么有限元项？
// 2. 模态归一化采用什么准则？
// 3. 如何将振型结果输出为PView？
```

---

## 知识图谱

```text
特征值问题
│
├── 问题类型
│   ├── 标准问题 Ax = λx
│   ├── 广义问题 Ax = λBx
│   └── 多项式问题
│
├── 求解算法
│   ├── 幂迭代（最大特征值）
│   ├── 反幂迭代（最小/指定特征值）
│   ├── QR算法（所有特征值）
│   └── Krylov方法（大规模稀疏）
│
├── SLEPc集成
│   ├── EPS上下文
│   ├── 频谱变换(ST)
│   └── 命令行配置
│
├── eigenSolver类
│   ├── 构造器（A, B矩阵）
│   ├── solve()方法
│   └── 结果访问接口
│
└── 应用
    ├── 结构振动模态
    ├── 热传导稳定性
    └── 流体稳定性
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 | 优先级 |
|------|----------|--------|--------|
| eigenSolver.h | 特征值求解器定义 | ~90行 | ★★★★★ |
| eigenSolver.cpp | SLEPc集成实现 | ~140行 | ★★★★★ |
| linearSystemPETSc.h | PETSc矩阵接口 | ~175行 | ★★★★☆ |
| elasticitySolver.cpp | 模态分析应用 | ~1000行 | ★★★☆☆ |

---

## 今日检查点

- [ ] 理解标准和广义特征值问题的定义
- [ ] 掌握幂迭代法的原理
- [ ] 了解Krylov子空间方法的基本思想
- [ ] 理解eigenSolver类的接口设计
- [ ] 知道如何配置SLEPc求解器
- [ ] 完成幂迭代练习

---

## 导航

- 上一天：[Day 65 - 线性系统求解](day-65.md)
- 下一天：[Day 67 - 有限元装配](day-67.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
