# Day 44：3D Delaunay实现深度分析

## 学习目标
深入分析Gmsh中3D Delaunay三角剖分的具体实现，理解点定位和空腔构建的关键算法。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 阅读delaunay3d.cpp整体架构 |
| 10:00-11:00 | 1h | 分析点定位（Walking）算法实现 |
| 11:00-12:00 | 1h | 研究空腔（Cavity）构建算法 |
| 14:00-15:00 | 1h | 分析新四面体创建与边界恢复 |

---

## 上午学习内容

### 1. delaunay3d.cpp架构概览

**文件位置**：`src/mesh/delaunay3d.cpp`

**核心数据结构**：
```cpp
// 四面体数据结构
class MTet4 {
private:
    MVertex *_v[4];           // 四个顶点
    MTet4 *_neigh[4];         // 四个相邻四面体
    bool _deleted;            // 删除标记
    int _gr;                  // 所属区域

public:
    // 获取顶点
    MVertex *getVertex(int i) const { return _v[i]; }

    // 获取对面（第i个顶点对应的面）
    void getFace(int i, MVertex *&v0, MVertex *&v1, MVertex *&v2) const {
        static int faces[4][3] = {{1,2,3}, {0,3,2}, {0,1,3}, {0,2,1}};
        v0 = _v[faces[i][0]];
        v1 = _v[faces[i][1]];
        v2 = _v[faces[i][2]];
    }

    // 检查点是否在外接球内
    bool inCircumSphere(const SPoint3 &p) const;

    // 获取邻居
    MTet4 *getNeigh(int i) const { return _neigh[i]; }
};
```

### 2. 点定位 - 行走算法

**基本思想**：
从任意四面体出发，沿着朝向目标点的方向移动到相邻四面体，直到找到包含目标点的四面体。

```cpp
// 行走定位算法
MTet4* walk(MTet4 *start, const SPoint3 &p,
            std::vector<MTet4*> &visited) {
    MTet4 *current = start;

    while (current != nullptr) {
        visited.push_back(current);

        // 检查p相对于每个面的位置
        bool found = true;
        for (int i = 0; i < 4; i++) {
            MVertex *v0, *v1, *v2;
            current->getFace(i, v0, v1, v2);

            // 计算点到面的有向距离
            double orient = robustPredicates::orient3d(
                v0->point(), v1->point(), v2->point(), p);

            if (orient < 0) {
                // p在面的外侧，移动到该邻居
                MTet4 *next = current->getNeigh(i);
                if (next != nullptr && !next->isDeleted()) {
                    current = next;
                    found = false;
                    break;
                }
            }
        }

        if (found) {
            // 所有面的orient >= 0，p在当前四面体内
            return current;
        }
    }

    return nullptr;  // 定位失败
}
```

### 3. orient3d谓词

**数学含义**：
判断点d相对于三角形abc的位置（在平面上方、下方还是在平面上）。

```cpp
// orient3d的计算
//     | ax-dx  ay-dy  az-dz |
// det | bx-dx  by-dy  bz-dz |
//     | cx-dx  cy-dy  cz-dz |
//
// > 0: d在abc平面的正侧（按右手法则）
// = 0: d在abc平面上
// < 0: d在abc平面的负侧

double orient3d(const SPoint3 &a, const SPoint3 &b,
                const SPoint3 &c, const SPoint3 &d) {
    double adx = a.x() - d.x();
    double ady = a.y() - d.y();
    double adz = a.z() - d.z();
    double bdx = b.x() - d.x();
    double bdy = b.y() - d.y();
    double bdz = b.z() - d.z();
    double cdx = c.x() - d.x();
    double cdy = c.y() - d.y();
    double cdz = c.z() - d.z();

    return adx * (bdy*cdz - bdz*cdy)
         + ady * (bdz*cdx - bdx*cdz)
         + adz * (bdx*cdy - bdy*cdx);
}
```

---

## 下午学习内容

### 4. 空腔构建算法

**核心思想**：
当插入新点p时，找出所有外接球包含p的四面体，这些四面体形成一个"空腔"。

```cpp
// 空腔构建（BFS方式）
void buildCavity(const SPoint3 &p, MTet4 *start,
                 std::vector<MTet4*> &cavity,
                 std::vector<MTet4*> &boundary) {
    std::queue<MTet4*> queue;
    std::set<MTet4*> visited;

    queue.push(start);
    visited.insert(start);

    while (!queue.empty()) {
        MTet4 *current = queue.front();
        queue.pop();

        if (current->inCircumSphere(p)) {
            // 当前四面体在空腔内
            cavity.push_back(current);

            // 检查邻居
            for (int i = 0; i < 4; i++) {
                MTet4 *neigh = current->getNeigh(i);
                if (neigh && visited.find(neigh) == visited.end()) {
                    visited.insert(neigh);
                    queue.push(neigh);
                }
            }
        } else {
            // 当前四面体不在空腔内，是边界
            boundary.push_back(current);
        }
    }
}
```

### 5. InSphere测试

**判断点是否在四面体的外接球内**：
```cpp
bool MTet4::inCircumSphere(const SPoint3 &p) const {
    // 使用insphere谓词
    // 返回值:
    //   > 0: p在外接球内部
    //   = 0: p在外接球上
    //   < 0: p在外接球外部

    double result = robustPredicates::insphere(
        _v[0]->point(),
        _v[1]->point(),
        _v[2]->point(),
        _v[3]->point(),
        p
    );

    return result > 0;
}

// insphere的行列式形式
//     | ax-ex  ay-ey  az-ez  (ax-ex)²+(ay-ey)²+(az-ez)² |
// det | bx-ex  by-ey  bz-ez  (bx-ex)²+(by-ey)²+(bz-ez)² |
//     | cx-ex  cy-ey  cz-ez  (cx-ex)²+(cy-ey)²+(cz-ez)² |
//     | dx-ex  dy-ey  dz-ez  (dx-ex)²+(dy-ey)²+(dz-ez)² |
```

### 6. 空腔边界提取

**提取空腔的边界面**：
```cpp
// 空腔边界由三角形面组成
struct BoundaryFace {
    MVertex *v[3];     // 三个顶点
    MTet4 *outsideTet; // 边界外侧的四面体
    int faceIdx;       // 在外侧四面体中的面索引
};

void extractCavityBoundary(const std::vector<MTet4*> &cavity,
                           std::vector<BoundaryFace> &faces) {
    std::set<MTet4*> cavitySet(cavity.begin(), cavity.end());

    for (MTet4 *tet : cavity) {
        for (int i = 0; i < 4; i++) {
            MTet4 *neigh = tet->getNeigh(i);

            // 如果邻居不在空腔内，则该面是边界面
            if (neigh == nullptr ||
                cavitySet.find(neigh) == cavitySet.end()) {
                BoundaryFace bf;
                tet->getFace(i, bf.v[0], bf.v[1], bf.v[2]);
                bf.outsideTet = neigh;
                bf.faceIdx = (neigh) ? neigh->findFace(tet) : -1;
                faces.push_back(bf);
            }
        }
    }
}
```

### 7. 新四面体创建

**从边界面和新点创建新四面体**：
```cpp
void fillCavity(MVertex *newVertex,
                const std::vector<BoundaryFace> &boundaryFaces,
                std::vector<MTet4*> &newTets) {
    // 为每个边界面创建新四面体
    for (const BoundaryFace &bf : boundaryFaces) {
        // 创建新四面体：边界面 + 新顶点
        MTet4 *newTet = new MTet4(bf.v[0], bf.v[1], bf.v[2], newVertex);
        newTets.push_back(newTet);

        // 设置与外部四面体的邻接关系
        if (bf.outsideTet) {
            newTet->setNeigh(3, bf.outsideTet);  // 假设新顶点是第3个
            bf.outsideTet->setNeigh(bf.faceIdx, newTet);
        }
    }

    // 设置新四面体之间的邻接关系
    connectNewTets(newTets);
}

void connectNewTets(std::vector<MTet4*> &tets) {
    // 使用哈希表快速查找共享面
    std::map<TripleFace, std::pair<MTet4*, int>> faceMap;

    for (MTet4 *tet : tets) {
        for (int i = 0; i < 3; i++) {  // 只检查包含新顶点的三个面
            TripleFace face = tet->getOrderedFace(i);
            auto it = faceMap.find(face);
            if (it != faceMap.end()) {
                // 找到共享面的另一个四面体
                MTet4 *other = it->second.first;
                int otherIdx = it->second.second;
                tet->setNeigh(i, other);
                other->setNeigh(otherIdx, tet);
            } else {
                faceMap[face] = {tet, i};
            }
        }
    }
}
```

### 8. 完整插入流程

```cpp
void insertVertex(MVertex *v, MTet4 *&startTet,
                  std::vector<MTet4*> &allTets) {
    SPoint3 p = v->point();

    // 1. 点定位
    std::vector<MTet4*> visited;
    MTet4 *containing = walk(startTet, p, visited);

    if (containing == nullptr) {
        // 点在当前网格外部
        return;
    }

    // 2. 构建空腔
    std::vector<MTet4*> cavity;
    std::vector<MTet4*> boundary;
    buildCavity(p, containing, cavity, boundary);

    // 3. 提取边界面
    std::vector<BoundaryFace> boundaryFaces;
    extractCavityBoundary(cavity, boundaryFaces);

    // 4. 删除空腔内的四面体
    for (MTet4 *tet : cavity) {
        tet->setDeleted(true);
    }

    // 5. 创建新四面体
    std::vector<MTet4*> newTets;
    fillCavity(v, boundaryFaces, newTets);

    // 6. 更新全局列表
    allTets.insert(allTets.end(), newTets.begin(), newTets.end());

    // 7. 更新起始四面体（用于下次定位）
    startTet = newTets[0];
}
```

---

## 练习作业

### 练习1：理解点定位（基础）
```cpp
// 实现简化版的2D行走定位
class SimpleTriangulation {
    std::vector<Triangle*> triangles;

public:
    // 从start三角形开始，找到包含点p的三角形
    Triangle* locate(Triangle *start, const SPoint2 &p) {
        // TODO: 实现行走定位算法
        // 提示：使用orient2d判断p在边的哪一侧
    }
};
```

### 练习2：空腔可视化（进阶）
```cpp
// 在2D Delaunay中可视化空腔构建过程
class CavityVisualizer {
public:
    void visualize(const SPoint2 &newPoint,
                   const std::vector<Triangle*> &cavity) {
        // TODO: 输出.geo文件可视化空腔
        // 1. 显示所有三角形
        // 2. 高亮显示空腔三角形（外接圆包含newPoint）
        // 3. 显示新点位置
    }
};
```

### 练习3：追踪源码（挑战）
在debugger中追踪delaunay3d.cpp的执行：

1. 设置断点：
   ```bash
   # 编译调试版本
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make

   # 使用gdb/lldb
   lldb ./gmsh
   (lldb) breakpoint set -f delaunay3d.cpp -l 300  # 设在insertVertex附近
   (lldb) run ../tutorials/t1.geo -3
   ```

2. 观察：
   - [ ] 记录一次完整的点插入过程
   - [ ] 观察空腔包含多少个四面体
   - [ ] 新创建了多少个四面体

---

## 源码导航

### 核心文件
| 文件 | 内容 | 行数 |
|------|------|------|
| `src/mesh/delaunay3d.cpp` | 3D Delaunay主实现 | ~1500行 |
| `src/mesh/delaunay3d.h` | 头文件定义 | ~200行 |
| `src/numeric/robustPredicates.cpp` | 几何谓词 | ~4000行 |

### 关键函数
```cpp
// delaunay3d.cpp
insertVertexDelaunay3d()     // 点插入入口
walkTet()                    // 行走定位
buildCavity()                // 空腔构建
fillCavity()                 // 空腔填充

// robustPredicates.cpp
orient3d()                   // 方向判断
insphere()                   // 外接球判断
```

### 数据结构继承
```
MTet4
├── 包含4个MVertex*
├── 包含4个MTet4*邻居
└── 方法：
    ├── inCircumSphere()
    ├── getVertex()
    ├── getNeigh()
    └── setNeigh()
```

---

## 今日检查点

- [ ] 能描述行走定位算法的工作原理
- [ ] 理解orient3d谓词的数学含义
- [ ] 能解释空腔构建的过程
- [ ] 理解insphere谓词的作用
- [ ] 能描述新四面体创建和邻接关系建立的过程

---

## 算法复杂度分析

| 操作 | 平均复杂度 | 最坏复杂度 |
|------|------------|------------|
| 点定位 | O(n^1/3) | O(n) |
| 空腔构建 | O(1) 平均 | O(n) |
| 单点插入 | O(log n) 平均 | O(n) |
| 总体构建 | O(n log n) | O(n²) |

---

## 导航

- 上一天：[Day 43 - Delaunay算法理论基础](day-43.md)
- 下一天：[Day 45 - 几何谓词与数值稳定性](day-45.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
