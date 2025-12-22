# Day 23：各类型网格元素

**所属周次**：第4周 - 网格数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

深入学习各维度网格元素的具体实现

---

## 学习文件

- `src/geo/MTriangle.h`（约200行）
- `src/geo/MQuadrangle.h`（约200行）
- `src/geo/MTetrahedron.h`（约250行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读MLine相关实现 |
| 09:45-10:30 | 45min | 阅读MTriangle实现 |
| 10:30-11:15 | 45min | 阅读MQuadrangle实现 |
| 11:15-12:00 | 45min | 对比三角形和四边形的差异 |
| 14:00-14:45 | 45min | 阅读MTetrahedron实现 |
| 14:45-15:30 | 45min | 阅读MHexahedron实现 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### MLine - 线元素

**文件路径**：`src/geo/MLine.h`

```cpp
class MLine : public MElement {
protected:
  MVertex *_v[2];  // 两个端点

public:
  MLine(MVertex *v0, MVertex *v1, int num = 0, int part = 0)
    : MElement(num, part) {
    _v[0] = v0;
    _v[1] = v1;
  }

  virtual int getDim() const { return 1; }
  virtual std::size_t getNumVertices() const { return 2; }
  virtual MVertex *getVertex(int num) const { return _v[num]; }

  virtual int getNumEdges() const { return 1; }
  virtual MEdge getEdge(int num) const { return MEdge(_v[0], _v[1]); }

  virtual int getNumFaces() const { return 0; }

  // 长度计算
  double getLength() const;
};

// 二次线元素（3节点）
class MLine3 : public MLine {
protected:
  MVertex *_vs[1];  // 中间节点

public:
  virtual std::size_t getNumVertices() const { return 3; }
  virtual int getPolynomialOrder() const { return 2; }
};
```

### MTriangle - 三角形元素

**文件路径**：`src/geo/MTriangle.h`

```cpp
class MTriangle : public MElement {
protected:
  MVertex *_v[3];  // 三个顶点

public:
  MTriangle(MVertex *v0, MVertex *v1, MVertex *v2,
            int num = 0, int part = 0);

  virtual int getDim() const { return 2; }
  virtual std::size_t getNumVertices() const { return 3; }
  virtual MVertex *getVertex(int num) const { return _v[num]; }

  // 三角形有3条边
  virtual int getNumEdges() const { return 3; }
  virtual MEdge getEdge(int num) const {
    // 边的顶点编号: (0,1), (1,2), (2,0)
    static const int edges[3][2] = {{0,1}, {1,2}, {2,0}};
    return MEdge(_v[edges[num][0]], _v[edges[num][1]]);
  }

  // 三角形有1个面（自身）
  virtual int getNumFaces() const { return 1; }
  virtual MFace getFace(int num) const {
    return MFace(_v[0], _v[1], _v[2]);
  }

  // 计算面积
  double getArea() const;

  // 计算外接圆圆心
  void circumcenter(double &x, double &y, double &z) const;

  // 计算内切圆圆心
  void incenter(double &x, double &y, double &z) const;
};
```

### MQuadrangle - 四边形元素

**文件路径**：`src/geo/MQuadrangle.h`

```cpp
class MQuadrangle : public MElement {
protected:
  MVertex *_v[4];  // 四个顶点（逆时针顺序）

public:
  MQuadrangle(MVertex *v0, MVertex *v1, MVertex *v2, MVertex *v3,
              int num = 0, int part = 0);

  virtual int getDim() const { return 2; }
  virtual std::size_t getNumVertices() const { return 4; }
  virtual MVertex *getVertex(int num) const { return _v[num]; }

  // 四边形有4条边
  virtual int getNumEdges() const { return 4; }
  virtual MEdge getEdge(int num) const {
    // 边的顶点编号: (0,1), (1,2), (2,3), (3,0)
    static const int edges[4][2] = {{0,1}, {1,2}, {2,3}, {3,0}};
    return MEdge(_v[edges[num][0]], _v[edges[num][1]]);
  }

  // 四边形有1个面（自身）
  virtual int getNumFaces() const { return 1; }

  // 计算面积
  double getArea() const;
};
```

### 顶点编号规则

```
三角形顶点编号（逆时针）：
      2
     /\
    /  \
   /    \
  0──────1

边编号：
  边0: (0,1)
  边1: (1,2)
  边2: (2,0)

四边形顶点编号（逆时针）：
  3───────2
  │       │
  │       │
  0───────1

边编号：
  边0: (0,1)
  边1: (1,2)
  边2: (2,3)
  边3: (3,0)
```

---

## 下午任务（2小时）

### MTetrahedron - 四面体元素

**文件路径**：`src/geo/MTetrahedron.h`

```cpp
class MTetrahedron : public MElement {
protected:
  MVertex *_v[4];  // 四个顶点

public:
  MTetrahedron(MVertex *v0, MVertex *v1, MVertex *v2, MVertex *v3,
               int num = 0, int part = 0);

  virtual int getDim() const { return 3; }
  virtual std::size_t getNumVertices() const { return 4; }
  virtual MVertex *getVertex(int num) const { return _v[num]; }

  // 四面体有6条边
  virtual int getNumEdges() const { return 6; }
  virtual MEdge getEdge(int num) const {
    static const int edges[6][2] = {
      {0,1}, {1,2}, {2,0},  // 底面边
      {0,3}, {1,3}, {2,3}   // 侧边
    };
    return MEdge(_v[edges[num][0]], _v[edges[num][1]]);
  }

  // 四面体有4个面
  virtual int getNumFaces() const { return 4; }
  virtual MFace getFace(int num) const {
    static const int faces[4][3] = {
      {0,2,1},  // 底面（从外看逆时针）
      {0,1,3},  // 面1
      {1,2,3},  // 面2
      {2,0,3}   // 面3
    };
    return MFace(_v[faces[num][0]], _v[faces[num][1]], _v[faces[num][2]]);
  }

  // 计算体积
  double getVolume() const;

  // 计算外接球圆心
  void circumcenter(double &x, double &y, double &z) const;

  // 计算内切球圆心
  void incenter(double &x, double &y, double &z) const;

  // 计算外接球半径
  double getCircumRadius() const;

  // 计算内切球半径
  double getInRadius() const;
};
```

### MHexahedron - 六面体元素

**文件路径**：`src/geo/MHexahedron.h`

```cpp
class MHexahedron : public MElement {
protected:
  MVertex *_v[8];  // 八个顶点

public:
  MHexahedron(MVertex *v0, MVertex *v1, MVertex *v2, MVertex *v3,
              MVertex *v4, MVertex *v5, MVertex *v6, MVertex *v7,
              int num = 0, int part = 0);

  virtual int getDim() const { return 3; }
  virtual std::size_t getNumVertices() const { return 8; }

  // 六面体有12条边
  virtual int getNumEdges() const { return 12; }

  // 六面体有6个面
  virtual int getNumFaces() const { return 6; }

  // 计算体积
  double getVolume() const;
};
```

### 3D元素顶点编号

```
四面体顶点编号：
        3
       /|\
      / | \
     /  |  \
    /   |   \
   0----+----2
    \   |   /
     \  |  /
      \ | /
        1

面编号（从外部看，顶点逆时针）：
  面0: (0,2,1) - 底面
  面1: (0,1,3)
  面2: (1,2,3)
  面3: (2,0,3)

六面体顶点编号：
      7───────6
     /│      /│
    / │     / │
   4───────5  │
   │  3────│──2
   │ /     │ /
   │/      │/
   0───────1

面编号：
  面0: (0,3,2,1) - 底面
  面1: (4,5,6,7) - 顶面
  面2: (0,1,5,4) - 前面
  面3: (2,3,7,6) - 后面
  面4: (1,2,6,5) - 右面
  面5: (3,0,4,7) - 左面
```

---

## 练习作业

### 1. 【基础】比较不同元素

列出各类型元素的关键属性：

```bash
# 查找各元素的顶点数、边数、面数
grep -n "getNumVertices\|getNumEdges\|getNumFaces" src/geo/MTriangle.h
grep -n "getNumVertices\|getNumEdges\|getNumFaces" src/geo/MTetrahedron.h
```

填写对比表格：

| 元素 | 维度 | 顶点数 | 边数 | 面数 | MSH类型号 |
|------|------|--------|------|------|-----------|
| MLine | 1 | 2 | 1 | 0 | 1 |
| MTriangle | 2 | 3 | 3 | 1 | 2 |
| MQuadrangle | 2 | ? | ? | ? | ? |
| MTetrahedron | 3 | ? | ? | ? | ? |
| MHexahedron | 3 | ? | ? | ? | ? |

### 2. 【进阶】理解高阶元素

分析高阶三角形元素：

```bash
# 查找高阶三角形
grep -rn "class MTriangle6\|class MTriangleN" src/geo/
cat src/geo/MTriangle.h | grep -A 30 "class MTriangle6"
```

二次三角形(6节点)的节点位置：
```
      2
     /\
    5  4
   /    \
  0──3───1

节点0,1,2: 角点
节点3: 边(0,1)中点
节点4: 边(1,2)中点
节点5: 边(2,0)中点
```

### 3. 【挑战】实现元素质量检查

查找元素质量计算的相关代码：

```bash
# 查找质量度量
grep -rn "getQuality\|gammaShapeMeasure" src/geo/MElement.cpp
```

理解常用的质量度量：
- 三角形：γ = 4√3 * Area / (Σ edge²)
- 四面体：γ = 12 * (3V)^(2/3) / (Σ face_area)

---

## 今日检查点

- [ ] 理解MLine、MTriangle、MQuadrangle的实现
- [ ] 理解MTetrahedron、MHexahedron的实现
- [ ] 能画出各元素的顶点编号规则
- [ ] 理解高阶元素的节点布局

---

## 核心概念

### 元素属性汇总

| 元素 | 维度 | 顶点 | 边 | 面 | 阶数 | MSH类型 |
|------|------|------|-----|-----|------|---------|
| MPoint | 0 | 1 | 0 | 0 | 1 | 15 |
| MLine | 1 | 2 | 1 | 0 | 1 | 1 |
| MLine3 | 1 | 3 | 1 | 0 | 2 | 8 |
| MTriangle | 2 | 3 | 3 | 1 | 1 | 2 |
| MTriangle6 | 2 | 6 | 3 | 1 | 2 | 9 |
| MQuadrangle | 2 | 4 | 4 | 1 | 1 | 3 |
| MQuadrangle8 | 2 | 8 | 4 | 1 | 2 | 16 |
| MQuadrangle9 | 2 | 9 | 4 | 1 | 2 | 10 |
| MTetrahedron | 3 | 4 | 6 | 4 | 1 | 4 |
| MTetrahedron10 | 3 | 10 | 6 | 4 | 2 | 11 |
| MHexahedron | 3 | 8 | 12 | 6 | 1 | 5 |
| MHexahedron20 | 3 | 20 | 12 | 6 | 2 | 17 |
| MHexahedron27 | 3 | 27 | 12 | 6 | 2 | 12 |
| MPrism | 3 | 6 | 9 | 5 | 1 | 6 |
| MPyramid | 3 | 5 | 8 | 5 | 1 | 7 |

### 欧拉公式验证

对于简单多面体：V - E + F = 2

- 四面体：4 - 6 + 4 = 2 ✓
- 六面体：8 - 12 + 6 = 2 ✓
- 三棱柱：6 - 9 + 5 = 2 ✓
- 金字塔：5 - 8 + 5 = 2 ✓

### 面的法向量方向

```
约定：从元素外部观察，面的顶点按逆时针排列
这样计算的法向量指向元素外部

例如四面体的面0 (0,2,1)：
  v02 = v2 - v0
  v01 = v1 - v0
  n = v02 × v01  （指向元素外部）
```

---

## 源码导航

### 关键文件

```
src/geo/
├── MLine.h              # 线元素
├── MTriangle.h/cpp      # 三角形
├── MQuadrangle.h/cpp    # 四边形
├── MTetrahedron.h/cpp   # 四面体
├── MHexahedron.h/cpp    # 六面体
├── MPrism.h/cpp         # 三棱柱
├── MPyramid.h/cpp       # 金字塔
└── MElementCut.h/cpp    # 元素切割
```

### 搜索命令

```bash
# 查找特定元素类型
grep -n "class MTriangle" src/geo/MTriangle.h

# 查找体积/面积计算
grep -n "getVolume\|getArea" src/geo/*.cpp

# 查找外接圆/球计算
grep -n "circumcenter\|circumRadius" src/geo/*.cpp
```

---

## 产出

- 掌握各类型网格元素的结构
- 记录元素属性对比表

---

## 导航

- **上一天**：[Day 22 - MElement网格元素基类](day-22.md)
- **下一天**：[Day 24 - MVertex网格顶点](day-24.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
