# Day 55：矩阵运算与数值工具

## 学习目标
掌握Gmsh中fullMatrix矩阵类的使用和Numeric通用数值函数库。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习fullMatrix矩阵类 |
| 10:00-11:00 | 1h | 阅读矩阵运算实现 |
| 11:00-12:00 | 1h | 学习Numeric数值工具 |
| 14:00-15:00 | 1h | 完成练习并总结 |

---

## 上午学习内容

### 1. fullMatrix类概览

**文件位置**：`src/numeric/fullMatrix.h`

```cpp
template <class scalar>
class fullMatrix {
private:
    int _r, _c;          // 行数和列数
    scalar *_data;       // 数据存储（列优先）
    bool _own;           // 是否拥有数据

public:
    // 构造函数
    fullMatrix(int r, int c);
    fullMatrix(int r, int c, scalar *data);  // 使用外部数据
    fullMatrix(const fullMatrix<scalar> &other);  // 拷贝

    // 基本操作
    int size1() const { return _r; }  // 行数
    int size2() const { return _c; }  // 列数
    scalar *getDataPtr() { return _data; }

    // 元素访问
    scalar &operator()(int i, int j);
    scalar operator()(int i, int j) const;

    // 矩阵运算
    void mult(const fullMatrix<scalar> &b, fullMatrix<scalar> &c) const;
    void mult(const fullVector<scalar> &x, fullVector<scalar> &y) const;
    void gemm(const fullMatrix<scalar> &a, const fullMatrix<scalar> &b,
              scalar alpha = 1., scalar beta = 1.);

    // 特殊操作
    bool invert(fullMatrix<scalar> &result) const;
    bool eig(fullVector<double> &eigenvalues,
             fullMatrix<scalar> &eigenvectors) const;
    bool svd(fullMatrix<scalar> &V, fullVector<scalar> &S);
    bool lu(fullVector<int> &perm);
    void luSolve(const fullVector<scalar> &b, fullVector<scalar> &x) const;

    // 范数和性质
    scalar norm() const;           // Frobenius范数
    scalar determinant() const;    // 行列式
    void transpose();              // 转置
};
```

### 2. fullVector类

```cpp
template <class scalar>
class fullVector {
private:
    int _r;              // 向量长度
    scalar *_data;       // 数据存储
    bool _own;           // 是否拥有数据

public:
    // 构造函数
    fullVector(int r = 0);
    fullVector(int r, scalar *data);

    // 基本操作
    int size() const { return _r; }
    void resize(int r);

    // 元素访问
    scalar &operator()(int i);
    scalar operator()(int i) const;

    // 向量运算
    void scale(scalar s);                        // v *= s
    void axpy(scalar a, const fullVector &x);    // v += a*x
    scalar operator*(const fullVector &x) const; // 点积

    // 范数
    scalar norm() const;    // L2范数
    scalar normInfty() const; // 无穷范数

    // 极值
    scalar min() const;
    scalar max() const;
};
```

### 3. 矩阵运算实现

**BLAS集成**：

```cpp
// fullMatrix使用BLAS库进行高效计算

// 矩阵-矩阵乘法：C = A * B
template <class scalar>
void fullMatrix<scalar>::mult(const fullMatrix<scalar> &b,
                               fullMatrix<scalar> &c) const
{
    // 使用BLAS的dgemm函数
    // c = alpha * a * b + beta * c
    double alpha = 1.0, beta = 0.0;
    int m = _r;      // A的行数
    int n = b._c;    // B的列数
    int k = _c;      // A的列数 = B的行数

#if defined(HAVE_BLAS)
    // 调用BLAS
    dgemm_("N", "N", &m, &n, &k,
           &alpha, _data, &m,
           b._data, &k,
           &beta, c._data, &m);
#else
    // 朴素实现
    for(int i = 0; i < m; i++) {
        for(int j = 0; j < n; j++) {
            c(i, j) = 0;
            for(int l = 0; l < k; l++) {
                c(i, j) += (*this)(i, l) * b(l, j);
            }
        }
    }
#endif
}
```

**矩阵求逆**：

```cpp
template <class scalar>
bool fullMatrix<scalar>::invert(fullMatrix<scalar> &result) const
{
    if(_r != _c) return false;  // 必须是方阵

    result = *this;  // 拷贝
    int n = _r;
    int info;

#if defined(HAVE_LAPACK)
    // 使用LAPACK的LU分解求逆
    fullVector<int> ipiv(n);

    // LU分解
    dgetrf_(&n, &n, result._data, &n, ipiv.getDataPtr(), &info);
    if(info != 0) return false;

    // 计算逆矩阵
    int lwork = n * n;
    fullVector<scalar> work(lwork);
    dgetri_(&n, result._data, &n, ipiv.getDataPtr(),
            work.getDataPtr(), &lwork, &info);

    return info == 0;
#else
    // 使用高斯消元法
    // ... 朴素实现
#endif
}
```

### 4. 特征值和SVD

```cpp
// 特征值分解
template <class scalar>
bool fullMatrix<scalar>::eig(fullVector<double> &eigenvalues,
                              fullMatrix<scalar> &eigenvectors) const
{
    if(_r != _c) return false;

#if defined(HAVE_LAPACK)
    // 对称矩阵使用dsyev
    // 一般矩阵使用dgeev

    char jobz = 'V';  // 计算特征向量
    char uplo = 'U';  // 上三角存储
    int n = _r;
    int info;
    int lwork = 3*n;
    fullVector<scalar> work(lwork);

    eigenvectors = *this;

    dsyev_(&jobz, &uplo, &n, eigenvectors._data, &n,
           eigenvalues.getDataPtr(), work.getDataPtr(), &lwork, &info);

    return info == 0;
#else
    return false;
#endif
}

// 奇异值分解：A = U * S * V^T
template <class scalar>
bool fullMatrix<scalar>::svd(fullMatrix<scalar> &V,
                              fullVector<scalar> &S)
{
#if defined(HAVE_LAPACK)
    char jobu = 'O';   // U覆盖到A
    char jobvt = 'S';  // 计算V^T
    int m = _r, n = _c;
    int minmn = std::min(m, n);

    S.resize(minmn);
    V.resize(n, minmn);

    int lwork = 5 * std::max(m, n);
    fullVector<scalar> work(lwork);
    int info;

    dgesvd_(&jobu, &jobvt, &m, &n, _data, &m,
            S.getDataPtr(), nullptr, &m,
            V._data, &n,
            work.getDataPtr(), &lwork, &info);

    return info == 0;
#else
    return false;
#endif
}
```

---

## 下午学习内容

### 5. Numeric通用数值函数

**文件位置**：`src/numeric/Numeric.h/cpp`

```cpp
// 几何运算
double triangle_area(double p0[3], double p1[3], double p2[3]);
double tetrahedron_volume(double p0[3], double p1[3],
                          double p2[3], double p3[3]);

// 向量运算
void crossprod(double a[3], double b[3], double c[3]);  // c = a × b
double scalarprod(double a[3], double b[3]);            // a · b
void normalize(double a[3]);                            // a = a / |a|
double norm3(double a[3]);                              // |a|

// 平面和直线
double signedDistancePointPlane(double point[3],
                                 double planePoint[3],
                                 double planeNormal[3]);
void projectPointOnPlane(double point[3],
                         double planePoint[3],
                         double planeNormal[3],
                         double proj[3]);

// 插值
double interpolate(double v0, double v1, double v2, double v3,
                   double u, double v, double w);

// 多项式
double horner(double *p, int n, double x);  // Horner求值
void polynomialDerivative(double *p, int n, double *dp);
void polynomialRoots(double *p, int n, double *roots, int &nroots);

// 数值微分
double numericalDerivative(double (*f)(double), double x, double h = 1e-6);
double numericalIntegral(double (*f)(double), double a, double b, int n = 100);
```

### 6. 几何计算函数

```cpp
// 三角形面积（3D）
double triangle_area(double p0[3], double p1[3], double p2[3])
{
    double a[3], b[3], c[3];

    // 边向量
    a[0] = p1[0] - p0[0]; a[1] = p1[1] - p0[1]; a[2] = p1[2] - p0[2];
    b[0] = p2[0] - p0[0]; b[1] = p2[1] - p0[1]; b[2] = p2[2] - p0[2];

    // 叉积
    crossprod(a, b, c);

    // 面积 = |叉积| / 2
    return 0.5 * norm3(c);
}

// 四面体体积
double tetrahedron_volume(double p0[3], double p1[3],
                          double p2[3], double p3[3])
{
    double mat[3][3];

    // 构造边向量矩阵
    for(int i = 0; i < 3; i++) {
        mat[0][i] = p1[i] - p0[i];
        mat[1][i] = p2[i] - p0[i];
        mat[2][i] = p3[i] - p0[i];
    }

    // 体积 = |行列式| / 6
    double det = mat[0][0]*(mat[1][1]*mat[2][2] - mat[1][2]*mat[2][1])
               - mat[0][1]*(mat[1][0]*mat[2][2] - mat[1][2]*mat[2][0])
               + mat[0][2]*(mat[1][0]*mat[2][1] - mat[1][1]*mat[2][0]);

    return std::abs(det) / 6.0;
}

// 点到平面的有符号距离
double signedDistancePointPlane(double point[3],
                                 double planePoint[3],
                                 double planeNormal[3])
{
    double v[3];
    v[0] = point[0] - planePoint[0];
    v[1] = point[1] - planePoint[1];
    v[2] = point[2] - planePoint[2];

    return scalarprod(v, planeNormal);
}
```

### 7. 正交多项式

**文件位置**：`src/numeric/OrthogonalPoly.h/cpp`

```cpp
// Legendre多项式
double LegendreP(int n, double x);
double LegendreP_derivative(int n, double x);

// Jacobi多项式
double JacobiP(int n, double alpha, double beta, double x);

// Chebyshev多项式
double ChebyshevT(int n, double x);
double ChebyshevU(int n, double x);

// 实现（递推关系）
double LegendreP(int n, double x)
{
    if(n == 0) return 1.0;
    if(n == 1) return x;

    double Pn_2 = 1.0;  // P_{n-2}
    double Pn_1 = x;    // P_{n-1}
    double Pn;

    // 递推：(n+1)P_{n+1} = (2n+1)xP_n - nP_{n-1}
    for(int k = 1; k < n; k++) {
        Pn = ((2*k + 1)*x*Pn_1 - k*Pn_2) / (k + 1);
        Pn_2 = Pn_1;
        Pn_1 = Pn;
    }

    return Pn_1;
}

double JacobiP(int n, double alpha, double beta, double x)
{
    if(n == 0) return 1.0;
    if(n == 1) return 0.5*(alpha - beta + (alpha + beta + 2)*x);

    // 三项递推关系
    double Pn_2 = 1.0;
    double Pn_1 = 0.5*(alpha - beta + (alpha + beta + 2)*x);
    double Pn;

    for(int k = 1; k < n; k++) {
        double a1 = 2*(k+1)*(k+alpha+beta+1)*(2*k+alpha+beta);
        double a2 = (2*k+alpha+beta+1)*(alpha*alpha - beta*beta);
        double a3 = (2*k+alpha+beta)*(2*k+alpha+beta+1)*(2*k+alpha+beta+2);
        double a4 = 2*(k+alpha)*(k+beta)*(2*k+alpha+beta+2);

        Pn = ((a2 + a3*x)*Pn_1 - a4*Pn_2) / a1;
        Pn_2 = Pn_1;
        Pn_1 = Pn;
    }

    return Pn_1;
}
```

---

## 练习作业

### 基础练习

**练习1**：使用fullMatrix进行基本运算

```cpp
#include <iostream>
#include <cmath>

// 简化的fullMatrix实现（用于学习）
template<typename T>
class SimpleMatrix {
    int rows, cols;
    T *data;

public:
    SimpleMatrix(int r, int c) : rows(r), cols(c) {
        data = new T[r * c]();
    }

    ~SimpleMatrix() { delete[] data; }

    T &operator()(int i, int j) {
        return data[j * rows + i];  // 列优先存储
    }

    T operator()(int i, int j) const {
        return data[j * rows + i];
    }

    int size1() const { return rows; }
    int size2() const { return cols; }

    // 矩阵乘法
    void mult(const SimpleMatrix &B, SimpleMatrix &C) const {
        for(int i = 0; i < rows; i++) {
            for(int j = 0; j < B.cols; j++) {
                C(i, j) = 0;
                for(int k = 0; k < cols; k++) {
                    C(i, j) += (*this)(i, k) * B(k, j);
                }
            }
        }
    }

    // Frobenius范数
    double norm() const {
        double sum = 0;
        for(int i = 0; i < rows * cols; i++) {
            sum += data[i] * data[i];
        }
        return std::sqrt(sum);
    }

    // 打印
    void print() const {
        for(int i = 0; i < rows; i++) {
            for(int j = 0; j < cols; j++) {
                std::cout << (*this)(i, j) << "\t";
            }
            std::cout << "\n";
        }
    }
};

int main() {
    // 创建2x3矩阵A
    SimpleMatrix<double> A(2, 3);
    A(0,0) = 1; A(0,1) = 2; A(0,2) = 3;
    A(1,0) = 4; A(1,1) = 5; A(1,2) = 6;

    std::cout << "Matrix A:\n";
    A.print();

    // 创建3x2矩阵B
    SimpleMatrix<double> B(3, 2);
    B(0,0) = 7; B(0,1) = 8;
    B(1,0) = 9; B(1,1) = 10;
    B(2,0) = 11; B(2,1) = 12;

    std::cout << "\nMatrix B:\n";
    B.print();

    // C = A * B（2x2）
    SimpleMatrix<double> C(2, 2);
    A.mult(B, C);

    std::cout << "\nMatrix C = A * B:\n";
    C.print();

    // 验证：
    // C(0,0) = 1*7 + 2*9 + 3*11 = 58
    // C(0,1) = 1*8 + 2*10 + 3*12 = 64
    // C(1,0) = 4*7 + 5*9 + 6*11 = 139
    // C(1,1) = 4*8 + 5*10 + 6*12 = 154

    std::cout << "\nNorm of A: " << A.norm() << "\n";

    return 0;
}
```

### 进阶练习

**练习2**：实现LU分解和求解

```cpp
#include <iostream>
#include <vector>
#include <cmath>

// LU分解类（不带置换）
class SimpleLU {
    int n;
    std::vector<std::vector<double>> L, U;

public:
    SimpleLU(int size) : n(size), L(size, std::vector<double>(size, 0)),
                                   U(size, std::vector<double>(size, 0)) {}

    // Doolittle LU分解
    bool decompose(const std::vector<std::vector<double>> &A) {
        // U的第一行 = A的第一行
        for(int j = 0; j < n; j++) {
            U[0][j] = A[0][j];
        }

        // L的第一列
        for(int i = 0; i < n; i++) {
            L[i][i] = 1.0;  // 对角线为1
        }
        for(int i = 1; i < n; i++) {
            if(std::abs(U[0][0]) < 1e-15) return false;
            L[i][0] = A[i][0] / U[0][0];
        }

        // 其余元素
        for(int i = 1; i < n; i++) {
            // U的第i行
            for(int j = i; j < n; j++) {
                double sum = 0;
                for(int k = 0; k < i; k++) {
                    sum += L[i][k] * U[k][j];
                }
                U[i][j] = A[i][j] - sum;
            }

            // L的第i列
            for(int j = i + 1; j < n; j++) {
                double sum = 0;
                for(int k = 0; k < i; k++) {
                    sum += L[j][k] * U[k][i];
                }
                if(std::abs(U[i][i]) < 1e-15) return false;
                L[j][i] = (A[j][i] - sum) / U[i][i];
            }
        }

        return true;
    }

    // 求解 Ax = b（先解Ly=b，再解Ux=y）
    std::vector<double> solve(const std::vector<double> &b) {
        std::vector<double> y(n), x(n);

        // 前代：Ly = b
        for(int i = 0; i < n; i++) {
            double sum = 0;
            for(int j = 0; j < i; j++) {
                sum += L[i][j] * y[j];
            }
            y[i] = b[i] - sum;
        }

        // 回代：Ux = y
        for(int i = n - 1; i >= 0; i--) {
            double sum = 0;
            for(int j = i + 1; j < n; j++) {
                sum += U[i][j] * x[j];
            }
            x[i] = (y[i] - sum) / U[i][i];
        }

        return x;
    }

    void printLU() const {
        std::cout << "L:\n";
        for(int i = 0; i < n; i++) {
            for(int j = 0; j < n; j++) {
                std::cout << L[i][j] << "\t";
            }
            std::cout << "\n";
        }

        std::cout << "\nU:\n";
        for(int i = 0; i < n; i++) {
            for(int j = 0; j < n; j++) {
                std::cout << U[i][j] << "\t";
            }
            std::cout << "\n";
        }
    }
};

int main() {
    // 测试矩阵
    std::vector<std::vector<double>> A = {
        {2, 1, 1},
        {4, 3, 3},
        {8, 7, 9}
    };

    std::vector<double> b = {4, 10, 24};

    SimpleLU lu(3);
    if(lu.decompose(A)) {
        std::cout << "LU decomposition:\n";
        lu.printLU();

        std::vector<double> x = lu.solve(b);
        std::cout << "\nSolution x = [";
        for(int i = 0; i < 3; i++) {
            std::cout << x[i];
            if(i < 2) std::cout << ", ";
        }
        std::cout << "]\n";

        // 验证
        std::cout << "\nVerification Ax = [";
        for(int i = 0; i < 3; i++) {
            double sum = 0;
            for(int j = 0; j < 3; j++) {
                sum += A[i][j] * x[j];
            }
            std::cout << sum;
            if(i < 2) std::cout << ", ";
        }
        std::cout << "] (should be [4, 10, 24])\n";
    }

    return 0;
}
```

**练习3**：实现几何计算函数

```cpp
#include <iostream>
#include <cmath>
#include <array>

// 向量类型
using Vec3 = std::array<double, 3>;

// 向量运算
Vec3 cross(const Vec3 &a, const Vec3 &b) {
    return {a[1]*b[2] - a[2]*b[1],
            a[2]*b[0] - a[0]*b[2],
            a[0]*b[1] - a[1]*b[0]};
}

double dot(const Vec3 &a, const Vec3 &b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

double norm(const Vec3 &a) {
    return std::sqrt(dot(a, a));
}

Vec3 sub(const Vec3 &a, const Vec3 &b) {
    return {a[0]-b[0], a[1]-b[1], a[2]-b[2]};
}

// 三角形面积
double triangleArea(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2) {
    Vec3 a = sub(p1, p0);
    Vec3 b = sub(p2, p0);
    Vec3 c = cross(a, b);
    return 0.5 * norm(c);
}

// 四面体体积
double tetrahedronVolume(const Vec3 &p0, const Vec3 &p1,
                          const Vec3 &p2, const Vec3 &p3) {
    Vec3 a = sub(p1, p0);
    Vec3 b = sub(p2, p0);
    Vec3 c = sub(p3, p0);

    // V = |a·(b×c)| / 6
    Vec3 bc = cross(b, c);
    return std::abs(dot(a, bc)) / 6.0;
}

// 点到平面的距离
double pointPlaneDistance(const Vec3 &point,
                          const Vec3 &planePoint,
                          const Vec3 &planeNormal) {
    Vec3 v = sub(point, planePoint);
    double n = norm(planeNormal);
    return std::abs(dot(v, planeNormal)) / n;
}

// 三角形的法向量
Vec3 triangleNormal(const Vec3 &p0, const Vec3 &p1, const Vec3 &p2) {
    Vec3 a = sub(p1, p0);
    Vec3 b = sub(p2, p0);
    Vec3 n = cross(a, b);
    double len = norm(n);
    if(len > 1e-15) {
        return {n[0]/len, n[1]/len, n[2]/len};
    }
    return {0, 0, 0};
}

int main() {
    // 测试三角形面积
    Vec3 t0 = {0, 0, 0};
    Vec3 t1 = {1, 0, 0};
    Vec3 t2 = {0, 1, 0};
    std::cout << "Right triangle area: " << triangleArea(t0, t1, t2)
              << " (expected: 0.5)\n";

    // 等边三角形
    Vec3 e0 = {0, 0, 0};
    Vec3 e1 = {1, 0, 0};
    Vec3 e2 = {0.5, std::sqrt(3)/2, 0};
    std::cout << "Equilateral triangle area: " << triangleArea(e0, e1, e2)
              << " (expected: " << std::sqrt(3)/4 << ")\n";

    // 测试四面体体积
    Vec3 v0 = {0, 0, 0};
    Vec3 v1 = {1, 0, 0};
    Vec3 v2 = {0, 1, 0};
    Vec3 v3 = {0, 0, 1};
    std::cout << "Unit tetrahedron volume: " << tetrahedronVolume(v0, v1, v2, v3)
              << " (expected: " << 1.0/6.0 << ")\n";

    // 测试点到平面距离
    Vec3 point = {1, 1, 1};
    Vec3 planePoint = {0, 0, 0};
    Vec3 planeNormal = {0, 0, 1};  // xy平面
    std::cout << "Point (1,1,1) to xy-plane: "
              << pointPlaneDistance(point, planePoint, planeNormal)
              << " (expected: 1)\n";

    return 0;
}
```

### 挑战练习

**练习4**：实现正交多项式计算

```cpp
#include <iostream>
#include <cmath>
#include <vector>

// Legendre多项式（递推）
double legendre(int n, double x) {
    if(n == 0) return 1.0;
    if(n == 1) return x;

    double P0 = 1.0;
    double P1 = x;
    double Pn;

    for(int k = 1; k < n; k++) {
        Pn = ((2*k + 1)*x*P1 - k*P0) / (k + 1);
        P0 = P1;
        P1 = Pn;
    }

    return P1;
}

// Legendre多项式导数
double legendreDerivative(int n, double x) {
    if(n == 0) return 0.0;
    if(n == 1) return 1.0;

    // P'_n(x) = n/(x^2-1) * (x*P_n(x) - P_{n-1}(x))
    // 或使用递推关系
    double Pn = legendre(n, x);
    double Pn_1 = legendre(n-1, x);

    if(std::abs(x*x - 1) < 1e-15) {
        // 边界处使用极限
        return n * (n + 1) / 2.0 * (x > 0 ? 1 : -1);
    }

    return n / (x*x - 1) * (x*Pn - Pn_1);
}

// 找Legendre多项式的根（用于高斯积分点）
std::vector<double> legendreRoots(int n, double tol = 1e-14) {
    std::vector<double> roots(n);

    for(int i = 0; i < n; i++) {
        // 初始猜测（Chebyshev节点的近似）
        double x = std::cos(M_PI * (4*i + 3) / (4*n + 2));

        // Newton迭代
        for(int iter = 0; iter < 100; iter++) {
            double Pn = legendre(n, x);
            double dPn = legendreDerivative(n, x);

            double dx = -Pn / dPn;
            x += dx;

            if(std::abs(dx) < tol) break;
        }

        roots[i] = x;
    }

    return roots;
}

// 计算Gauss-Legendre权重
std::vector<double> gaussWeights(int n,
                                  const std::vector<double> &roots) {
    std::vector<double> weights(n);

    for(int i = 0; i < n; i++) {
        double x = roots[i];
        double dPn = legendreDerivative(n, x);
        weights[i] = 2.0 / ((1 - x*x) * dPn * dPn);
    }

    return weights;
}

int main() {
    // 打印Legendre多项式值
    std::cout << "Legendre polynomials at x=0.5:\n";
    for(int n = 0; n <= 5; n++) {
        std::cout << "P_" << n << "(0.5) = " << legendre(n, 0.5) << "\n";
    }

    // 计算5点Gauss-Legendre积分点和权重
    std::cout << "\n5-point Gauss-Legendre quadrature:\n";
    std::vector<double> roots = legendreRoots(5);
    std::vector<double> weights = gaussWeights(5, roots);

    double sumW = 0;
    for(int i = 0; i < 5; i++) {
        std::cout << "x_" << i << " = " << roots[i]
                  << ", w_" << i << " = " << weights[i] << "\n";
        sumW += weights[i];
    }
    std::cout << "Sum of weights: " << sumW << " (should be 2.0)\n";

    // 使用高斯积分计算 ∫_{-1}^{1} x^4 dx = 2/5
    double integral = 0;
    for(int i = 0; i < 5; i++) {
        double x = roots[i];
        integral += weights[i] * x * x * x * x;
    }
    std::cout << "\n∫x^4 dx from -1 to 1 = " << integral
              << " (exact: " << 0.4 << ")\n";

    return 0;
}
```

---

## 知识图谱

```
矩阵运算与数值工具
│
├── fullMatrix/fullVector
│   ├── 基本运算（+, -, *, /）
│   ├── 矩阵乘法（BLAS加速）
│   ├── LU分解与求解
│   ├── 特征值分解
│   └── SVD分解
│
├── Numeric通用函数
│   ├── 几何计算
│   │   ├── 三角形面积
│   │   ├── 四面体体积
│   │   └── 点面距离
│   ├── 向量运算
│   │   ├── 叉积、点积
│   │   └── 归一化
│   └── 插值计算
│
└── OrthogonalPoly
    ├── Legendre多项式
    ├── Jacobi多项式
    └── Chebyshev多项式
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 |
|------|----------|--------|
| fullMatrix.h | 矩阵类定义 | ~700行 |
| fullMatrix.cpp | 矩阵运算实现 | ~500行 |
| Numeric.h | 数值函数声明 | ~250行 |
| Numeric.cpp | 数值函数实现 | ~1500行 |
| OrthogonalPoly.h | 正交多项式 | ~50行 |
| OrthogonalPoly.cpp | 正交多项式实现 | ~400行 |

---

## 今日检查点

- [ ] 理解fullMatrix的列优先存储格式
- [ ] 能使用fullMatrix进行基本矩阵运算
- [ ] 理解BLAS/LAPACK在矩阵计算中的作用
- [ ] 掌握常用几何计算函数
- [ ] 理解正交多项式的递推计算

---

## 导航

- 上一天：[Day 54 - Bezier基与几何表示](day-54.md)
- 下一天：[Day 56 - 第八周复习](day-56.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
