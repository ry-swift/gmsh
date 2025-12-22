# Day 65：线性系统求解

## 学习目标

深入理解Gmsh的线性系统抽象接口，掌握CSR稀疏矩阵格式的原理与实现，学习不同存储格式的特点和适用场景。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 线性系统抽象接口详解 |
| 10:00-11:00 | 1h | CSR稀疏矩阵格式 |
| 11:00-12:00 | 1h | 阅读linearSystemCSR实现 |
| 14:00-15:00 | 1h | 练习与总结 |

---

## 上午学习内容

### 1. 线性系统抽象接口

**文件位置**：`src/solver/linearSystem.h`

**设计动机**：
- 统一不同求解器后端的接口
- 支持多种标量类型（实数、复数、矩阵）
- 允许运行时切换求解策略

**接口层次**：

```text
linearSystemBase (非模板基类)
        │
        ├── isAllocated(), allocate(), clear()
        ├── systemSolve()
        └── setParameter()
        │
        ▼
linearSystem<T> (模板化接口)
        │
        ├── addToMatrix(), getFromMatrix()
        ├── addToRightHandSide(), getFromRightHandSide()
        ├── addToSolution(), getFromSolution()
        └── zero*() 系列函数
        │
        ▼
具体实现
        │
        ├── linearSystemFull<T>     (稠密矩阵)
        ├── linearSystemCSR<T>      (CSR格式)
        ├── linearSystemPETSc<T>    (PETSc后端)
        ├── linearSystemMUMPS<T>    (MUMPS后端)
        └── linearSystemEigen<T>    (Eigen后端)
```

**核心接口详解**：

```cpp
// 非模板基类：提供类型无关的操作
class linearSystemBase {
public:
    virtual ~linearSystemBase() {}

    // 参数配置（如迭代容差、最大迭代次数）
    virtual void setParameter(const std::string &key,
                              const std::string &value) {}

    // 内存管理
    virtual bool isAllocated() const = 0;
    virtual void allocate(int nbRows) = 0;
    virtual void clear() = 0;

    // 求解核心
    virtual int systemSolve() = 0;

    // 预处理器精度设置
    virtual void setPrec(double p) {}
};

// 模板化接口：标量类型参数化
template<class scalar>
class linearSystem : public linearSystemBase {
public:
    // ========== 矩阵操作 ==========

    // 累加到矩阵元素 A[row][col] += val
    virtual void addToMatrix(int row, int col, const scalar &val) = 0;

    // 读取矩阵元素
    virtual scalar getFromMatrix(int row, int col) const = 0;

    // ========== 右端向量操作 ==========

    // 累加到右端向量 b[row] += val
    // ith: 支持多右端向量（默认第0个）
    virtual void addToRightHandSide(int row, const scalar &val,
                                     int ith = 0) = 0;
    virtual scalar getFromRightHandSide(int row) const = 0;

    // ========== 解向量操作 ==========

    virtual void addToSolution(int row, const scalar &val) = 0;
    virtual scalar getFromSolution(int row) const = 0;

    // ========== 归零操作 ==========

    virtual void zeroMatrix() = 0;
    virtual void zeroRightHandSide() = 0;
    virtual void zeroSolution() = 0;
};
```

**为什么需要非模板基类**：

```cpp
// 场景：需要存储不同类型的线性系统
std::vector<linearSystemBase*> solvers;

// 可以存储不同标量类型
solvers.push_back(new linearSystemCSR<double>());
solvers.push_back(new linearSystemCSR<std::complex<double>>());

// 统一调用求解
for(auto s : solvers) {
    s->systemSolve();  // 类型无关的操作
}
```

### 2. 稀疏矩阵存储格式

**为什么需要稀疏存储**：

```text
有限元矩阵特点：
• 大规模：典型问题 N = 10^5 ~ 10^7
• 稀疏：非零元素占比通常 < 1%
• 结构化：非零元素分布有规律

存储对比（N=100,000，每行平均50个非零元素）：
┌─────────────┬──────────────┬─────────────┐
│   格式      │    内存      │   效率      │
├─────────────┼──────────────┼─────────────┤
│ 稠密矩阵    │ 80 GB        │ 访问O(1)    │
│ CSR格式     │ 60 MB        │ 访问O(nnz/n)│
│ COO格式     │ 80 MB        │ 随机插入快  │
└─────────────┴──────────────┴─────────────┘
```

**常见稀疏格式对比**：

| 格式 | 全称 | 特点 | 适用场景 |
|------|------|------|----------|
| CSR | Compressed Sparse Row | 行优先，乘法快 | 迭代求解 |
| CSC | Compressed Sparse Column | 列优先 | 转置操作 |
| COO | Coordinate | 三元组，构建快 | 装配阶段 |
| DIA | Diagonal | 对角带状 | 结构化网格 |
| BSR | Block Sparse Row | 块存储 | 向量场问题 |

### 3. CSR格式详解

**CSR (Compressed Sparse Row) 结构**：

```text
原始矩阵 A (4x4):
┌                    ┐
│  5   0   0   2     │   行0: [5, 2]
│  0   8   0   0     │   行1: [8]
│  0   0   3   0     │   行2: [3]
│  0   6   0   9     │   行3: [6, 9]
└                    ┘

CSR表示：
values   = [5, 2, 8, 3, 6, 9]        // 非零值（按行存储）
columns  = [0, 3, 1, 2, 1, 3]        // 列索引
rowptr   = [0, 2, 3, 4, 6]           // 行起始位置

解读：
• 行0: values[0:2] = [5, 2], columns[0:2] = [0, 3]
• 行1: values[2:3] = [8],    columns[2:3] = [1]
• 行2: values[3:4] = [3],    columns[3:4] = [2]
• 行3: values[4:6] = [6, 9], columns[4:6] = [1, 3]
```

**CSR格式优势**：

```cpp
// 高效的矩阵-向量乘法 y = A * x
void csrMatVec(const CSRMatrix &A, const double *x, double *y) {
    int n = A.nRows;

    #pragma omp parallel for
    for(int i = 0; i < n; i++) {
        double sum = 0.0;
        // 只遍历非零元素
        for(int j = A.rowptr[i]; j < A.rowptr[i+1]; j++) {
            sum += A.values[j] * x[A.columns[j]];
        }
        y[i] = sum;
    }
}

// 复杂度：O(nnz) 而非 O(n²)
```

---

## 下午学习内容

### 4. linearSystemCSR实现分析

**文件位置**：`src/solver/linearSystemCSR.h`

```cpp
template<class scalar>
class linearSystemCSR : public linearSystem<scalar> {
protected:
    // CSR存储结构
    int *_ptr;       // 行指针数组 (大小: nRows + 1)
    int *_jcol;      // 列索引数组 (大小: nnz)
    scalar *_a;      // 值数组 (大小: nnz)
    scalar *_b;      // 右端向量 (大小: nRows)
    scalar *_x;      // 解向量 (大小: nRows)

    // 尺寸信息
    int _nbRows;     // 行数
    int _nnz;        // 非零元素数

    // 稀疏模式（用于预分配）
    sparsityPattern *_sparsity;

    // 快速查找表：(row, col) -> 值数组索引
    std::vector<std::map<int, int>> _ij;

public:
    linearSystemCSR() : _ptr(nullptr), _jcol(nullptr), _a(nullptr),
                        _b(nullptr), _x(nullptr), _nbRows(0), _nnz(0),
                        _sparsity(nullptr) {}

    ~linearSystemCSR() {
        clear();
    }

    // 分配内存
    void allocate(int nbRows) override {
        if(_sparsity == nullptr) {
            Msg::Error("CSR需要先设置稀疏模式");
            return;
        }

        _nbRows = nbRows;
        _b = new scalar[nbRows]();
        _x = new scalar[nbRows]();

        // 从稀疏模式构建CSR结构
        _ptr = new int[nbRows + 1];
        _ptr[0] = 0;

        for(int i = 0; i < nbRows; i++) {
            int rowNnz;
            const int *cols = _sparsity->getRow(i, rowNnz);
            _ptr[i + 1] = _ptr[i] + rowNnz;

            // 构建列索引到值数组的映射
            for(int j = 0; j < rowNnz; j++) {
                _ij[i][cols[j]] = _ptr[i] + j;
            }
        }

        _nnz = _ptr[nbRows];
        _jcol = new int[_nnz];
        _a = new scalar[_nnz]();

        // 填充列索引
        for(int i = 0; i < nbRows; i++) {
            int rowNnz;
            const int *cols = _sparsity->getRow(i, rowNnz);
            for(int j = 0; j < rowNnz; j++) {
                _jcol[_ptr[i] + j] = cols[j];
            }
        }
    }

    // 累加到矩阵
    void addToMatrix(int row, int col, const scalar &val) override {
        auto it = _ij[row].find(col);
        if(it != _ij[row].end()) {
            _a[it->second] += val;
        } else {
            // 错误：该位置不在稀疏模式中
            Msg::Error("CSR: 位置(%d,%d)不在预分配模式中", row, col);
        }
    }

    // 获取矩阵元素
    scalar getFromMatrix(int row, int col) const override {
        auto it = _ij[row].find(col);
        return (it != _ij[row].end()) ? _a[it->second] : scalar(0);
    }

    // 右端向量操作
    void addToRightHandSide(int row, const scalar &val, int ith = 0) override {
        _b[row] += val;
    }

    scalar getFromRightHandSide(int row) const override {
        return _b[row];
    }

    // 解向量操作
    void addToSolution(int row, const scalar &val) override {
        _x[row] += val;
    }

    scalar getFromSolution(int row) const override {
        return _x[row];
    }

    // 归零
    void zeroMatrix() override {
        std::fill(_a, _a + _nnz, scalar(0));
    }

    void zeroRightHandSide() override {
        std::fill(_b, _b + _nbRows, scalar(0));
    }

    void zeroSolution() override {
        std::fill(_x, _x + _nbRows, scalar(0));
    }

    // 清理
    void clear() override {
        delete[] _ptr; _ptr = nullptr;
        delete[] _jcol; _jcol = nullptr;
        delete[] _a; _a = nullptr;
        delete[] _b; _b = nullptr;
        delete[] _x; _x = nullptr;
        _ij.clear();
        _nbRows = _nnz = 0;
    }
};
```

### 5. 稀疏模式预分配

**文件位置**：`src/solver/sparsityPattern.h`

```cpp
class sparsityPattern {
    int _nRows;           // 行数
    int *_nByRow;         // 每行非零元素数
    int **_rowsj;         // 每行的列索引数组

public:
    sparsityPattern(int n) : _nRows(n) {
        _nByRow = new int[n]();
        _rowsj = new int*[n]();
    }

    ~sparsityPattern() {
        for(int i = 0; i < _nRows; i++) {
            delete[] _rowsj[i];
        }
        delete[] _nByRow;
        delete[] _rowsj;
    }

    // 插入稀疏位置
    void insertEntry(int i, int j) {
        // 检查是否已存在
        for(int k = 0; k < _nByRow[i]; k++) {
            if(_rowsj[i][k] == j) return;  // 已存在
        }

        // 扩展存储
        int *newRow = new int[_nByRow[i] + 1];
        for(int k = 0; k < _nByRow[i]; k++) {
            newRow[k] = _rowsj[i][k];
        }
        newRow[_nByRow[i]] = j;

        delete[] _rowsj[i];
        _rowsj[i] = newRow;
        _nByRow[i]++;
    }

    // 获取某行的列索引
    const int* getRow(int i, int &size) const {
        size = _nByRow[i];
        return _rowsj[i];
    }
};
```

**预分配流程**：

```cpp
// 在有限元装配前确定稀疏模式
void precomputeSparsity(dofManager<double> &dm,
                        const std::vector<MElement*> &elements) {
    sparsityPattern sp(dm.sizeOfR());

    for(auto e : elements) {
        // 获取元素自由度
        std::vector<Dof> dofs = getElementDofs(e);

        // 插入所有可能的(i,j)位置
        for(auto &dofI : dofs) {
            for(auto &dofJ : dofs) {
                int i = dm.getRow(dofI);
                int j = dm.getRow(dofJ);
                if(i >= 0 && j >= 0) {  // 都是未知自由度
                    sp.insertEntry(i, j);
                }
            }
        }
    }

    // 将稀疏模式传递给线性系统
    linearSystemCSR<double> *sys = ...;
    sys->setSparsityPattern(&sp);
    sys->allocate(dm.sizeOfR());
}
```

### 6. CSR求解器集成

**Gmm++迭代求解器**：

```cpp
// linearSystemCSRGmm 使用Gmm++库进行迭代求解
template<class scalar>
class linearSystemCSRGmm : public linearSystemCSR<scalar> {
public:
    int systemSolve() override {
        // 创建Gmm++稀疏矩阵视图
        gmm::csr_matrix_ref<scalar*, int*, int*>
            A_ref(this->_a, this->_jcol, this->_ptr, this->_nbRows, this->_nbRows);

        // 创建向量视图
        std::vector<scalar> x(this->_nbRows), b(this->_nbRows);
        std::copy(this->_b, this->_b + this->_nbRows, b.begin());

        // 设置预处理器（ILU）
        gmm::ilu_precond<gmm::csr_matrix<scalar>> P(A_ref);

        // GMRES迭代求解
        gmm::iteration iter(1e-8);  // 容差
        iter.set_maxiter(1000);     // 最大迭代
        gmm::gmres(A_ref, x, b, P, 50, iter);  // 重启参数50

        // 复制解
        std::copy(x.begin(), x.end(), this->_x);

        return iter.converged() ? 1 : 0;
    }
};
```

---

## 练习作业

### 基础练习

**练习1**：CSR格式手工转换

```text
给定矩阵：
┌                    ┐
│  1   0   2   0     │
│  0   3   0   4     │
│  5   0   6   0     │
│  0   7   0   8     │
└                    ┘

写出CSR表示：
values  = ?
columns = ?
rowptr  = ?

验证：通过CSR表示还原矩阵
```

**练习2**：理解接口设计

```cpp
// 回答以下问题：

// 1. addToMatrix 为什么是累加而不是赋值？
// 答：_______

// 2. 为什么需要 zeroMatrix() 而不是在allocate时自动清零？
// 答：_______

// 3. getFromSolution 和 addToSolution 的典型使用场景？
// 答：_______
```

### 进阶练习

**练习3**：实现简单的CSR矩阵类

```cpp
#include <iostream>
#include <vector>
#include <map>
#include <cmath>

class SimpleCSR {
    int _n;                          // 矩阵尺寸
    std::vector<double> _values;     // 非零值
    std::vector<int> _columns;       // 列索引
    std::vector<int> _rowptr;        // 行指针

    // 用于构建阶段的临时存储
    std::vector<std::map<int, double>> _buildMap;
    bool _finalized;

public:
    SimpleCSR(int n) : _n(n), _buildMap(n), _finalized(false) {
        _rowptr.resize(n + 1);
    }

    // 构建阶段：累加元素
    void addEntry(int row, int col, double val) {
        if(_finalized) {
            std::cerr << "CSR已finalize，不能添加新元素\n";
            return;
        }
        _buildMap[row][col] += val;
    }

    // 完成构建，转换为CSR格式
    void finalize() {
        if(_finalized) return;

        _rowptr[0] = 0;
        for(int i = 0; i < _n; i++) {
            _rowptr[i + 1] = _rowptr[i] + _buildMap[i].size();
        }

        int nnz = _rowptr[_n];
        _values.resize(nnz);
        _columns.resize(nnz);

        int idx = 0;
        for(int i = 0; i < _n; i++) {
            for(auto &kv : _buildMap[i]) {
                _columns[idx] = kv.first;
                _values[idx] = kv.second;
                idx++;
            }
        }

        _buildMap.clear();  // 释放临时存储
        _finalized = true;
    }

    // 矩阵-向量乘法
    std::vector<double> multiply(const std::vector<double> &x) const {
        if(!_finalized) {
            std::cerr << "CSR未finalize\n";
            return {};
        }

        std::vector<double> y(_n, 0.0);
        for(int i = 0; i < _n; i++) {
            for(int j = _rowptr[i]; j < _rowptr[i + 1]; j++) {
                y[i] += _values[j] * x[_columns[j]];
            }
        }
        return y;
    }

    // 获取元素
    double get(int row, int col) const {
        for(int j = _rowptr[row]; j < _rowptr[row + 1]; j++) {
            if(_columns[j] == col) return _values[j];
        }
        return 0.0;
    }

    // 打印CSR结构
    void printCSR() const {
        std::cout << "values:  [";
        for(auto v : _values) std::cout << v << " ";
        std::cout << "]\n";

        std::cout << "columns: [";
        for(auto c : _columns) std::cout << c << " ";
        std::cout << "]\n";

        std::cout << "rowptr:  [";
        for(auto p : _rowptr) std::cout << p << " ";
        std::cout << "]\n";
    }

    // 打印矩阵形式
    void printMatrix() const {
        for(int i = 0; i < _n; i++) {
            for(int j = 0; j < _n; j++) {
                std::cout << get(i, j) << "\t";
            }
            std::cout << "\n";
        }
    }

    int nnz() const { return _values.size(); }
    int size() const { return _n; }
};

// Jacobi迭代求解器
bool jacobiSolve(const SimpleCSR &A, const std::vector<double> &b,
                 std::vector<double> &x, int maxIter = 1000,
                 double tol = 1e-8) {
    int n = A.size();
    std::vector<double> x_new(n);

    for(int iter = 0; iter < maxIter; iter++) {
        // Jacobi迭代：x_new[i] = (b[i] - Σ(A[i,j]*x[j], j≠i)) / A[i,i]
        for(int i = 0; i < n; i++) {
            double sigma = 0.0;
            for(int j = 0; j < n; j++) {
                if(j != i) {
                    sigma += A.get(i, j) * x[j];
                }
            }
            double diag = A.get(i, i);
            if(std::abs(diag) < 1e-15) {
                std::cerr << "对角元素接近0\n";
                return false;
            }
            x_new[i] = (b[i] - sigma) / diag;
        }

        // 检查收敛
        double err = 0.0;
        for(int i = 0; i < n; i++) {
            err += (x_new[i] - x[i]) * (x_new[i] - x[i]);
        }
        err = std::sqrt(err);

        x = x_new;

        if(err < tol) {
            std::cout << "Jacobi收敛，迭代" << iter + 1 << "次\n";
            return true;
        }
    }

    std::cout << "Jacobi未收敛\n";
    return false;
}

int main() {
    // 创建测试矩阵（对角占优，保证Jacobi收敛）
    // 4x + y = 5
    // x + 3y + z = 6
    // y + 4z + t = 7
    // z + 2t = 3

    SimpleCSR A(4);
    A.addEntry(0, 0, 4); A.addEntry(0, 1, 1);
    A.addEntry(1, 0, 1); A.addEntry(1, 1, 3); A.addEntry(1, 2, 1);
    A.addEntry(2, 1, 1); A.addEntry(2, 2, 4); A.addEntry(2, 3, 1);
    A.addEntry(3, 2, 1); A.addEntry(3, 3, 2);

    A.finalize();

    std::cout << "=== CSR结构 ===\n";
    A.printCSR();

    std::cout << "\n=== 矩阵形式 ===\n";
    A.printMatrix();

    std::cout << "\n=== 求解 Ax = b ===\n";
    std::vector<double> b = {5, 6, 7, 3};
    std::vector<double> x(4, 0.0);

    if(jacobiSolve(A, b, x)) {
        std::cout << "解: [";
        for(auto v : x) std::cout << v << " ";
        std::cout << "]\n";

        // 验证
        std::cout << "\n验证 A*x = ";
        auto Ax = A.multiply(x);
        for(auto v : Ax) std::cout << v << " ";
        std::cout << "\n期望  b = ";
        for(auto v : b) std::cout << v << " ";
        std::cout << "\n";
    }

    return 0;
}
```

### 挑战练习

**练习4**：实现CG（共轭梯度）求解器

```cpp
#include <iostream>
#include <vector>
#include <cmath>

// 向量运算辅助函数
double dot(const std::vector<double> &a, const std::vector<double> &b) {
    double s = 0;
    for(size_t i = 0; i < a.size(); i++) s += a[i] * b[i];
    return s;
}

std::vector<double> add(const std::vector<double> &a,
                        const std::vector<double> &b, double scale = 1.0) {
    std::vector<double> c(a.size());
    for(size_t i = 0; i < a.size(); i++) c[i] = a[i] + scale * b[i];
    return c;
}

std::vector<double> scale(const std::vector<double> &a, double s) {
    std::vector<double> c(a.size());
    for(size_t i = 0; i < a.size(); i++) c[i] = s * a[i];
    return c;
}

// CG求解器（要求A对称正定）
// 完成以下TODO部分
bool cgSolve(const SimpleCSR &A, const std::vector<double> &b,
             std::vector<double> &x, int maxIter = 1000,
             double tol = 1e-8) {
    int n = A.size();

    // 初始残差 r = b - A*x
    auto Ax = A.multiply(x);
    std::vector<double> r(n);
    for(int i = 0; i < n; i++) r[i] = b[i] - Ax[i];

    // 初始搜索方向 p = r
    std::vector<double> p = r;

    // 初始残差范数平方
    double rsold = dot(r, r);

    for(int iter = 0; iter < maxIter; iter++) {
        // TODO: 实现CG迭代
        // 1. 计算 Ap = A * p
        // 2. 计算步长 alpha = rsold / (p' * Ap)
        // 3. 更新解 x = x + alpha * p
        // 4. 更新残差 r = r - alpha * Ap
        // 5. 检查收敛 ||r|| < tol
        // 6. 计算新的残差范数平方 rsnew = r' * r
        // 7. 更新搜索方向 p = r + (rsnew/rsold) * p
        // 8. rsold = rsnew

        // 你的代码在这里
    }

    return false;
}
```

**练习5**：分析Gmsh源码

```cpp
// 阅读 src/solver/linearSystemCSR.cpp 并回答：

// 1. 该实现如何处理稀疏模式未预分配的情况？

// 2. systemSolve() 方法使用什么求解器？

// 3. 对比linearSystemCSRGmm，两者的主要差异是什么？
```

---

## 知识图谱

```text
线性系统求解
│
├── 抽象接口
│   ├── linearSystemBase（非模板）
│   ├── linearSystem<T>（模板化）
│   └── 矩阵/向量操作分离
│
├── 稀疏存储格式
│   ├── CSR（行压缩）
│   ├── CSC（列压缩）
│   ├── COO（坐标）
│   └── 格式选择依据
│
├── CSR实现要点
│   ├── 三数组结构（values, columns, rowptr）
│   ├── 稀疏模式预分配
│   ├── (row,col)→index映射
│   └── 矩阵-向量乘法
│
├── 迭代求解器
│   ├── Jacobi
│   ├── Gauss-Seidel
│   ├── CG（共轭梯度）
│   └── GMRES
│
└── 预处理技术
    ├── ILU分解
    ├── 对角预处理
    └── 多重网格
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 | 优先级 |
|------|----------|--------|--------|
| linearSystem.h | 抽象接口定义 | ~50行 | ★★★★★ |
| linearSystemCSR.h | CSR格式实现 | ~470行 | ★★★★★ |
| sparsityPattern.h | 稀疏模式管理 | ~100行 | ★★★★☆ |
| linearSystemFull.h | 稠密矩阵（对照） | ~150行 | ★★★☆☆ |
| linearSystemGmm.h | Gmm++接口 | ~200行 | ★★★☆☆ |

---

## 今日检查点

- [ ] 理解线性系统抽象接口的设计
- [ ] 掌握CSR稀疏矩阵格式
- [ ] 能手工转换矩阵到CSR格式
- [ ] 理解稀疏模式预分配的必要性
- [ ] 完成CSR矩阵类练习
- [ ] 了解迭代求解器的基本原理

---

## 导航

- 上一天：[Day 64 - 求解器模块概述](day-64.md)
- 下一天：[Day 66 - 特征值问题](day-66.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
