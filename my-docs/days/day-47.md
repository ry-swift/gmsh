# Day 47：网格优化算法

## 学习目标
理解网格优化技术，掌握2D和3D网格优化的核心算法，学习Gmsh中的优化实现。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习2D网格优化技术（Laplacian平滑、边翻转） |
| 10:00-11:00 | 1h | 阅读meshGFaceOptimize.cpp |
| 11:00-12:00 | 1h | 学习3D网格优化技术（面翻转、边操作） |
| 14:00-15:00 | 1h | 阅读meshGRegionOptimize.cpp和实践 |

---

## 上午学习内容

### 1. 网格优化概述

**优化目标**：
- 提高网格质量（改善形状比、最小角等）
- 保持边界和特征约束
- 维护网格拓扑有效性

**优化策略分类**：
```
网格优化
├── 拓扑优化（改变连接关系）
│   ├── 边翻转 (Edge Flip)
│   ├── 边折叠 (Edge Collapse)
│   └── 边分裂 (Edge Split)
│
└── 几何优化（移动节点位置）
    ├── Laplacian平滑
    ├── 优化平滑
    └── CVT优化
```

### 2. Laplacian平滑

**基本思想**：
将每个内部节点移动到其邻居的质心位置。

```cpp
// Laplacian平滑实现
void laplacianSmooth(GFace *gf, int iterations) {
    for (int iter = 0; iter < iterations; iter++) {
        // 收集所有需要平滑的顶点
        std::map<MVertex*, SPoint3> newPositions;

        for (MVertex *v : gf->mesh_vertices) {
            // 跳过边界顶点
            if (v->onWhat()->dim() < 2) continue;

            // 获取邻居顶点
            std::vector<MVertex*> neighbors;
            getNeighbors(v, neighbors);

            if (neighbors.empty()) continue;

            // 计算质心
            SPoint3 centroid(0, 0, 0);
            for (MVertex *n : neighbors) {
                centroid += n->point();
            }
            centroid /= neighbors.size();

            newPositions[v] = centroid;
        }

        // 同时更新所有顶点位置
        for (auto &kv : newPositions) {
            MVertex *v = kv.first;
            const SPoint3 &newPos = kv.second;

            // 可选：限制移动量
            // SPoint3 oldPos = v->point();
            // SPoint3 limitedPos = oldPos + 0.5 * (newPos - oldPos);

            v->setXYZ(newPos.x(), newPos.y(), newPos.z());
        }
    }
}

// 获取顶点的邻居
void getNeighbors(MVertex *v, std::vector<MVertex*> &neighbors) {
    std::set<MVertex*> neighborSet;

    // 遍历包含v的所有单元
    for (MElement *e : v->getElements()) {
        for (int i = 0; i < e->getNumVertices(); i++) {
            MVertex *other = e->getVertex(i);
            if (other != v) {
                neighborSet.insert(other);
            }
        }
    }

    neighbors.assign(neighborSet.begin(), neighborSet.end());
}
```

### 3. 优化平滑（Optimization-based Smoothing）

**目标函数**：
最小化某种质量度量的倒数之和。

```cpp
// 基于优化的平滑
void optimizationSmooth(MVertex *v, const std::vector<MElement*> &elements) {
    // 使用梯度下降或牛顿法优化顶点位置

    // 目标函数：f(x) = sum_e 1/q(e)
    // 其中q(e)是包含顶点v的单元e的质量

    auto objective = [&](const SPoint3 &pos) -> double {
        double f = 0;
        for (MElement *e : elements) {
            // 临时设置顶点位置
            SPoint3 oldPos = v->point();
            v->setXYZ(pos.x(), pos.y(), pos.z());

            double q = e->minSICNShapeMeasure();
            if (q > 0) {
                f += 1.0 / q;
            } else {
                f += 1e10;  // 惩罚无效单元
            }

            // 恢复
            v->setXYZ(oldPos.x(), oldPos.y(), oldPos.z());
        }
        return f;
    };

    // 梯度下降
    SPoint3 pos = v->point();
    double alpha = 0.01;  // 步长

    for (int i = 0; i < 100; i++) {
        // 数值梯度
        double eps = 1e-6;
        SVector3 grad;
        double f0 = objective(pos);

        grad.x() = (objective(pos + SVector3(eps,0,0)) - f0) / eps;
        grad.y() = (objective(pos + SVector3(0,eps,0)) - f0) / eps;
        grad.z() = (objective(pos + SVector3(0,0,eps)) - f0) / eps;

        pos -= alpha * grad;
    }

    v->setXYZ(pos.x(), pos.y(), pos.z());
}
```

### 4. 边翻转（Edge Flip）

**2D边翻转**：
```
翻转前：           翻转后：
   B                  B
  /|\                / \
 / | \              /   \
A--+--C    →       A-----C
 \ | /              \   /
  \|/                \ /
   D                  D

边BD变为边AC
```

```cpp
// 2D边翻转
bool flipEdge(MVertex *v1, MVertex *v2,
              MTriangle *t1, MTriangle *t2) {
    // t1包含顶点v1, v2, v3
    // t2包含顶点v1, v2, v4

    MVertex *v3 = getThirdVertex(t1, v1, v2);
    MVertex *v4 = getThirdVertex(t2, v1, v2);

    // 检查翻转后是否合法（Delaunay准则）
    if (!shouldFlip(v1, v2, v3, v4)) {
        return false;
    }

    // 检查翻转后是否产生翻转的三角形
    double orient1 = orient2d(v1, v3, v4);
    double orient2 = orient2d(v2, v4, v3);
    if (orient1 * orient2 < 0) {
        return false;  // 会产生翻转
    }

    // 执行翻转
    // 删除旧边 (v1, v2)
    // 创建新边 (v3, v4)

    // 更新三角形
    // t1: v1, v2, v3 → v1, v3, v4
    // t2: v1, v2, v4 → v2, v4, v3

    return true;
}

// 判断是否应该翻转（Delaunay准则）
bool shouldFlip(MVertex *v1, MVertex *v2,
                MVertex *v3, MVertex *v4) {
    // 检查v4是否在三角形v1v2v3的外接圆内
    double inCircle = robustPredicates::incircle(
        v1->point(), v2->point(), v3->point(), v4->point());

    return inCircle > 0;
}
```

### 5. meshGFaceOptimize.cpp 源码分析

**主要函数**：
```cpp
// src/mesh/meshGFaceOptimize.cpp

// 主优化入口
void optimizeMeshGFace(GFace *gf);

// Laplacian平滑
void smoothMeshGFace(GFace *gf, int niter);

// 边翻转
void edgeFlipGFace(GFace *gf);

// 组合优化
void optimizeMeshGFaceLloyd(GFace *gf, int niter);
```

---

## 下午学习内容

### 6. 3D网格优化技术

**3D特有操作**：

| 操作 | 说明 | 复杂度 |
|------|------|--------|
| 面翻转 (Face Flip) | 2-3翻转或3-2翻转 | 局部 |
| 边折叠 (Edge Collapse) | 收缩边为点 | 局部 |
| 边分裂 (Edge Split) | 在边上插入点 | 局部 |
| 壳翻转 (Shell Flip) | 多面体组合操作 | 区域 |

### 7. 2-3翻转和3-2翻转

```
2-3翻转（两个四面体变三个）：
共享一个面的两个四面体 → 共享一条边的三个四面体

     A                    A
    /|\                  /|\
   / | \                / | \
  /  |  \              /  E  \
 B---+---C    →       B--/|\--C
  \  |  /              \/   \/
   \ | /               /\   /\
    \|/               /  \ /  \
     D               D    |    D
                          E

3-2翻转是逆操作
```

```cpp
// 2-3翻转
bool flip23(MTet4 *t1, MTet4 *t2, int sharedFace,
            MTet4 *&nt1, MTet4 *&nt2, MTet4 *&nt3) {
    // 获取共享面的顶点和非共享顶点
    MVertex *a, *b, *c, *d, *e;
    getSharedFaceVertices(t1, t2, sharedFace, a, b, c);
    d = getOppositeVertex(t1, sharedFace);
    e = getOppositeVertex(t2, sharedFace);

    // 检查新边de是否在凸壳内
    if (!isValidConfiguration(a, b, c, d, e)) {
        return false;
    }

    // 创建三个新四面体
    // (a, b, d, e), (b, c, d, e), (c, a, d, e)
    nt1 = new MTet4(a, b, d, e);
    nt2 = new MTet4(b, c, d, e);
    nt3 = new MTet4(c, a, d, e);

    // 更新邻接关系
    updateNeighbors(t1, t2, nt1, nt2, nt3);

    // 删除旧四面体
    delete t1;
    delete t2;

    return true;
}
```

### 8. 边折叠

```cpp
// 边折叠：将边(v1, v2)收缩为一个点
bool collapseEdge(MVertex *v1, MVertex *v2,
                  std::vector<MTet4*> &tets) {
    // 检查是否可以折叠
    // - 拓扑检查：不会改变拓扑
    // - 质量检查：折叠后质量不会太差

    // 收集所有包含v1或v2的四面体
    std::set<MTet4*> affectedTets;
    collectTetsContaining(v1, affectedTets);
    collectTetsContaining(v2, affectedTets);

    // 计算新顶点位置
    SPoint3 newPos = 0.5 * (v1->point() + v2->point());

    // 检查折叠后的质量
    for (MTet4 *t : affectedTets) {
        if (containsBothVertices(t, v1, v2)) {
            continue;  // 这个四面体会消失
        }

        // 模拟替换顶点后的质量
        double q = simulateQuality(t, v1, v2, newPos);
        if (q < MIN_QUALITY_THRESHOLD) {
            return false;  // 质量太差，拒绝
        }
    }

    // 执行折叠
    // 1. 移动v1到新位置
    v1->setXYZ(newPos.x(), newPos.y(), newPos.z());

    // 2. 将所有引用v2的地方改为v1
    replaceVertex(v2, v1, affectedTets);

    // 3. 删除包含重复顶点的四面体
    removeDegenerateTets(affectedTets);

    // 4. 删除v2
    delete v2;

    return true;
}
```

### 9. meshGRegionOptimize.cpp 源码分析

```cpp
// src/mesh/meshGRegionOptimize.cpp

// 主入口
void optimizeMeshGRegion(GRegion *gr);

// 翻转优化
void flipOptimize(GRegion *gr);

// 平滑
void smoothMeshGRegion(GRegion *gr, int niter);

// Netgen优化接口
void optimizeMeshNetgen(GRegion *gr);

// 组合优化策略
void optimizeMesh(GRegion *gr, int passes) {
    for (int i = 0; i < passes; i++) {
        // 平滑
        smoothMeshGRegion(gr, 3);

        // 翻转
        flipOptimize(gr);

        // 再次平滑
        smoothMeshGRegion(gr, 3);

        // 检查是否收敛
        if (hasConverged(gr)) break;
    }
}
```

### 10. 优化终止条件

```cpp
// 判断优化是否收敛
bool hasConverged(GRegion *gr) {
    static double prevMinQuality = 0;

    // 计算当前最小质量
    double minQ = 1e30;
    for (auto it = gr->tetrahedra.begin();
         it != gr->tetrahedra.end(); ++it) {
        double q = (*it)->minSICNShapeMeasure();
        minQ = std::min(minQ, q);
    }

    // 检查改进量
    double improvement = minQ - prevMinQuality;
    prevMinQuality = minQ;

    // 改进量小于阈值则认为收敛
    return improvement < 0.001;
}
```

---

## 练习作业

### 练习1：Laplacian平滑（基础）
```cpp
// 实现简化版2D Laplacian平滑
class SimpleMesh2D {
    std::vector<SPoint2> vertices;
    std::vector<std::array<int, 3>> triangles;
    std::set<int> boundaryVertices;

public:
    void laplacianSmooth(int iterations) {
        for (int iter = 0; iter < iterations; iter++) {
            std::vector<SPoint2> newPos(vertices.size());

            for (int i = 0; i < vertices.size(); i++) {
                if (boundaryVertices.count(i)) {
                    // TODO: 边界顶点不移动
                    newPos[i] = vertices[i];
                } else {
                    // TODO: 计算邻居质心
                    // newPos[i] = ...;
                }
            }

            vertices = newPos;
        }
    }
};
```

### 练习2：边翻转判断（进阶）
```cpp
// 实现Delaunay翻转判断
bool shouldFlipDelaunay(const SPoint2 &a, const SPoint2 &b,
                        const SPoint2 &c, const SPoint2 &d) {
    // a, b, c 是第一个三角形的顶点
    // b, a, d 是第二个三角形的顶点
    // 共享边是 (a, b)

    // TODO: 判断是否应该翻转
    // 提示：检查d是否在三角形abc的外接圆内

    return false;  // 实现判断
}

// 测试用例
void testFlip() {
    SPoint2 a(0, 0), b(2, 0), c(1, 2), d(1, -0.5);

    // d在abc外接圆内 → 应该翻转
    assert(shouldFlipDelaunay(a, b, c, d) == true);

    SPoint2 d2(1, -2);
    // d2在abc外接圆外 → 不应该翻转
    assert(shouldFlipDelaunay(a, b, c, d2) == false);
}
```

### 练习3：优化效果对比（挑战）
```cpp
// 对比不同优化方法的效果
class OptimizationComparison {
public:
    struct OptResult {
        double initialMinQ;
        double finalMinQ;
        double initialAvgQ;
        double finalAvgQ;
        int iterations;
        double time;
    };

    // TODO: 实现以下对比实验
    // 1. 生成一个低质量初始网格
    // 2. 分别应用：
    //    - 纯Laplacian平滑
    //    - 纯边翻转
    //    - 组合优化
    // 3. 记录并对比结果

    void runComparison(GFace *gf) {
        // 记录初始质量
        double initMin, initMax, initAvg;
        computeQualityStats(gf, initMin, initMax, initAvg);

        // TODO: 运行不同优化方法
        // TODO: 输出对比表格
    }
};
```

---

## 源码导航

### 核心文件
| 文件 | 内容 | 行数 |
|------|------|------|
| `src/mesh/meshGFaceOptimize.cpp` | 2D网格优化 | ~2000行 |
| `src/mesh/meshGRegionOptimize.cpp` | 3D网格优化 | ~1500行 |
| `src/mesh/meshOptimize.cpp` | 通用优化入口 | ~500行 |

### 关键函数
```cpp
// meshGFaceOptimize.cpp
void smoothMeshGFace(GFace *gf, int niter);
void edgeFlipGFace(GFace *gf);
void optimizeMeshGFaceLloyd(GFace *gf, int niter);

// meshGRegionOptimize.cpp
void smoothMeshGRegion(GRegion *gr, int niter);
void flipOptimize(GRegion *gr);
void collapseSmallEdges(GRegion *gr);

// meshOptimize.cpp
void OptimizeMesh(GModel *m);
```

---

## 今日检查点

- [ ] 理解Laplacian平滑的原理和局限性
- [ ] 能描述边翻转的几何操作
- [ ] 理解2-3翻转和3-2翻转的区别
- [ ] 了解边折叠操作的有效性检查
- [ ] 知道组合优化策略的一般流程

---

## 优化算法对比

| 方法 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| Laplacian平滑 | 简单快速 | 可能翻转单元 | 初步平滑 |
| 优化平滑 | 保证质量提升 | 计算量大 | 精细优化 |
| 边翻转 | 改善拓扑 | 不改变边界 | Delaunay化 |
| 边折叠 | 减少单元数 | 可能失去细节 | 简化网格 |

---

## 导航

- 上一天：[Day 46 - 网格质量度量](day-46.md)
- 下一天：[Day 48 - Netgen集成分析](day-48.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
