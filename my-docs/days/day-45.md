# Day 45：几何谓词与数值稳定性

## 学习目标
理解几何谓词的实现原理，掌握浮点精度问题的处理方法，学习Gmsh中的鲁棒性保证机制。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习orient2d/orient3d谓词原理 |
| 10:00-11:00 | 1h | 学习incircle/insphere谓词实现 |
| 11:00-12:00 | 1h | 理解浮点误差对几何计算的影响 |
| 14:00-15:00 | 1h | 研究Gmsh的自适应精度算术和退化处理 |

---

## 上午学习内容

### 1. 基本几何谓词

**四个核心谓词**：

| 谓词 | 维度 | 功能 |
|------|------|------|
| orient2d | 2D | 判断点相对于线的位置 |
| orient3d | 3D | 判断点相对于平面的位置 |
| incircle | 2D | 判断点是否在三角形外接圆内 |
| insphere | 3D | 判断点是否在四面体外接球内 |

### 2. orient2d谓词

**数学定义**：
```
判断点c在有向线段ab的左侧、右侧还是在线上。

orient2d(a, b, c) = det | ax-cx  ay-cy |
                        | bx-cx  by-cy |

返回值：
  > 0: c在ab的左侧（逆时针）
  = 0: c在ab上（共线）
  < 0: c在ab的右侧（顺时针）
```

**实现代码**：
```cpp
// 朴素实现
double orient2d_naive(const double *pa, const double *pb,
                      const double *pc) {
    double acx = pa[0] - pc[0];
    double acy = pa[1] - pc[1];
    double bcx = pb[0] - pc[0];
    double bcy = pb[1] - pc[1];

    return acx * bcy - acy * bcx;
}

// 问题：当结果接近0时，浮点误差可能导致错误判断
```

### 3. orient3d谓词

**数学定义**：
```
判断点d在有向平面abc的上方、下方还是在平面上。

orient3d(a, b, c, d) = det | ax-dx  ay-dy  az-dz |
                           | bx-dx  by-dy  bz-dz |
                           | cx-dx  cy-dy  cz-dz |

返回值：
  > 0: d在abc平面上方（按右手法则）
  = 0: d在abc平面上（共面）
  < 0: d在abc平面下方
```

**实现代码**：
```cpp
double orient3d_naive(const double *pa, const double *pb,
                      const double *pc, const double *pd) {
    double adx = pa[0] - pd[0];
    double ady = pa[1] - pd[1];
    double adz = pa[2] - pd[2];
    double bdx = pb[0] - pd[0];
    double bdy = pb[1] - pd[1];
    double bdz = pb[2] - pd[2];
    double cdx = pc[0] - pd[0];
    double cdy = pc[1] - pd[1];
    double cdz = pc[2] - pd[2];

    return adx * (bdy * cdz - bdz * cdy)
         + ady * (bdz * cdx - bdx * cdz)
         + adz * (bdx * cdy - bdy * cdx);
}
```

### 4. incircle谓词

**数学定义**：
```
判断点d是否在三角形abc的外接圆内。

incircle(a, b, c, d) = det | ax-dx  ay-dy  (ax-dx)²+(ay-dy)² |
                           | bx-dx  by-dy  (bx-dx)²+(by-dy)² |
                           | cx-dx  cy-dy  (cx-dx)²+(cy-dy)² |

前提：a, b, c按逆时针排列

返回值：
  > 0: d在外接圆内
  = 0: d在外接圆上
  < 0: d在外接圆外
```

### 5. insphere谓词

**数学定义**：
```
判断点e是否在四面体abcd的外接球内。

insphere(a, b, c, d, e) = 5x5行列式

| ax-ex  ay-ey  az-ez  (ax-ex)²+(ay-ey)²+(az-ez)² |
| bx-ex  by-ey  bz-ez  (bx-ex)²+(by-ey)²+(bz-ez)² |
| cx-ex  cy-ey  cz-ez  (cx-ex)²+(cy-ey)²+(cz-ez)² |
| dx-ex  dy-ey  dz-ez  (dx-ex)²+(dy-ey)²+(dz-ez)² |

前提：a, b, c, d构成正向四面体

返回值：
  > 0: e在外接球内
  = 0: e在外接球上（五点共球）
  < 0: e在外接球外
```

---

## 下午学习内容

### 6. 浮点误差问题

**误差来源**：
```cpp
// 浮点表示的局限性
double a = 0.1;
double b = 0.2;
double c = 0.3;

// a + b != c（由于浮点表示误差）
std::cout << (a + b == c) << std::endl;  // 输出0（false）

// 在几何计算中的影响
// 当行列式结果接近0时：
//   - 真实值可能是正数，但计算结果为负数
//   - 导致错误的几何判断
//   - 产生不一致的拓扑结构
```

**问题示例**：
```
考虑三个几乎共线的点：
A(0, 0), B(1, 0), C(0.5, 1e-16)

朴素orient2d结果可能因计算顺序不同而正负颠倒：
- orient2d(A, B, C) 可能返回正值
- orient2d(B, C, A) 可能返回负值
这违反了几何谓词的一致性要求！
```

### 7. 自适应精度算术

**Shewchuk的方法**：
分层计算，只在必要时使用高精度。

```cpp
// 自适应精度orient2d
double orient2dAdapt(const double *pa, const double *pb,
                     const double *pc) {
    // 第一层：快速近似
    double detleft = (pa[0] - pc[0]) * (pb[1] - pc[1]);
    double detright = (pa[1] - pc[1]) * (pb[0] - pc[0]);
    double det = detleft - detright;

    // 计算误差边界
    double detsum = std::abs(detleft) + std::abs(detright);
    double errbound = ccwerrboundA * detsum;

    if (std::abs(det) > errbound) {
        return det;  // 快速结果可靠
    }

    // 第二层：更精确的计算
    return orient2dExact(pa, pb, pc);
}
```

**误差边界常量**：
```cpp
// 预计算的误差边界
static double splitter;       // 2^ceil(p/2) + 1, p是尾数位数
static double epsilon;        // 机器epsilon
static double resulterrbound;
static double ccwerrboundA, ccwerrboundB, ccwerrboundC;
static double o3derrboundA, o3derrboundB, o3derrboundC;
static double iccerrboundA, iccerrboundB, iccerrboundC;
static double isperrboundA, isperrboundB, isperrboundC;

void exactinit() {
    double half = 0.5;
    double check = 1.0, lastcheck;

    // 计算epsilon
    do {
        lastcheck = check;
        epsilon = lastcheck;
        check *= half;
        check += 1.0;
    } while ((check != 1.0) && (check != lastcheck));

    // 计算splitter
    splitter = 1.0;
    double every_other = 1.0;
    while (every_other != epsilon) {
        splitter *= 2.0;
        every_other *= half;
    }
    splitter += 1.0;

    // 计算误差边界
    ccwerrboundA = (3.0 + 16.0 * epsilon) * epsilon;
    // ... 其他边界
}
```

### 8. 精确算术

**展开乘法（Expansion Arithmetic）**：
```cpp
// 将浮点数乘积精确表示为多个分量的和
// Two-Product算法
void twoProduct(double a, double b,
                double &x, double &y) {
    x = a * b;

    // Split a into high and low parts
    double c = splitter * a;
    double abig = c - a;
    double ahi = c - abig;
    double alo = a - ahi;

    // Split b into high and low parts
    double d = splitter * b;
    double bbig = d - b;
    double bhi = d - bbig;
    double blo = b - bhi;

    // 计算误差项
    double err1 = x - (ahi * bhi);
    double err2 = err1 - (alo * bhi);
    double err3 = err2 - (ahi * blo);
    y = (alo * blo) - err3;
}
```

### 9. Gmsh中的实现

**文件位置**：`src/numeric/robustPredicates.cpp`

```cpp
namespace robustPredicates {

// 初始化（程序启动时调用一次）
void exactinit();

// 2D方向判断
double orient2d(double *pa, double *pb, double *pc);

// 3D方向判断
double orient3d(double *pa, double *pb, double *pc, double *pd);

// 2D外接圆测试
double incircle(double *pa, double *pb, double *pc, double *pd);

// 3D外接球测试
double insphere(double *pa, double *pb, double *pc, double *pd, double *pe);

}  // namespace robustPredicates
```

### 10. 退化情况处理

**共面/共球点**：
```cpp
// 当返回值为0时，表示退化情况
// 处理策略：

// 1. 符号扰动（Simulation of Simplicity）
// 假设输入点有微小扰动，消除退化
double orient3d_sos(const double *pa, const double *pb,
                    const double *pc, const double *pd) {
    double result = orient3d(pa, pb, pc, pd);

    if (result == 0.0) {
        // 使用符号扰动规则
        return symbolicPerturbation(pa, pb, pc, pd);
    }

    return result;
}

// 2. 基于输入顺序的确定性选择
// 确保相同输入总是得到相同结果
```

### 11. 性能考虑

| 谓词 | 快速路径 | 精确路径 |
|------|----------|----------|
| orient2d | ~10 FLOPS | ~200 FLOPS |
| orient3d | ~50 FLOPS | ~1000 FLOPS |
| incircle | ~50 FLOPS | ~2000 FLOPS |
| insphere | ~200 FLOPS | ~10000 FLOPS |

**关键优化**：
- 大多数情况下快速路径足够
- 只有接近退化时才需要精确算术
- 在实践中，精确路径使用率通常<1%

---

## 练习作业

### 练习1：理解浮点误差（基础）
```cpp
// 实验浮点误差
#include <iostream>
#include <iomanip>
#include <cmath>

void floatExperiment() {
    // 1. 验证浮点误差
    double a = 0.1;
    for (int i = 0; i < 10; i++) {
        std::cout << std::setprecision(20) << a << std::endl;
        a += 0.1;
    }
    std::cout << "Is a == 1.0? " << (a == 1.0) << std::endl;

    // 2. 找出导致orient2d朴素实现失败的点
    // TODO: 构造三个几乎共线的点，使得
    //       orient2d(a,b,c) 和 orient2d(b,c,a) 符号不同
}
```

### 练习2：实现Two-Sum（进阶）
```cpp
// Two-Sum: 精确计算两个浮点数的和
// 返回 x + y，其中 x 是近似和，y 是误差
void twoSum(double a, double b, double &x, double &y) {
    // TODO: 实现Two-Sum算法
    // 提示：
    // x = a + b
    // 计算 y 使得 a + b = x + y（精确成立）
}

// 测试
void testTwoSum() {
    double a = 1.0;
    double b = 1e-20;
    double x, y;
    twoSum(a, b, x, y);

    std::cout << "a = " << a << std::endl;
    std::cout << "b = " << b << std::endl;
    std::cout << "x = " << x << std::endl;
    std::cout << "y = " << y << std::endl;
    std::cout << "x + y == a + b? " << (x + y == a + b) << std::endl;
}
```

### 练习3：谓词测试套件（挑战）
```cpp
// 创建测试用例验证谓词实现
class PredicateTest {
public:
    void testOrient2d() {
        // 明确情况
        double a[] = {0, 0};
        double b[] = {1, 0};
        double c[] = {0.5, 1};

        assert(orient2d(a, b, c) > 0);  // c在ab左侧
        assert(orient2d(a, c, b) < 0);  // b在ac右侧

        // 边界情况 - 共线
        double d[] = {0.5, 0};
        assert(orient2d(a, b, d) == 0);

        // TODO: 添加更多测试用例
        // - 非常接近共线的情况
        // - 很大的坐标值
        // - 很小的坐标值
    }

    void testIncircle() {
        // TODO: 类似地测试incircle
    }
};
```

---

## 源码导航

### 核心文件
| 文件 | 内容 | 行数 |
|------|------|------|
| `src/numeric/robustPredicates.cpp` | 鲁棒几何谓词实现 | ~4000行 |
| `src/numeric/robustPredicates.h` | 头文件 | ~50行 |

### 关键函数
```cpp
// robustPredicates.cpp

// 初始化
void exactinit();

// 自适应精度谓词
double orient2dadapt(double *pa, double *pb, double *pc, double detsum);
double orient3dadapt(double *pa, double *pb, double *pc, double *pd,
                     double permanent);
double incircleadapt(double *pa, double *pb, double *pc, double *pd,
                     double permanent);
double insphereadapt(double *pa, double *pb, double *pc, double *pd,
                     double *pe, double permanent);

// 精确算术基本操作
void two_product(double a, double b, double &x, double &y);
void two_sum(double a, double b, double &x, double &y);
int scale_expansion_zeroelim(int elen, double *e, double b, double *h);
```

---

## 今日检查点

- [ ] 理解orient2d/orient3d的数学含义
- [ ] 理解incircle/insphere的数学含义
- [ ] 能解释浮点误差如何影响几何判断
- [ ] 理解自适应精度算术的基本思想
- [ ] 了解Gmsh如何处理退化情况

---

## 扩展阅读

### 必读论文
1. Shewchuk, "Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates"
   - 详细解释了自适应精度算术的实现

### 代码参考
- Shewchuk的参考实现：https://www.cs.cmu.edu/~quake/robust.html
- CGAL的精确计算内核

### 相关概念
- IEEE 754浮点标准
- 区间算术
- 有理数算术

---

## 导航

- 上一天：[Day 44 - 3D Delaunay实现深度分析](day-44.md)
- 下一天：[Day 46 - 网格质量度量](day-46.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
