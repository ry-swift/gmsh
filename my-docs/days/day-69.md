# Day 69：求解器后端

## 学习目标

深入了解Gmsh集成的三大求解器后端（PETSc、MUMPS、Eigen），掌握它们的特点、配置方法和适用场景选择策略。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | PETSc集成分析 |
| 10:00-11:00 | 1h | MUMPS直接求解器 |
| 11:00-12:00 | 1h | Eigen库集成与对比 |
| 14:00-15:00 | 1h | 求解器选择策略与练习 |

---

## 上午学习内容

### 1. PETSc集成分析

**PETSc简介**：

```text
PETSc (Portable, Extensible Toolkit for Scientific Computation)

特点：
• 大规模并行计算支持
• 丰富的Krylov子空间求解器
• 多种预处理器
• 完善的性能监控

层次结构：
MPI (并行通信)
  ↓
BLAS/LAPACK (基本线性代数)
  ↓
PETSc (高级求解器接口)
  ↓
应用程序
```

**文件位置**：`src/solver/linearSystemPETSc.h`

```cpp
#ifndef LINEAR_SYSTEM_PETSC_H
#define LINEAR_SYSTEM_PETSC_H

#include "linearSystem.h"

#if defined(HAVE_PETSC)
#include <petsc.h>

template<class scalar>
class linearSystemPETSc : public linearSystem<scalar> {
private:
    Mat _a;              // PETSc矩阵对象
    Vec _b, _x;          // 右端向量和解向量
    KSP _ksp;            // Krylov求解器上下文
    PC _pc;              // 预处理器上下文

    int _localSize;      // 本地行数（并行）
    int _globalSize;     // 全局行数
    MPI_Comm _comm;      // MPI通信子

    bool _isAllocated;
    bool _kspAllocated;

    // 稀疏模式预分配
    int *_nnz;           // 每行非零元素数

public:
    linearSystemPETSc(MPI_Comm comm = PETSC_COMM_WORLD);
    ~linearSystemPETSc();

    // 设置稀疏预分配
    void preAllocateEntries() {
        if(_nnz) {
            MatSeqAIJSetPreallocation(_a, 0, _nnz);
            MatMPIAIJSetPreallocation(_a, 0, _nnz, 0, _nnz);
        }
    }

    void allocate(int nbRows) override {
        _globalSize = nbRows;
        _localSize = nbRows;  // 串行情况

        // 创建矩阵
        MatCreate(_comm, &_a);
        MatSetSizes(_a, _localSize, _localSize,
                    _globalSize, _globalSize);
        MatSetType(_a, MATMPIAIJ);
        preAllocateEntries();
        MatSetUp(_a);

        // 创建向量
        VecCreate(_comm, &_b);
        VecSetSizes(_b, _localSize, _globalSize);
        VecSetFromOptions(_b);
        VecDuplicate(_b, &_x);

        _isAllocated = true;
    }

    void addToMatrix(int row, int col, const scalar &val) override {
        MatSetValue(_a, row, col, val, ADD_VALUES);
    }

    scalar getFromMatrix(int row, int col) const override {
        scalar val;
        MatGetValue(_a, row, col, &val);
        return val;
    }

    void addToRightHandSide(int row, const scalar &val, int ith = 0) override {
        VecSetValue(_b, row, val, ADD_VALUES);
    }

    void zeroMatrix() override {
        MatZeroEntries(_a);
    }

    void zeroRightHandSide() override {
        VecZeroEntries(_b);
    }

    int systemSolve() override {
        // 组装矩阵和向量
        MatAssemblyBegin(_a, MAT_FINAL_ASSEMBLY);
        MatAssemblyEnd(_a, MAT_FINAL_ASSEMBLY);
        VecAssemblyBegin(_b);
        VecAssemblyEnd(_b);

        // 创建KSP求解器
        if(!_kspAllocated) {
            KSPCreate(_comm, &_ksp);
            KSPSetOperators(_ksp, _a, _a);
            KSPSetFromOptions(_ksp);  // 允许命令行覆盖
            _kspAllocated = true;
        }

        // 求解
        KSPSolve(_ksp, _b, _x);

        // 检查收敛
        KSPConvergedReason reason;
        KSPGetConvergedReason(_ksp, &reason);
        return (reason > 0) ? 1 : 0;
    }

    // 获取PETSc对象（供eigenSolver使用）
    Mat getMatrix() { return _a; }
    Vec getSolution() { return _x; }
};

#else  // 无PETSc时的空实现

template<class scalar>
class linearSystemPETSc : public linearSystem<scalar> {
public:
    linearSystemPETSc(int comm = 0) {
        Msg::Error("PETSc未启用");
    }
    // ... 虚拟实现
};

#endif

#endif
```

**PETSc配置选项**：

```bash
# ~/.petscrc 或命令行参数

# === 迭代求解器（KSP） ===
-ksp_type gmres           # GMRES（默认）
-ksp_type cg              # 共轭梯度（对称正定）
-ksp_type bicg            # 双共轭梯度
-ksp_type bcgs            # 稳定双共轭梯度
-ksp_type preonly         # 仅预处理（配合直接法）

# === 预处理器（PC） ===
-pc_type ilu              # 不完全LU分解（默认）
-pc_type icc              # 不完全Cholesky
-pc_type jacobi           # Jacobi（对角）
-pc_type sor              # 逐次超松弛
-pc_type lu               # 完全LU分解
-pc_type cholesky         # 完全Cholesky
-pc_type mg               # 多重网格
-pc_type asm              # 加性Schwarz

# === ILU参数 ===
-pc_factor_levels 6       # ILU填充等级
-pc_factor_fill 5.0       # 预估填充因子

# === 收敛控制 ===
-ksp_rtol 1e-8            # 相对容差
-ksp_atol 1e-15           # 绝对容差
-ksp_max_it 1000          # 最大迭代次数
-ksp_gmres_restart 30     # GMRES重启参数

# === 矩阵重排序 ===
-pc_factor_mat_ordering rcm              # Reverse Cuthill-McKee
-pc_factor_mat_ordering nested_dissection  # 嵌套剖分

# === 使用外部直接求解器 ===
-ksp_type preonly
-pc_type lu
-pc_factor_mat_solver_type mumps         # MUMPS
-pc_factor_mat_solver_type superlu       # SuperLU
-pc_factor_mat_solver_type umfpack       # UMFPACK

# === 监控和调试 ===
-ksp_monitor              # 打印残差
-ksp_view                 # 显示求解器配置
-log_view                 # 性能分析
```

### 2. MUMPS直接求解器

**MUMPS简介**：

```text
MUMPS (MUltifrontal Massively Parallel sparse direct Solver)

特点：
• 稀疏直接求解器
• 多核并行（OpenMP + MPI）
• 自动符号分析
• 支持对称/非对称矩阵

适用场景：
• 小到中等规模问题（< 1M自由度）
• 需要高精度解
• 多右端向量问题
• 结构分析（通常对称正定）
```

**文件位置**：`src/solver/linearSystemMUMPS.h`

```cpp
#if defined(HAVE_MUMPS)
#include <dmumps_c.h>

template<class scalar>
class linearSystemMUMPS : public linearSystem<scalar> {
private:
    DMUMPS_STRUC_C _id;    // MUMPS结构

    // COO格式存储（MUMPS要求）
    std::vector<int> _irn;        // 行索引（1-based）
    std::vector<int> _jcn;        // 列索引（1-based）
    std::vector<DMUMPS_REAL> _a;  // 值
    std::vector<DMUMPS_REAL> _b;  // 右端向量
    std::vector<DMUMPS_REAL> _x;  // 解向量

    int _n;                        // 矩阵阶数
    int _nnz;                      // 非零元素数

    // (row, col) -> 值数组索引的映射
    std::vector<std::map<int, int>> _ij;

    bool _analyzed;

public:
    linearSystemMUMPS() : _n(0), _nnz(0), _analyzed(false) {
        // 初始化MUMPS
        _id.job = -1;      // 初始化
        _id.comm_fortran = -987654;  // 使用默认通信
        _id.par = 1;       // 主机参与计算
        _id.sym = 0;       // 非对称（可改为1=对称正定，2=一般对称）
        dmumps_c(&_id);
    }

    ~linearSystemMUMPS() {
        _id.job = -2;      // 终止
        dmumps_c(&_id);
    }

    void allocate(int nbRows) override {
        _n = nbRows;
        _ij.resize(nbRows);
        _b.resize(nbRows, 0.0);
        _x.resize(nbRows, 0.0);
    }

    void addToMatrix(int row, int col, const scalar &val) override {
        auto it = _ij[row].find(col);
        if(it != _ij[row].end()) {
            // 已存在，累加
            _a[it->second] += val;
        } else {
            // 新元素
            int idx = _a.size();
            _a.push_back(val);
            _irn.push_back(row + 1);  // MUMPS使用1-based索引
            _jcn.push_back(col + 1);
            _ij[row][col] = idx;
        }
    }

    void addToRightHandSide(int row, const scalar &val, int ith = 0) override {
        _b[row] += val;
    }

    int systemSolve() override {
        _nnz = _a.size();

        // 设置矩阵数据
        _id.n = _n;
        _id.nz = _nnz;
        _id.irn = _irn.data();
        _id.jcn = _jcn.data();
        _id.a = _a.data();
        _id.rhs = _b.data();

        // 输出控制
        _id.icntl[0] = -1;  // 错误输出关闭
        _id.icntl[1] = -1;  // 诊断输出关闭
        _id.icntl[2] = -1;  // 全局输出关闭
        _id.icntl[3] = 0;   // 输出级别

        if(!_analyzed) {
            // 符号分析
            _id.job = 1;
            dmumps_c(&_id);
            if(_id.info[0] < 0) {
                Msg::Error("MUMPS分析失败: %d", _id.info[0]);
                return 0;
            }

            // 数值分解
            _id.job = 2;
            dmumps_c(&_id);
            if(_id.info[0] < 0) {
                Msg::Error("MUMPS分解失败: %d", _id.info[0]);
                return 0;
            }

            _analyzed = true;
        }

        // 求解
        _id.job = 3;
        dmumps_c(&_id);

        // 解存储在rhs中
        std::copy(_b.begin(), _b.end(), _x.begin());

        return (_id.info[0] >= 0) ? 1 : 0;
    }

    scalar getFromSolution(int row) const override {
        return _x[row];
    }

    void zeroMatrix() override {
        std::fill(_a.begin(), _a.end(), 0.0);
        _analyzed = false;  // 需要重新分解
    }
};

#endif
```

**MUMPS求解阶段**：

```text
阶段1 (job=1): 符号分析
• 矩阵重排序（最小度/嵌套剖分）
• 消元树构建
• 工作内存估计

阶段2 (job=2): 数值分解
• LU分解（或LDL^T/LL^T）
• 多波前算法
• 部分主元选择

阶段3 (job=3): 回代求解
• 前向消元
• 对角缩放
• 后向消元

可选：job=4 同时分解和求解
     job=5 仅分解
     job=6 求解（需要已分解）
```

### 3. Eigen库集成

**Eigen简介**：

```text
Eigen：C++模板线性代数库

特点：
• 头文件库，无需编译
• 表达式模板优化
• 自动向量化（SIMD）
• 丰富的稀疏求解器

适用场景：
• 轻量化应用
• 不依赖外部库
• 中小规模问题
```

**文件位置**：`src/solver/linearSystemEigen.h`

```cpp
#if defined(HAVE_EIGEN)
#include <Eigen/Sparse>
#include <Eigen/SparseLU>
#include <Eigen/SparseCholesky>
#include <Eigen/IterativeLinearSolvers>

// Eigen求解器类型枚举
enum linearSystemEigenSolver {
    EigenCholeskyLLT,      // LLT（对称正定）
    EigenCholeskyLDLT,     // LDLT（对称不定）
    EigenSparseLU,         // 稀疏LU（一般矩阵）
    EigenSparseQR,         // 稀疏QR（最小二乘）
    EigenCG,               // 共轭梯度
    EigenCGLeastSquare,    // 最小二乘CG
    EigenBiCGSTAB          // 双共轭梯度稳定
};

template<class scalar>
class linearSystemEigen : public linearSystem<scalar> {
    using SpMat = Eigen::SparseMatrix<scalar, Eigen::RowMajor>;
    using Vec = Eigen::Matrix<scalar, Eigen::Dynamic, 1>;
    using Triplet = Eigen::Triplet<scalar>;

private:
    SpMat _A;
    Vec _b, _x;
    int _n;

    std::vector<Triplet> _triplets;  // 构建阶段使用

    linearSystemEigenSolver _solverType;
    double _tolerance;
    int _maxIterations;

    bool _matrixBuilt;

public:
    linearSystemEigen(linearSystemEigenSolver type = EigenSparseLU)
        : _n(0), _solverType(type), _tolerance(1e-10),
          _maxIterations(1000), _matrixBuilt(false) {}

    void allocate(int nbRows) override {
        _n = nbRows;
        _b.resize(nbRows);
        _b.setZero();
        _x.resize(nbRows);
        _x.setZero();
    }

    void addToMatrix(int row, int col, const scalar &val) override {
        _triplets.push_back(Triplet(row, col, val));
        _matrixBuilt = false;
    }

    void addToRightHandSide(int row, const scalar &val, int ith = 0) override {
        _b(row) += val;
    }

    void buildMatrix() {
        if(!_matrixBuilt) {
            _A.resize(_n, _n);
            _A.setFromTriplets(_triplets.begin(), _triplets.end());
            _matrixBuilt = true;
        }
    }

    int systemSolve() override {
        buildMatrix();

        bool success = false;

        switch(_solverType) {
        case EigenCholeskyLLT: {
            Eigen::SimplicialLLT<SpMat> solver(_A);
            _x = solver.solve(_b);
            success = (solver.info() == Eigen::Success);
            break;
        }

        case EigenCholeskyLDLT: {
            Eigen::SimplicialLDLT<SpMat> solver(_A);
            _x = solver.solve(_b);
            success = (solver.info() == Eigen::Success);
            break;
        }

        case EigenSparseLU: {
            Eigen::SparseLU<SpMat> solver;
            solver.analyzePattern(_A);
            solver.factorize(_A);
            _x = solver.solve(_b);
            success = (solver.info() == Eigen::Success);
            break;
        }

        case EigenSparseQR: {
            Eigen::SparseQR<SpMat, Eigen::COLAMDOrdering<int>> solver;
            solver.compute(_A);
            _x = solver.solve(_b);
            success = (solver.info() == Eigen::Success);
            break;
        }

        case EigenCG: {
            Eigen::ConjugateGradient<SpMat> solver;
            solver.setTolerance(_tolerance);
            solver.setMaxIterations(_maxIterations);
            solver.compute(_A);
            _x = solver.solve(_b);
            success = (solver.info() == Eigen::Success);
            break;
        }

        case EigenBiCGSTAB: {
            Eigen::BiCGSTAB<SpMat> solver;
            solver.setTolerance(_tolerance);
            solver.setMaxIterations(_maxIterations);
            solver.compute(_A);
            _x = solver.solve(_b);
            success = (solver.info() == Eigen::Success);
            break;
        }
        }

        return success ? 1 : 0;
    }

    scalar getFromSolution(int row) const override {
        return _x(row);
    }

    void zeroMatrix() override {
        _triplets.clear();
        _matrixBuilt = false;
    }

    void zeroRightHandSide() override {
        _b.setZero();
    }

    // 设置迭代求解器参数
    void setTolerance(double tol) { _tolerance = tol; }
    void setMaxIterations(int maxIt) { _maxIterations = maxIt; }
};

#endif
```

---

## 下午学习内容

### 4. 求解器对比与选择

**求解器特性对比**：

| 特性 | PETSc | MUMPS | Eigen |
|------|-------|-------|-------|
| **类型** | 迭代为主 | 直接 | 两者兼有 |
| **并行** | MPI+OpenMP | MPI+OpenMP | OpenMP |
| **规模** | 大规模(>1M) | 中等(<1M) | 小规模(<100K) |
| **依赖** | 重量级 | 中等 | 头文件库 |
| **精度** | 可控（迭代） | 机器精度 | 机器精度/可控 |
| **内存** | 较少 | 较多 | 中等 |
| **鲁棒性** | 需调参 | 自动 | 需选择求解器 |

**选择决策树**：

```text
问题规模？
│
├── < 10,000 自由度
│   └── Eigen (简单，无依赖)
│
├── 10,000 ~ 100,000
│   ├── 需要高精度？
│   │   └── MUMPS 或 Eigen(LU)
│   └── 性能优先？
│       └── PETSc + 迭代
│
├── 100,000 ~ 1,000,000
│   ├── 对称正定？
│   │   └── PETSc + CG + ICC
│   └── 一般矩阵？
│       └── PETSc + GMRES + ILU
│       └── 或 MUMPS（如有足够内存）
│
└── > 1,000,000
    └── PETSc + 并行 + 多级预处理
        或 PETSc + MUMPS后端
```

**矩阵类型与求解器匹配**：

| 矩阵类型 | 推荐求解器 | 预处理器 |
|----------|------------|----------|
| 对称正定 | CG | Cholesky/ICC |
| 对称不定 | MINRES | LDLT |
| 非对称 | GMRES/BiCGSTAB | ILU |
| 病态矩阵 | 直接法 | - |
| 多右端 | 直接法（分解复用） | - |

### 5. 性能调优策略

**PETSc性能调优**：

```bash
# 1. 预处理器级别调整
-pc_factor_levels 0     # 快速，精度低
-pc_factor_levels 3     # 平衡
-pc_factor_levels 6     # 精确，慢

# 2. GMRES重启参数
-ksp_gmres_restart 30   # 默认
-ksp_gmres_restart 100  # 困难问题

# 3. 残差监控
-ksp_monitor_true_residual  # 真实残差（非预处理后）

# 4. 性能分析
-log_view                   # 详细计时
-malloc_log                 # 内存分析
```

**MUMPS性能调优**：

```cpp
// 内存分配策略
_id.icntl[13] = 50;   // 内存增量百分比

// 矩阵排序
_id.icntl[6] = 5;     // 0=AMD, 5=METIS, 7=自动

// 并行度
_id.icntl[27] = 2;    // 并行分析级别
_id.icntl[28] = 2;    // 并行排序

// 数值精度
_id.cntl[0] = 0.01;   // 主元阈值（数值稳定性）
```

### 6. 实际应用示例

**选择合适求解器的代码模式**：

```cpp
// 根据问题特性选择求解器
linearSystemBase* createSolver(int problemSize, bool symmetric,
                                bool positiveDef) {
#if defined(HAVE_PETSC)
    if(problemSize > 100000) {
        // 大规模问题用PETSc
        auto sys = new linearSystemPETSc<double>();
        // 配置参数...
        return sys;
    }
#endif

#if defined(HAVE_MUMPS)
    if(problemSize > 10000 && problemSize < 500000) {
        // 中等规模用MUMPS
        auto sys = new linearSystemMUMPS<double>();
        return sys;
    }
#endif

#if defined(HAVE_EIGEN)
    // 小规模或无其他选择时用Eigen
    auto type = positiveDef ? EigenCholeskyLLT : EigenSparseLU;
    return new linearSystemEigen<double>(type);
#endif

    // 回退到CSR内置实现
    return new linearSystemCSR<double>();
}
```

---

## 练习作业

### 基础练习

**练习1**：理解求解器差异

```text
给定同一个线性系统：

1. 分别用以下方式求解并比较：
   - 高斯消元（直接法）
   - Jacobi迭代
   - Gauss-Seidel迭代
   - CG（如果对称正定）

2. 记录：
   - 迭代次数（迭代法）
   - 计算时间
   - 最终残差

3. 讨论：何时选择直接法vs迭代法？
```

**练习2**：PETSc选项实验

```bash
# 运行Gmsh弹性问题，测试不同配置

# 配置1：默认迭代
gmsh -3 mesh.geo -setnumber ElasticSolver 1

# 配置2：直接求解
gmsh -3 mesh.geo -setnumber ElasticSolver 1 \
  -ksp_type preonly -pc_type lu

# 配置3：高精度迭代
gmsh -3 mesh.geo -setnumber ElasticSolver 1 \
  -ksp_rtol 1e-12 -pc_factor_levels 8

# 比较结果精度和求解时间
```

### 进阶练习

**练习3**：实现求解器包装类

```cpp
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>

// 统一的求解器接口
class LinearSolverBase {
public:
    virtual ~LinearSolverBase() {}
    virtual void setMatrix(int n, const std::vector<std::vector<double>> &A) = 0;
    virtual void setRHS(const std::vector<double> &b) = 0;
    virtual bool solve() = 0;
    virtual std::vector<double> getSolution() const = 0;
    virtual int getIterations() const { return 0; }
    virtual double getResidual() const = 0;
};

// 高斯消元（直接法）
class GaussElimination : public LinearSolverBase {
    int _n;
    std::vector<std::vector<double>> _A;
    std::vector<double> _b, _x;

public:
    void setMatrix(int n, const std::vector<std::vector<double>> &A) override {
        _n = n;
        _A = A;
    }

    void setRHS(const std::vector<double> &b) override {
        _b = b;
    }

    bool solve() override {
        _x.resize(_n);
        auto A = _A;
        auto b = _b;

        // 前向消元
        for(int k = 0; k < _n; k++) {
            // 部分主元
            int maxRow = k;
            for(int i = k + 1; i < _n; i++) {
                if(std::abs(A[i][k]) > std::abs(A[maxRow][k])) {
                    maxRow = i;
                }
            }
            std::swap(A[k], A[maxRow]);
            std::swap(b[k], b[maxRow]);

            for(int i = k + 1; i < _n; i++) {
                double factor = A[i][k] / A[k][k];
                for(int j = k; j < _n; j++) {
                    A[i][j] -= factor * A[k][j];
                }
                b[i] -= factor * b[k];
            }
        }

        // 回代
        for(int i = _n - 1; i >= 0; i--) {
            _x[i] = b[i];
            for(int j = i + 1; j < _n; j++) {
                _x[i] -= A[i][j] * _x[j];
            }
            _x[i] /= A[i][i];
        }

        return true;
    }

    std::vector<double> getSolution() const override { return _x; }

    double getResidual() const override {
        double res = 0;
        for(int i = 0; i < _n; i++) {
            double ri = _b[i];
            for(int j = 0; j < _n; j++) {
                ri -= _A[i][j] * _x[j];
            }
            res += ri * ri;
        }
        return std::sqrt(res);
    }
};

// 共轭梯度法
class ConjugateGradient : public LinearSolverBase {
    int _n;
    std::vector<std::vector<double>> _A;
    std::vector<double> _b, _x;
    double _tol;
    int _maxIter;
    int _iterations;

    std::vector<double> matVec(const std::vector<double> &v) {
        std::vector<double> result(_n, 0.0);
        for(int i = 0; i < _n; i++) {
            for(int j = 0; j < _n; j++) {
                result[i] += _A[i][j] * v[j];
            }
        }
        return result;
    }

    double dot(const std::vector<double> &a, const std::vector<double> &b) {
        double s = 0;
        for(int i = 0; i < _n; i++) s += a[i] * b[i];
        return s;
    }

public:
    ConjugateGradient(double tol = 1e-10, int maxIter = 1000)
        : _tol(tol), _maxIter(maxIter), _iterations(0) {}

    void setMatrix(int n, const std::vector<std::vector<double>> &A) override {
        _n = n;
        _A = A;
    }

    void setRHS(const std::vector<double> &b) override {
        _b = b;
    }

    bool solve() override {
        _x.assign(_n, 0.0);

        // r = b - A*x
        auto Ax = matVec(_x);
        std::vector<double> r(_n), p(_n);
        for(int i = 0; i < _n; i++) {
            r[i] = _b[i] - Ax[i];
            p[i] = r[i];
        }

        double rsold = dot(r, r);

        for(_iterations = 0; _iterations < _maxIter; _iterations++) {
            auto Ap = matVec(p);
            double pAp = dot(p, Ap);
            double alpha = rsold / pAp;

            for(int i = 0; i < _n; i++) {
                _x[i] += alpha * p[i];
                r[i] -= alpha * Ap[i];
            }

            double rsnew = dot(r, r);
            if(std::sqrt(rsnew) < _tol) {
                _iterations++;
                return true;
            }

            double beta = rsnew / rsold;
            for(int i = 0; i < _n; i++) {
                p[i] = r[i] + beta * p[i];
            }

            rsold = rsnew;
        }

        return false;
    }

    std::vector<double> getSolution() const override { return _x; }
    int getIterations() const override { return _iterations; }

    double getResidual() const override {
        double res = 0;
        for(int i = 0; i < _n; i++) {
            double ri = _b[i];
            for(int j = 0; j < _n; j++) {
                ri -= _A[i][j] * _x[j];
            }
            res += ri * ri;
        }
        return std::sqrt(res);
    }
};

// 测试函数
void benchmark(LinearSolverBase *solver, const std::string &name,
               int n, const std::vector<std::vector<double>> &A,
               const std::vector<double> &b) {
    solver->setMatrix(n, A);
    solver->setRHS(b);

    auto start = std::chrono::high_resolution_clock::now();
    bool success = solver->solve();
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << name << ":\n";
    std::cout << "  成功: " << (success ? "是" : "否") << "\n";
    std::cout << "  时间: " << ms << " ms\n";
    std::cout << "  残差: " << solver->getResidual() << "\n";
    if(solver->getIterations() > 0) {
        std::cout << "  迭代: " << solver->getIterations() << "\n";
    }
    std::cout << "\n";
}

int main() {
    // 创建对称正定矩阵（用于CG）
    int n = 100;
    std::vector<std::vector<double>> A(n, std::vector<double>(n, 0.0));
    std::vector<double> b(n, 1.0);

    // 三对角矩阵
    for(int i = 0; i < n; i++) {
        A[i][i] = 4.0;
        if(i > 0) A[i][i-1] = -1.0;
        if(i < n-1) A[i][i+1] = -1.0;
    }

    // 测试不同求解器
    GaussElimination gauss;
    ConjugateGradient cg;

    benchmark(&gauss, "高斯消元", n, A, b);
    benchmark(&cg, "共轭梯度", n, A, b);

    return 0;
}
```

### 挑战练习

**练习4**：实现预处理CG

```cpp
// 添加Jacobi预处理到CG算法
class PreconditionedCG : public LinearSolverBase {
    // TODO: 实现
    // 1. 提取对角元素作为预处理器 M = diag(A)
    // 2. 修改CG算法使用 M^{-1}
    //    z = M^{-1} r
    //    使用z而非r进行计算
};
```

**练习5**：分析Gmsh求解器选择

```cpp
// 阅读Gmsh源码，回答：

// 1. 如何在运行时切换求解器后端？
// 2. elasticitySolver默认使用什么求解器？
// 3. 如何通过命令行参数影响求解器行为？
```

---

## 知识图谱

```text
求解器后端
│
├── PETSc
│   ├── KSP（Krylov求解器）
│   ├── PC（预处理器）
│   ├── 并行支持
│   └── 命令行配置
│
├── MUMPS
│   ├── 多波前算法
│   ├── 符号分析
│   ├── 数值分解
│   └── 回代求解
│
├── Eigen
│   ├── 直接法（LU/Cholesky）
│   ├── 迭代法（CG/BiCGSTAB）
│   └── 头文件库
│
├── 选择策略
│   ├── 问题规模
│   ├── 矩阵类型
│   ├── 精度要求
│   └── 资源限制
│
└── 性能调优
    ├── 预处理器选择
    ├── 迭代参数
    └── 内存管理
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 | 优先级 |
|------|----------|--------|--------|
| linearSystemPETSc.h | PETSc集成 | ~175行 | ★★★★★ |
| linearSystemMUMPS.h | MUMPS集成 | ~85行 | ★★★★☆ |
| linearSystemMUMPS.cpp | MUMPS实现 | ~170行 | ★★★★☆ |
| linearSystemEigen.h | Eigen集成 | ~217行 | ★★★★☆ |
| linearSystemCSR.h | CSR内置实现 | ~470行 | ★★★☆☆ |

---

## 今日检查点

- [ ] 理解PETSc的KSP/PC体系
- [ ] 掌握MUMPS的三阶段求解流程
- [ ] 了解Eigen求解器的选项
- [ ] 能根据问题特性选择合适求解器
- [ ] 知道基本的性能调优策略
- [ ] 完成求解器对比练习

---

## 导航

- 上一天：[Day 68 - 边界条件处理](day-68.md)
- 下一天：[Day 70 - 第十周复习](day-70.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
