/**
 * ============================================================================
 * 习题 1.3：平面分割数据结构与直线相交查询
 * 解法三：R-Tree 版本
 * ============================================================================
 *
 * 【思考过程记录】
 *
 * 问题回顾：
 * ----------
 * 在解法一和解法二中，我们分别使用了：
 * - 均匀网格：简单但对非均匀分布效果不佳
 * - 射线行走：理论最优但需要完整拓扑信息
 *
 * R-Tree 的优势：
 * ----------
 * 1. 自适应性强：自动适应数据的空间分布
 * 2. 通用性好：不需要完整的拓扑信息
 * 3. 工业标准：广泛应用于空间数据库和GIS系统
 *
 * R-Tree 的核心思想：
 * ----------
 * R-Tree 是一种平衡树，专门用于索引多维空间数据：
 *
 * 1. 每个节点对应一个最小包围矩形（MBR，Minimum Bounding Rectangle）
 * 2. 叶子节点存储实际数据（多边形的包围盒）
 * 3. 内部节点的 MBR 包含其所有子节点的 MBR
 * 4. 查询时从根节点开始，只遍历与查询区域相交的子树
 *
 * 树结构示意：
 *         [根节点 MBR]
 *        /           \
 *   [子树1 MBR]    [子树2 MBR]
 *    /    \          /    \
 *  [面A] [面B]    [面C] [面D]
 *
 * 查询算法：
 * ----------
 * 1. 计算查询直线的包围盒
 * 2. 从根节点开始递归搜索
 * 3. 对于每个节点：
 *    - 如果节点 MBR 与查询包围盒不相交，跳过整个子树（剪枝）
 *    - 如果相交，递归检查子节点
 * 4. 对于候选的叶子节点（面），进行精确相交测试
 *
 * 时间复杂度：
 * ----------
 * - 构建：O(n log n)
 * - 查询：O(log n + k)，k 是候选面数
 *
 * 参考实现：
 * ----------
 * Gmsh 的 rtree.h（基于 Guttman 的经典 R-Tree 算法）
 *
 * ============================================================================
 */

// ============================================================================
// 头文件包含区域
// ============================================================================

#include <iostream>   // 标准输入输出流
                      // 用途：输出调试信息和结果

#include <vector>     // 动态数组容器
                      // 用途：存储顶点、半边、面等列表

#include <cmath>      // 数学函数库
                      // 用途：提供 sqrt、abs、min、max 等数学运算

#include <algorithm>  // STL 算法库
                      // 用途：提供 min、max 等函数

#include <chrono>     // C++11 时间库
                      // 用途：精确测量算法性能

#include <limits>     // 数值极限库
                      // 用途：提供类型的最大/最小值

#include <cassert>    // 断言宏
                      // 用途：调试时验证程序假设

using namespace std;  // 使用标准命名空间

// ============================================================================
// 第一部分：基础数据结构定义
// ============================================================================

/**
 * 2D 点结构
 *
 * 【设计说明】
 * 与前两个解法相同，提供基本的点/向量运算
 */
struct Point2D {
    double x, y;  // 坐标分量

    /**
     * 构造函数
     */
    Point2D(double _x = 0, double _y = 0) : x(_x), y(_y) {}

    // 向量加法
    Point2D operator+(const Point2D& p) const { return Point2D(x + p.x, y + p.y); }

    // 向量减法
    Point2D operator-(const Point2D& p) const { return Point2D(x - p.x, y - p.y); }

    // 标量乘法
    Point2D operator*(double s) const { return Point2D(x * s, y * s); }

    // 输出运算符重载
    friend ostream& operator<<(ostream& os, const Point2D& p) {
        os << "(" << p.x << ", " << p.y << ")";
        return os;
    }
};

// ========== 前向声明 ==========
class HalfEdge;
class Face;

/**
 * 顶点类
 *
 * 【与前两个解法相同】
 */
class Vertex {
public:
    Point2D position;        // 顶点坐标
    HalfEdge* incidentEdge;  // 一条从该顶点出发的半边
    int id;                  // 顶点唯一标识

    /**
     * 构造函数
     * 注意：使用 NULL 而非 nullptr，以兼容 C++03
     */
    Vertex(double x = 0, double y = 0, int _id = -1)
        : position(x, y), incidentEdge(NULL), id(_id) {}

    Vertex(const Point2D& p, int _id = -1)
        : position(p), incidentEdge(NULL), id(_id) {}
};

/**
 * 半边类
 *
 * 【R-Tree 版本的简化】
 * 在本解法中，我们主要依赖 R-Tree 进行空间查询
 * twin 指针不是必需的，但保留 DCEL 结构以便遍历面的边界
 */
class HalfEdge {
public:
    Vertex* origin;         // 半边起点
    HalfEdge* twin;         // 对偶半边（本解法中可能未设置）
    HalfEdge* next;         // 同一面上的下一条半边
    HalfEdge* prev;         // 同一面上的上一条半边
    Face* incidentFace;     // 所属的面
    int id;                 // 半边唯一标识

    HalfEdge(Vertex* v = NULL, int _id = -1)
        : origin(v), twin(NULL), next(NULL), prev(NULL),
          incidentFace(NULL), id(_id) {}

    /**
     * 获取半边终点
     */
    Vertex* destination() const {
        return next ? next->origin : NULL;
    }
};

/**
 * 面类
 *
 * 【与前两个解法相同】
 * 存储外边界半边指针和包围盒
 */
class Face {
public:
    HalfEdge* outerComponent;  // 外边界上的一条半边
    int id;                    // 面唯一标识

    // 包围盒坐标
    double minX, minY, maxX, maxY;

    Face(HalfEdge* he = NULL, int _id = -1)
        : outerComponent(he), id(_id),
          minX(1e18), minY(1e18), maxX(-1e18), maxY(-1e18) {}

    /**
     * 计算包围盒
     */
    void computeBoundingBox() {
        if (!outerComponent) return;

        minX = minY = 1e18;
        maxX = maxY = -1e18;

        HalfEdge* he = outerComponent;
        do {
            double x = he->origin->position.x;
            double y = he->origin->position.y;
            minX = min(minX, x);
            minY = min(minY, y);
            maxX = max(maxX, x);
            maxY = max(maxY, y);
            he = he->next;
        } while (he != outerComponent);
    }

    /**
     * 获取顶点列表
     */
    vector<Point2D> getVertices() const {
        vector<Point2D> vertices;
        if (!outerComponent) return vertices;

        HalfEdge* he = outerComponent;
        do {
            vertices.push_back(he->origin->position);
            he = he->next;
        } while (he != outerComponent);

        return vertices;
    }
};

// ============================================================================
// 第二部分：几何算法
// ============================================================================

// 浮点数容差常量
const double EPS = 1e-10;

/**
 * orient2d - 方向判定函数
 *
 * 与前两个解法相同，这是计算几何的核心函数
 */
double orient2d(const Point2D& a, const Point2D& b, const Point2D& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/**
 * onSegment - 判断点是否在线段上
 *
 * 前提：已知点与线段共线
 */
bool onSegment(const Point2D& p, const Point2D& q, const Point2D& r) {
    return r.x <= max(p.x, q.x) && r.x >= min(p.x, q.x) &&
           r.y <= max(p.y, q.y) && r.y >= min(p.y, q.y);
}

/**
 * segmentsIntersect - 判断两条线段是否相交
 *
 * 使用叉积进行跨立测试
 */
bool segmentsIntersect(const Point2D& p1, const Point2D& p2,
                       const Point2D& p3, const Point2D& p4) {
    // 计算四个方向值
    double d1 = orient2d(p3, p4, p1);
    double d2 = orient2d(p3, p4, p2);
    double d3 = orient2d(p1, p2, p3);
    double d4 = orient2d(p1, p2, p4);

    // 标准相交：两条线段互相跨越
    if (((d1 > EPS && d2 < -EPS) || (d1 < -EPS && d2 > EPS)) &&
        ((d3 > EPS && d4 < -EPS) || (d3 < -EPS && d4 > EPS))) {
        return true;
    }

    // 处理共线和端点情况
    if (abs(d1) < EPS && onSegment(p3, p4, p1)) return true;
    if (abs(d2) < EPS && onSegment(p3, p4, p2)) return true;
    if (abs(d3) < EPS && onSegment(p1, p2, p3)) return true;
    if (abs(d4) < EPS && onSegment(p1, p2, p4)) return true;

    return false;
}

/**
 * lineIntersectsPolygon - 判断线段是否与多边形相交
 *
 * 检查线段是否与多边形的任一边相交
 */
bool lineIntersectsPolygon(const Point2D& p1, const Point2D& p2, Face* face) {
    HalfEdge* start = face->outerComponent;
    HalfEdge* he = start;

    do {
        Point2D a = he->origin->position;
        Point2D b = he->next->origin->position;

        if (segmentsIntersect(p1, p2, a, b)) {
            return true;
        }
        he = he->next;
    } while (he != start);

    return false;
}

// ============================================================================
// 第三部分：R-Tree 实现
// ============================================================================

/**
 * 2D 包围盒（最小包围矩形 MBR）
 *
 * 【R-Tree 的基本元素】
 * R-Tree 使用轴对齐的矩形包围盒来组织空间数据
 * 每个节点都有一个 MBR，包含其所有子节点/数据的范围
 */
struct Rect2D {
    double minX, minY;  // 矩形左下角坐标
    double maxX, maxY;  // 矩形右上角坐标

    /**
     * 默认构造函数
     * 初始化为"空"矩形（反向无穷）
     * 这样第一次 expand 调用就能正确设置边界
     */
    Rect2D() : minX(1e18), minY(1e18), maxX(-1e18), maxY(-1e18) {}

    /**
     * 带参数的构造函数
     * 直接设置矩形边界
     */
    Rect2D(double _minX, double _minY, double _maxX, double _maxY)
        : minX(_minX), minY(_minY), maxX(_maxX), maxY(_maxY) {}

    /**
     * 计算矩形面积
     *
     * 【用途】
     * R-Tree 插入时使用面积来选择最佳子节点
     * 选择使面积增量最小的子节点
     */
    double area() const {
        return (maxX - minX) * (maxY - minY);
    }

    /**
     * 扩展包围盒以包含另一个包围盒
     *
     * 【用途】
     * 1. 插入新数据后更新父节点的 MBR
     * 2. 计算面积增量时创建临时的扩展矩形
     */
    void expand(const Rect2D& other) {
        minX = min(minX, other.minX);
        minY = min(minY, other.minY);
        maxX = max(maxX, other.maxX);
        maxY = max(maxY, other.maxY);
    }

    /**
     * 判断两个包围盒是否相交
     *
     * 【算法】
     * 两个轴对齐矩形相交，当且仅当它们在 x 和 y 两个轴上都重叠
     *
     * 【用途】
     * R-Tree 查询时的剪枝条件
     * 如果节点 MBR 与查询矩形不相交，跳过整个子树
     */
    bool intersects(const Rect2D& other) const {
        // 在 x 轴上分离
        if (maxX < other.minX || minX > other.maxX) return false;
        // 在 y 轴上分离
        if (maxY < other.minY || minY > other.maxY) return false;
        // 两个轴都有重叠，相交
        return true;
    }

    /**
     * 计算扩展后的面积增量
     *
     * 【用途】
     * R-Tree 插入时选择最佳子节点的依据
     * 选择使面积增量最小的子节点，这样可以保持 MBR 尽量小
     *
     * @param other 要加入的矩形
     * @return 面积增量 = 扩展后面积 - 原面积
     */
    double enlargement(const Rect2D& other) const {
        // 计算扩展后的边界
        double newMinX = min(minX, other.minX);
        double newMinY = min(minY, other.minY);
        double newMaxX = max(maxX, other.maxX);
        double newMaxY = max(maxY, other.maxY);

        // 计算扩展后的面积
        double newArea = (newMaxX - newMinX) * (newMaxY - newMinY);

        // 返回面积增量
        return newArea - area();
    }
};

/**
 * R-Tree 节点类
 *
 * 【设计说明】
 * R-Tree 是一种平衡树，每个节点可以有多个子节点/数据
 * 节点分为两种类型：
 * 1. 叶子节点（isLeaf = true）：存储实际数据（Face*）
 * 2. 内部节点（isLeaf = false）：存储子节点指针
 *
 * 【参数选择】
 * - MAXNODES: 每个节点最多容纳的子节点/数据数
 *   太小：树太深，查询效率低
 *   太大：节点内线性搜索开销大
 *   典型值：4-16
 *
 * - MINNODES: 节点分裂后每部分最少包含的数量
 *   通常设为 MAXNODES / 2
 */
class RTreeNode {
public:
    // R-Tree 参数
    static const int MAXNODES = 8;               // 每个节点最多 8 个子节点/数据
    static const int MINNODES = MAXNODES / 2;    // 分裂后每部分至少 4 个

    bool isLeaf;       // 是否是叶子节点
    int count;         // 当前子节点/数据数量
    Rect2D mbr;        // 节点的最小包围矩形

    // 子节点的包围盒数组
    // 每个子节点（或数据）都有自己的 MBR
    Rect2D childMBR[MAXNODES];

    // 使用 union 节省内存
    // 叶子节点存储 Face* 指针
    // 内部节点存储 RTreeNode* 指针
    union {
        RTreeNode* children[MAXNODES];  // 内部节点的子节点指针
        Face* data[MAXNODES];           // 叶子节点的数据指针
    };

    /**
     * 构造函数
     * @param leaf 是否是叶子节点，默认为 true
     */
    RTreeNode(bool leaf = true) : isLeaf(leaf), count(0) {
        // 初始化所有指针为空
        for (int i = 0; i < MAXNODES; i++) {
            children[i] = NULL;
        }
    }

    /**
     * 析构函数
     * 递归删除所有子节点
     */
    ~RTreeNode() {
        // 只有内部节点需要删除子节点
        // 叶子节点的 Face* 由外部管理
        if (!isLeaf) {
            for (int i = 0; i < count; i++) {
                delete children[i];
            }
        }
    }

    /**
     * 更新节点的 MBR
     * 根据所有子节点的 MBR 重新计算
     */
    void updateMBR() {
        mbr = Rect2D();  // 重置为空矩形
        for (int i = 0; i < count; i++) {
            mbr.expand(childMBR[i]);  // 扩展以包含每个子节点
        }
    }
};

/**
 * R-Tree 类
 *
 * 【实现说明】
 * 这是一个简化版的 R-Tree 实现，参考 Gmsh 的 rtree.h
 *
 * 支持的操作：
 * - insert: 插入一个面
 * - search: 范围查询（返回与给定矩形相交的所有面）
 * - lineQuery: 直线查询（返回与给定直线相交的所有面）
 *
 * 【简化点】
 * - 使用简单的"选择最小面积增量"策略选择子节点
 * - 使用简单的"二分法"进行节点分裂
 *
 * 【更优化的实现】
 * 完整的 R-Tree 实现（如 R*-Tree）会使用：
 * - 更复杂的节点选择策略（考虑重叠面积）
 * - 更优的分裂算法（如二次分裂、线性分裂）
 * - 强制重新插入以优化树结构
 */
class RTree {
private:
    RTreeNode* root;  // 根节点指针
    int numItems;     // 存储的数据项数量

public:
    /**
     * 构造函数
     */
    RTree() : root(NULL), numItems(0) {}

    /**
     * 析构函数
     * 删除根节点（会递归删除所有子节点）
     */
    ~RTree() {
        delete root;
    }

    /**
     * 插入一个面到 R-Tree
     *
     * 【算法流程】
     * 1. 创建面的包围盒
     * 2. 如果根为空，创建叶子节点作为根
     * 3. 递归向下找到合适的叶子节点
     * 4. 插入数据，如果节点满了则分裂
     * 5. 分裂可能向上传播到根节点
     *
     * @param face 要插入的面
     */
    void insert(Face* face) {
        // 创建面的包围盒
        Rect2D rect(face->minX, face->minY, face->maxX, face->maxY);

        // 如果根为空，创建新的叶子节点
        if (root == NULL) {
            root = new RTreeNode(true);
        }

        // 递归插入，获取可能产生的新节点
        RTreeNode* newNode = insertInternal(root, face, rect);

        if (newNode != NULL) {
            // 根节点分裂了，需要创建新的根
            RTreeNode* newRoot = new RTreeNode(false);  // 新根是内部节点

            // 设置第一个子节点（原根）
            newRoot->childMBR[0] = root->mbr;
            newRoot->children[0] = root;

            // 设置第二个子节点（分裂产生的新节点）
            newRoot->childMBR[1] = newNode->mbr;
            newRoot->children[1] = newNode;

            newRoot->count = 2;
            newRoot->updateMBR();

            root = newRoot;  // 更新根指针
        }

        numItems++;
    }

    /**
     * 范围查询：返回与给定矩形相交的所有面
     *
     * 【算法】
     * 从根节点开始递归搜索
     * 对于每个节点，只遍历 MBR 与查询矩形相交的子节点
     *
     * @param rect 查询矩形
     * @return 与矩形相交的所有面
     */
    vector<Face*> search(const Rect2D& rect) {
        vector<Face*> results;
        if (root != NULL) {
            searchInternal(root, rect, results);
        }
        return results;
    }

    /**
     * 直线查询：返回与给定直线相交的所有面
     *
     * 【算法】
     * 1. 计算直线的包围盒
     * 2. 用 R-Tree 找到候选面（MBR 与直线包围盒相交的面）
     * 3. 对候选面进行精确相交测试
     *
     * 【为什么这样做？】
     * R-Tree 本身只支持矩形查询，不直接支持直线查询
     * 所以我们先用矩形查询获取候选，再做精确测试
     * 这仍然比暴力方法快很多，因为 R-Tree 的剪枝效果
     *
     * @param p1 直线起点
     * @param p2 直线终点
     * @return 与直线相交的所有面
     */
    vector<Face*> lineQuery(const Point2D& p1, const Point2D& p2) {
        // 计算直线包围盒
        Rect2D lineRect(
            min(p1.x, p2.x), min(p1.y, p2.y),
            max(p1.x, p2.x), max(p1.y, p2.y)
        );

        // R-Tree 范围查询获取候选面
        vector<Face*> candidates = search(lineRect);

        // 精确相交测试
        vector<Face*> results;
        for (size_t i = 0; i < candidates.size(); i++) {
            Face* f = candidates[i];
            if (lineIntersectsPolygon(p1, p2, f)) {
                results.push_back(f);
            }
        }

        return results;
    }

    /**
     * 获取存储的数据项数量
     */
    int size() const { return numItems; }

private:
    /**
     * 内部插入函数（递归）
     *
     * 【算法】
     * 1. 如果是叶子节点：
     *    - 如果未满，直接插入
     *    - 如果满了，分裂节点
     * 2. 如果是内部节点：
     *    - 选择最佳子节点（面积增量最小）
     *    - 递归插入到子节点
     *    - 更新 MBR
     *    - 如果子节点分裂了，处理新节点
     *
     * @param node 当前节点
     * @param face 要插入的面
     * @param rect 面的包围盒
     * @return 如果发生分裂，返回新节点；否则返回 NULL
     */
    RTreeNode* insertInternal(RTreeNode* node, Face* face, const Rect2D& rect) {
        if (node->isLeaf) {
            // ===== 叶子节点：直接插入数据 =====
            if (node->count < RTreeNode::MAXNODES) {
                // 节点未满，直接插入
                node->data[node->count] = face;
                node->childMBR[node->count] = rect;
                node->count++;
                node->updateMBR();
                return NULL;  // 不需要分裂
            } else {
                // 节点已满，需要分裂
                return splitLeaf(node, face, rect);
            }
        } else {
            // ===== 内部节点：选择子节点并递归 =====

            // 选择最佳子节点（面积增量最小）
            int bestChild = chooseSubtree(node, rect);

            // 递归插入到选中的子节点
            RTreeNode* newNode = insertInternal(node->children[bestChild], face, rect);

            // 更新子节点的 MBR（因为可能插入了新数据）
            node->childMBR[bestChild] = node->children[bestChild]->mbr;
            node->updateMBR();

            if (newNode != NULL) {
                // 子节点分裂了，需要将新节点加入当前节点
                if (node->count < RTreeNode::MAXNODES) {
                    // 当前节点未满，直接添加
                    node->children[node->count] = newNode;
                    node->childMBR[node->count] = newNode->mbr;
                    node->count++;
                    node->updateMBR();
                    return NULL;
                } else {
                    // 当前节点也满了，需要分裂
                    return splitInternal(node, newNode);
                }
            }

            return NULL;
        }
    }

    /**
     * 选择最佳子节点
     *
     * 【策略】
     * 选择使面积增量最小的子节点
     * 这是经典 R-Tree 的标准策略
     *
     * @param node 当前节点
     * @param rect 要插入的矩形
     * @return 最佳子节点的索引
     */
    int chooseSubtree(RTreeNode* node, const Rect2D& rect) {
        int bestChild = 0;
        double minEnlargement = node->childMBR[0].enlargement(rect);

        // 遍历所有子节点，找面积增量最小的
        for (int i = 1; i < node->count; i++) {
            double enlargement = node->childMBR[i].enlargement(rect);
            if (enlargement < minEnlargement) {
                minEnlargement = enlargement;
                bestChild = i;
            }
        }

        return bestChild;
    }

    /**
     * 分裂叶子节点
     *
     * 【简化策略：二分法】
     * 将所有数据（包括新数据）分成两半
     * 前半部分留在原节点，后半部分放入新节点
     *
     * 【更优策略】
     * - 二次分裂：选择"种子"对，最大化分离度
     * - 线性分裂：O(n) 时间的近似二次分裂
     *
     * @param node 要分裂的节点
     * @param face 新插入的数据
     * @param rect 新数据的包围盒
     * @return 分裂产生的新节点
     */
    RTreeNode* splitLeaf(RTreeNode* node, Face* face, const Rect2D& rect) {
        // 收集所有数据（包括新数据）
        Face* allData[RTreeNode::MAXNODES + 1];
        Rect2D allRects[RTreeNode::MAXNODES + 1];

        for (int i = 0; i < node->count; i++) {
            allData[i] = node->data[i];
            allRects[i] = node->childMBR[i];
        }
        allData[node->count] = face;
        allRects[node->count] = rect;

        // 创建新节点
        RTreeNode* newNode = new RTreeNode(true);

        // 简单二分：前半部分留在原节点，后半部分移到新节点
        int total = node->count + 1;
        int firstHalf = total / 2;

        // 重新填充原节点
        node->count = 0;
        for (int i = 0; i < firstHalf; i++) {
            node->data[i] = allData[i];
            node->childMBR[i] = allRects[i];
            node->count++;
        }

        // 填充新节点
        for (int i = firstHalf; i < total; i++) {
            newNode->data[newNode->count] = allData[i];
            newNode->childMBR[newNode->count] = allRects[i];
            newNode->count++;
        }

        // 更新两个节点的 MBR
        node->updateMBR();
        newNode->updateMBR();

        return newNode;
    }

    /**
     * 分裂内部节点
     *
     * 逻辑与 splitLeaf 类似，但处理的是子节点指针
     *
     * @param node 要分裂的节点
     * @param newChild 需要加入的新子节点
     * @return 分裂产生的新节点
     */
    RTreeNode* splitInternal(RTreeNode* node, RTreeNode* newChild) {
        // 收集所有子节点（包括新子节点）
        RTreeNode* allChildren[RTreeNode::MAXNODES + 1];
        Rect2D allRects[RTreeNode::MAXNODES + 1];

        for (int i = 0; i < node->count; i++) {
            allChildren[i] = node->children[i];
            allRects[i] = node->childMBR[i];
        }
        allChildren[node->count] = newChild;
        allRects[node->count] = newChild->mbr;

        // 创建新的内部节点
        RTreeNode* newNode = new RTreeNode(false);

        // 简单二分
        int total = node->count + 1;
        int firstHalf = total / 2;

        // 重新填充原节点
        node->count = 0;
        for (int i = 0; i < firstHalf; i++) {
            node->children[i] = allChildren[i];
            node->childMBR[i] = allRects[i];
            node->count++;
        }

        // 填充新节点
        for (int i = firstHalf; i < total; i++) {
            newNode->children[newNode->count] = allChildren[i];
            newNode->childMBR[newNode->count] = allRects[i];
            newNode->count++;
        }

        // 更新 MBR
        node->updateMBR();
        newNode->updateMBR();

        return newNode;
    }

    /**
     * 内部搜索函数（递归）
     *
     * 【R-Tree 查询的核心】
     * 遍历树，只访问 MBR 与查询矩形相交的节点
     * 这就是 R-Tree 高效的原因：通过 MBR 剪枝，跳过大量不相关的子树
     *
     * @param node 当前节点
     * @param rect 查询矩形
     * @param results 结果向量（输出参数）
     */
    void searchInternal(RTreeNode* node, const Rect2D& rect, vector<Face*>& results) {
        // 遍历节点的所有子节点/数据
        for (int i = 0; i < node->count; i++) {
            // 检查子节点的 MBR 是否与查询矩形相交
            if (node->childMBR[i].intersects(rect)) {
                if (node->isLeaf) {
                    // 叶子节点：将数据加入结果
                    results.push_back(node->data[i]);
                } else {
                    // 内部节点：递归搜索子节点
                    searchInternal(node->children[i], rect, results);
                }
            }
            // 如果不相交，跳过这个子树（剪枝）
        }
    }
};

// ============================================================================
// 第四部分：平面分割类（R-Tree 版本）
// ============================================================================

/**
 * 平面分割类
 *
 * 使用 R-Tree 作为空间索引
 *
 * 【与前两个解法的比较】
 * - 解法一（均匀网格）：简单但对非均匀分布效果差
 * - 解法二（射线行走）：需要完整拓扑，理论最优
 * - 解法三（R-Tree）：自适应，不需要完整拓扑，工业标准
 */
class PlanarSubdivision {
private:
    vector<Vertex*> vertices;      // 顶点列表
    vector<HalfEdge*> halfEdges;   // 半边列表
    vector<Face*> faces;           // 面列表

    RTree rtree;  // R-Tree 空间索引

public:
    /**
     * 析构函数
     * 释放所有动态分配的内存
     */
    ~PlanarSubdivision() {
        // 使用索引遍历，兼容 C++03
        for (size_t i = 0; i < vertices.size(); i++) delete vertices[i];
        for (size_t i = 0; i < halfEdges.size(); i++) delete halfEdges[i];
        for (size_t i = 0; i < faces.size(); i++) delete faces[i];
    }

    /**
     * 添加多边形面
     *
     * 与解法一类似，但额外将面插入 R-Tree
     *
     * @param polygon 多边形顶点列表（按逆时针顺序）
     * @return 创建的面指针
     */
    Face* addFace(const vector<Point2D>& polygon) {
        int n = polygon.size();
        if (n < 3) return NULL;

        // 创建面
        Face* face = new Face();
        face->id = faces.size();

        // 创建顶点
        vector<Vertex*> faceVertices(n);
        for (int i = 0; i < n; i++) {
            Vertex* v = new Vertex(polygon[i], vertices.size());
            vertices.push_back(v);
            faceVertices[i] = v;
        }

        // 创建半边
        vector<HalfEdge*> faceEdges(n);
        for (int i = 0; i < n; i++) {
            HalfEdge* he = new HalfEdge(faceVertices[i], halfEdges.size());
            he->incidentFace = face;
            faceVertices[i]->incidentEdge = he;
            halfEdges.push_back(he);
            faceEdges[i] = he;
        }

        // 连接半边形成环
        for (int i = 0; i < n; i++) {
            faceEdges[i]->next = faceEdges[(i + 1) % n];
            faceEdges[i]->prev = faceEdges[(i - 1 + n) % n];
        }

        // 设置面属性
        face->outerComponent = faceEdges[0];
        face->computeBoundingBox();
        faces.push_back(face);

        // 【关键】将面插入 R-Tree
        rtree.insert(face);

        return face;
    }

    /**
     * 直线相交查询
     *
     * 直接调用 R-Tree 的 lineQuery 方法
     */
    vector<Face*> queryLineIntersection(const Point2D& p1, const Point2D& p2) {
        return rtree.lineQuery(p1, p2);
    }

    /**
     * 暴力查询（用于验证）
     */
    vector<Face*> bruteForceQuery(const Point2D& p1, const Point2D& p2) {
        vector<Face*> result;
        for (size_t i = 0; i < faces.size(); i++) {
            Face* f = faces[i];
            if (lineIntersectsPolygon(p1, p2, f)) {
                result.push_back(f);
            }
        }
        return result;
    }

    /**
     * 获取面的数量
     */
    int numFaces() const { return faces.size(); }

    /**
     * 打印统计信息
     */
    void printStatistics() const {
        cout << "\n=== 数据结构统计 ===" << endl;
        cout << "顶点数: " << vertices.size() << endl;
        cout << "半边数: " << halfEdges.size() << endl;
        cout << "面数: " << faces.size() << endl;
        cout << "R-Tree 项数: " << rtree.size() << endl;
    }
};

// ============================================================================
// 第五部分：测试程序
// ============================================================================

/**
 * 创建测试用的三角网格
 *
 * 【网格布局】
 * 与前两个解法相同的 2×2 方格网格
 */
void createTestMesh(PlanarSubdivision& subdivision) {
    cout << "=== 创建测试三角网格 ===" << endl;

    // 使用 vector 的向量存储三角形
    // 兼容 C++03，不使用初始化列表
    vector<vector<Point2D> > triangles;

    // 手动创建每个三角形
    // 注意：不使用 C++11 的初始化列表语法

    // 三角形 0
    vector<Point2D> t0;
    t0.push_back(Point2D(0, 1));
    t0.push_back(Point2D(0, 2));
    t0.push_back(Point2D(1, 2));

    // 三角形 1
    vector<Point2D> t1;
    t1.push_back(Point2D(0, 1));
    t1.push_back(Point2D(1, 2));
    t1.push_back(Point2D(1, 1));

    // 三角形 2
    vector<Point2D> t2;
    t2.push_back(Point2D(1, 1));
    t2.push_back(Point2D(1, 2));
    t2.push_back(Point2D(2, 2));

    // 三角形 3
    vector<Point2D> t3;
    t3.push_back(Point2D(1, 1));
    t3.push_back(Point2D(2, 2));
    t3.push_back(Point2D(2, 1));

    // 三角形 4
    vector<Point2D> t4;
    t4.push_back(Point2D(0, 0));
    t4.push_back(Point2D(0, 1));
    t4.push_back(Point2D(1, 1));

    // 三角形 5
    vector<Point2D> t5;
    t5.push_back(Point2D(0, 0));
    t5.push_back(Point2D(1, 1));
    t5.push_back(Point2D(1, 0));

    // 三角形 6
    vector<Point2D> t6;
    t6.push_back(Point2D(1, 0));
    t6.push_back(Point2D(1, 1));
    t6.push_back(Point2D(2, 1));

    // 三角形 7
    vector<Point2D> t7;
    t7.push_back(Point2D(1, 0));
    t7.push_back(Point2D(2, 1));
    t7.push_back(Point2D(2, 0));

    // 添加到三角形列表
    triangles.push_back(t0);
    triangles.push_back(t1);
    triangles.push_back(t2);
    triangles.push_back(t3);
    triangles.push_back(t4);
    triangles.push_back(t5);
    triangles.push_back(t6);
    triangles.push_back(t7);

    // 添加所有三角形到平面分割
    for (size_t i = 0; i < triangles.size(); i++) {
        subdivision.addFace(triangles[i]);
    }

    cout << "创建了 " << subdivision.numFaces() << " 个三角形面" << endl;
}

/**
 * 运行查询测试
 */
void runQueryTests(PlanarSubdivision& subdivision) {
    cout << "\n=== 运行直线相交查询测试（R-Tree 版本）===" << endl;

    // ========== 测试用例 1：对角线查询 ==========
    {
        Point2D p1(0.2, 0.2);
        Point2D p2(1.8, 1.8);

        cout << "\n测试 1：对角线查询" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        // 使用显式类型避免 auto（兼容 C++03）
        chrono::high_resolution_clock::time_point start = chrono::high_resolution_clock::now();
        vector<Face*> result = subdivision.queryLineIntersection(p1, p2);
        chrono::high_resolution_clock::time_point end = chrono::high_resolution_clock::now();

        chrono::microseconds duration = chrono::duration_cast<chrono::microseconds>(end - start);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (size_t i = 0; i < result.size(); i++) {
            cout << result[i]->id << " ";
        }
        cout << endl;
        cout << "查询耗时: " << duration.count() << " 微秒" << endl;

        // 验证结果
        vector<Face*> bruteResult = subdivision.bruteForceQuery(p1, p2);
        cout << "暴力验证结果: " << bruteResult.size() << " 个面" << endl;
    }

    // ========== 测试用例 2：水平线查询 ==========
    {
        Point2D p1(0.0, 1.0);
        Point2D p2(2.0, 1.0);

        cout << "\n测试 2：水平线查询（沿 y=1）" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        vector<Face*> result = subdivision.queryLineIntersection(p1, p2);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (size_t i = 0; i < result.size(); i++) {
            cout << result[i]->id << " ";
        }
        cout << endl;
    }

    // ========== 测试用例 3：垂直线查询 ==========
    {
        Point2D p1(1.0, 0.0);
        Point2D p2(1.0, 2.0);

        cout << "\n测试 3：垂直线查询（沿 x=1）" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        vector<Face*> result = subdivision.queryLineIntersection(p1, p2);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (size_t i = 0; i < result.size(); i++) {
            cout << result[i]->id << " ";
        }
        cout << endl;
    }

    // ========== 测试用例 4：短线段查询 ==========
    {
        Point2D p1(0.3, 0.3);
        Point2D p2(0.4, 0.4);

        cout << "\n测试 4：短线段查询（在单个三角形内）" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        vector<Face*> result = subdivision.queryLineIntersection(p1, p2);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (size_t i = 0; i < result.size(); i++) {
            cout << result[i]->id << " ";
        }
        cout << endl;
    }
}

/**
 * 性能测试
 * 比较 R-Tree 与暴力方法的性能
 */
void runPerformanceTest() {
    cout << "\n=== 性能测试 ===" << endl;

    PlanarSubdivision subdivision;

    int gridSize = 10;
    cout << "生成 " << gridSize << "x" << gridSize << " 三角网格 ("
         << gridSize * gridSize * 2 << " 个三角形)" << endl;

    // 添加三角形
    for (int j = 0; j < gridSize; j++) {
        for (int i = 0; i < gridSize; i++) {
            double x0 = i, y0 = j;
            double x1 = i + 1, y1 = j + 1;

            // 三角形 1
            vector<Point2D> t1;
            t1.push_back(Point2D(x0, y0));
            t1.push_back(Point2D(x0, y1));
            t1.push_back(Point2D(x1, y1));

            // 三角形 2
            vector<Point2D> t2;
            t2.push_back(Point2D(x0, y0));
            t2.push_back(Point2D(x1, y1));
            t2.push_back(Point2D(x1, y0));

            subdivision.addFace(t1);
            subdivision.addFace(t2);
        }
    }

    subdivision.printStatistics();

    // 执行多次随机查询
    int numQueries = 1000;
    cout << "\n执行 " << numQueries << " 次随机查询..." << endl;

    srand(42);

    int totalIntersections = 0;
    chrono::high_resolution_clock::time_point queryStart = chrono::high_resolution_clock::now();

    for (int q = 0; q < numQueries; q++) {
        double x1 = (double)rand() / RAND_MAX * gridSize;
        double y1 = (double)rand() / RAND_MAX * gridSize;
        double x2 = (double)rand() / RAND_MAX * gridSize;
        double y2 = (double)rand() / RAND_MAX * gridSize;

        vector<Face*> result = subdivision.queryLineIntersection(
            Point2D(x1, y1), Point2D(x2, y2));
        totalIntersections += result.size();
    }

    chrono::high_resolution_clock::time_point queryEnd = chrono::high_resolution_clock::now();
    chrono::microseconds queryDuration = chrono::duration_cast<chrono::microseconds>(queryEnd - queryStart);

    cout << "总查询耗时: " << queryDuration.count() << " 微秒" << endl;
    cout << "平均每次查询: " << queryDuration.count() / numQueries << " 微秒" << endl;
    cout << "平均每次查询相交的多边形数: " << (double)totalIntersections / numQueries << endl;

    // 对比暴力方法
    cout << "\n对比暴力方法..." << endl;
    chrono::high_resolution_clock::time_point bruteStart = chrono::high_resolution_clock::now();

    srand(42);  // 重置随机种子
    for (int q = 0; q < numQueries; q++) {
        double x1 = (double)rand() / RAND_MAX * gridSize;
        double y1 = (double)rand() / RAND_MAX * gridSize;
        double x2 = (double)rand() / RAND_MAX * gridSize;
        double y2 = (double)rand() / RAND_MAX * gridSize;

        subdivision.bruteForceQuery(Point2D(x1, y1), Point2D(x2, y2));
    }

    chrono::high_resolution_clock::time_point bruteEnd = chrono::high_resolution_clock::now();
    chrono::microseconds bruteDuration = chrono::duration_cast<chrono::microseconds>(bruteEnd - bruteStart);

    cout << "暴力方法总耗时: " << bruteDuration.count() << " 微秒" << endl;
    cout << "加速比: " << (double)bruteDuration.count() / queryDuration.count() << "x" << endl;
}

/**
 * 主函数
 * 程序入口点
 */
int main() {
    cout << "========================================================" << endl;
    cout << "  习题 1.3：平面分割数据结构与直线相交查询" << endl;
    cout << "  解法三：R-Tree 版本" << endl;
    cout << "========================================================" << endl;

    // 创建平面分割数据结构
    PlanarSubdivision subdivision;

    // 创建测试网格
    createTestMesh(subdivision);

    // 打印统计信息
    subdivision.printStatistics();

    // 运行查询测试
    runQueryTests(subdivision);

    // 运行性能测试
    runPerformanceTest();

    cout << "\n=== 程序结束 ===" << endl;

    return 0;
}
