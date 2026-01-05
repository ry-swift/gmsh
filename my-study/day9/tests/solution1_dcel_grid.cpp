/**
 * ============================================================================
 * 习题 1.3：平面分割数据结构与直线相交查询
 * 解法一：DCEL（双向连接边表）+ 均匀网格方法
 * ============================================================================
 *
 * 【思考过程记录】
 *
 * 问题分析：
 * ----------
 * 给定一个由线段将平面分割成的多边形划分（如三角网格），需要：
 * 1. 设计数据结构存储平面分割信息
 * 2. 给定直线 p1p2，找出所有与之相交的多边形
 *
 * 方案选择：
 * ----------
 * 选择 DCEL + 均匀网格的组合方案，原因如下：
 *
 * 1. DCEL（半边结构）的优势：
 *    - 高效存储拓扑信息（顶点、边、面的邻接关系）
 *    - O(1) 时间访问相邻的边、面
 *    - 支持快速遍历多边形边界
 *
 * 2. 均匀网格的优势：
 *    - 实现简单，易于理解和调试
 *    - 对均匀分布的数据效果好
 *    - 空间查询复杂度接近 O(1)
 *
 * 算法流程：
 * ----------
 * 构建阶段：
 *   1. 从多边形列表构建 DCEL 结构
 *   2. 计算全局包围盒，确定网格参数
 *   3. 将每个面注册到其包围盒覆盖的网格单元中
 *
 * 查询阶段：
 *   1. 使用 DDA 算法遍历直线经过的网格单元
 *   2. 收集所有候选面（使用集合去重）
 *   3. 对每个候选面进行精确的直线-多边形相交测试
 *   4. 返回所有相交的多边形
 *
 * 时间复杂度分析：
 * ----------------
 * - 预处理：O(n × k)，n 为面数，k 为平均每面覆盖的网格单元数
 * - 查询：O(L/c × f)，L 为直线长度，c 为单元尺寸，f 为平均每单元的面数
 *
 * 优缺点：
 * --------
 * 优点：实现简单、对均匀分布数据效果好
 * 缺点：对非均匀分布数据效率可能下降
 *
 * ============================================================================
 */

// ============================================================================
// 头文件包含区域
// ============================================================================

#include <iostream>   // 标准输入输出流，用于 cout、endl 等
                      // 原因：需要在控制台输出调试信息和结果

#include <vector>     // 动态数组容器，用于存储顶点、边、面等列表
                      // 原因：需要动态管理大小不定的元素集合

#include <set>        // 有序集合容器，用于去重候选面
                      // 原因：同一个面可能被多个网格单元引用，需要去重

#include <cmath>      // 数学函数库，提供 sqrt、abs、min、max 等
                      // 原因：几何计算需要这些数学运算

#include <algorithm>  // STL 算法库，提供 min、max 等
                      // 原因：需要比较和排序操作

#include <chrono>     // C++11 时间库，用于性能测量
                      // 原因：需要精确测量算法执行时间

#include <iomanip>    // 输出格式控制，用于设置数字精度
                      // 原因：控制浮点数输出格式

using namespace std;  // 使用标准命名空间，避免每次写 std:: 前缀
                      // 原因：简化代码书写，提高可读性

// ============================================================================
// 第一部分：基础数据结构定义
// ============================================================================

/**
 * 2D 点结构
 * 用于存储平面上的坐标点
 *
 * 【设计说明】
 * - 使用 struct 而非 class，因为这是一个简单的数据容器
 * - 提供常用的向量运算符重载，方便几何计算
 */
struct Point2D {
    double x, y;  // x 坐标和 y 坐标
                  // 使用 double 而非 float，保证足够的计算精度

    /**
     * 构造函数
     * @param _x x 坐标，默认为 0
     * @param _y y 坐标，默认为 0
     *
     * 使用初始化列表 : x(_x), y(_y) 而非函数体内赋值
     * 原因：初始化列表效率更高，避免先默认初始化再赋值
     */
    Point2D(double _x = 0, double _y = 0) : x(_x), y(_y) {}

    // ========== 向量运算符重载 ==========
    // 这些运算符将点视为二维向量进行运算

    /**
     * 向量加法：返回两点（向量）之和
     * 用途：计算位移后的点位置
     */
    Point2D operator+(const Point2D& p) const { return Point2D(x + p.x, y + p.y); }

    /**
     * 向量减法：返回两点（向量）之差
     * 用途：计算从点 p 到当前点的方向向量
     */
    Point2D operator-(const Point2D& p) const { return Point2D(x - p.x, y - p.y); }

    /**
     * 标量乘法：返回向量与标量的乘积
     * 用途：缩放向量或计算线性插值
     */
    Point2D operator*(double s) const { return Point2D(x * s, y * s); }

    /**
     * 友元函数：输出运算符重载
     * 用途：方便调试时打印点的坐标
     * 格式：(x, y)
     */
    friend ostream& operator<<(ostream& os, const Point2D& p) {
        os << "(" << p.x << ", " << p.y << ")";
        return os;  // 返回 ostream 引用，支持链式调用如 cout << p1 << p2
    }
};

// ========== 前向声明 ==========
// 在 Vertex 类中需要使用 HalfEdge 指针，但 HalfEdge 定义在后面
// 因此需要前向声明，告诉编译器这些类将在后面定义
class HalfEdge;  // 半边类的前向声明
class Face;      // 面类的前向声明

/**
 * 顶点类（Vertex）
 *
 * 【DCEL 中的顶点】
 * 顶点是 DCEL 中最基本的元素，存储：
 * 1. 顶点的几何位置（坐标）
 * 2. 从该顶点出发的一条半边（用于遍历邻接结构）
 * 3. 顶点的唯一标识 ID
 */
class Vertex {
public:
    Point2D position;       // 顶点在平面上的坐标位置
                            // 类型为 Point2D，包含 x、y 两个分量

    HalfEdge* incidentEdge; // 指向一条从该顶点出发的半边
                            // 【重要】通过这条边可以遍历所有从该顶点出发的边
                            // 使用指针而非引用，因为初始时可能为空

    int id;                 // 顶点的唯一标识符
                            // 用于调试输出和查找

    /**
     * 构造函数（使用坐标值）
     * @param x x 坐标，默认 0
     * @param y y 坐标，默认 0
     * @param _id 顶点 ID，默认 -1 表示未设置
     */
    Vertex(double x = 0, double y = 0, int _id = -1)
        : position(x, y),           // 初始化位置
          incidentEdge(nullptr),    // 初始时没有关联的边，设为空指针
          id(_id) {}                // 设置 ID

    /**
     * 构造函数（使用 Point2D 对象）
     * @param p Point2D 类型的坐标
     * @param _id 顶点 ID
     *
     * 提供这个重载版本是为了方便从已有的 Point2D 创建顶点
     */
    Vertex(const Point2D& p, int _id = -1)
        : position(p),              // 直接使用 Point2D 初始化
          incidentEdge(nullptr),
          id(_id) {}
};

/**
 * 半边类（Half-Edge）
 *
 * 【DCEL 的核心结构】
 * DCEL（Doubly-Connected Edge List，双向连接边表）使用"半边"概念：
 * - 每条几何边由两个方向相反的半边组成
 * - 半边是有方向的，从 origin 指向 destination
 *
 * 【半边的五个关键指针】
 * 1. origin: 半边的起点顶点
 * 2. twin: 方向相反的配对半边（共享同一条几何边）
 * 3. next: 同一个面上逆时针方向的下一条半边
 * 4. prev: 同一个面上逆时针方向的上一条半边
 * 5. incidentFace: 半边所属的面（半边在面的边界上，面在半边的左侧）
 *
 * 【示意图】
 *        v2
 *       /  \
 *   he1 /    \ he2
 *     /  F1   \
 *    v0-------v1
 *       he0
 *
 * 对于半边 he0（从 v0 到 v1）：
 * - origin = v0
 * - next = he1（逆时针下一条）
 * - prev = he2（逆时针上一条）
 * - incidentFace = F1
 * - twin 指向从 v1 到 v0 的半边（属于相邻面）
 */
class HalfEdge {
public:
    Vertex* origin;         // 半边的起点（origin），不是终点！
                            // 终点通过 next->origin 获取

    HalfEdge* twin;         // 对偶半边（twin），方向相反
                            // 【关键】通过 twin 可以访问相邻面
                            // 如果 twin 为 nullptr，表示这是边界边

    HalfEdge* next;         // 同一面上逆时针方向的下一条半边
                            // 【作用】遍历面的边界时沿 next 方向走

    HalfEdge* prev;         // 同一面上逆时针方向的上一条半边
                            // 【作用】反向遍历面的边界

    Face* incidentFace;     // 半边所属的面
                            // 【规定】面在半边的左侧

    int id;                 // 半边的唯一标识符

    /**
     * 构造函数
     * @param v 半边的起点顶点
     * @param _id 半边 ID
     *
     * 所有指针初始化为 nullptr，后续构建 DCEL 时再设置
     */
    HalfEdge(Vertex* v = nullptr, int _id = -1)
        : origin(v),
          twin(nullptr),      // 初始无配对边
          next(nullptr),      // 初始无下一条边
          prev(nullptr),      // 初始无上一条边
          incidentFace(nullptr),  // 初始不属于任何面
          id(_id) {}

    /**
     * 获取半边的终点
     *
     * 【为什么不直接存储终点？】
     * DCEL 设计中不存储终点，而是通过 next->origin 获取
     * 原因：避免数据冗余，保证一致性
     *
     * @return 半边终点的顶点指针
     */
    Vertex* destination() const {
        // 下一条半边的起点就是当前半边的终点
        return next ? next->origin : nullptr;
    }
};

/**
 * 面类（Face）
 *
 * 【DCEL 中的面】
 * 面表示平面分割中的一个多边形区域
 * 存储指向边界上一条半边的指针，通过该指针可遍历整个边界
 *
 * 【包围盒优化】
 * 额外存储面的轴对齐包围盒（AABB），用于：
 * 1. 快速排除不可能相交的多边形
 * 2. 构建均匀网格索引
 */
class Face {
public:
    HalfEdge* outerComponent;  // 外边界上的一条半边
                               // 通过沿 next 方向遍历可以访问所有边界半边

    int id;                    // 面的唯一标识符

    // ========== 包围盒（用于空间索引加速）==========
    double minX, minY;         // 包围盒的左下角坐标
    double maxX, maxY;         // 包围盒的右上角坐标

    /**
     * 构造函数
     * @param he 边界上的一条半边
     * @param _id 面 ID
     *
     * 包围盒初始化为"反向无穷"，便于后续取 min/max 更新
     */
    Face(HalfEdge* he = nullptr, int _id = -1)
        : outerComponent(he), id(_id),
          minX(1e18), minY(1e18),    // 初始化为极大值
          maxX(-1e18), maxY(-1e18) {} // 初始化为极小值

    /**
     * 计算面的轴对齐包围盒（AABB）
     *
     * 【算法】
     * 遍历面的所有顶点，更新包围盒的四个边界值
     *
     * 【为什么需要包围盒？】
     * 1. 快速排除：如果直线不与包围盒相交，则不可能与面相交
     * 2. 空间索引：将面注册到均匀网格时需要知道它覆盖哪些单元
     */
    void computeBoundingBox() {
        // 如果没有边界半边，无法计算
        if (!outerComponent) return;

        // 重置包围盒为反向无穷
        minX = minY = 1e18;
        maxX = maxY = -1e18;

        // 从第一条半边开始遍历
        HalfEdge* he = outerComponent;
        do {
            // 获取当前半边起点的坐标
            double x = he->origin->position.x;
            double y = he->origin->position.y;

            // 更新包围盒边界
            minX = min(minX, x);  // 取最小 x
            minY = min(minY, y);  // 取最小 y
            maxX = max(maxX, x);  // 取最大 x
            maxY = max(maxY, y);  // 取最大 y

            // 移动到下一条半边
            he = he->next;
        } while (he != outerComponent);  // 直到回到起始半边
    }

    /**
     * 获取面的所有顶点坐标列表
     *
     * 【用途】
     * 1. 调试时输出面的形状
     * 2. 进行精确的相交测试
     *
     * @return 顶点坐标的向量，按逆时针顺序
     */
    vector<Point2D> getVertices() const {
        vector<Point2D> vertices;  // 存储顶点的向量

        // 检查面是否有效
        if (!outerComponent) return vertices;

        // 遍历边界收集顶点
        HalfEdge* he = outerComponent;
        do {
            vertices.push_back(he->origin->position);  // 添加当前半边起点
            he = he->next;  // 移动到下一条半边
        } while (he != outerComponent);  // 回到起点结束

        return vertices;
    }
};

// ============================================================================
// 第二部分：几何算法
// ============================================================================

/**
 * orient2d - 方向判定函数（2D 方向测试）
 *
 * 【核心几何算法 - 计算几何的基石】
 *
 * 功能：判断点 c 相对于有向线段 ab 的位置关系
 *
 * 返回值含义：
 * - 返回值 > 0: 点 c 在有向线段 ab 的左侧（逆时针方向）
 * - 返回值 < 0: 点 c 在有向线段 ab 的右侧（顺时针方向）
 * - 返回值 = 0: 点 c 在直线 ab 上（三点共线）
 *
 * 【数学原理】
 * 计算向量 (b-a) 和 (c-a) 的叉积（2D 叉积的 z 分量）：
 *
 * (b-a) × (c-a) = | (b.x - a.x)  (c.x - a.x) |
 *                 | (b.y - a.y)  (c.y - a.y) |
 *               = (b.x - a.x)(c.y - a.y) - (b.y - a.y)(c.x - a.x)
 *
 * 几何意义：
 * - 这个值是由向量 (b-a) 和 (c-a) 构成的平行四边形的有符号面积的两倍
 * - 正值表示从 (b-a) 到 (c-a) 是逆时针旋转
 *
 * 【应用场景】
 * 1. 判断点在直线的哪一侧
 * 2. 判断三角形的朝向（顺时针/逆时针）
 * 3. 判断线段是否相交
 * 4. 凸包算法
 *
 * @param a 线段起点
 * @param b 线段终点
 * @param c 待判断的点
 * @return 叉积值（有符号）
 */
double orient2d(const Point2D& a, const Point2D& b, const Point2D& c) {
    // 计算 2D 叉积
    // 展开：(b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/**
 * onSegment - 判断点是否在线段上
 *
 * 【前提条件】已知点 r 与线段 pq 共线
 *
 * 【算法原理】
 * 如果点 r 在线段 pq 上，那么 r 的坐标必须在 p 和 q 的坐标范围内
 *
 * @param p 线段端点 1
 * @param q 线段端点 2
 * @param r 待判断的点（已知与 pq 共线）
 * @return true 如果 r 在线段 pq 上
 */
bool onSegment(const Point2D& p, const Point2D& q, const Point2D& r) {
    // 检查 r 的 x 坐标是否在 [min(p.x, q.x), max(p.x, q.x)] 范围内
    // 同时检查 y 坐标是否在 [min(p.y, q.y), max(p.y, q.y)] 范围内
    return r.x <= max(p.x, q.x) && r.x >= min(p.x, q.x) &&
           r.y <= max(p.y, q.y) && r.y >= min(p.y, q.y);
}

/**
 * segmentsIntersect - 判断两条线段是否相交
 *
 * 【算法原理】
 * 使用方向测试（orient2d）判断：
 *
 * 两条线段 AB 和 CD 相交，当且仅当：
 * 1. C 和 D 分别在 AB 的两侧，且
 * 2. A 和 B 分别在 CD 的两侧
 *
 * 特殊情况：端点重合或共线时需要额外处理
 *
 * 【图示】
 *     A -------- B          A -------- B
 *          \/                    |
 *          /\                    |
 *     C -------- D          C ---+---- D  （不相交）
 *     （相交）
 *
 * @param p1, p2 第一条线段的两个端点
 * @param p3, p4 第二条线段的两个端点
 * @return true 如果两条线段相交
 */
bool segmentsIntersect(const Point2D& p1, const Point2D& p2,
                       const Point2D& p3, const Point2D& p4) {
    // 计算 p1、p2 相对于线段 p3p4 的方向
    double d1 = orient2d(p3, p4, p1);  // p1 相对于 p3p4 的方向
    double d2 = orient2d(p3, p4, p2);  // p2 相对于 p3p4 的方向

    // 计算 p3、p4 相对于线段 p1p2 的方向
    double d3 = orient2d(p1, p2, p3);  // p3 相对于 p1p2 的方向
    double d4 = orient2d(p1, p2, p4);  // p4 相对于 p1p2 的方向

    // 标准相交情况：两条线段互相跨越
    // d1 和 d2 异号 表示 p1 和 p2 在 p3p4 的两侧
    // d3 和 d4 异号 表示 p3 和 p4 在 p1p2 的两侧
    if (((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
        ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0))) {
        return true;
    }

    // 处理共线和端点重合的特殊情况
    const double EPS = 1e-10;  // 容差值，处理浮点数精度问题

    // 如果 p1 在 p3p4 上（d1 ≈ 0）且在线段范围内
    if (abs(d1) < EPS && onSegment(p3, p4, p1)) return true;

    // 如果 p2 在 p3p4 上
    if (abs(d2) < EPS && onSegment(p3, p4, p2)) return true;

    // 如果 p3 在 p1p2 上
    if (abs(d3) < EPS && onSegment(p1, p2, p3)) return true;

    // 如果 p4 在 p1p2 上
    if (abs(d4) < EPS && onSegment(p1, p2, p4)) return true;

    // 不相交
    return false;
}

/**
 * lineIntersectsBox - 判断线段是否与矩形包围盒相交
 *
 * 【用途】
 * 作为预筛选步骤，快速排除不可能相交的多边形
 *
 * 【算法】
 * 1. 首先进行快速排除测试
 * 2. 然后检查线段与矩形四条边的相交
 * 3. 最后检查线段端点是否在矩形内
 *
 * @param p1, p2 线段的两个端点
 * @param minX, minY, maxX, maxY 矩形包围盒的边界
 * @return true 如果线段与矩形相交或接触
 */
bool lineIntersectsBox(const Point2D& p1, const Point2D& p2,
                       double minX, double minY,
                       double maxX, double maxY) {
    // ===== 快速排除测试 =====
    // 如果线段完全在矩形的某一侧外面，则不可能相交

    // 线段完全在矩形左侧或右侧
    if (max(p1.x, p2.x) < minX || min(p1.x, p2.x) > maxX) return false;

    // 线段完全在矩形上方或下方
    if (max(p1.y, p2.y) < minY || min(p1.y, p2.y) > maxY) return false;

    // ===== 检查与四条边的相交 =====
    // 矩形的四个角点（按顺时针或逆时针顺序）
    Point2D corners[4] = {
        Point2D(minX, minY),   // 左下角
        Point2D(maxX, minY),   // 右下角
        Point2D(maxX, maxY),   // 右上角
        Point2D(minX, maxY)    // 左上角
    };

    // 检查线段与矩形每条边的相交
    for (int i = 0; i < 4; i++) {
        // corners[i] 到 corners[(i+1)%4] 是一条边
        if (segmentsIntersect(p1, p2, corners[i], corners[(i+1)%4])) {
            return true;
        }
    }

    // ===== 检查端点是否在矩形内 =====
    // 如果线段完全在矩形内部，上面的边相交测试会失败
    // 所以还需要检查端点是否在矩形内

    // 检查 p1 是否在矩形内
    if (p1.x >= minX && p1.x <= maxX && p1.y >= minY && p1.y <= maxY) return true;

    // 检查 p2 是否在矩形内
    if (p2.x >= minX && p2.x <= maxX && p2.y >= minY && p2.y <= maxY) return true;

    return false;
}

/**
 * lineIntersectsPolygon - 判断线段是否与多边形相交
 *
 * 【算法】
 * 1. 先用包围盒快速排除（剪枝优化）
 * 2. 检查线段是否与多边形的任一边相交
 *
 * 【注意】
 * 这个函数没有检查线段端点是否在多边形内部
 * 对于本问题（查找直线穿过的多边形）这足够了
 * 因为如果线段穿过多边形内部，必然与某条边相交
 *
 * @param p1, p2 线段的两个端点
 * @param face 待检测的多边形面
 * @return true 如果线段与多边形相交
 */
bool lineIntersectsPolygon(const Point2D& p1, const Point2D& p2, Face* face) {
    // ===== 包围盒快速排除 =====
    // 如果线段与包围盒都不相交，则不可能与多边形相交
    if (!lineIntersectsBox(p1, p2, face->minX, face->minY,
                                  face->maxX, face->maxY)) {
        return false;
    }

    // ===== 检查与多边形各边的相交 =====
    HalfEdge* start = face->outerComponent;  // 获取起始半边
    HalfEdge* he = start;

    do {
        // 获取当前边的两个端点
        Point2D a = he->origin->position;           // 边的起点
        Point2D b = he->next->origin->position;     // 边的终点

        // 检查线段是否与这条边相交
        if (segmentsIntersect(p1, p2, a, b)) {
            return true;  // 找到相交，立即返回
        }

        he = he->next;  // 移动到下一条边
    } while (he != start);  // 遍历完所有边

    return false;  // 没有找到相交的边
}

// ============================================================================
// 第三部分：DCEL + 均匀网格数据结构
// ============================================================================

/**
 * 平面分割类（PlanarSubdivision）
 *
 * 【核心数据结构】
 * 结合 DCEL 和均匀网格实现高效的直线相交查询
 *
 * 【设计理念】
 * - DCEL 部分：存储平面分割的拓扑结构
 * - 均匀网格部分：提供快速的空间查询能力
 */
class PlanarSubdivision {
private:
    // ========== DCEL 存储 ==========
    vector<Vertex*> vertices;      // 所有顶点的列表
    vector<HalfEdge*> halfEdges;   // 所有半边的列表
    vector<Face*> faces;           // 所有面的列表

    // ========== 均匀网格参数 ==========
    int gridNx, gridNy;            // 网格在 x 和 y 方向的单元数
                                   // 例如 gridNx=50 表示 x 方向分成 50 个单元

    double cellWidth, cellHeight;  // 每个网格单元的宽度和高度
                                   // cellWidth = (globalMaxX - globalMinX) / gridNx

    double globalMinX, globalMinY; // 整个网格的左下角坐标（原点）
    double globalMaxX, globalMaxY; // 整个网格的右上角坐标

    // ========== 网格存储 ==========
    // 三维向量：grid[i][j] 存储覆盖单元 (i,j) 的所有面
    // 为什么是三维？
    // - 第一维 [i]：x 方向的单元索引
    // - 第二维 [j]：y 方向的单元索引
    // - 第三维：该单元内的面列表
    vector<vector<vector<Face*>>> grid;

public:
    /**
     * 构造函数
     * @param nx x 方向的网格数，默认 50
     * @param ny y 方向的网格数，默认 50
     *
     * 【参数选择建议】
     * - 太少：每个单元包含太多面，失去加速效果
     * - 太多：空间浪费，遍历单元开销增加
     * - 经验值：取 sqrt(面数) 左右
     */
    PlanarSubdivision(int nx = 50, int ny = 50)
        : gridNx(nx), gridNy(ny),
          cellWidth(0), cellHeight(0),
          globalMinX(0), globalMinY(0),
          globalMaxX(0), globalMaxY(0) {}

    /**
     * 析构函数
     * 释放所有动态分配的内存
     *
     * 【内存管理】
     * DCEL 中的顶点、半边、面都是通过 new 分配的
     * 需要在析构时 delete 释放，否则会内存泄漏
     */
    ~PlanarSubdivision() {
        // 释放所有顶点
        for (auto v : vertices) delete v;
        // 释放所有半边
        for (auto e : halfEdges) delete e;
        // 释放所有面
        for (auto f : faces) delete f;
    }

    /**
     * 添加一个多边形面
     *
     * 【功能】
     * 根据多边形顶点列表创建 DCEL 结构
     *
     * 【简化说明】
     * 这是一个简化实现：
     * - 每个多边形独立创建顶点和半边
     * - 不处理共享边的 twin 指针连接
     * - 完整的 DCEL 构建需要更复杂的边匹配算法
     *
     * @param polygon 多边形顶点列表（按逆时针顺序）
     * @return 创建的面指针
     */
    Face* addFace(const vector<Point2D>& polygon) {
        int n = polygon.size();

        // 多边形至少需要 3 个顶点
        if (n < 3) return nullptr;

        // ===== 创建面 =====
        Face* face = new Face();
        face->id = faces.size();  // ID 为当前面的数量（从 0 开始）

        // ===== 创建顶点 =====
        vector<Vertex*> faceVertices(n);  // 存储这个面的顶点
        for (int i = 0; i < n; i++) {
            // 创建新顶点，ID 为当前顶点总数
            Vertex* v = new Vertex(polygon[i], vertices.size());
            vertices.push_back(v);     // 添加到全局顶点列表
            faceVertices[i] = v;       // 记录到局部列表
        }

        // ===== 创建半边 =====
        vector<HalfEdge*> faceEdges(n);  // 存储这个面的半边
        for (int i = 0; i < n; i++) {
            // 创建半边，起点为 faceVertices[i]
            HalfEdge* he = new HalfEdge(faceVertices[i], halfEdges.size());
            he->incidentFace = face;              // 设置所属面
            faceVertices[i]->incidentEdge = he;   // 顶点关联这条出边
            halfEdges.push_back(he);              // 添加到全局半边列表
            faceEdges[i] = he;                    // 记录到局部列表
        }

        // ===== 连接半边形成环 =====
        for (int i = 0; i < n; i++) {
            // next 指向逆时针下一条边
            faceEdges[i]->next = faceEdges[(i + 1) % n];
            // prev 指向逆时针上一条边
            // (i - 1 + n) % n 确保索引非负
            faceEdges[i]->prev = faceEdges[(i - 1 + n) % n];
        }

        // ===== 设置面的属性 =====
        face->outerComponent = faceEdges[0];  // 外边界从第一条半边开始
        face->computeBoundingBox();           // 计算包围盒
        faces.push_back(face);                // 添加到全局面列表

        return face;
    }

    /**
     * 构建空间索引（均匀网格）
     *
     * 【算法步骤】
     * 1. 计算所有面的全局包围盒
     * 2. 确定网格单元尺寸
     * 3. 将每个面注册到其覆盖的所有网格单元中
     *
     * 【注意】
     * 必须在所有面添加完成后调用
     */
    void buildSpatialIndex() {
        // 检查是否有面需要索引
        if (faces.empty()) return;

        cout << "\n=== 构建均匀网格空间索引 ===" << endl;

        // ===== 步骤 1：计算全局包围盒 =====
        // 遍历所有面，找到整体的坐标范围
        globalMinX = globalMinY = 1e18;   // 初始化为极大值
        globalMaxX = globalMaxY = -1e18;  // 初始化为极小值

        for (Face* f : faces) {
            // 用每个面的包围盒更新全局包围盒
            globalMinX = min(globalMinX, f->minX);
            globalMinY = min(globalMinY, f->minY);
            globalMaxX = max(globalMaxX, f->maxX);
            globalMaxY = max(globalMaxY, f->maxY);
        }

        // 稍微扩展边界，避免边界上的浮点精度问题
        double pad = 0.01;
        globalMinX -= pad; globalMinY -= pad;
        globalMaxX += pad; globalMaxY += pad;

        // ===== 步骤 2：计算单元尺寸 =====
        cellWidth = (globalMaxX - globalMinX) / gridNx;
        cellHeight = (globalMaxY - globalMinY) / gridNy;

        // 输出网格参数
        cout << "全局包围盒: [" << globalMinX << ", " << globalMinY << "] -> ["
             << globalMaxX << ", " << globalMaxY << "]" << endl;
        cout << "网格维度: " << gridNx << " x " << gridNy << endl;
        cout << "单元尺寸: " << cellWidth << " x " << cellHeight << endl;

        // ===== 步骤 3：初始化网格 =====
        // 分配 gridNx × gridNy 的二维向量，每个元素是 Face* 的向量
        grid.assign(gridNx, vector<vector<Face*>>(gridNy));

        // ===== 步骤 4：将面注册到网格单元 =====
        int totalRegistrations = 0;  // 统计总注册次数

        for (Face* f : faces) {
            // 计算面的包围盒覆盖的网格范围
            // 将坐标转换为网格索引
            int minI = (int)((f->minX - globalMinX) / cellWidth);
            int maxI = (int)((f->maxX - globalMinX) / cellWidth);
            int minJ = (int)((f->minY - globalMinY) / cellHeight);
            int maxJ = (int)((f->maxY - globalMinY) / cellHeight);

            // 边界检查，确保索引在有效范围内
            minI = max(0, min(minI, gridNx - 1));
            maxI = max(0, min(maxI, gridNx - 1));
            minJ = max(0, min(minJ, gridNy - 1));
            maxJ = max(0, min(maxJ, gridNy - 1));

            // 将面注册到所有覆盖的单元
            // 一个面可能覆盖多个单元，所以用双重循环
            for (int i = minI; i <= maxI; i++) {
                for (int j = minJ; j <= maxJ; j++) {
                    grid[i][j].push_back(f);  // 将面添加到单元
                    totalRegistrations++;
                }
            }
        }

        // 输出统计信息
        cout << "面数量: " << faces.size() << endl;
        cout << "总注册次数: " << totalRegistrations << endl;
        cout << "平均每面注册单元数: " << (double)totalRegistrations / faces.size() << endl;
    }

    /**
     * 获取坐标对应的网格索引
     *
     * 【公式】
     * i = (x - globalMinX) / cellWidth
     * j = (y - globalMinY) / cellHeight
     *
     * @param x, y 坐标点
     * @return pair<i, j> 网格索引
     */
    pair<int, int> getCellIndex(double x, double y) const {
        // 计算索引（向下取整）
        int i = (int)((x - globalMinX) / cellWidth);
        int j = (int)((y - globalMinY) / cellHeight);

        // 边界检查，确保索引在 [0, gridN-1] 范围内
        i = max(0, min(i, gridNx - 1));
        j = max(0, min(j, gridNy - 1));

        return make_pair(i, j);
    }

    /**
     * 直线相交查询
     *
     * 【核心查询算法】
     *
     * 算法步骤：
     * 1. 使用 DDA（Digital Differential Analyzer）算法
     *    遍历直线经过的所有网格单元
     * 2. 收集这些单元中的所有候选面（使用 set 去重）
     * 3. 对每个候选面进行精确的相交测试
     * 4. 返回真正相交的面列表
     *
     * 【DDA 算法简介】
     * 沿直线方向以固定步长采样点，
     * 将采样点所在的网格单元加入候选集
     *
     * @param p1 直线起点
     * @param p2 直线终点
     * @return 与直线相交的面列表
     */
    vector<Face*> queryLineIntersection(const Point2D& p1, const Point2D& p2) {
        set<Face*> candidateSet;  // 候选面集合，使用 set 自动去重
        vector<Face*> result;     // 最终结果

        // ===== 计算直线方向和长度 =====
        double dx = p2.x - p1.x;  // x 方向分量
        double dy = p2.y - p1.y;  // y 方向分量
        double length = sqrt(dx * dx + dy * dy);  // 直线长度

        // ===== 处理退化情况 =====
        if (length < 1e-10) {
            // 两点重合，只检查这一个点所在的单元
            pair<int, int> cell = getCellIndex(p1.x, p1.y);
            int ci = cell.first, cj = cell.second;

            // 遍历该单元中的所有面
            for (size_t fi = 0; fi < grid[ci][cj].size(); fi++) {
                Face* f = grid[ci][cj][fi];
                // 进行精确相交测试
                if (lineIntersectsPolygon(p1, p2, f)) {
                    result.push_back(f);
                }
            }
            return result;
        }

        // ===== DDA 算法遍历网格单元 =====
        // 步数计算：确保不遗漏任何单元
        // 使用 直线长度 / 最小单元尺寸 × 2 作为步数
        int steps = max(1, (int)(length / min(cellWidth, cellHeight) * 2));

        for (int s = 0; s <= steps; s++) {
            // 计算当前采样点的参数 t ∈ [0, 1]
            double t = (double)s / steps;

            // 计算采样点坐标：P(t) = p1 + t * (p2 - p1)
            double x = p1.x + t * dx;
            double y = p1.y + t * dy;

            // 获取采样点所在的网格单元
            pair<int, int> cell = getCellIndex(x, y);
            int ci = cell.first, cj = cell.second;

            // 将该单元中的所有面加入候选集
            for (size_t fi = 0; fi < grid[ci][cj].size(); fi++) {
                candidateSet.insert(grid[ci][cj][fi]);
                // set 的 insert 会自动忽略重复元素
            }
        }

        // ===== 对候选面进行精确相交测试 =====
        for (Face* f : candidateSet) {
            if (lineIntersectsPolygon(p1, p2, f)) {
                result.push_back(f);
            }
        }

        return result;
    }

    /**
     * 打印统计信息
     * 用于调试和性能分析
     */
    void printStatistics() const {
        cout << "\n=== 数据结构统计 ===" << endl;
        cout << "顶点数: " << vertices.size() << endl;
        cout << "半边数: " << halfEdges.size() << endl;
        cout << "面数: " << faces.size() << endl;

        // 统计网格使用情况
        int nonEmptyCells = 0;      // 非空单元数
        int maxFacesInCell = 0;     // 单元中最大面数
        int totalFacesInCells = 0;  // 所有单元的面数总和

        for (int i = 0; i < gridNx; i++) {
            for (int j = 0; j < gridNy; j++) {
                int count = grid[i][j].size();
                if (count > 0) {
                    nonEmptyCells++;
                    totalFacesInCells += count;
                    maxFacesInCell = max(maxFacesInCell, count);
                }
            }
        }

        cout << "非空网格单元数: " << nonEmptyCells << " / " << (gridNx * gridNy) << endl;
        cout << "单元最大面数: " << maxFacesInCell << endl;
        if (nonEmptyCells > 0) {
            cout << "平均每单元面数: " << (double)totalFacesInCells / nonEmptyCells << endl;
        }
    }

    /**
     * 获取面的数量
     */
    int numFaces() const { return faces.size(); }

    /**
     * 获取指定索引的面
     */
    Face* getFace(int i) const { return faces[i]; }
};

// ============================================================================
// 第四部分：测试程序
// ============================================================================

/**
 * 创建测试用的三角网格
 *
 * 【网格布局】
 * 创建一个 2×2 的方格区域，每个方格分成 2 个三角形
 * 共计 8 个三角形面
 *
 * 网格布局示意图：
 *
 *     (0,2)-----(1,2)-----(2,2)
 *       |  \   1  |  \  3   |
 *       | 0  \    | 2  \    |
 *     (0,1)-----(1,1)-----(2,1)
 *       |  \   5  |  \  7   |
 *       | 4  \    | 6  \    |
 *     (0,0)-----(1,0)-----(2,0)
 *
 * 面编号从 0 到 7
 */
void createTestMesh(PlanarSubdivision& subdivision) {
    cout << "=== 创建测试三角网格 ===" << endl;

    // 定义顶点坐标
    // 三角形按逆时针方向定义（这是 DCEL 的规定）
    vector<Point2D> tri;  // 临时存储三角形顶点

    // ========== 第一行（y = 1 到 y = 2）==========

    // 面 0：左上方格的左下三角形
    tri.clear();
    tri.push_back(Point2D(0, 1));  // 左下角
    tri.push_back(Point2D(0, 2));  // 左上角
    tri.push_back(Point2D(1, 2));  // 右上角
    subdivision.addFace(tri);

    // 面 1：左上方格的右上三角形
    tri.clear();
    tri.push_back(Point2D(0, 1));  // 左下角
    tri.push_back(Point2D(1, 2));  // 右上角
    tri.push_back(Point2D(1, 1));  // 右下角
    subdivision.addFace(tri);

    // 面 2：右上方格的左下三角形
    tri.clear();
    tri.push_back(Point2D(1, 1));
    tri.push_back(Point2D(1, 2));
    tri.push_back(Point2D(2, 2));
    subdivision.addFace(tri);

    // 面 3：右上方格的右上三角形
    tri.clear();
    tri.push_back(Point2D(1, 1));
    tri.push_back(Point2D(2, 2));
    tri.push_back(Point2D(2, 1));
    subdivision.addFace(tri);

    // ========== 第二行（y = 0 到 y = 1）==========

    // 面 4：左下方格的左下三角形
    tri.clear();
    tri.push_back(Point2D(0, 0));
    tri.push_back(Point2D(0, 1));
    tri.push_back(Point2D(1, 1));
    subdivision.addFace(tri);

    // 面 5：左下方格的右上三角形
    tri.clear();
    tri.push_back(Point2D(0, 0));
    tri.push_back(Point2D(1, 1));
    tri.push_back(Point2D(1, 0));
    subdivision.addFace(tri);

    // 面 6：右下方格的左下三角形
    tri.clear();
    tri.push_back(Point2D(1, 0));
    tri.push_back(Point2D(1, 1));
    tri.push_back(Point2D(2, 1));
    subdivision.addFace(tri);

    // 面 7：右下方格的右上三角形
    tri.clear();
    tri.push_back(Point2D(1, 0));
    tri.push_back(Point2D(2, 1));
    tri.push_back(Point2D(2, 0));
    subdivision.addFace(tri);

    cout << "创建了 " << subdivision.numFaces() << " 个三角形面" << endl;
}

/**
 * 运行查询测试
 * 测试不同方向和长度的直线查询
 */
void runQueryTests(PlanarSubdivision& subdivision) {
    cout << "\n=== 运行直线相交查询测试 ===" << endl;

    // ========== 测试用例 1：对角线查询 ==========
    {
        Point2D p1(0.2, 0.2);  // 起点在左下角区域
        Point2D p2(1.8, 1.8);  // 终点在右上角区域

        cout << "\n测试 1：对角线查询" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        // 计时开始
        auto start = chrono::high_resolution_clock::now();

        // 执行查询
        vector<Face*> result = subdivision.queryLineIntersection(p1, p2);

        // 计时结束
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        // 输出结果
        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (Face* f : result) {
            cout << f->id << " ";
        }
        cout << endl;
        cout << "查询耗时: " << duration.count() << " 微秒" << endl;
    }

    // ========== 测试用例 2：水平线查询 ==========
    {
        Point2D p1(0.0, 1.0);  // 左边界中点
        Point2D p2(2.0, 1.0);  // 右边界中点

        cout << "\n测试 2：水平线查询（沿 y=1）" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        vector<Face*> result = subdivision.queryLineIntersection(p1, p2);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (Face* f : result) {
            cout << f->id << " ";
        }
        cout << endl;
    }

    // ========== 测试用例 3：垂直线查询 ==========
    {
        Point2D p1(1.0, 0.0);  // 下边界中点
        Point2D p2(1.0, 2.0);  // 上边界中点

        cout << "\n测试 3：垂直线查询（沿 x=1）" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        vector<Face*> result = subdivision.queryLineIntersection(p1, p2);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (Face* f : result) {
            cout << f->id << " ";
        }
        cout << endl;
    }

    // ========== 测试用例 4：短线段查询 ==========
    {
        Point2D p1(0.3, 0.3);  // 完全在面 4/5 区域内
        Point2D p2(0.4, 0.4);

        cout << "\n测试 4：短线段查询（在单个三角形内）" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        vector<Face*> result = subdivision.queryLineIntersection(p1, p2);

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
 * 生成大量随机三角形，测试查询性能
 */
void runPerformanceTest() {
    cout << "\n=== 性能测试 ===" << endl;

    // 创建大规模网格，使用 100×100 的均匀网格索引
    PlanarSubdivision subdivision(100, 100);

    // 生成 10×10 的三角网格（200 个三角形）
    int gridSize = 10;
    cout << "生成 " << gridSize << "x" << gridSize << " 三角网格 ("
         << gridSize * gridSize * 2 << " 个三角形)" << endl;

    // 添加三角形
    vector<Point2D> tri;
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            // 当前方格的四个角坐标
            double x0 = i, y0 = j;      // 左下角
            double x1 = i + 1, y1 = j + 1;  // 右上角

            // 每个方格分成两个三角形
            // 三角形 1：左下-左上-右上
            tri.clear();
            tri.push_back(Point2D(x0, y0));
            tri.push_back(Point2D(x0, y1));
            tri.push_back(Point2D(x1, y1));
            subdivision.addFace(tri);

            // 三角形 2：左下-右上-右下
            tri.clear();
            tri.push_back(Point2D(x0, y0));
            tri.push_back(Point2D(x1, y1));
            tri.push_back(Point2D(x1, y0));
            subdivision.addFace(tri);
        }
    }

    // 构建空间索引
    auto buildStart = chrono::high_resolution_clock::now();
    subdivision.buildSpatialIndex();
    auto buildEnd = chrono::high_resolution_clock::now();

    auto buildDuration = chrono::duration_cast<chrono::microseconds>(buildEnd - buildStart);
    cout << "构建空间索引耗时: " << buildDuration.count() << " 微秒" << endl;

    subdivision.printStatistics();

    // ========== 执行多次随机查询 ==========
    int numQueries = 1000;  // 查询次数
    cout << "\n执行 " << numQueries << " 次随机查询..." << endl;

    srand(42);  // 固定随机种子，保证结果可复现

    int totalIntersections = 0;  // 统计总相交面数
    auto queryStart = chrono::high_resolution_clock::now();

    for (int q = 0; q < numQueries; q++) {
        // 生成随机线段端点
        double x1 = (double)rand() / RAND_MAX * gridSize;
        double y1 = (double)rand() / RAND_MAX * gridSize;
        double x2 = (double)rand() / RAND_MAX * gridSize;
        double y2 = (double)rand() / RAND_MAX * gridSize;

        // 执行查询
        vector<Face*> result = subdivision.queryLineIntersection(
            Point2D(x1, y1), Point2D(x2, y2));
        totalIntersections += result.size();
    }

    auto queryEnd = chrono::high_resolution_clock::now();
    auto queryDuration = chrono::duration_cast<chrono::microseconds>(queryEnd - queryStart);

    // 输出性能统计
    cout << "总查询耗时: " << queryDuration.count() << " 微秒" << endl;
    cout << "平均每次查询: " << queryDuration.count() / numQueries << " 微秒" << endl;
    cout << "平均每次查询相交的多边形数: " << (double)totalIntersections / numQueries << endl;
}

/**
 * 主函数
 * 程序入口点
 */
int main() {
    // 打印标题
    cout << "========================================================" << endl;
    cout << "  习题 1.3：平面分割数据结构与直线相交查询" << endl;
    cout << "  解法一：DCEL + 均匀网格方法" << endl;
    cout << "========================================================" << endl;

    // 创建平面分割数据结构，使用 10×10 的网格
    PlanarSubdivision subdivision(10, 10);

    // 创建测试网格
    createTestMesh(subdivision);

    // 构建空间索引
    subdivision.buildSpatialIndex();

    // 打印统计信息
    subdivision.printStatistics();

    // 运行查询测试
    runQueryTests(subdivision);

    // 运行性能测试
    runPerformanceTest();

    cout << "\n=== 程序结束 ===" << endl;

    return 0;  // 程序正常退出
}
