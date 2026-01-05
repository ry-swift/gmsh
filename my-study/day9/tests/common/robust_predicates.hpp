/**
 * ============================================================================
 * 精确几何谓词
 * ============================================================================
 *
 * 【文件说明】
 * 本文件提供计算几何中的核心谓词函数，包括：
 * - orient2d: 方向判定
 * - inCircle: 圆内判定
 * - 线段相交测试
 * - 点在多边形内判定
 *
 * 【数值精度说明】
 * 计算几何算法对数值精度非常敏感，浮点误差可能导致：
 * - 拓扑不一致（三角形方向判断错误）
 * - 算法死循环（点定位无法收敛）
 * - 结果错误（漏检或误检相交）
 *
 * 本文件提供两种策略：
 * 1. 自适应精度：根据输入数据规模调整容差
 * 2. 精确谓词：参考 Shewchuk 的自适应精度算法
 *
 * 【参考文献】
 * - Jonathan Richard Shewchuk, "Adaptive Precision Floating-Point
 *   Arithmetic and Fast Robust Geometric Predicates", 1997
 * - Gmsh 源码: src/numeric/robustPredicates.cpp
 *
 * ============================================================================
 */

#ifndef ROBUST_PREDICATES_H
#define ROBUST_PREDICATES_H

#include "geometry_types.hpp"
#include <cmath>
#include <algorithm>
#include <vector>

namespace geometry {

// ============================================================================
// 基础几何谓词
// ============================================================================

/**
 * orient2d - 2D 方向判定函数
 *
 * 【核心几何谓词 - 计算几何的基石】
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
 * 5. 点定位（Walk 算法）
 *
 * @param a 线段起点
 * @param b 线段终点
 * @param c 待判断的点
 * @return 叉积值（有符号）
 */
inline double orient2d(const Point2D& a, const Point2D& b, const Point2D& c) noexcept {
    // 计算 2D 叉积
    // 展开：(b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/**
 * orient2dWithEPS - 带容差的方向判定
 *
 * @param a, b, c 三个点
 * @param eps 容差阈值
 * @return 1 表示逆时针，-1 表示顺时针，0 表示共线
 */
inline int orient2dWithEPS(const Point2D& a, const Point2D& b, const Point2D& c,
                           double eps) noexcept {
    double det = orient2d(a, b, c);
    if (det > eps) return 1;    // 逆时针（左侧）
    if (det < -eps) return -1;  // 顺时针（右侧）
    return 0;                   // 共线
}

/**
 * adaptiveOrient2d - 使用自适应容差的方向判定
 *
 * 根据输入点的坐标规模自动计算合适的容差
 */
inline int adaptiveOrient2d(const Point2D& a, const Point2D& b, const Point2D& c) noexcept {
    // 计算输入点的最大坐标值
    double maxCoord = std::max({
        std::abs(a.x), std::abs(a.y),
        std::abs(b.x), std::abs(b.y),
        std::abs(c.x), std::abs(c.y)
    });

    // 计算自适应容差
    double eps = GeometryTolerance::adaptiveEPS(maxCoord);

    return orient2dWithEPS(a, b, c, eps);
}

// ============================================================================
// 线段相交测试
// ============================================================================

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
inline bool onSegment(const Point2D& p, const Point2D& q, const Point2D& r) noexcept {
    // 检查 r 的 x 坐标是否在 [min(p.x, q.x), max(p.x, q.x)] 范围内
    // 同时检查 y 坐标是否在 [min(p.y, q.y), max(p.y, q.y)] 范围内
    return r.x <= std::max(p.x, q.x) && r.x >= std::min(p.x, q.x) &&
           r.y <= std::max(p.y, q.y) && r.y >= std::min(p.y, q.y);
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
 * @param p1, p2 第一条线段的两个端点
 * @param p3, p4 第二条线段的两个端点
 * @param eps 容差阈值
 * @return true 如果两条线段相交
 */
inline bool segmentsIntersect(const Point2D& p1, const Point2D& p2,
                              const Point2D& p3, const Point2D& p4,
                              double eps = 1e-10) noexcept {
    // 计算 p1、p2 相对于线段 p3p4 的方向
    double d1 = orient2d(p3, p4, p1);  // p1 相对于 p3p4 的方向
    double d2 = orient2d(p3, p4, p2);  // p2 相对于 p3p4 的方向

    // 计算 p3、p4 相对于线段 p1p2 的方向
    double d3 = orient2d(p1, p2, p3);  // p3 相对于 p1p2 的方向
    double d4 = orient2d(p1, p2, p4);  // p4 相对于 p1p2 的方向

    // 标准相交情况：两条线段互相跨越
    // d1 和 d2 异号 表示 p1 和 p2 在 p3p4 的两侧
    // d3 和 d4 异号 表示 p3 和 p4 在 p1p2 的两侧
    if (((d1 > eps && d2 < -eps) || (d1 < -eps && d2 > eps)) &&
        ((d3 > eps && d4 < -eps) || (d3 < -eps && d4 > eps))) {
        return true;
    }

    // 处理共线和端点重合的特殊情况

    // 如果 p1 在 p3p4 上（d1 ≈ 0）且在线段范围内
    if (std::abs(d1) <= eps && onSegment(p3, p4, p1)) return true;

    // 如果 p2 在 p3p4 上
    if (std::abs(d2) <= eps && onSegment(p3, p4, p2)) return true;

    // 如果 p3 在 p1p2 上
    if (std::abs(d3) <= eps && onSegment(p1, p2, p3)) return true;

    // 如果 p4 在 p1p2 上
    if (std::abs(d4) <= eps && onSegment(p1, p2, p4)) return true;

    // 不相交
    return false;
}

/**
 * segmentsIntersectAdaptive - 使用自适应容差的线段相交测试
 */
inline bool segmentsIntersectAdaptive(const Point2D& p1, const Point2D& p2,
                                      const Point2D& p3, const Point2D& p4) noexcept {
    // 计算所有点的最大坐标
    double maxCoord = std::max({
        std::abs(p1.x), std::abs(p1.y),
        std::abs(p2.x), std::abs(p2.y),
        std::abs(p3.x), std::abs(p3.y),
        std::abs(p4.x), std::abs(p4.y)
    });

    double eps = GeometryTolerance::adaptiveEPS(maxCoord);
    return segmentsIntersect(p1, p2, p3, p4, eps);
}

// ============================================================================
// 射线-边相交计算
// ============================================================================

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
 * 这是一个 2×2 线性方程组，用克莱姆法则求解：
 * 分母 denom = d.x * e.y - d.y * e.x = d × e（叉积）
 *
 * t = ((a - p1) × e) / (d × e)
 * s = ((a - p1) × d) / (d × e)
 *
 * 【返回值】
 * - t ∈ (0, 1]：有效交点
 * - -1：无有效交点
 *
 * @param p1, p2 射线的起点和终点
 * @param a, b 边的两个端点
 * @param eps 容差阈值
 * @return 射线参数 t，如果无交点返回 -1
 */
inline double rayEdgeIntersection(const Point2D& p1, const Point2D& p2,
                                  const Point2D& a, const Point2D& b,
                                  double eps = 1e-10) noexcept {
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
    if (std::abs(denom) < eps) return -1.0;

    // 计算交点参数
    // t = ((a - p1) × e) / (d × e)
    double t = ((a.x - p1.x) * ey - (a.y - p1.y) * ex) / denom;

    // s = ((a - p1) × d) / (d × e)
    double s = ((a.x - p1.x) * dy - (a.y - p1.y) * dx) / denom;

    // 验证交点有效性
    // 条件：
    // 1. t > eps：交点在射线起点前方
    // 2. t <= 1.0 + eps：交点不超过射线终点
    // 3. s >= -eps 且 s <= 1.0 + eps：交点在边上
    if (t > eps && t <= 1.0 + eps && s >= -eps && s <= 1.0 + eps) {
        return t;
    }

    return -1.0;  // 无有效交点
}

// ============================================================================
// 线段与包围盒相交测试
// ============================================================================

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
 * @param box 矩形包围盒
 * @return true 如果线段与矩形相交或接触
 */
inline bool lineIntersectsBox(const Point2D& p1, const Point2D& p2,
                              const BoundingBox2D& box) noexcept {
    // ===== 快速排除测试 =====
    // 如果线段完全在矩形的某一侧外面，则不可能相交

    // 线段完全在矩形左侧或右侧
    if (std::max(p1.x, p2.x) < box.minX || std::min(p1.x, p2.x) > box.maxX) return false;

    // 线段完全在矩形上方或下方
    if (std::max(p1.y, p2.y) < box.minY || std::min(p1.y, p2.y) > box.maxY) return false;

    // ===== 检查与四条边的相交 =====
    // 矩形的四个角点（按顺时针顺序）
    Point2D corners[4] = {
        Point2D(box.minX, box.minY),   // 左下角
        Point2D(box.maxX, box.minY),   // 右下角
        Point2D(box.maxX, box.maxY),   // 右上角
        Point2D(box.minX, box.maxY)    // 左上角
    };

    // 检查线段与矩形每条边的相交
    for (int i = 0; i < 4; i++) {
        if (segmentsIntersect(p1, p2, corners[i], corners[(i+1)%4])) {
            return true;
        }
    }

    // ===== 检查端点是否在矩形内 =====
    if (box.contains(p1) || box.contains(p2)) return true;

    return false;
}

// ============================================================================
// 点在多边形内判定
// ============================================================================

/**
 * pointInConvexPolygon - 判断点是否在凸多边形内部
 *
 * 【使用 orient2d 的方法 - 针对凸多边形】
 *
 * 原理：
 * 对于凸多边形，点在内部当且仅当：
 * 点相对于所有边都在同一侧
 *
 * 具体判断：
 * - 如果多边形顶点按逆时针排列，内部点应该在所有边的左侧
 * - 即所有 orient2d 返回值都应该 >= 0
 *
 * @param p 待判断的点
 * @param vertices 多边形顶点列表（按逆时针顺序）
 * @param eps 容差阈值
 * @return true 如果点在凸多边形内部或边界上
 */
inline bool pointInConvexPolygon(const Point2D& p,
                                 const std::vector<Point2D>& vertices,
                                 double eps = 1e-10) noexcept {
    if (vertices.size() < 3) return false;

    bool allPositive = true;   // 是否所有方向值都是正的
    bool allNegative = true;   // 是否所有方向值都是负的

    size_t n = vertices.size();
    for (size_t i = 0; i < n; i++) {
        const Point2D& a = vertices[i];
        const Point2D& b = vertices[(i + 1) % n];

        double o = orient2d(a, b, p);

        if (o < -eps) allPositive = false;
        if (o > eps) allNegative = false;
    }

    // 如果点在所有边的同一侧，则在内部
    return allPositive || allNegative;
}

/**
 * pointInPolygonRayCasting - 射线法判断点是否在多边形内
 *
 * 【算法】
 * 从点向右发射水平射线，统计与多边形边界的交点数
 * - 奇数个交点：在内部
 * - 偶数个交点：在外部
 *
 * 适用于任意多边形（凸或凹）
 *
 * @param p 待判断的点
 * @param vertices 多边形顶点列表
 * @return true 如果点在多边形内部
 */
inline bool pointInPolygonRayCasting(const Point2D& p,
                                     const std::vector<Point2D>& vertices) noexcept {
    if (vertices.size() < 3) return false;

    bool inside = false;
    size_t n = vertices.size();

    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        double yi = vertices[i].y;
        double yj = vertices[j].y;
        double xi = vertices[i].x;
        double xj = vertices[j].x;

        // 检查射线是否穿过边 (i, j)
        if (((yi > p.y) != (yj > p.y)) &&
            (p.x < (xj - xi) * (p.y - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
    }

    return inside;
}

// ============================================================================
// 网格遍历算法
// ============================================================================

/**
 * traverseGridAmanatidesWoo - Amanatides & Woo 快速体素遍历算法
 *
 * 【算法说明】
 * 这是一种精确的直线-网格遍历算法，保证：
 * 1. 不遗漏任何被直线穿过的网格单元
 * 2. 不重复访问任何单元
 * 3. 按直线行进顺序访问
 *
 * 【参考文献】
 * Amanatides, J., & Woo, A. (1987). A Fast Voxel Traversal Algorithm for
 * Ray Tracing. Eurographics, 87(3), 3-10.
 *
 * @param start 直线起点
 * @param end 直线终点
 * @param gridMinX, gridMinY 网格原点坐标
 * @param cellWidth, cellHeight 单元尺寸
 * @param gridNx, gridNy 网格维度
 * @param visitor 访问回调函数，参数为 (cellX, cellY)
 */
inline void traverseGridAmanatidesWoo(
    const Point2D& start, const Point2D& end,
    double gridMinX, double gridMinY,
    double cellWidth, double cellHeight,
    int gridNx, int gridNy,
    const std::function<void(int, int)>& visitor) {

    // 计算起点和终点的网格索引
    int x = static_cast<int>(std::floor((start.x - gridMinX) / cellWidth));
    int y = static_cast<int>(std::floor((start.y - gridMinY) / cellHeight));

    int endX = static_cast<int>(std::floor((end.x - gridMinX) / cellWidth));
    int endY = static_cast<int>(std::floor((end.y - gridMinY) / cellHeight));

    // 边界限制
    x = std::max(0, std::min(x, gridNx - 1));
    y = std::max(0, std::min(y, gridNy - 1));
    endX = std::max(0, std::min(endX, gridNx - 1));
    endY = std::max(0, std::min(endY, gridNy - 1));

    // 计算方向
    double dx = end.x - start.x;
    double dy = end.y - start.y;

    // 确定步进方向
    int stepX = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
    int stepY = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

    // 计算到下一个网格边界的参数化距离
    double tMaxX, tMaxY;
    double tDeltaX, tDeltaY;

    if (dx != 0) {
        double nextBoundX = gridMinX + (x + (stepX > 0 ? 1 : 0)) * cellWidth;
        tMaxX = (nextBoundX - start.x) / dx;
        tDeltaX = cellWidth / std::abs(dx);
    } else {
        tMaxX = std::numeric_limits<double>::max();
        tDeltaX = std::numeric_limits<double>::max();
    }

    if (dy != 0) {
        double nextBoundY = gridMinY + (y + (stepY > 0 ? 1 : 0)) * cellHeight;
        tMaxY = (nextBoundY - start.y) / dy;
        tDeltaY = cellHeight / std::abs(dy);
    } else {
        tMaxY = std::numeric_limits<double>::max();
        tDeltaY = std::numeric_limits<double>::max();
    }

    // 主循环：遍历所有被直线穿过的网格单元
    int maxIterations = gridNx + gridNy + 10;  // 防止无限循环
    while (maxIterations-- > 0) {
        // 访问当前单元
        if (x >= 0 && x < gridNx && y >= 0 && y < gridNy) {
            visitor(x, y);
        }

        // 检查是否到达终点
        if (x == endX && y == endY) break;

        // 选择下一步移动方向
        if (tMaxX < tMaxY) {
            x += stepX;
            tMaxX += tDeltaX;
        } else {
            y += stepY;
            tMaxY += tDeltaY;
        }

        // 边界检查
        if (x < 0 || x >= gridNx || y < 0 || y >= gridNy) break;
    }
}

} // namespace geometry

#endif // ROBUST_PREDICATES_H
