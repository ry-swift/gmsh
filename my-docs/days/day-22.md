# Day 22：MElement网格元素基类

**所属周次**：第4周 - 网格数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解MElement继承体系和网格元素抽象

---

## 学习文件

- `src/geo/MElement.h`（约800行）
- `src/geo/MElement.cpp`（约2000行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读MElement.h类定义 |
| 09:45-10:30 | 45min | 理解元素类型枚举和维度 |
| 10:30-11:15 | 45min | 理解顶点访问接口 |
| 11:15-12:00 | 45min | 理解边和面的提取 |
| 14:00-14:45 | 45min | 理解形函数相关接口 |
| 14:45-15:30 | 45min | 理解雅可比矩阵计算 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 阅读 MElement.h

**文件路径**：`src/geo/MElement.h`

MElement是所有网格元素的抽象基类。

```cpp
class MElement {
protected:
  // 元素编号
  std::size_t _num;

  // 分区信息（用于并行计算）
  short _partition;

  // 可见性
  char _visible;

public:
  // 获取元素编号
  std::size_t getNum() const { return _num; }

  // 维度：0=点, 1=线, 2=面, 3=体
  virtual int getDim() const = 0;

  // 元素类型（MSH格式类型号）
  virtual int getTypeForMSH() const = 0;

  // 元素名称
  virtual const char *getStringForPOS() const = 0;

  // 顶点数量
  virtual std::size_t getNumVertices() const = 0;

  // 获取顶点
  virtual MVertex *getVertex(int num) const = 0;

  // 获取所有顶点
  virtual void getVertices(std::vector<MVertex*> &v) const;
};
```

### 元素类型系统

```cpp
// MSH元素类型编号（src/geo/MElement.h）
enum ElementType {
  TYPE_PNT = 1,    // 点
  TYPE_LIN = 2,    // 2节点线
  TYPE_TRI = 3,    // 3节点三角形
  TYPE_QUA = 4,    // 4节点四边形
  TYPE_TET = 5,    // 4节点四面体
  TYPE_HEX = 6,    // 8节点六面体
  TYPE_PRI = 7,    // 6节点三棱柱
  TYPE_PYR = 8,    // 5节点金字塔
  // 高阶元素...
  TYPE_LIN3 = 9,   // 3节点线（二次）
  TYPE_TRI6 = 10,  // 6节点三角形（二次）
  TYPE_QUA9 = 11,  // 9节点四边形（二次）
  TYPE_TET10 = 12, // 10节点四面体（二次）
  // ...更多高阶类型
};
```

### 元素维度对应

```
维度  元素类型              派生类
─────────────────────────────────────────
0D   点                    MPoint
1D   线段                  MLine, MLine3, MLineN
2D   三角形、四边形         MTriangle, MQuadrangle, ...
3D   四面体、六面体等       MTetrahedron, MHexahedron, ...
```

---

## 下午任务（2小时）

### 边和面的提取

```cpp
class MElement {
public:
  // 边的数量
  virtual int getNumEdges() const = 0;

  // 获取边（返回MEdge对象）
  virtual MEdge getEdge(int num) const = 0;

  // 面的数量
  virtual int getNumFaces() const = 0;

  // 获取面（返回MFace对象）
  virtual MFace getFace(int num) const = 0;

  // 获取所有边
  void getEdges(std::vector<MEdge> &edges) const;

  // 获取所有面
  void getFaces(std::vector<MFace> &faces) const;
};
```

### 形函数接口

```cpp
class MElement {
public:
  // 获取形函数值
  // uvw: 参考坐标 (ξ, η, ζ)
  // sf: 输出形函数值数组
  virtual void getShapeFunctions(double u, double v, double w,
                                  double *sf, int order = -1) const;

  // 获取形函数梯度
  // grads[i] = (∂Ni/∂ξ, ∂Ni/∂η, ∂Ni/∂ζ)
  virtual void getGradShapeFunctions(double u, double v, double w,
                                      double (*grads)[3], int order = -1) const;
};
```

### 雅可比矩阵

```cpp
class MElement {
public:
  // 计算雅可比矩阵
  // J = [∂x/∂ξ  ∂y/∂ξ  ∂z/∂ξ]
  //     [∂x/∂η  ∂y/∂η  ∂z/∂η]
  //     [∂x/∂ζ  ∂y/∂ζ  ∂z/∂ζ]
  virtual double getJacobian(double u, double v, double w,
                              double jac[3][3]) const;

  // 雅可比行列式（用于积分变换）
  // detJ = |J|
  virtual double getJacobianDeterminant(double u, double v, double w) const;

  // 积分点处的雅可比
  virtual double getJacobian(const fullMatrix<double> &gsf,
                              double jac[3][3]) const;
};
```

### 雅可比的几何意义

```
参考空间 (ξ, η, ζ)          物理空间 (x, y, z)
┌─────────┐                  ╭─────────╮
│         │     J            │         │
│  ref    │  ────→           │  real   │
│  elem   │                  │  elem   │
└─────────┘                  ╰─────────╯

积分变换：
∫∫∫_Ω f(x,y,z) dV = ∫∫∫_Ω_ref f(ξ,η,ζ) |J| dξdηdζ

元素质量：
- detJ > 0: 正向（有效元素）
- detJ < 0: 翻转（无效元素）
- detJ = 0: 退化（零体积）
```

---

## 练习作业

### 1. 【基础】统计元素类型

查找所有MElement的派生类：

```bash
# 查找所有派生类
grep -rn "class M.*: public MElement" src/geo/
grep -rn "class M.*: public MTriangle" src/geo/
grep -rn "class M.*: public MTetrahedron" src/geo/
```

记录至少10种不同的元素类型。

### 2. 【进阶】理解边/面提取

分析三角形元素如何提取边：

```bash
# 查找MTriangle的getEdge实现
grep -A 10 "MTriangle::getEdge" src/geo/MTriangle.cpp

# 查找四面体的getFace实现
grep -A 15 "MTetrahedron::getFace" src/geo/MTetrahedron.cpp
```

画出三角形的3条边和四面体的4个面的编号规则。

### 3. 【挑战】追踪形函数计算

找到三角形形函数的具体实现：

```bash
# 查找形函数实现
grep -rn "getShapeFunctions" src/geo/MTriangle.cpp
grep -rn "getShapeFunctions" src/numeric/
```

验证线性三角形的形函数：
```
N1(ξ,η) = 1 - ξ - η
N2(ξ,η) = ξ
N3(ξ,η) = η
```

---

## 今日检查点

- [ ] 理解MElement作为网格元素基类的设计
- [ ] 能说出MSH元素类型编号的规则
- [ ] 理解getVertex、getEdge、getFace接口的作用
- [ ] 理解雅可比矩阵在有限元中的意义

---

## 核心概念

### MElement继承体系

```
MElement（抽象基类）
├── MPoint           # 0D点元素
├── MLine            # 1D线元素
│   ├── MLine3       # 二次线（3节点）
│   └── MLineN       # 高阶线
├── MTriangle        # 2D三角形
│   ├── MTriangle6   # 二次三角形（6节点）
│   ├── MTriangleN   # 高阶三角形
│   └── MTriangleBorder
├── MQuadrangle      # 2D四边形
│   ├── MQuadrangle8 # 二次四边形（8节点）
│   ├── MQuadrangle9 # 二次四边形（9节点）
│   └── MQuadrangleN
├── MTetrahedron     # 3D四面体
│   ├── MTetrahedron10
│   └── MTetrahedronN
├── MHexahedron      # 3D六面体
│   ├── MHexahedron20
│   ├── MHexahedron27
│   └── MHexahedronN
├── MPrism           # 3D三棱柱
├── MPyramid         # 3D金字塔
└── MPolyhedron      # 多面体
```

### 元素的关键属性

| 属性 | 方法 | 说明 |
|------|------|------|
| 编号 | getNum() | 全局唯一ID |
| 维度 | getDim() | 0/1/2/3 |
| 类型 | getTypeForMSH() | MSH格式类型号 |
| 顶点数 | getNumVertices() | 包含的顶点数量 |
| 边数 | getNumEdges() | 边的数量 |
| 面数 | getNumFaces() | 面的数量 |
| 阶数 | getPolynomialOrder() | 1=线性, 2=二次... |

### 参考元素坐标

```
线段 (ξ ∈ [-1, 1])：
   0 ───────── 1
  ξ=-1        ξ=1

三角形 (ξ,η ∈ [0,1], ξ+η≤1)：
       2 (0,1)
      /\
     /  \
    /    \
   /______\
  0        1
(0,0)    (1,0)

四面体 (ξ,η,ζ ∈ [0,1], ξ+η+ζ≤1)：
      3 (0,0,1)
     /|\
    / | \
   /  |  \
  /   |   \
 /___ 2____\
0    (0,1,0) 1
(0,0,0)   (1,0,0)
```

---

## 源码导航

### 关键文件

```
src/geo/
├── MElement.h/cpp        # 元素基类
├── MPoint.h              # 点元素
├── MLine.h/cpp           # 线元素
├── MTriangle.h/cpp       # 三角形
├── MQuadrangle.h/cpp     # 四边形
├── MTetrahedron.h/cpp    # 四面体
├── MHexahedron.h/cpp     # 六面体
├── MPrism.h/cpp          # 三棱柱
├── MPyramid.h/cpp        # 金字塔
├── MEdge.h               # 网格边
└── MFace.h               # 网格面

src/numeric/
├── BasisFactory.h/cpp    # 基函数工厂
├── nodalBasis.h/cpp      # 节点基函数
└── polynomialBasis.h/cpp # 多项式基函数
```

### 搜索命令

```bash
# 查找所有元素类型
grep -rn "TYPE_" src/geo/MElement.h

# 查找形函数实现
grep -rn "getShapeFunctions" src/geo/*.cpp

# 查找雅可比计算
grep -rn "getJacobian" src/geo/MElement.cpp
```

---

## 产出

- 理解MElement继承体系
- 记录元素类型编号规则

---

## 导航

- **上一天**：[Day 21 - 第三周复习](day-21.md)
- **下一天**：[Day 23 - 各类型网格元素](day-23.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
