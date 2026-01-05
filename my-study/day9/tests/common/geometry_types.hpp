/**
 * ============================================================================
 * 公共几何类型定义
 * ============================================================================
 *
 * 【文件说明】
 * 本文件提供平面分割数据结构所需的基础几何类型和工具函数
 * 采用现代 C++ 最佳实践，包括：
 * - constexpr 支持编译期计算
 * - noexcept 标记无异常函数
 * - 自适应数值容差
 * - 精确几何谓词
 *
 * 【设计原则】
 * 1. 高内聚：所有几何基础类型集中管理
 * 2. 低耦合：不依赖具体的 DCEL 或索引结构
 * 3. 可测试：提供独立的单元测试支持
 *
 * ============================================================================
 */

#ifndef GEOMETRY_TYPES_H
#define GEOMETRY_TYPES_H

#include <cmath>
#include <limits>
#include <algorithm>
#include <iostream>
#include <functional>

namespace geometry {

// ============================================================================
// 数值常量和容差处理
// ============================================================================

/**
 * 几何容差管理类
 *
 * 【问题】
 * 固定的 EPS = 1e-10 在以下情况会失效：
 * - 大坐标值（如 1e6）时，1e-10 相对误差太小
 * - 小坐标值（如 1e-6）时，1e-10 可能太大
 *
 * 【解决方案】
 * 使用自适应容差：相对误差 + 绝对误差混合策略
 */
class GeometryTolerance {
public:
    // 绝对容差下限（防止除零）
    static constexpr double ABSOLUTE_EPS = 1e-14;

    // 相对容差因子（基于机器精度的倍数）
    static constexpr double RELATIVE_FACTOR = 100.0;

    /**
     * 计算自适应容差
     *
     * 公式：eps = max(ABSOLUTE_EPS, maxCoord * machine_epsilon * RELATIVE_FACTOR)
     *
     * @param maxCoord 参与计算的最大坐标绝对值
     * @return 适应当前数据规模的容差值
     */
    static double adaptiveEPS(double maxCoord) noexcept {
        return std::max(ABSOLUTE_EPS,
                        maxCoord * std::numeric_limits<double>::epsilon() * RELATIVE_FACTOR);
    }

    /**
     * 判断两个浮点数是否相等（使用自适应容差）
     */
    static bool isEqual(double a, double b, double eps) noexcept {
        return std::abs(a - b) <= eps;
    }

    /**
     * 判断浮点数是否为零（使用自适应容差）
     */
    static bool isZero(double a, double eps) noexcept {
        return std::abs(a) <= eps;
    }
};

// C++14 需要类外定义 constexpr 静态成员（避免 ODR-use 链接错误）
constexpr double GeometryTolerance::ABSOLUTE_EPS;
constexpr double GeometryTolerance::RELATIVE_FACTOR;

// ============================================================================
// 2D 点/向量结构
// ============================================================================

/**
 * 2D 点结构
 *
 * 【设计说明】
 * - 使用 struct 表示这是一个数据容器
 * - 支持编译期常量表达式（constexpr）
 * - 所有无异常函数标记 noexcept
 * - 同时可作为点和向量使用
 */
struct Point2D {
    double x, y;  // 坐标分量

    // ========== 构造函数 ==========

    /**
     * 默认构造函数
     * 使用 constexpr 支持编译期初始化
     */
    constexpr Point2D(double _x = 0.0, double _y = 0.0) noexcept : x(_x), y(_y) {}

    // ========== 向量运算 ==========

    /**
     * 向量加法
     */
    constexpr Point2D operator+(const Point2D& p) const noexcept {
        return Point2D(x + p.x, y + p.y);
    }

    /**
     * 向量减法
     */
    constexpr Point2D operator-(const Point2D& p) const noexcept {
        return Point2D(x - p.x, y - p.y);
    }

    /**
     * 标量乘法
     */
    constexpr Point2D operator*(double s) const noexcept {
        return Point2D(x * s, y * s);
    }

    /**
     * 标量除法
     */
    constexpr Point2D operator/(double s) const noexcept {
        return Point2D(x / s, y / s);
    }

    /**
     * 复合赋值：加法
     */
    Point2D& operator+=(const Point2D& p) noexcept {
        x += p.x;
        y += p.y;
        return *this;
    }

    /**
     * 复合赋值：减法
     */
    Point2D& operator-=(const Point2D& p) noexcept {
        x -= p.x;
        y -= p.y;
        return *this;
    }

    /**
     * 复合赋值：标量乘法
     */
    Point2D& operator*=(double s) noexcept {
        x *= s;
        y *= s;
        return *this;
    }

    /**
     * 取反（单目负号）
     */
    constexpr Point2D operator-() const noexcept {
        return Point2D(-x, -y);
    }

    // ========== 几何运算 ==========

    /**
     * 点积（内积）
     *
     * 数学意义：
     * - a · b = |a| |b| cos(θ)
     * - 正值表示夹角 < 90°
     * - 零表示垂直
     * - 负值表示夹角 > 90°
     */
    constexpr double dot(const Point2D& p) const noexcept {
        return x * p.x + y * p.y;
    }

    /**
     * 2D 叉积（返回标量，实际是 z 分量）
     *
     * 数学意义：
     * - a × b = |a| |b| sin(θ)
     * - 等于由 a 和 b 构成的平行四边形的有符号面积
     * - 正值表示 b 在 a 的逆时针方向
     * - 负值表示 b 在 a 的顺时针方向
     */
    constexpr double cross(const Point2D& p) const noexcept {
        return x * p.y - y * p.x;
    }

    /**
     * 向量模长（欧几里得范数）
     *
     * 注意：涉及开方运算，不是 constexpr
     */
    double norm() const noexcept {
        return std::sqrt(x * x + y * y);
    }

    /**
     * 向量模长的平方
     *
     * 用途：比较距离时避免开方运算，提高效率
     */
    constexpr double squaredNorm() const noexcept {
        return x * x + y * y;
    }

    /**
     * 返回单位向量
     *
     * 注意：如果当前向量为零向量，返回零向量
     */
    Point2D normalized() const noexcept {
        double n = norm();
        if (n < std::numeric_limits<double>::epsilon()) {
            return Point2D(0.0, 0.0);
        }
        return *this / n;
    }

    /**
     * 返回垂直向量（逆时针旋转 90°）
     */
    constexpr Point2D perpendicular() const noexcept {
        return Point2D(-y, x);
    }

    /**
     * 获取最大坐标的绝对值
     *
     * 用途：计算自适应容差
     */
    double maxAbsCoord() const noexcept {
        return std::max(std::abs(x), std::abs(y));
    }

    // ========== 比较运算 ==========

    /**
     * 相等比较（精确比较，谨慎使用）
     */
    constexpr bool operator==(const Point2D& p) const noexcept {
        return x == p.x && y == p.y;
    }

    /**
     * 不等比较
     */
    constexpr bool operator!=(const Point2D& p) const noexcept {
        return !(*this == p);
    }

    /**
     * 近似相等比较（推荐使用）
     */
    bool isApprox(const Point2D& p, double eps) const noexcept {
        return std::abs(x - p.x) <= eps && std::abs(y - p.y) <= eps;
    }

    // ========== 输出支持 ==========

    /**
     * 输出运算符重载
     */
    friend std::ostream& operator<<(std::ostream& os, const Point2D& p) {
        os << "(" << p.x << ", " << p.y << ")";
        return os;
    }
};

/**
 * 标量 × 向量（支持 2.0 * v 语法）
 */
inline constexpr Point2D operator*(double s, const Point2D& p) noexcept {
    return p * s;
}

// ============================================================================
// 2D 包围盒
// ============================================================================

/**
 * 2D 轴对齐包围盒（AABB）
 *
 * 【用途】
 * - 快速空间排除测试
 * - R-Tree 节点表示
 * - 均匀网格的全局范围
 */
struct BoundingBox2D {
    double minX, minY;  // 左下角
    double maxX, maxY;  // 右上角

    /**
     * 默认构造函数
     * 初始化为"空"包围盒（反向无穷）
     */
    BoundingBox2D() noexcept
        : minX(std::numeric_limits<double>::max()),
          minY(std::numeric_limits<double>::max()),
          maxX(std::numeric_limits<double>::lowest()),
          maxY(std::numeric_limits<double>::lowest()) {}

    /**
     * 带参数构造函数
     */
    BoundingBox2D(double _minX, double _minY, double _maxX, double _maxY) noexcept
        : minX(_minX), minY(_minY), maxX(_maxX), maxY(_maxY) {}

    /**
     * 从单点构造包围盒
     */
    explicit BoundingBox2D(const Point2D& p) noexcept
        : minX(p.x), minY(p.y), maxX(p.x), maxY(p.y) {}

    /**
     * 检查包围盒是否有效
     */
    bool isValid() const noexcept {
        return minX <= maxX && minY <= maxY;
    }

    /**
     * 扩展包围盒以包含一个点
     */
    void expand(const Point2D& p) noexcept {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }

    /**
     * 扩展包围盒以包含另一个包围盒
     */
    void expand(const BoundingBox2D& other) noexcept {
        minX = std::min(minX, other.minX);
        minY = std::min(minY, other.minY);
        maxX = std::max(maxX, other.maxX);
        maxY = std::max(maxY, other.maxY);
    }

    /**
     * 添加边距
     */
    void pad(double padding) noexcept {
        minX -= padding;
        minY -= padding;
        maxX += padding;
        maxY += padding;
    }

    /**
     * 计算面积
     */
    double area() const noexcept {
        if (!isValid()) return 0.0;
        return (maxX - minX) * (maxY - minY);
    }

    /**
     * 计算宽度
     */
    double width() const noexcept {
        return maxX - minX;
    }

    /**
     * 计算高度
     */
    double height() const noexcept {
        return maxY - minY;
    }

    /**
     * 获取中心点
     */
    Point2D center() const noexcept {
        return Point2D((minX + maxX) * 0.5, (minY + maxY) * 0.5);
    }

    /**
     * 判断点是否在包围盒内
     */
    bool contains(const Point2D& p) const noexcept {
        return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
    }

    /**
     * 判断两个包围盒是否相交
     */
    bool intersects(const BoundingBox2D& other) const noexcept {
        // 分离轴测试
        if (maxX < other.minX || minX > other.maxX) return false;
        if (maxY < other.minY || minY > other.maxY) return false;
        return true;
    }

    /**
     * 计算扩展后的面积增量
     *
     * 用途：R-Tree 插入时选择最佳子节点
     */
    double enlargement(const BoundingBox2D& other) const noexcept {
        double newMinX = std::min(minX, other.minX);
        double newMinY = std::min(minY, other.minY);
        double newMaxX = std::max(maxX, other.maxX);
        double newMaxY = std::max(maxY, other.maxY);
        double newArea = (newMaxX - newMinX) * (newMaxY - newMinY);
        return newArea - area();
    }

    /**
     * 返回两个包围盒的并集
     */
    BoundingBox2D unionWith(const BoundingBox2D& other) const noexcept {
        return BoundingBox2D(
            std::min(minX, other.minX),
            std::min(minY, other.minY),
            std::max(maxX, other.maxX),
            std::max(maxY, other.maxY)
        );
    }

    /**
     * 返回两个包围盒的交集（如果不相交返回无效包围盒）
     */
    BoundingBox2D intersectionWith(const BoundingBox2D& other) const noexcept {
        return BoundingBox2D(
            std::max(minX, other.minX),
            std::max(minY, other.minY),
            std::min(maxX, other.maxX),
            std::min(maxY, other.maxY)
        );
    }

    /**
     * 输出运算符重载
     */
    friend std::ostream& operator<<(std::ostream& os, const BoundingBox2D& bb) {
        os << "[(" << bb.minX << ", " << bb.minY << ") - ("
           << bb.maxX << ", " << bb.maxY << ")]";
        return os;
    }
};

// ============================================================================
// 边的键值类型（用于 twin 指针匹配）
// ============================================================================

/**
 * 边的规范化键值
 *
 * 【设计说明】
 * 两个方向相反的半边共享同一条几何边
 * 使用规范化的顶点 ID 对作为键值，使得：
 * - EdgeKey(v1, v2) == EdgeKey(v2, v1)
 *
 * 【使用方式】
 * std::unordered_map<EdgeKey, HalfEdge*, EdgeKeyHash> edgeMap;
 */
struct EdgeKey {
    int v1, v2;  // 顶点 ID，保证 v1 <= v2

    /**
     * 构造函数（自动规范化）
     */
    EdgeKey(int a, int b) noexcept : v1(std::min(a, b)), v2(std::max(a, b)) {}

    /**
     * 相等比较
     */
    bool operator==(const EdgeKey& other) const noexcept {
        return v1 == other.v1 && v2 == other.v2;
    }

    /**
     * 不等比较
     */
    bool operator!=(const EdgeKey& other) const noexcept {
        return !(*this == other);
    }

    /**
     * 小于比较（用于 std::map）
     */
    bool operator<(const EdgeKey& other) const noexcept {
        if (v1 != other.v1) return v1 < other.v1;
        return v2 < other.v2;
    }
};

/**
 * EdgeKey 的哈希函数
 *
 * 用于 std::unordered_map
 */
struct EdgeKeyHash {
    std::size_t operator()(const EdgeKey& key) const noexcept {
        // 使用位运算组合两个整数的哈希值
        std::size_t h1 = std::hash<int>()(key.v1);
        std::size_t h2 = std::hash<int>()(key.v2);
        // 黄金比例常数，减少哈希冲突
        return h1 ^ (h2 * 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

} // namespace geometry

#endif // GEOMETRY_TYPES_H
