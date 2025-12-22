# Day 43：Delaunay算法理论基础

## 学习目标
深入理解Delaunay三角剖分的数学理论基础，为后续源码分析奠定理论基础。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习Delaunay三角剖分的空圆性质 |
| 10:00-11:00 | 1h | 理解Voronoi图与Delaunay的对偶关系 |
| 11:00-12:00 | 1h | 研究Delaunay的唯一性与最优性证明 |
| 14:00-15:00 | 1h | 学习增量插入算法与点定位策略 |

---

## 上午学习内容

### 1. Delaunay三角剖分定义

**核心性质 - 空圆性质（Empty Circle Property）**：
对于Delaunay三角剖分中的每个三角形，其外接圆内部不包含任何其他点。

```
          C
         /\
        /  \
       /    \
      / 外接圆 \
     /   ⊙    \
    /_________ \
   A            B

   外接圆内无其他点 → Delaunay三角形
```

### 2. Voronoi图与Delaunay对偶

**Voronoi图定义**：
给定点集P，Voronoi图将平面划分为区域，每个区域包含离某个点最近的所有位置。

**对偶关系**：
- Voronoi边 ↔ Delaunay边（连接共享Voronoi边的两个点）
- Voronoi顶点 ↔ Delaunay三角形（外接圆圆心）

```cpp
// 对偶关系示意
// Voronoi顶点 = Delaunay三角形的外接圆圆心
SPoint3 circumcenter(MTriangle *t) {
    // 三个顶点
    SPoint3 a = t->getVertex(0)->point();
    SPoint3 b = t->getVertex(1)->point();
    SPoint3 c = t->getVertex(2)->point();

    // 计算外接圆圆心
    double ax = a.x(), ay = a.y();
    double bx = b.x(), by = b.y();
    double cx = c.x(), cy = c.y();

    double d = 2 * (ax*(by-cy) + bx*(cy-ay) + cx*(ay-by));
    double ux = ((ax*ax + ay*ay)*(by-cy) +
                 (bx*bx + by*by)*(cy-ay) +
                 (cx*cx + cy*cy)*(ay-by)) / d;
    double uy = ((ax*ax + ay*ay)*(cx-bx) +
                 (bx*bx + by*by)*(ax-cx) +
                 (cx*cx + cy*cy)*(bx-ax)) / d;

    return SPoint3(ux, uy, 0);
}
```

### 3. Delaunay三角剖分的性质

| 性质 | 描述 |
|------|------|
| **唯一性** | 当没有四点共圆时，Delaunay三角剖分唯一 |
| **最大最小角** | 最大化所有三角形的最小角 |
| **最近邻** | Delaunay边连接的点在某种意义上是"最近邻" |
| **凸包** | Delaunay三角剖分包含点集的凸包 |

### 4. 退化情况处理

**四点共圆（Co-circular Points）**：
```
当四个或更多点位于同一圆上时，Delaunay三角剖分不唯一。
处理方法：
1. 符号扰动（Symbolic Perturbation）
2. 任意选择对角线
3. 使用特定规则（如字典序）
```

---

## 下午学习内容

### 5. 增量插入算法

**基本步骤**：
1. 初始化：创建包含所有点的超级三角形
2. 逐点插入：
   - 定位包含新点的三角形
   - 删除违反空圆性质的三角形
   - 创建新三角形连接到新点
3. 清理：删除与超级三角形相关的元素

```cpp
// 增量插入算法伪代码
class IncrementalDelaunay {
public:
    void insert(const SPoint3 &p) {
        // 1. 点定位 - 找到包含p的三角形
        Triangle *t = locate(p);

        // 2. 空腔构建 - 找所有外接圆包含p的三角形
        std::vector<Triangle*> cavity;
        buildCavity(p, t, cavity);

        // 3. 重构 - 删除空腔，创建新三角形
        std::vector<Edge> boundary;
        getCavityBoundary(cavity, boundary);

        for (Edge &e : boundary) {
            createTriangle(e.v1, e.v2, p);
        }
    }

private:
    Triangle* locate(const SPoint3 &p);
    void buildCavity(const SPoint3 &p, Triangle *start,
                     std::vector<Triangle*> &cavity);
};
```

### 6. 点定位策略

**行走法（Walking Algorithm）**：
```cpp
Triangle* walkToPoint(Triangle *start, const SPoint3 &p) {
    Triangle *current = start;

    while (true) {
        // 检查p在当前三角形的哪一侧
        int edge = -1;
        for (int i = 0; i < 3; i++) {
            if (orient2d(current->vertex(i),
                        current->vertex((i+1)%3), p) < 0) {
                edge = i;
                break;
            }
        }

        if (edge < 0) {
            // p在三角形内部
            return current;
        }

        // 移动到相邻三角形
        current = current->neighbor(edge);
        if (current == nullptr) {
            // 点在凸包外
            return nullptr;
        }
    }
}
```

**跳跃定位（Jump-and-Walk）**：
- 使用空间索引快速找到附近的起始点
- 然后使用行走法精确定位
- 平均时间复杂度：O(√n)

### 7. 时间复杂度分析

| 算法 | 最坏情况 | 期望情况 |
|------|----------|----------|
| 朴素增量 | O(n²) | O(n log n) |
| 随机增量 | O(n²) | O(n log n) |
| 分治法 | O(n log n) | O(n log n) |
| 扫描线 | O(n log n) | O(n log n) |

### 8. InCircle谓词

**核心判断函数**：
```cpp
// 判断点d是否在三角形abc的外接圆内
// 返回值: >0 在圆内, =0 在圆上, <0 在圆外
double incircle(const SPoint3 &a, const SPoint3 &b,
                const SPoint3 &c, const SPoint3 &d) {
    // 使用行列式计算
    //     | ax-dx  ay-dy  (ax-dx)²+(ay-dy)² |
    // det | bx-dx  by-dy  (bx-dx)²+(by-dy)² |
    //     | cx-dx  cy-dy  (cx-dx)²+(cy-dy)² |

    double adx = a.x() - d.x();
    double ady = a.y() - d.y();
    double bdx = b.x() - d.x();
    double bdy = b.y() - d.y();
    double cdx = c.x() - d.x();
    double cdy = c.y() - d.y();

    double abdet = adx * bdy - bdx * ady;
    double bcdet = bdx * cdy - cdx * bdy;
    double cadet = cdx * ady - adx * cdy;

    double alift = adx * adx + ady * ady;
    double blift = bdx * bdx + bdy * bdy;
    double clift = cdx * cdx + cdy * cdy;

    return alift * bcdet + blift * cadet + clift * abdet;
}
```

---

## 练习作业

### 练习1：手工Delaunay（基础）
给定以下5个点，手工构造Delaunay三角剖分：
- P1(0, 0), P2(4, 0), P3(2, 3), P4(1, 1), P5(3, 1)

步骤：
1. 画出所有点
2. 尝试不同的三角剖分
3. 用圆规验证每个三角形的空圆性质
4. 找出唯一的Delaunay三角剖分

### 练习2：Voronoi-Delaunay对偶（进阶）
```cpp
// 实现Voronoi图生成器
// 基于Delaunay三角剖分
class VoronoiFromDelaunay {
public:
    void compute(const std::vector<SPoint3> &points) {
        // 1. 构建Delaunay三角剖分
        // 2. 为每个三角形计算外接圆圆心
        // 3. 连接相邻三角形的圆心形成Voronoi边
        // TODO: 补充实现
    }

    void draw() {
        // 绘制Voronoi图
    }
};
```

### 练习3：InCircle测试实现（挑战）
```cpp
// 实现高精度incircle谓词
// 使用自适应精度算术

#include <array>

class RobustPredicates {
public:
    // 初始化误差边界
    static void initialize();

    // 精确incircle判断
    // 先用快速近似，必要时用精确算术
    static double incircle(const double *pa, const double *pb,
                          const double *pc, const double *pd) {
        // 快速滤波器
        double result = incirclefast(pa, pb, pc, pd);
        double bound = computeErrorBound(pa, pb, pc, pd);

        if (std::abs(result) > bound) {
            return result;  // 快速结果可靠
        }

        // 需要精确计算
        return incircleexact(pa, pb, pc, pd);
    }

private:
    static double incirclefast(const double *pa, const double *pb,
                              const double *pc, const double *pd);
    static double incircleexact(const double *pa, const double *pb,
                               const double *pc, const double *pd);
    static double computeErrorBound(const double *pa, const double *pb,
                                   const double *pc, const double *pd);
};
```

---

## 源码导航

### 核心文件
| 文件 | 内容 |
|------|------|
| `src/mesh/delaunay3d.cpp` | 3D Delaunay实现 |
| `src/mesh/meshGFaceDelaunayInsertion.cpp` | 2D Delaunay点插入 |
| `src/numeric/robustPredicates.cpp` | 几何谓词 |
| `src/mesh/qualityMeasures.cpp` | 质量度量 |

### 关键函数
```cpp
// src/numeric/robustPredicates.cpp
double robustPredicates::orient2d(double *pa, double *pb, double *pc);
double robustPredicates::incircle(double *pa, double *pb, double *pc, double *pd);

// src/mesh/delaunay3d.cpp
void insertVertex(MVertex *v, ...);
void cavityDelaunay(MVertex *v, std::vector<MTet4*> &cavity);
```

---

## 今日检查点

- [ ] 能解释Delaunay三角剖分的空圆性质
- [ ] 理解Voronoi图与Delaunay的对偶关系
- [ ] 能描述增量插入算法的三个主要步骤
- [ ] 理解incircle谓词的数学含义和计算方法
- [ ] 能分析Delaunay算法的时间复杂度

---

## 扩展阅读

### 经典教材
1. "Computational Geometry: Algorithms and Applications" - Chapter 9
2. "Delaunay Mesh Generation" by Cheng, Dey, Shewchuk

### 论文
1. Shewchuk, "Triangle: Engineering a 2D Quality Mesh Generator"
2. Si, "TetGen: A Delaunay-Based Quality Tetrahedral Mesh Generator"

### 在线资源
- Shewchuk's Robust Predicates: https://www.cs.cmu.edu/~quake/robust.html

---

## 导航

- 上一天：[Day 42 - 第六周复习](day-42.md)
- 下一天：[Day 44 - 3D Delaunay实现深度分析](day-44.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
