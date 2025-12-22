# Day 59：数据插值与空间查询

## 学习目标
掌握Gmsh后处理模块的空间索引结构和数据插值算法，理解OctreePost和shapeFunctions的实现原理。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习空间索引结构概述 |
| 10:00-11:00 | 1h | 阅读OctreePost实现 |
| 11:00-12:00 | 1h | 分析形状函数与插值 |
| 14:00-15:00 | 1h | 完成练习并总结 |

---

## 上午学习内容

### 1. 空间查询需求分析

**后处理中的空间查询场景**：

```
场景1：点值查询
├── 输入：空间坐标(x, y, z)
├── 输出：该点处的场值
└── 挑战：快速定位包含该点的单元

场景2：等值线/等值面提取
├── 输入：等值线值
├── 输出：等值线上的点集
└── 挑战：遍历所有单元的交点

场景3：粒子追踪
├── 输入：初始位置和速度场
├── 输出：粒子轨迹
└── 挑战：连续的空间定位

场景4：探针采样
├── 输入：采样线/面
├── 输出：沿线/面的场值分布
└── 挑战：批量空间查询
```

**朴素查询 vs 空间索引**：

```
朴素查询（遍历所有单元）：
├── 时间复杂度：O(N) per query
├── N = 单元数量（可能数百万）
└── 不适合交互式可视化

空间索引（八叉树）：
├── 构建时间：O(N log N)
├── 查询时间：O(log N) per query
└── 适合频繁查询场景
```

### 2. OctreePost八叉树

**文件位置**：`src/post/OctreePost.h`

```cpp
class OctreePost {
private:
    // 八叉树根节点
    Octree *_octree;

    // 关联的后处理数据
    PViewData *_theViewData;

    // 当前时间步
    int _theCurrentTimeStep;

    // 缓存：最近查询到的单元
    int _lastElement;

public:
    OctreePost(PViewData *data);
    ~OctreePost();

    // 标量/矢量/张量查询
    bool searchScalar(double x, double y, double z,
                      double *values, int step = -1,
                      double *size = nullptr,
                      int qn = 0, double *qx = nullptr,
                      double *qy = nullptr, double *qz = nullptr,
                      bool grad = false, int dim = -1);

    bool searchVector(double x, double y, double z,
                      double *values, int step = -1, ...);

    bool searchTensor(double x, double y, double z,
                      double *values, int step = -1, ...);

    // 在指定单元内插值
    bool searchScalarWithTol(double x, double y, double z,
                             double *values, int step,
                             double *size, double tol,
                             int qn, double *qx, double *qy, double *qz,
                             bool grad, int dim);
};
```

### 3. 八叉树结构

**基础数据结构**：

```cpp
// 八叉树节点（简化表示）
struct OctreeNode {
    // 节点边界
    double xmin, xmax;
    double ymin, ymax;
    double zmin, zmax;

    // 子节点（最多8个）
    OctreeNode *children[8];

    // 叶节点存储的单元列表
    std::vector<int> elements;

    // 节点深度
    int depth;

    // 是否为叶节点
    bool isLeaf() const { return children[0] == nullptr; }
};
```

**八叉树空间划分**：

```
                    根节点（整个包围盒）
                           |
    ┌──────┬──────┬──────┬──────┬──────┬──────┬──────┬──────┐
    │      │      │      │      │      │      │      │      │
   [0]    [1]    [2]    [3]    [4]    [5]    [6]    [7]

子节点编号与空间位置对应：
  [0] = (xmin, xmid) × (ymin, ymid) × (zmin, zmid)  左下前
  [1] = (xmid, xmax) × (ymin, ymid) × (zmin, zmid)  右下前
  [2] = (xmin, xmid) × (ymid, ymax) × (zmin, zmid)  左上前
  [3] = (xmid, xmax) × (ymid, ymax) × (zmin, zmid)  右上前
  [4] = (xmin, xmid) × (ymin, ymid) × (zmid, zmax)  左下后
  [5] = (xmid, xmax) × (ymin, ymid) × (zmid, zmax)  右下后
  [6] = (xmin, xmid) × (ymid, ymax) × (zmid, zmax)  左上后
  [7] = (xmid, xmax) × (ymid, ymax) × (zmid, zmax)  右上后
```

### 4. 八叉树查询算法

```cpp
// 查询包含点(x,y,z)的单元
int OctreePost::findElement(double x, double y, double z) {
    // 1. 从根节点开始
    OctreeNode *node = root;

    // 2. 递归下降到叶节点
    while(!node->isLeaf()) {
        // 计算点应该在哪个子节点
        int idx = getChildIndex(node, x, y, z);
        if(node->children[idx] == nullptr) {
            return -1;  // 点不在任何单元内
        }
        node = node->children[idx];
    }

    // 3. 在叶节点中遍历候选单元
    for(int elemId : node->elements) {
        if(isPointInElement(elemId, x, y, z)) {
            return elemId;
        }
    }

    return -1;  // 未找到
}

// 计算子节点索引
int getChildIndex(OctreeNode *node, double x, double y, double z) {
    double xmid = (node->xmin + node->xmax) / 2;
    double ymid = (node->ymin + node->ymax) / 2;
    double zmid = (node->zmin + node->zmax) / 2;

    int idx = 0;
    if(x > xmid) idx |= 1;
    if(y > ymid) idx |= 2;
    if(z > zmid) idx |= 4;

    return idx;
}
```

---

## 下午学习内容

### 5. 形状函数与插值

**文件位置**：`src/post/shapeFunctions.h`

```cpp
// 形状函数基类
class shapeFunctions {
public:
    // 获取单元类型对应的形状函数
    static void getShapeFunctions(int type, double u, double v, double w,
                                  double *sf);

    // 获取形状函数的梯度
    static void getGradShapeFunctions(int type, double u, double v, double w,
                                      double (*gsf)[3]);

    // 计算参数坐标到物理坐标的映射
    static void xyz2uvw(int type, double *xyz, double *node_xyz,
                        double *uvw, double tol = 1e-8);
};
```

**单元类型与形状函数**：

```
线性单元形状函数：

三角形（3节点）：
  N1 = 1 - u - v
  N2 = u
  N3 = v

四边形（4节点）：
  N1 = (1-u)(1-v)/4
  N2 = (1+u)(1-v)/4
  N3 = (1+u)(1+v)/4
  N4 = (1-u)(1+v)/4

四面体（4节点）：
  N1 = 1 - u - v - w
  N2 = u
  N3 = v
  N4 = w

六面体（8节点）：
  Ni = (1±u)(1±v)(1±w)/8
```

### 6. 点在单元内判断

```cpp
// 判断点是否在三角形内（使用重心坐标）
bool pointInTriangle(double x, double y, double z,
                     double *p1, double *p2, double *p3,
                     double &u, double &v) {
    // 计算三角形的两条边向量
    double e1[3] = {p2[0]-p1[0], p2[1]-p1[1], p2[2]-p1[2]};
    double e2[3] = {p3[0]-p1[0], p3[1]-p1[1], p3[2]-p1[2]};

    // 计算点到p1的向量
    double ep[3] = {x-p1[0], y-p1[1], z-p1[2]};

    // 计算点积
    double d00 = e1[0]*e1[0] + e1[1]*e1[1] + e1[2]*e1[2];
    double d01 = e1[0]*e2[0] + e1[1]*e2[1] + e1[2]*e2[2];
    double d11 = e2[0]*e2[0] + e2[1]*e2[1] + e2[2]*e2[2];
    double d20 = ep[0]*e1[0] + ep[1]*e1[1] + ep[2]*e1[2];
    double d21 = ep[0]*e2[0] + ep[1]*e2[1] + ep[2]*e2[2];

    // 求解重心坐标
    double denom = d00*d11 - d01*d01;
    u = (d11*d20 - d01*d21) / denom;
    v = (d00*d21 - d01*d20) / denom;

    // 检查是否在三角形内
    double tol = 1e-10;
    return (u >= -tol) && (v >= -tol) && (u + v <= 1 + tol);
}
```

### 7. 数据插值流程

```cpp
// 在单元内插值场值
bool interpolateInElement(int elemId, double x, double y, double z,
                          double *values, int numComp) {
    // 1. 获取单元节点坐标
    std::vector<double> nodeXYZ;
    getElementNodes(elemId, nodeXYZ);

    // 2. 计算参数坐标(u,v,w)
    double uvw[3];
    int elemType = getElementType(elemId);
    shapeFunctions::xyz2uvw(elemType,
                            new double[3]{x,y,z},
                            nodeXYZ.data(),
                            uvw);

    // 3. 计算形状函数值
    int numNodes = getNumNodes(elemType);
    std::vector<double> sf(numNodes);
    shapeFunctions::getShapeFunctions(elemType, uvw[0], uvw[1], uvw[2],
                                       sf.data());

    // 4. 获取节点上的场值
    std::vector<std::vector<double>> nodeValues(numNodes);
    for(int i = 0; i < numNodes; i++) {
        getNodeValues(elemId, i, nodeValues[i]);
    }

    // 5. 插值
    for(int c = 0; c < numComp; c++) {
        values[c] = 0;
        for(int i = 0; i < numNodes; i++) {
            values[c] += sf[i] * nodeValues[i][c];
        }
    }

    return true;
}
```

**插值流程示意**：

```
物理坐标(x,y,z) → 参数坐标(u,v,w) → 形状函数N(u,v,w) → 插值结果

               xyz2uvw()          getShapeFunctions()
    (x,y,z) ──────────────→ (u,v,w) ──────────────────→ [N1,N2,...,Nn]
                                                            │
                                                            │ Σ Ni×Vi
                                                            ↓
                                                      value = Σ Ni×Vi
```

---

## 练习作业

### 基础练习

**练习1**：理解八叉树查询过程

```cpp
// 追踪以下查询过程
OctreePost *octree = new OctreePost(viewData);

double values[9];  // 最多9分量（张量）
double point[3] = {0.5, 0.3, 0.1};

// 查询标量值
bool found = octree->searchScalar(point[0], point[1], point[2],
                                   values, 0,  // step 0
                                   nullptr,    // size
                                   0, nullptr, nullptr, nullptr,  // query points
                                   false,      // no gradient
                                   -1);        // any dimension

// 问题：
// 1. searchScalar内部如何使用八叉树？
// 2. 如果点在单元边界上会发生什么？
// 3. step参数的作用是什么？
```

### 进阶练习

**练习2**：实现简单的空间索引

```cpp
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

// 2D包围盒
struct BBox2D {
    double xmin, xmax, ymin, ymax;

    BBox2D() : xmin(1e30), xmax(-1e30), ymin(1e30), ymax(-1e30) {}

    BBox2D(double x0, double x1, double y0, double y1)
        : xmin(x0), xmax(x1), ymin(y0), ymax(y1) {}

    bool contains(double x, double y) const {
        return x >= xmin && x <= xmax && y >= ymin && y <= ymax;
    }

    void expand(double x, double y) {
        xmin = std::min(xmin, x);
        xmax = std::max(xmax, x);
        ymin = std::min(ymin, y);
        ymax = std::max(ymax, y);
    }
};

// 简单的四叉树节点
class QuadTreeNode {
public:
    BBox2D bbox;
    std::vector<int> elements;  // 叶节点存储的单元ID
    QuadTreeNode *children[4];   // 子节点
    int depth;

    static const int MAX_DEPTH = 8;
    static const int MAX_ELEMENTS = 10;

    QuadTreeNode(const BBox2D &b, int d = 0)
        : bbox(b), depth(d) {
        for(int i = 0; i < 4; i++) children[i] = nullptr;
    }

    ~QuadTreeNode() {
        for(int i = 0; i < 4; i++) {
            if(children[i]) delete children[i];
        }
    }

    bool isLeaf() const { return children[0] == nullptr; }

    // 插入单元（使用单元中心点）
    void insert(int elemId, double cx, double cy,
                const std::vector<BBox2D> &elemBBoxes) {
        if(!bbox.contains(cx, cy)) return;

        if(isLeaf()) {
            elements.push_back(elemId);

            // 如果元素过多且深度不够，则分裂
            if(elements.size() > MAX_ELEMENTS && depth < MAX_DEPTH) {
                subdivide(elemBBoxes);
            }
        } else {
            // 找到合适的子节点
            int idx = getChildIndex(cx, cy);
            children[idx]->insert(elemId, cx, cy, elemBBoxes);
        }
    }

    // 查询包含点(x,y)的候选单元
    void query(double x, double y, std::vector<int> &candidates) const {
        if(!bbox.contains(x, y)) return;

        if(isLeaf()) {
            candidates.insert(candidates.end(),
                              elements.begin(), elements.end());
        } else {
            int idx = getChildIndex(x, y);
            if(children[idx]) {
                children[idx]->query(x, y, candidates);
            }
        }
    }

private:
    int getChildIndex(double x, double y) const {
        double xmid = (bbox.xmin + bbox.xmax) / 2;
        double ymid = (bbox.ymin + bbox.ymax) / 2;

        int idx = 0;
        if(x > xmid) idx |= 1;
        if(y > ymid) idx |= 2;
        return idx;
    }

    void subdivide(const std::vector<BBox2D> &elemBBoxes) {
        double xmid = (bbox.xmin + bbox.xmax) / 2;
        double ymid = (bbox.ymin + bbox.ymax) / 2;

        children[0] = new QuadTreeNode(
            BBox2D(bbox.xmin, xmid, bbox.ymin, ymid), depth + 1);
        children[1] = new QuadTreeNode(
            BBox2D(xmid, bbox.xmax, bbox.ymin, ymid), depth + 1);
        children[2] = new QuadTreeNode(
            BBox2D(bbox.xmin, xmid, ymid, bbox.ymax), depth + 1);
        children[3] = new QuadTreeNode(
            BBox2D(xmid, bbox.xmax, ymid, bbox.ymax), depth + 1);

        // 重新分配元素
        for(int elemId : elements) {
            double cx = (elemBBoxes[elemId].xmin + elemBBoxes[elemId].xmax) / 2;
            double cy = (elemBBoxes[elemId].ymin + elemBBoxes[elemId].ymax) / 2;
            int idx = getChildIndex(cx, cy);
            children[idx]->elements.push_back(elemId);
        }
        elements.clear();
    }
};

// 三角形网格的空间索引
class TriangleMeshIndex {
    struct Triangle {
        std::array<double, 2> p[3];
        int id;

        BBox2D getBBox() const {
            BBox2D b;
            for(int i = 0; i < 3; i++) {
                b.expand(p[i][0], p[i][1]);
            }
            return b;
        }

        // 获取中心点
        void getCenter(double &cx, double &cy) const {
            cx = (p[0][0] + p[1][0] + p[2][0]) / 3;
            cy = (p[0][1] + p[1][1] + p[2][1]) / 3;
        }

        // 检查点是否在三角形内
        bool contains(double x, double y, double &u, double &v) const {
            double v0[2] = {p[2][0] - p[0][0], p[2][1] - p[0][1]};
            double v1[2] = {p[1][0] - p[0][0], p[1][1] - p[0][1]};
            double v2[2] = {x - p[0][0], y - p[0][1]};

            double dot00 = v0[0]*v0[0] + v0[1]*v0[1];
            double dot01 = v0[0]*v1[0] + v0[1]*v1[1];
            double dot02 = v0[0]*v2[0] + v0[1]*v2[1];
            double dot11 = v1[0]*v1[0] + v1[1]*v1[1];
            double dot12 = v1[0]*v2[0] + v1[1]*v2[1];

            double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
            u = (dot11 * dot02 - dot01 * dot12) * invDenom;
            v = (dot00 * dot12 - dot01 * dot02) * invDenom;

            double tol = 1e-10;
            return (u >= -tol) && (v >= -tol) && (u + v <= 1 + tol);
        }
    };

    std::vector<Triangle> triangles;
    std::vector<BBox2D> bboxes;
    QuadTreeNode *root;

public:
    TriangleMeshIndex() : root(nullptr) {}

    ~TriangleMeshIndex() {
        if(root) delete root;
    }

    // 添加三角形
    void addTriangle(int id, double x0, double y0,
                     double x1, double y1, double x2, double y2) {
        Triangle t;
        t.id = id;
        t.p[0] = {x0, y0};
        t.p[1] = {x1, y1};
        t.p[2] = {x2, y2};
        triangles.push_back(t);
        bboxes.push_back(t.getBBox());
    }

    // 构建四叉树
    void build() {
        if(triangles.empty()) return;

        // 计算总包围盒
        BBox2D total;
        for(const auto &t : triangles) {
            for(int i = 0; i < 3; i++) {
                total.expand(t.p[i][0], t.p[i][1]);
            }
        }

        // 稍微扩大一点
        double dx = (total.xmax - total.xmin) * 0.01;
        double dy = (total.ymax - total.ymin) * 0.01;
        total.xmin -= dx; total.xmax += dx;
        total.ymin -= dy; total.ymax += dy;

        // 创建根节点并插入所有三角形
        root = new QuadTreeNode(total);
        for(size_t i = 0; i < triangles.size(); i++) {
            double cx, cy;
            triangles[i].getCenter(cx, cy);
            root->insert(i, cx, cy, bboxes);
        }
    }

    // 查找包含点(x,y)的三角形
    int findTriangle(double x, double y, double &u, double &v) const {
        if(!root) return -1;

        // 获取候选三角形
        std::vector<int> candidates;
        root->query(x, y, candidates);

        // 精确检查每个候选
        for(int idx : candidates) {
            if(triangles[idx].contains(x, y, u, v)) {
                return triangles[idx].id;
            }
        }

        return -1;
    }

    // 统计信息
    void printStats() const {
        std::cout << "TriangleMeshIndex:\n";
        std::cout << "  Triangles: " << triangles.size() << "\n";
        if(root) {
            int leafCount = 0, maxDepth = 0;
            countNodes(root, leafCount, maxDepth);
            std::cout << "  Leaf nodes: " << leafCount << "\n";
            std::cout << "  Max depth: " << maxDepth << "\n";
        }
    }

private:
    void countNodes(const QuadTreeNode *node, int &leafCount, int &maxDepth) const {
        if(node->isLeaf()) {
            leafCount++;
            maxDepth = std::max(maxDepth, node->depth);
        } else {
            for(int i = 0; i < 4; i++) {
                if(node->children[i]) {
                    countNodes(node->children[i], leafCount, maxDepth);
                }
            }
        }
    }
};

int main() {
    TriangleMeshIndex index;

    // 创建一个简单的三角形网格
    // 4x4的正方形区域，划分为三角形
    int triId = 0;
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            double x0 = i * 0.25;
            double y0 = j * 0.25;
            double x1 = (i + 1) * 0.25;
            double y1 = (j + 1) * 0.25;

            // 每个正方形分成两个三角形
            index.addTriangle(triId++, x0, y0, x1, y0, x1, y1);
            index.addTriangle(triId++, x0, y0, x1, y1, x0, y1);
        }
    }

    // 构建索引
    index.build();
    index.printStats();

    // 测试查询
    std::cout << "\nQuery tests:\n";
    double testPoints[][2] = {
        {0.1, 0.1}, {0.5, 0.5}, {0.9, 0.9}, {0.3, 0.7}
    };

    for(auto &pt : testPoints) {
        double u, v;
        int triId = index.findTriangle(pt[0], pt[1], u, v);
        std::cout << "  Point (" << pt[0] << ", " << pt[1] << "): ";
        if(triId >= 0) {
            std::cout << "Triangle " << triId
                      << ", barycentric (" << u << ", " << v << ")\n";
        } else {
            std::cout << "Not found\n";
        }
    }

    return 0;
}
```

**练习3**：理解形状函数插值

```cpp
#include <iostream>
#include <vector>
#include <cmath>

// 三角形形状函数
class TriangleShapeFunctions {
public:
    // 计算线性形状函数
    static void linear(double u, double v, double *N) {
        N[0] = 1 - u - v;
        N[1] = u;
        N[2] = v;
    }

    // 计算二次形状函数（6节点三角形）
    static void quadratic(double u, double v, double *N) {
        double w = 1 - u - v;
        N[0] = w * (2*w - 1);  // 顶点0
        N[1] = u * (2*u - 1);  // 顶点1
        N[2] = v * (2*v - 1);  // 顶点2
        N[3] = 4 * w * u;       // 边01中点
        N[4] = 4 * u * v;       // 边12中点
        N[5] = 4 * v * w;       // 边20中点
    }

    // 计算形状函数的梯度
    static void linearGrad(double u, double v, double (*dN)[2]) {
        // dN[i][0] = dNi/du, dN[i][1] = dNi/dv
        dN[0][0] = -1; dN[0][1] = -1;
        dN[1][0] =  1; dN[1][1] =  0;
        dN[2][0] =  0; dN[2][1] =  1;
    }
};

// 在三角形内插值
class TriangleInterpolator {
    // 节点坐标
    double x[3], y[3];
    // 节点值
    std::vector<double> values[3];
    int numComp;

public:
    TriangleInterpolator(int nComp) : numComp(nComp) {}

    void setNode(int i, double px, double py, const std::vector<double> &v) {
        x[i] = px;
        y[i] = py;
        values[i] = v;
    }

    // 从物理坐标计算参数坐标
    bool xyz2uv(double px, double py, double &u, double &v) {
        // 使用Cramer法则求解
        double det = (y[1]-y[2])*(x[0]-x[2]) + (x[2]-x[1])*(y[0]-y[2]);
        if(std::abs(det) < 1e-15) return false;

        u = ((y[1]-y[2])*(px-x[2]) + (x[2]-x[1])*(py-y[2])) / det;
        v = ((y[2]-y[0])*(px-x[2]) + (x[0]-x[2])*(py-y[2])) / det;

        return true;
    }

    // 在指定点插值
    bool interpolate(double px, double py, std::vector<double> &result) {
        double u, v;
        if(!xyz2uv(px, py, u, v)) return false;

        // 检查是否在三角形内
        double tol = 1e-10;
        if(u < -tol || v < -tol || u + v > 1 + tol) return false;

        // 计算形状函数
        double N[3];
        TriangleShapeFunctions::linear(u, v, N);

        // 插值
        result.resize(numComp, 0.0);
        for(int c = 0; c < numComp; c++) {
            for(int i = 0; i < 3; i++) {
                result[c] += N[i] * values[i][c];
            }
        }

        return true;
    }

    // 计算梯度
    bool gradient(double px, double py, std::vector<std::vector<double>> &grad) {
        double u, v;
        if(!xyz2uv(px, py, u, v)) return false;

        // 计算形状函数梯度（相对于u,v）
        double dNduvw[3][2];
        TriangleShapeFunctions::linearGrad(u, v, dNduvw);

        // 计算Jacobian矩阵
        double J[2][2] = {{0,0}, {0,0}};
        for(int i = 0; i < 3; i++) {
            J[0][0] += dNduvw[i][0] * x[i];  // dx/du
            J[0][1] += dNduvw[i][1] * x[i];  // dx/dv
            J[1][0] += dNduvw[i][0] * y[i];  // dy/du
            J[1][1] += dNduvw[i][1] * y[i];  // dy/dv
        }

        // 计算Jacobian逆矩阵
        double detJ = J[0][0]*J[1][1] - J[0][1]*J[1][0];
        if(std::abs(detJ) < 1e-15) return false;

        double Jinv[2][2];
        Jinv[0][0] =  J[1][1] / detJ;
        Jinv[0][1] = -J[0][1] / detJ;
        Jinv[1][0] = -J[1][0] / detJ;
        Jinv[1][1] =  J[0][0] / detJ;

        // 计算形状函数相对于x,y的梯度
        double dNdxy[3][2];
        for(int i = 0; i < 3; i++) {
            dNdxy[i][0] = Jinv[0][0]*dNduvw[i][0] + Jinv[0][1]*dNduvw[i][1];
            dNdxy[i][1] = Jinv[1][0]*dNduvw[i][0] + Jinv[1][1]*dNduvw[i][1];
        }

        // 计算场的梯度
        grad.resize(numComp, std::vector<double>(2, 0.0));
        for(int c = 0; c < numComp; c++) {
            for(int i = 0; i < 3; i++) {
                grad[c][0] += dNdxy[i][0] * values[i][c];  // df/dx
                grad[c][1] += dNdxy[i][1] * values[i][c];  // df/dy
            }
        }

        return true;
    }
};

int main() {
    // 创建一个三角形
    TriangleInterpolator interp(1);  // 1个分量（标量）

    // 设置节点（等边三角形）
    interp.setNode(0, 0.0, 0.0, {100.0});  // 节点0: T=100
    interp.setNode(1, 1.0, 0.0, {200.0});  // 节点1: T=200
    interp.setNode(2, 0.5, 0.866, {150.0}); // 节点2: T=150

    std::cout << "Triangle interpolation test:\n";

    // 在不同点插值
    double testPoints[][2] = {
        {0.0, 0.0},      // 节点0
        {1.0, 0.0},      // 节点1
        {0.5, 0.866},    // 节点2
        {0.5, 0.289},    // 重心
        {0.5, 0.0},      // 边01中点
    };

    for(auto &pt : testPoints) {
        std::vector<double> val;
        if(interp.interpolate(pt[0], pt[1], val)) {
            std::cout << "  Point (" << pt[0] << ", " << pt[1] << "): "
                      << "T = " << val[0] << "\n";
        } else {
            std::cout << "  Point (" << pt[0] << ", " << pt[1] << "): "
                      << "Outside triangle\n";
        }
    }

    // 计算梯度
    std::cout << "\nGradient at centroid:\n";
    std::vector<std::vector<double>> grad;
    if(interp.gradient(0.5, 0.289, grad)) {
        std::cout << "  dT/dx = " << grad[0][0] << "\n";
        std::cout << "  dT/dy = " << grad[0][1] << "\n";
    }

    return 0;
}
```

### 挑战练习

**练习4**：实现探针采样功能

```cpp
#include <iostream>
#include <vector>
#include <cmath>

// 沿线段采样
class LineSampler {
public:
    struct Sample {
        double x, y, z;       // 位置
        double t;             // 参数[0,1]
        double value;         // 采样值
        bool valid;           // 是否有效
    };

private:
    // 采样线起点和终点
    double start[3], end[3];

    // 采样结果
    std::vector<Sample> samples;

public:
    LineSampler(double x0, double y0, double z0,
                double x1, double y1, double z1) {
        start[0] = x0; start[1] = y0; start[2] = z0;
        end[0] = x1; end[1] = y1; end[2] = z1;
    }

    // 均匀采样
    void sampleUniform(int numSamples) {
        samples.clear();
        samples.resize(numSamples);

        for(int i = 0; i < numSamples; i++) {
            double t = double(i) / (numSamples - 1);
            samples[i].t = t;
            samples[i].x = start[0] + t * (end[0] - start[0]);
            samples[i].y = start[1] + t * (end[1] - start[1]);
            samples[i].z = start[2] + t * (end[2] - start[2]);
            samples[i].valid = false;
        }
    }

    // 模拟从数据场中查询值
    void queryField(std::function<bool(double,double,double,double&)> fieldQuery) {
        for(auto &s : samples) {
            s.valid = fieldQuery(s.x, s.y, s.z, s.value);
        }
    }

    // 打印结果
    void printResults() const {
        std::cout << "Line samples from ("
                  << start[0] << "," << start[1] << "," << start[2]
                  << ") to ("
                  << end[0] << "," << end[1] << "," << end[2] << "):\n";

        for(const auto &s : samples) {
            std::cout << "  t=" << s.t << " pos=(" << s.x << "," << s.y << ")";
            if(s.valid) {
                std::cout << " value=" << s.value;
            } else {
                std::cout << " [invalid]";
            }
            std::cout << "\n";
        }
    }

    // 计算沿线的积分（梯形法则）
    double integrate() const {
        double result = 0;
        double length = std::sqrt(
            std::pow(end[0]-start[0], 2) +
            std::pow(end[1]-start[1], 2) +
            std::pow(end[2]-start[2], 2));

        for(size_t i = 0; i < samples.size() - 1; i++) {
            if(samples[i].valid && samples[i+1].valid) {
                double dt = samples[i+1].t - samples[i].t;
                result += 0.5 * (samples[i].value + samples[i+1].value) * dt * length;
            }
        }

        return result;
    }

    // 获取样本
    const std::vector<Sample>& getSamples() const { return samples; }
};

int main() {
    // 创建一个简单的测试场
    // f(x,y,z) = sin(pi*x) * cos(pi*y)
    auto testField = [](double x, double y, double z, double &value) -> bool {
        // 只在[0,1]×[0,1]区域内有效
        if(x < 0 || x > 1 || y < 0 || y > 1) return false;
        value = std::sin(M_PI * x) * std::cos(M_PI * y);
        return true;
    };

    // 沿对角线采样
    LineSampler sampler(0, 0, 0, 1, 1, 0);
    sampler.sampleUniform(11);
    sampler.queryField(testField);
    sampler.printResults();

    std::cout << "\nLine integral: " << sampler.integrate() << "\n";

    return 0;
}
```

---

## 知识图谱

```
数据插值与空间查询
│
├── 空间索引结构
│   ├── OctreePost（八叉树）
│   ├── QuadTree（四叉树，2D）
│   └── R-Tree（可选）
│
├── 八叉树操作
│   ├── 构建（O(N log N)）
│   ├── 查询（O(log N)）
│   └── 更新（增量）
│
├── 形状函数
│   ├── 线性（三角形、四面体）
│   ├── 二次（高阶单元）
│   └── Lagrange/层次基
│
├── 插值流程
│   ├── xyz → uvw（参数坐标）
│   ├── 形状函数计算
│   └── 加权求和
│
└── 应用场景
    ├── 点值查询
    ├── 等值线提取
    ├── 粒子追踪
    └── 探针采样
```

---

## 关键源码索引

| 文件 | 核心内容 | 代码量 |
|------|----------|--------|
| OctreePost.h | 八叉树类定义 | ~100行 |
| OctreePost.cpp | 八叉树实现 | ~600行 |
| shapeFunctions.h | 形状函数声明 | ~150行 |
| shapeFunctions.cpp | 形状函数实现 | ~1000行 |
| PViewDataGModel.cpp | 数据查询实现 | ~1500行 |

---

## 今日检查点

- [ ] 理解八叉树空间索引的结构和原理
- [ ] 能解释八叉树查询的时间复杂度优势
- [ ] 理解形状函数在插值中的作用
- [ ] 能描述从物理坐标到参数坐标的转换
- [ ] 理解数据插值的完整流程

---

## 导航

- 上一天：[Day 58 - PView视图系统](day-58.md)
- 下一天：[Day 60 - 可视化渲染](day-60.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
