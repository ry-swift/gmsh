/**
 * ============================================================================
 * 习题 1.3：平面分割数据结构与直线相交查询
 * 解法二：射线行走法（Ray Walking）
 * ============================================================================
 *
 * 【思考过程记录】
 *
 * 问题分析：
 * ----------
 * 在解法一中，我们使用均匀网格作为空间索引。虽然有效，但存在以下问题：
 * 1. 需要额外的网格数据结构占用空间
 * 2. 可能检查一些实际上不相交的多边形（假阳性）
 * 3. 网格参数需要调优
 *
 * 核心洞察：
 * ----------
 * 如果我们已经有了完整的拓扑信息（DCEL），为什么不直接利用它呢？
 *
 * 射线行走算法的核心思想：
 * 1. 找到射线起点所在的多边形（点定位）
 * 2. 确定射线穿出当前多边形的边
 * 3. 通过 twin 指针直接跳转到相邻多边形
 * 4. 重复步骤 2-3，直到射线离开平面分割区域或到达终点
 *
 * 时间复杂度优势：
 * ----------
 * - 均匀网格：O(c)，c 是候选多边形数（可能 > 实际相交数）
 * - 射线行走：O(k)，k 是实际相交的多边形数（输出敏感！）
 *
 * 由于 k ≤ c，射线行走是理论最优的！
 *
 * 算法细节：
 * ----------
 * 1. 点定位（Walk 算法）：
 *    - 从任意一个面开始
 *    - 使用 orient2d 判断点相对于当前面各边的位置
 *    - 如果点不在当前面内，通过 twin 跳转到更近的面
 *    - 重复直到找到包含该点的面
 *
 * 2. 找穿出边：
 *    - 对当前面的每条边，计算射线与边的交点参数 t
 *    - 选择 t 最小且 > 0 的边（第一个穿出的边）
 *
 * 3. 跳转：
 *    - 通过选中边的 twin 指针获取相邻面
 *    - 如果 twin 为空，说明到达边界
 *
 * 参考实现：
 * ----------
 * Gmsh 的 meshTriangulation.cpp 中的 Walk 函数（第 395-444 行）
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

#include <map>        // 有序映射容器
                      // 用途：存储边的映射关系，用于设置 twin 指针

#include <cmath>      // 数学函数库
                      // 用途：提供 sqrt、abs 等数学运算

#include <algorithm>  // STL 算法库
                      // 用途：提供 min、max 等函数

#include <chrono>     // C++11 时间库
                      // 用途：精确测量算法性能

#include <set>        // 有序集合容器
                      // 用途：在射线行走时记录已访问的面，防止重复

#include <limits>     // 数值极限库
                      // 用途：提供 numeric_limits 获取类型的最大/最小值

using namespace std;  // 使用标准命名空间

// ============================================================================
// 第一部分：基础数据结构定义
// ============================================================================

/**
 * 2D 点结构
 *
 * 【设计说明】
 * 与解法一相同，提供基本的点/向量运算
 */
struct Point2D {
    double x, y;  // 坐标分量

    /**
     * 构造函数
     * 使用默认参数支持无参构造
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
 * 【与解法一的区别】
 * 完全相同，存储顶点位置和一条出边
 */
class Vertex {
public:
    Point2D position;        // 顶点坐标
    HalfEdge* incidentEdge;  // 一条从该顶点出发的半边
    int id;                  // 顶点唯一标识

    Vertex(double x = 0, double y = 0, int _id = -1)
        : position(x, y), incidentEdge(nullptr), id(_id) {}

    Vertex(const Point2D& p, int _id = -1)
        : position(p), incidentEdge(nullptr), id(_id) {}
};

/**
 * 半边类
 *
 * 【射线行走的关键】
 * twin 指针让我们能够 O(1) 时间跳转到相邻面
 * 这是射线行走算法高效的核心原因！
 *
 * 【与解法一的关键区别】
 * 在解法一中，twin 指针可能未设置
 * 在本解法中，必须正确设置 twin 指针才能进行射线行走
 */
class HalfEdge {
public:
    Vertex* origin;         // 半边起点
    HalfEdge* twin;         // 对偶半边 - 【关键】射线行走通过它跳转到相邻面
    HalfEdge* next;         // 同一面上的下一条半边
    HalfEdge* prev;         // 同一面上的上一条半边
    Face* incidentFace;     // 所属的面
    int id;                 // 半边唯一标识

    HalfEdge(Vertex* v = nullptr, int _id = -1)
        : origin(v), twin(nullptr), next(nullptr), prev(nullptr),
          incidentFace(nullptr), id(_id) {}

    /**
     * 获取半边终点
     * 终点 = 下一条半边的起点
     */
    Vertex* destination() const {
        return next ? next->origin : nullptr;
    }
};

/**
 * 面类
 *
 * 【与解法一相同】
 * 存储外边界半边指针和包围盒
 */
class Face {
public:
    HalfEdge* outerComponent;  // 外边界上的一条半边
    int id;                    // 面唯一标识

    // 包围盒坐标
    double minX, minY, maxX, maxY;

    Face(HalfEdge* he = nullptr, int _id = -1)
        : outerComponent(he), id(_id),
          minX(1e18), minY(1e18), maxX(-1e18), maxY(-1e18) {}

    /**
     * 计算包围盒
     * 遍历所有顶点，找到坐标范围
     */
    void computeBoundingBox() {
        if (!outerComponent) return;

        // 重置为极值
        minX = minY = 1e18;
        maxX = maxY = -1e18;

        // 遍历边界半边
        HalfEdge* he = outerComponent;
        do {
            double x = he->origin->position.x;
            double y = he->origin->position.y;
            // 更新边界
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
// 用于处理浮点计算的精度问题
const double EPS = 1e-10;

/**
 * orient2d - 方向判定函数
 *
 * 【这是射线行走算法的核心工具】
 *
 * 主要用途：
 * 1. 点定位（Walk）时判断点相对于边的位置
 * 2. 确定射线穿出当前面的边
 *
 * 返回值含义：
 * - 正值：c 在有向线段 ab 的左侧（逆时针方向）
 * - 负值：c 在有向线段 ab 的右侧（顺时针方向）
 * - 零：三点共线
 *
 * 【数学公式】
 * 计算 2D 叉积：(b-a) × (c-a)
 * = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)
 */
double orient2d(const Point2D& a, const Point2D& b, const Point2D& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/**
 * rayEdgeIntersection - 计算射线与线段的交点参数
 *
 * 【射线行走的核心几何算法】
 *
 * 问题描述：
 * - 射线定义：P(t) = p1 + t * (p2 - p1)，t ∈ [0, 1]
 *   - t = 0 对应起点 p1
 *   - t = 1 对应终点 p2
 * - 线段定义：Q(s) = a + s * (b - a)，s ∈ [0, 1]
 *   - s = 0 对应端点 a
 *   - s = 1 对应端点 b
 *
 * 目标：找到 t 值使得 P(t) 在线段 ab 上
 *
 * 【算法推导】
 * 设射线方向向量 d = p2 - p1
 * 设边的方向向量 e = b - a
 *
 * 交点满足：p1 + t*d = a + s*e
 * 移项得：t*d - s*e = a - p1
 *
 * 这是一个 2×2 线性方程组：
 * | d.x  -e.x | | t |   | (a.x - p1.x) |
 * | d.y  -e.y | | s | = | (a.y - p1.y) |
 *
 * 用克莱姆法则求解：
 * 分母 denom = d.x * e.y - d.y * e.x = d × e（叉积）
 *
 * t = ((a - p1) × e) / (d × e)
 * s = ((a - p1) × d) / (d × e)
 *
 * 【返回值】
 * - t ∈ (0, 1]：有效交点（射线在起点前方且不超过终点）
 * - t ≤ 0：交点在起点后方（无效）
 * - t > 1：交点超过终点（无效）
 * - 返回 -1：平行或无交点
 *
 * @param p1, p2 射线的起点和终点
 * @param a, b 边的两个端点
 * @return 射线参数 t，如果无交点返回 -1
 */
double rayEdgeIntersection(const Point2D& p1, const Point2D& p2,
                           const Point2D& a, const Point2D& b) {
    // 计算射线方向向量
    double dx = p2.x - p1.x;  // 射线 x 分量
    double dy = p2.y - p1.y;  // 射线 y 分量

    // 计算边的方向向量
    double ex = b.x - a.x;    // 边 x 分量
    double ey = b.y - a.y;    // 边 y 分量

    // 计算叉积（分母）
    // denom = d × e
    double denom = dx * ey - dy * ex;

    // 检查是否平行
    // 如果叉积接近零，射线和边几乎平行，无有效交点
    if (abs(denom) < EPS) return -1;

    // 计算交点参数
    // t = ((a - p1) × e) / (d × e)
    // 展开：t = ((a.x - p1.x) * ey - (a.y - p1.y) * ex) / denom
    double t = ((a.x - p1.x) * ey - (a.y - p1.y) * ex) / denom;

    // s = ((a - p1) × d) / (d × e)
    // 展开：s = ((a.x - p1.x) * dy - (a.y - p1.y) * dx) / denom
    double s = ((a.x - p1.x) * dy - (a.y - p1.y) * dx) / denom;

    // 验证交点有效性
    // 条件：
    // 1. t > EPS：交点在射线起点前方（稍微大于0避免精度问题）
    // 2. t <= 1.0 + EPS：交点不超过射线终点
    // 3. s >= -EPS 且 s <= 1.0 + EPS：交点在边上
    if (t > EPS && t <= 1.0 + EPS && s >= -EPS && s <= 1.0 + EPS) {
        return t;
    }

    return -1;  // 无有效交点
}

/**
 * pointInFace - 判断点是否在多边形（面）内部
 *
 * 【使用 orient2d 的方法 - 针对凸多边形】
 *
 * 原理：
 * 对于凸多边形（如三角形），点在内部当且仅当：
 * 点相对于所有边都在同一侧
 *
 * 具体判断：
 * - 如果多边形顶点按逆时针排列，内部点应该在所有边的左侧
 * - 即所有 orient2d 返回值都应该 >= 0
 *
 * 【算法步骤】
 * 1. 遍历多边形的每条边
 * 2. 计算点相对于每条边的 orient2d 值
 * 3. 如果所有值都非负（或都非正），点在内部
 *
 * @param p 待判断的点
 * @param face 多边形面
 * @return true 如果点在面内部或边界上
 */
bool pointInFace(const Point2D& p, Face* face) {
    // 检查面是否有效
    if (!face || !face->outerComponent) return false;

    HalfEdge* he = face->outerComponent;
    bool allPositive = true;   // 是否所有方向值都是正的
    bool allNegative = true;   // 是否所有方向值都是负的

    do {
        // 获取当前边的起点和终点
        Point2D a = he->origin->position;
        Point2D b = he->next->origin->position;

        // 计算点 p 相对于边 ab 的方向
        double o = orient2d(a, b, p);

        // 更新标志
        if (o < -EPS) allPositive = false;  // 存在负值
        if (o > EPS) allNegative = false;   // 存在正值

        he = he->next;
    } while (he != face->outerComponent);

    // 如果点在所有边的同一侧（全正或全负），则在内部
    // 这对于凸多边形是正确的判断方法
    return allPositive || allNegative;
}

// ============================================================================
// 第三部分：带完整拓扑的 DCEL 数据结构
// ============================================================================

/**
 * 平面分割类（射线行走版本）
 *
 * 【与解法一的关键区别】
 * 这个版本需要正确设置 twin 指针，以支持射线行走
 * twin 指针是相邻面之间的桥梁，通过它实现 O(1) 时间的面间跳转
 *
 * 【设计要点】
 * 1. 使用 map 存储边的映射，用于查找配对的半边
 * 2. 缓存最后定位的面，加速连续查询
 */
class PlanarSubdivision {
private:
    vector<Vertex*> vertices;      // 顶点列表
    vector<HalfEdge*> halfEdges;   // 半边列表
    vector<Face*> faces;           // 面列表

    // 边映射表：用于查找共享边的配对半边
    // key: (min_vertex_id, max_vertex_id) - 规范化的边表示
    // value: 第一次遇到的半边指针
    //
    // 【工作原理】
    // 当添加一条边时：
    // 1. 计算边的规范化 key（较小顶点ID在前）
    // 2. 查找 map 中是否已有这条边
    // 3. 如果有，设置双向的 twin 指针
    // 4. 如果没有，将当前半边存入 map
    map<pair<int, int>, HalfEdge*> edgeMap;

    // 缓存最后定位成功的面
    // 【优化作用】
    // 在进行连续的点定位查询时，如果新的点靠近上一个点，
    // 从缓存的面开始 walk 可以减少遍历步数
    Face* lastLocatedFace;

public:
    /**
     * 构造函数
     */
    PlanarSubdivision() : lastLocatedFace(nullptr) {}

    /**
     * 析构函数
     * 释放所有动态分配的内存
     */
    ~PlanarSubdivision() {
        for (auto v : vertices) delete v;
        for (auto e : halfEdges) delete e;
        for (auto f : faces) delete f;
    }

    /**
     * 批量添加顶点
     *
     * 【使用方法】
     * 先批量添加所有顶点，然后通过顶点索引添加三角形
     * 这种方式可以正确处理共享边的 twin 指针
     *
     * @param points 顶点坐标列表
     */
    void addVertices(const vector<Point2D>& points) {
        for (const auto& p : points) {
            // 创建顶点，ID 为当前顶点数量
            Vertex* v = new Vertex(p, vertices.size());
            vertices.push_back(v);
        }
    }

    /**
     * 添加三角形面（使用顶点索引）
     *
     * 【关键】正确设置 twin 指针
     *
     * 这是与解法一的核心区别：
     * 当添加新的三角形时，检查其每条边是否与已存在的三角形共享
     * 如果是，设置双向的 twin 指针，建立相邻面之间的连接
     *
     * 【算法】
     * 对于三角形的每条边 (vi, vj)：
     * 1. 创建规范化的边标识 (min(vi, vj), max(vi, vj))
     * 2. 在 edgeMap 中查找这个边标识
     * 3. 如果找到：
     *    - 说明这条边已经被另一个三角形使用
     *    - 设置当前半边的 twin 指向已存在的半边
     *    - 设置已存在的半边的 twin 指向当前半边
     * 4. 如果没找到：
     *    - 这是新边，将当前半边存入 edgeMap
     *
     * @param v0, v1, v2 三角形三个顶点的索引（按逆时针顺序）
     * @return 创建的面指针
     */
    Face* addTriangle(int v0, int v1, int v2) {
        // 创建面
        Face* face = new Face();
        face->id = faces.size();

        // 获取顶点指针
        Vertex* verts[3] = {vertices[v0], vertices[v1], vertices[v2]};

        // 创建三条半边
        HalfEdge* edges[3];
        for (int i = 0; i < 3; i++) {
            // 创建半边，起点为 verts[i]
            edges[i] = new HalfEdge(verts[i], halfEdges.size());
            edges[i]->incidentFace = face;  // 设置所属面
            halfEdges.push_back(edges[i]);   // 添加到全局列表
        }

        // 连接半边形成环
        for (int i = 0; i < 3; i++) {
            edges[i]->next = edges[(i + 1) % 3];  // 下一条边
            edges[i]->prev = edges[(i + 2) % 3];  // 上一条边
            verts[i]->incidentEdge = edges[i];   // 顶点关联出边
        }

        // 设置面的属性
        face->outerComponent = edges[0];
        face->computeBoundingBox();
        faces.push_back(face);

        // ===== 【关键步骤】设置 twin 指针 =====
        int vertIds[3] = {v0, v1, v2};

        for (int i = 0; i < 3; i++) {
            // 当前边的两个顶点 ID
            int id1 = vertIds[i];
            int id2 = vertIds[(i + 1) % 3];

            // 规范化边的表示：小顶点 ID 在前
            // 这样无论边的方向如何，同一条几何边有相同的 key
            auto edgeKey = make_pair(min(id1, id2), max(id1, id2));

            // 查找是否存在配对的半边
            auto it = edgeMap.find(edgeKey);

            if (it != edgeMap.end()) {
                // 找到了配对边
                HalfEdge* existingEdge = it->second;

                // 设置双向的 twin 指针
                // 当前半边的 twin 是已存在的半边
                edges[i]->twin = existingEdge;
                // 已存在的半边的 twin 是当前半边
                existingEdge->twin = edges[i];

                // 注意：不需要从 map 中移除，因为一条边最多被两个面共享
            } else {
                // 没有配对边，注册当前边
                edgeMap[edgeKey] = edges[i];
            }
        }

        return face;
    }

    /**
     * 点定位（Walk 算法）
     *
     * 【核心算法 - 参考 Gmsh 的 meshTriangulation.cpp】
     *
     * 功能：给定一个点，找到包含它的三角形面
     *
     * 【算法原理】
     * 1. 从任意一个面开始（使用缓存的面可以加速）
     * 2. 使用 orient2d 判断点相对于当前面各边的位置
     * 3. 如果点在所有边的同一侧，它在当前面内，返回
     * 4. 否则，找到点"最偏离"的边，通过 twin 跳转到相邻面
     * 5. 重复步骤 2-4
     *
     * 【为什么选择"最偏离"的边？】
     * 如果点在某条边的外侧（orient 值为负），
     * 选择 orient 值最负的边可以最快地向点靠近
     *
     * 【复杂度分析】
     * - 平均情况：O(√n)，n 是面的数量
     * - 最坏情况：O(n)
     * - 使用缓存可以进一步优化连续查询
     *
     * @param p 待定位的点
     * @return 包含该点的面，如果点在外部返回 nullptr
     */
    Face* locatePoint(const Point2D& p) {
        // 检查是否有面
        if (faces.empty()) return nullptr;

        // 选择起始面：优先使用缓存，否则使用第一个面
        Face* current = lastLocatedFace ? lastLocatedFace : faces[0];

        // 防止无限循环（理论上在有效的三角剖分中不会发生）
        // 设置最大迭代次数为面数 + 余量
        int maxIterations = faces.size() + 100;

        while (maxIterations-- > 0) {
            // 获取当前面的边界半边
            HalfEdge* he = current->outerComponent;

            // 假设面是三角形，获取三个顶点
            Vertex* v0 = he->origin;
            Vertex* v1 = he->next->origin;
            Vertex* v2 = he->next->next->origin;

            // 计算点相对于三条边的位置
            // 注意：这里使用负号，与 Gmsh 代码保持一致
            // 负号的效果是翻转方向判断的含义
            double s0 = -orient2d(v0->position, v1->position, p);
            double s1 = -orient2d(v1->position, v2->position, p);
            double s2 = -orient2d(v2->position, v0->position, p);

            // 判断点是否在三角形内
            // 如果三个值都 >= 0（考虑数值误差），点在内部
            if (s0 >= -EPS && s1 >= -EPS && s2 >= -EPS) {
                // 找到了！更新缓存并返回
                lastLocatedFace = current;
                return current;
            }

            // ===== 选择跳转方向 =====
            // 策略：选择 orient 值最负的边（点最"偏离"的方向）
            // 这样可以最快地向点移动

            HalfEdge* nextEdge = nullptr;

            // 检查边 0（v0 -> v1）
            // 条件：s0 < -EPS（点在边外侧）且 s0 是最负的
            if (s0 < -EPS && (s0 <= s1 || s1 >= -EPS) && (s0 <= s2 || s2 >= -EPS)) {
                nextEdge = he;  // 第一条边
            }
            // 检查边 1（v1 -> v2）
            else if (s1 < -EPS && (s1 <= s0 || s0 >= -EPS) && (s1 <= s2 || s2 >= -EPS)) {
                nextEdge = he->next;  // 第二条边
            }
            // 检查边 2（v2 -> v0）
            else if (s2 < -EPS) {
                nextEdge = he->next->next;  // 第三条边
            }

            // 尝试跳转
            if (nextEdge && nextEdge->twin) {
                // 通过 twin 跳转到相邻面
                current = nextEdge->twin->incidentFace;
            } else {
                // 到达边界（没有相邻面），点在平面分割外部
                return nullptr;
            }
        }

        // 超过最大迭代次数（不应该发生）
        return nullptr;
    }

    /**
     * 射线行走查询
     *
     * 【核心算法 - 本解法的精华】
     *
     * 功能：找到射线穿过的所有三角形面
     *
     * 【算法步骤】
     * 1. 定位起点所在的面（使用 locatePoint）
     * 2. 将当前面加入结果
     * 3. 在当前面中找到射线穿出的边（使用 rayEdgeIntersection）
     * 4. 通过 twin 跳转到相邻面
     * 5. 重复步骤 2-4，直到：
     *    - 到达终点（minT > 1）
     *    - 到达边界（twin 为空）
     *    - 或离开网格区域
     *
     * 【时间复杂度】
     * O(点定位) + O(k)
     * 其中 k 是射线实际穿过的面数
     * 这是输出敏感的最优复杂度！
     *
     * @param p1 射线起点
     * @param p2 射线终点
     * @return 射线穿过的所有面的列表
     */
    vector<Face*> rayWalkQuery(const Point2D& p1, const Point2D& p2) {
        vector<Face*> result;

        // ===== 第一步：定位起点所在的面 =====
        Face* startFace = locatePoint(p1);

        if (!startFace) {
            // 起点在平面分割外部
            // 需要找到射线进入的第一个面

            // 简化处理：遍历所有边界边，找到射线与之相交的边
            for (HalfEdge* he : halfEdges) {
                // 边界边的特征：twin 为空
                if (he->twin == nullptr) {
                    // 获取边的端点
                    Point2D a = he->origin->position;
                    Point2D b = he->next->origin->position;

                    // 计算射线与边的交点
                    double t = rayEdgeIntersection(p1, p2, a, b);

                    // 如果有有效交点
                    if (t > 0 && t <= 1) {
                        // 从这条边的面开始
                        startFace = he->incidentFace;
                        break;
                    }
                }
            }

            // 如果仍然没找到，射线不与任何面相交
            if (!startFace) return result;
        }

        // ===== 第二步：射线行走 =====
        Face* current = startFace;

        // 使用 set 记录已访问的面，防止由于数值误差导致的重复访问
        set<Face*> visited;

        // 设置最大迭代次数，防止无限循环
        int maxIterations = faces.size() + 100;

        while (current && maxIterations-- > 0) {
            // 检查是否已访问（避免重复）
            if (visited.count(current)) break;

            // 标记为已访问
            visited.insert(current);

            // 将当前面加入结果
            result.push_back(current);

            // ===== 找射线穿出的边 =====
            HalfEdge* exitEdge = nullptr;    // 穿出的边
            double minT = numeric_limits<double>::max();  // 最小的 t 值

            // 遍历当前面的所有边
            HalfEdge* he = current->outerComponent;
            do {
                // 获取边的端点
                Point2D a = he->origin->position;
                Point2D b = he->next->origin->position;

                // 计算射线与边的交点参数
                double t = rayEdgeIntersection(p1, p2, a, b);

                // 选择 t 最小的穿出边
                // 条件：t > EPS（确保是前方的交点，不是起点处的边）
                if (t > EPS && t < minT) {
                    minT = t;
                    exitEdge = he;
                }

                he = he->next;
            } while (he != current->outerComponent);

            // ===== 判断是否继续 =====
            // 如果没有穿出边，或者 minT > 1（终点在当前面内），结束
            if (!exitEdge || minT > 1.0 + EPS) {
                break;
            }

            // ===== 通过 twin 跳转到相邻面 =====
            if (exitEdge->twin) {
                current = exitEdge->twin->incidentFace;
            } else {
                // 到达边界，结束
                break;
            }
        }

        return result;
    }

    /**
     * 暴力查询（用于验证正确性）
     *
     * 遍历所有面，检查每个面是否与射线相交
     * 时间复杂度：O(n)，n 是面数
     */
    vector<Face*> bruteForceQuery(const Point2D& p1, const Point2D& p2) {
        vector<Face*> result;

        for (Face* f : faces) {
            // 遍历面的每条边，检查与射线的相交
            HalfEdge* he = f->outerComponent;
            bool intersects = false;

            do {
                Point2D a = he->origin->position;
                Point2D b = he->next->origin->position;

                // 计算方向值
                double d1 = orient2d(a, b, p1);
                double d2 = orient2d(a, b, p2);
                double d3 = orient2d(p1, p2, a);
                double d4 = orient2d(p1, p2, b);

                // 判断是否相交（标准跨越测试）
                if (((d1 > EPS && d2 < -EPS) || (d1 < -EPS && d2 > EPS)) &&
                    ((d3 > EPS && d4 < -EPS) || (d3 < -EPS && d4 > EPS))) {
                    intersects = true;
                    break;
                }

                he = he->next;
            } while (he != f->outerComponent);

            // 也检查端点是否在面内
            if (!intersects) {
                if (pointInFace(p1, f) || pointInFace(p2, f)) {
                    intersects = true;
                }
            }

            if (intersects) {
                result.push_back(f);
            }
        }

        return result;
    }

    // ========== 辅助函数 ==========

    int numFaces() const { return faces.size(); }
    int numVertices() const { return vertices.size(); }
    int numHalfEdges() const { return halfEdges.size(); }
    Face* getFace(int i) const { return faces[i]; }

    /**
     * 打印统计信息
     */
    void printStatistics() const {
        cout << "\n=== 数据结构统计 ===" << endl;
        cout << "顶点数: " << vertices.size() << endl;
        cout << "半边数: " << halfEdges.size() << endl;
        cout << "面数: " << faces.size() << endl;

        // 统计 twin 指针的设置情况
        int twinCount = 0;       // 有 twin 的半边数
        int boundaryCount = 0;   // 边界边数

        for (HalfEdge* he : halfEdges) {
            if (he->twin) {
                twinCount++;
            } else {
                boundaryCount++;
            }
        }

        // 内部边数 = 有 twin 的半边数 / 2（因为成对）
        cout << "内部边（有 twin）: " << twinCount / 2 << endl;
        // 边界边没有 twin
        cout << "边界边（无 twin）: " << boundaryCount << endl;
    }
};

// ============================================================================
// 第四部分：测试程序
// ============================================================================

/**
 * 创建测试用的三角网格（带完整拓扑）
 *
 * 【网格布局】
 *     (0,2)-----(1,2)-----(2,2)
 *       |  \   1  |  \  3   |
 *       | 0  \    | 2  \    |
 *     (0,1)-----(1,1)-----(2,1)
 *       |  \   5  |  \  7   |
 *       | 4  \    | 6  \    |
 *     (0,0)-----(1,0)-----(2,0)
 *
 * 【顶点编号】
 *     6 ----- 7 ----- 8
 *     |       |       |
 *     3 ----- 4 ----- 5
 *     |       |       |
 *     0 ----- 1 ----- 2
 *
 * 使用顶点索引方式添加三角形，确保共享边的 twin 指针正确设置
 */
void createTestMesh(PlanarSubdivision& subdivision) {
    cout << "=== 创建测试三角网格（带完整拓扑）===" << endl;

    // ===== 添加顶点 =====
    // 按照从左到右、从下到上的顺序添加 9 个顶点
    vector<Point2D> points;
    points.push_back(Point2D(0, 0));  // 索引 0
    points.push_back(Point2D(1, 0));  // 索引 1
    points.push_back(Point2D(2, 0));  // 索引 2
    points.push_back(Point2D(0, 1));  // 索引 3
    points.push_back(Point2D(1, 1));  // 索引 4
    points.push_back(Point2D(2, 1));  // 索引 5
    points.push_back(Point2D(0, 2));  // 索引 6
    points.push_back(Point2D(1, 2));  // 索引 7
    points.push_back(Point2D(2, 2));  // 索引 8

    subdivision.addVertices(points);

    // ===== 添加三角形（按逆时针顺序指定顶点）=====

    // 第一行（y = 1 到 y = 2）
    subdivision.addTriangle(3, 6, 7);  // 面 0：左上三角形
    subdivision.addTriangle(3, 7, 4);  // 面 1：与面 0 共享边 (3,7)
    subdivision.addTriangle(4, 7, 8);  // 面 2：与面 1 共享边 (4,7)
    subdivision.addTriangle(4, 8, 5);  // 面 3：与面 2 共享边 (4,8)

    // 第二行（y = 0 到 y = 1）
    subdivision.addTriangle(0, 3, 4);  // 面 4：左下三角形，与面 0 共享边 (3,4)
    subdivision.addTriangle(0, 4, 1);  // 面 5：与面 4 共享边 (0,4)
    subdivision.addTriangle(1, 4, 5);  // 面 6：与面 5 共享边 (1,4)，与面 3 共享边 (4,5)
    subdivision.addTriangle(1, 5, 2);  // 面 7：与面 6 共享边 (1,5)

    cout << "创建了 " << subdivision.numFaces() << " 个三角形面" << endl;
    subdivision.printStatistics();
}

/**
 * 运行查询测试
 */
void runQueryTests(PlanarSubdivision& subdivision) {
    cout << "\n=== 运行直线相交查询测试（射线行走法）===" << endl;

    // ========== 测试用例 1：对角线查询 ==========
    {
        Point2D p1(0.2, 0.2);
        Point2D p2(1.8, 1.8);

        cout << "\n测试 1：对角线查询" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        // 计时
        auto start = chrono::high_resolution_clock::now();
        vector<Face*> result = subdivision.rayWalkQuery(p1, p2);
        auto end = chrono::high_resolution_clock::now();

        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (Face* f : result) {
            cout << f->id << " ";
        }
        cout << endl;
        cout << "查询耗时: " << duration.count() << " 微秒" << endl;

        // 用暴力方法验证结果
        vector<Face*> bruteResult = subdivision.bruteForceQuery(p1, p2);
        cout << "暴力验证结果: " << bruteResult.size() << " 个面" << endl;
    }

    // ========== 测试用例 2：水平线查询 ==========
    {
        Point2D p1(0.0, 1.0);
        Point2D p2(2.0, 1.0);

        cout << "\n测试 2：水平线查询（沿 y=1）" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        vector<Face*> result = subdivision.rayWalkQuery(p1, p2);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (Face* f : result) {
            cout << f->id << " ";
        }
        cout << endl;
    }

    // ========== 测试用例 3：垂直线查询 ==========
    {
        Point2D p1(1.0, 0.0);
        Point2D p2(1.0, 2.0);

        cout << "\n测试 3：垂直线查询（沿 x=1）" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        vector<Face*> result = subdivision.rayWalkQuery(p1, p2);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (Face* f : result) {
            cout << f->id << " ";
        }
        cout << endl;
    }

    // ========== 测试用例 4：短线段查询 ==========
    {
        Point2D p1(0.3, 0.3);
        Point2D p2(0.4, 0.4);

        cout << "\n测试 4：短线段查询（在单个三角形内）" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        vector<Face*> result = subdivision.rayWalkQuery(p1, p2);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (Face* f : result) {
            cout << f->id << " ";
        }
        cout << endl;
    }
}

/**
 * 性能测试
 * 比较射线行走法与暴力方法的性能
 */
void runPerformanceTest() {
    cout << "\n=== 性能测试 ===" << endl;

    // 创建大规模网格
    PlanarSubdivision subdivision;

    int gridSize = 10;
    cout << "生成 " << gridSize << "x" << gridSize << " 三角网格 ("
         << gridSize * gridSize * 2 << " 个三角形)" << endl;

    // ===== 添加顶点 =====
    // (gridSize + 1) × (gridSize + 1) 个顶点
    vector<Point2D> points;
    for (int j = 0; j <= gridSize; j++) {
        for (int i = 0; i <= gridSize; i++) {
            points.push_back(Point2D(i, j));
        }
    }
    subdivision.addVertices(points);

    // ===== 添加三角形 =====
    // 每个方格分成两个三角形
    for (int j = 0; j < gridSize; j++) {
        for (int i = 0; i < gridSize; i++) {
            // 计算方格四个角的顶点索引
            int v00 = j * (gridSize + 1) + i;        // 左下
            int v10 = j * (gridSize + 1) + i + 1;    // 右下
            int v01 = (j + 1) * (gridSize + 1) + i;  // 左上
            int v11 = (j + 1) * (gridSize + 1) + i + 1;  // 右上

            // 添加两个三角形
            subdivision.addTriangle(v00, v01, v11);  // 左下-左上-右上
            subdivision.addTriangle(v00, v11, v10);  // 左下-右上-右下
        }
    }

    subdivision.printStatistics();

    // ===== 执行多次随机查询 =====
    int numQueries = 1000;
    cout << "\n执行 " << numQueries << " 次随机查询..." << endl;

    srand(42);  // 固定随机种子

    int totalIntersections = 0;
    auto queryStart = chrono::high_resolution_clock::now();

    for (int q = 0; q < numQueries; q++) {
        // 生成随机射线
        double x1 = (double)rand() / RAND_MAX * gridSize;
        double y1 = (double)rand() / RAND_MAX * gridSize;
        double x2 = (double)rand() / RAND_MAX * gridSize;
        double y2 = (double)rand() / RAND_MAX * gridSize;

        vector<Face*> result = subdivision.rayWalkQuery(
            Point2D(x1, y1), Point2D(x2, y2));
        totalIntersections += result.size();
    }

    auto queryEnd = chrono::high_resolution_clock::now();
    auto queryDuration = chrono::duration_cast<chrono::microseconds>(queryEnd - queryStart);

    cout << "总查询耗时: " << queryDuration.count() << " 微秒" << endl;
    cout << "平均每次查询: " << queryDuration.count() / numQueries << " 微秒" << endl;
    cout << "平均每次查询相交的多边形数: " << (double)totalIntersections / numQueries << endl;

    // ===== 对比暴力方法 =====
    cout << "\n对比暴力方法..." << endl;
    auto bruteStart = chrono::high_resolution_clock::now();

    srand(42);  // 重置随机种子，确保相同的查询
    for (int q = 0; q < numQueries; q++) {
        double x1 = (double)rand() / RAND_MAX * gridSize;
        double y1 = (double)rand() / RAND_MAX * gridSize;
        double x2 = (double)rand() / RAND_MAX * gridSize;
        double y2 = (double)rand() / RAND_MAX * gridSize;

        subdivision.bruteForceQuery(Point2D(x1, y1), Point2D(x2, y2));
    }

    auto bruteEnd = chrono::high_resolution_clock::now();
    auto bruteDuration = chrono::duration_cast<chrono::microseconds>(bruteEnd - bruteStart);

    cout << "暴力方法总耗时: " << bruteDuration.count() << " 微秒" << endl;
    cout << "加速比: " << (double)bruteDuration.count() / queryDuration.count() << "x" << endl;
}

/**
 * 主函数
 */
int main() {
    cout << "========================================================" << endl;
    cout << "  习题 1.3：平面分割数据结构与直线相交查询" << endl;
    cout << "  解法二：射线行走法（Ray Walking）" << endl;
    cout << "========================================================" << endl;

    // 创建平面分割数据结构
    PlanarSubdivision subdivision;

    // 创建测试网格
    createTestMesh(subdivision);

    // 运行查询测试
    runQueryTests(subdivision);

    // 运行性能测试
    runPerformanceTest();

    cout << "\n=== 程序结束 ===" << endl;

    return 0;
}
