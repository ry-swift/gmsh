/**
 * ============================================================================
 * 习题 1.3：平面分割数据结构与直线相交查询
 * 解法二（优化版）：射线行走法（Ray Walking）
 * ============================================================================
 *
 * 【优化说明】
 *
 * 1. 内存管理：使用 std::unique_ptr
 * 2. edgeMap 优化：使用 unordered_map + 自定义哈希
 * 3. 边界边预处理：构建边界边索引加速外部起点入口查找
 * 4. 点定位健壮性：使用精确谓词和退化情况处理
 * 5. 代码规范：引用公共头文件
 *
 * ============================================================================
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <limits>
#include <set>

#include "common/geometry_types.hpp"
#include "common/robust_predicates.hpp"

using namespace std;
using namespace geometry;

// ============================================================================
// 第一部分：DCEL 数据结构
// ============================================================================

class HalfEdge;
class Face;

class Vertex {
public:
    Point2D position;
    HalfEdge* incidentEdge;
    int id;

    Vertex(double x = 0, double y = 0, int _id = -1)
        : position(x, y), incidentEdge(nullptr), id(_id) {}

    explicit Vertex(const Point2D& p, int _id = -1)
        : position(p), incidentEdge(nullptr), id(_id) {}
};

class HalfEdge {
public:
    Vertex* origin;
    HalfEdge* twin;
    HalfEdge* next;
    HalfEdge* prev;
    Face* incidentFace;
    int id;

    explicit HalfEdge(Vertex* v = nullptr, int _id = -1)
        : origin(v), twin(nullptr), next(nullptr), prev(nullptr),
          incidentFace(nullptr), id(_id) {}

    Vertex* destination() const {
        return next ? next->origin : nullptr;
    }
};

class Face {
public:
    HalfEdge* outerComponent;
    int id;
    BoundingBox2D boundingBox;

    explicit Face(HalfEdge* he = nullptr, int _id = -1)
        : outerComponent(he), id(_id) {}

    void computeBoundingBox() {
        if (!outerComponent) return;
        boundingBox = BoundingBox2D();

        HalfEdge* he = outerComponent;
        do {
            boundingBox.expand(he->origin->position);
            he = he->next;
        } while (he != outerComponent);
    }

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
// 第二部分：平面分割类（射线行走优化版）
// ============================================================================

/**
 * 平面分割类（PlanarSubdivision）- 射线行走法实现
 *
 * 【设计说明】
 * 本类使用射线行走（Ray Walking）算法进行直线相交查询：
 * 1. 先定位起点所在的三角形（使用 Walk 算法）
 * 2. 沿射线方向依次穿过相邻三角形
 * 3. 利用 DCEL 的 twin 指针快速跳转到相邻面
 *
 * 【核心思想】
 * 射线行走法的核心优势是"输出敏感"：
 * - 复杂度仅与相交的面数 k 相关，为 O(√n + k)
 * - 对于稀疏查询（k << n）非常高效
 *
 * 【与均匀网格方法的比较】
 * - 优点：不需要预处理空间索引
 * - 优点：内存占用更小
 * - 缺点：需要完整的 DCEL 拓扑信息（twin 指针）
 * - 缺点：起点定位依赖 Walk 算法的效率
 *
 * 【使用示例】
 * PlanarSubdivision subdivision;
 * subdivision.addVertices(points);
 * subdivision.addTriangle(v0, v1, v2);
 * subdivision.buildBoundaryIndex();  // 可选：加速边界外起点处理
 * auto result = subdivision.rayWalkQuery(p1, p2);
 */
class PlanarSubdivision {
private:
    // ========== DCEL 数据存储 ==========
    vector<unique_ptr<Vertex>> vertices;    // 顶点集合
    vector<unique_ptr<HalfEdge>> halfEdges; // 半边集合
    vector<unique_ptr<Face>> faces;         // 面集合

    // 边映射表：用于建立 twin 指针关系
    // 使用 unordered_map 替代 map，查找复杂度从 O(log n) 降至 O(1)
    unordered_map<EdgeKey, HalfEdge*, EdgeKeyHash> edgeMap;

    // 【优化】边界边索引
    // 存储所有没有 twin 的半边（即网格边界上的边）
    // 用途：当查询起点在网格外部时，快速找到射线进入网格的边界边
    vector<HalfEdge*> boundaryEdges;

    // 点定位缓存：存储上一次定位到的面
    // 利用空间局部性原理：相邻查询的起点很可能在同一面或相邻面
    Face* lastLocatedFace;

public:
    PlanarSubdivision() : lastLocatedFace(nullptr) {}

    PlanarSubdivision(const PlanarSubdivision&) = delete;
    PlanarSubdivision& operator=(const PlanarSubdivision&) = delete;

    ~PlanarSubdivision() = default;

    /**
     * 批量添加顶点
     */
    void addVertices(const vector<Point2D>& points) {
        for (const auto& p : points) {
            auto v = make_unique<Vertex>(p, static_cast<int>(vertices.size()));
            vertices.push_back(std::move(v));
        }
    }

    /**
     * 添加三角形面（使用顶点索引）
     */
    Face* addTriangle(int v0, int v1, int v2) {
        auto face = make_unique<Face>();
        face->id = static_cast<int>(faces.size());

        Vertex* verts[3] = {vertices[v0].get(), vertices[v1].get(), vertices[v2].get()};

        HalfEdge* edges[3];
        for (int i = 0; i < 3; i++) {
            auto he = make_unique<HalfEdge>(verts[i], static_cast<int>(halfEdges.size()));
            he->incidentFace = face.get();
            edges[i] = he.get();
            halfEdges.push_back(std::move(he));
        }

        for (int i = 0; i < 3; i++) {
            edges[i]->next = edges[(i + 1) % 3];
            edges[i]->prev = edges[(i + 2) % 3];
            verts[i]->incidentEdge = edges[i];
        }

        face->outerComponent = edges[0];
        face->computeBoundingBox();

        // 设置 twin 指针
        int vertIds[3] = {v0, v1, v2};
        for (int i = 0; i < 3; i++) {
            int id1 = vertIds[i];
            int id2 = vertIds[(i + 1) % 3];

            EdgeKey key(id1, id2);
            auto it = edgeMap.find(key);

            if (it != edgeMap.end()) {
                HalfEdge* existingEdge = it->second;
                edges[i]->twin = existingEdge;
                existingEdge->twin = edges[i];
            } else {
                edgeMap[key] = edges[i];
            }
        }

        Face* facePtr = face.get();
        faces.push_back(std::move(face));
        return facePtr;
    }

    /**
     * 构建边界边索引
     */
    void buildBoundaryIndex() {
        boundaryEdges.clear();
        for (const auto& he : halfEdges) {
            if (he->twin == nullptr) {
                boundaryEdges.push_back(he.get());
            }
        }
        cout << "边界边数量: " << boundaryEdges.size() << endl;
    }

    /**
     * 点定位（Walk 算法 - 优化版）
     *
     * 【算法原理】
     * Walk 算法是一种启发式的点定位方法：
     * 1. 从某个三角形开始（通常是上次定位的结果）
     * 2. 判断目标点相对于当前三角形各边的位置
     * 3. 如果点在某条边的"外侧"，则跳转到该边的对偶三角形
     * 4. 重复直到找到包含点的三角形
     *
     * 【orient2d 符号含义】
     * 对于三角形的每条边 (v_i, v_{i+1})，计算 orient2d(v_i, v_{i+1}, p)：
     * - 正值：p 在边的左侧（三角形内部方向）
     * - 负值：p 在边的右侧（三角形外部方向）
     * - 零值：p 在边上
     *
     * 注意：这里使用取反（-adaptiveOrient2d），使得：
     * - s >= 0 表示点在边的内侧或边上
     * - s < 0 表示点在边的外侧
     *
     * 【跳转策略】
     * 当点在多条边外侧时，选择"最外侧"的边（s 值最小的）进行跳转
     * 这种贪心策略可以加快收敛
     *
     * 【防循环机制】
     * 使用 visited 集合记录已访问的面，检测并处理循环
     * 循环可能由数值误差导致，此时尝试从其他面重新开始
     *
     * 【复杂度】
     * - 期望：O(√n)，适用于均匀分布的三角形
     * - 最坏：O(n)，退化情况（如细长三角形链）
     *
     * @param p 待定位的点
     * @return 包含该点的面，如果点在网格外则返回 nullptr
     */
    Face* locatePoint(const Point2D& p) {
        if (faces.empty()) return nullptr;

        // 从上次定位的面开始，利用空间局部性
        Face* current = lastLocatedFace ? lastLocatedFace : faces[0].get();

        // 设置最大迭代次数，防止死循环
        int maxIterations = static_cast<int>(faces.size()) + 100;
        set<Face*> visited;  // 记录已访问的面，用于检测循环

        while (maxIterations-- > 0) {
            // ===== 循环检测与处理 =====
            if (visited.count(current)) {
                // 检测到循环（可能由数值误差导致）
                // 尝试从一个未访问的面重新开始
                for (const auto& f : faces) {
                    if (!visited.count(f.get())) {
                        current = f.get();
                        break;
                    }
                }
                // 如果所有面都已访问，放弃
                if (visited.count(current)) return nullptr;
            }
            visited.insert(current);

            // ===== 获取三角形的三个顶点 =====
            HalfEdge* he = current->outerComponent;
            Vertex* v0 = he->origin;
            Vertex* v1 = he->next->origin;
            Vertex* v2 = he->next->next->origin;

            // ===== 计算点相对于三条边的位置 =====
            // 使用自适应精度的 orient2d 谓词
            // 取反使得：正值=内侧，负值=外侧
            int s0 = -adaptiveOrient2d(v0->position, v1->position, p);  // 边 v0-v1
            int s1 = -adaptiveOrient2d(v1->position, v2->position, p);  // 边 v1-v2
            int s2 = -adaptiveOrient2d(v2->position, v0->position, p);  // 边 v2-v0

            // ===== 判断点是否在三角形内部 =====
            // 如果点在所有边的内侧或边上，则找到了包含点的三角形
            if (s0 >= 0 && s1 >= 0 && s2 >= 0) {
                lastLocatedFace = current;  // 缓存结果
                return current;
            }

            // ===== 选择跳转方向 =====
            // 选择点"最外侧"的边进行跳转
            HalfEdge* nextEdge = nullptr;

            if (s0 < 0 && (s0 <= s1 || s1 >= 0) && (s0 <= s2 || s2 >= 0)) {
                nextEdge = he;              // 跳过边 v0-v1
            } else if (s1 < 0 && (s1 <= s0 || s0 >= 0) && (s1 <= s2 || s2 >= 0)) {
                nextEdge = he->next;        // 跳过边 v1-v2
            } else if (s2 < 0) {
                nextEdge = he->next->next;  // 跳过边 v2-v0
            }

            // ===== 执行跳转 =====
            if (nextEdge && nextEdge->twin) {
                current = nextEdge->twin->incidentFace;
            } else {
                return nullptr;  // 到达边界，点在网格外
            }
        }

        return nullptr;  // 超过最大迭代次数
    }

    /**
     * 射线行走查询（优化版）
     *
     * 【算法原理】
     * 射线行走是一种"输出敏感"的直线查询算法：
     *
     * 步骤 1：起点定位
     * - 使用 Walk 算法定位起点所在的三角形
     * - 如果起点在网格外，遍历边界边找到射线进入网格的位置
     *
     * 步骤 2：射线行走
     * - 从起始三角形开始
     * - 计算射线与三角形各边的交点
     * - 选择最近的交点（t 值最小）作为退出边
     * - 通过 twin 指针跳转到相邻三角形
     * - 重复直到射线离开网格或到达终点
     *
     * 【退出边选择】
     * 使用 rayEdgeIntersection 计算射线参数 t：
     * - t ∈ (0, 1]：交点在射线上且在线段 p1-p2 范围内
     * - 选择 t 值最小的边作为退出边
     *
     * 【边界情况处理】
     * - 起点在网格外：使用边界边索引快速找入口
     * - 循环检测：使用 visited 集合防止无限循环
     * - 数值误差：使用自适应容差进行比较
     *
     * 【复杂度】
     * - 时间：O(√n + k)，n 为总面数，k 为相交面数
     * - 空间：O(k) 用于存储结果
     *
     * @param p1 射线起点
     * @param p2 射线终点
     * @return 射线穿过的所有面（按穿过顺序）
     */
    vector<Face*> rayWalkQuery(const Point2D& p1, const Point2D& p2) {
        vector<Face*> result;

        // ===== 步骤 1：定位起点所在的三角形 =====
        Face* startFace = locatePoint(p1);

        if (!startFace) {
            // 起点在网格外部，需要找到射线进入网格的位置
            // 【优化】使用预先构建的边界边索引，避免遍历所有边
            for (HalfEdge* he : boundaryEdges) {
                Point2D a = he->origin->position;
                Point2D b = he->next->origin->position;

                // 计算射线与边界边的交点参数
                double t = rayEdgeIntersection(p1, p2, a, b);
                if (t > 0 && t <= 1) {
                    // 找到入口边，从该边所属的面开始行走
                    startFace = he->incidentFace;
                    break;
                }
            }

            // 如果仍然找不到入口，说明射线不与网格相交
            if (!startFace) return result;
        }

        // ===== 步骤 2：射线行走主循环 =====
        Face* current = startFace;
        set<Face*> visited;  // 防止循环访问

        int maxIterations = static_cast<int>(faces.size()) + 100;

        while (current && maxIterations-- > 0) {
            // 循环检测
            if (visited.count(current)) break;
            visited.insert(current);

            // 记录当前面到结果
            result.push_back(current);

            // ===== 找射线穿出当前三角形的边 =====
            HalfEdge* exitEdge = nullptr;
            double minT = numeric_limits<double>::max();

            // 遍历三角形的三条边
            HalfEdge* he = current->outerComponent;
            do {
                Point2D a = he->origin->position;
                Point2D b = he->next->origin->position;

                // 计算射线与边的交点参数
                double t = rayEdgeIntersection(p1, p2, a, b);

                // 使用自适应容差，避免将入口边误判为退出边
                double eps = GeometryTolerance::adaptiveEPS(
                    max({abs(a.x), abs(a.y), abs(b.x), abs(b.y)}));

                // 选择 t > eps 的最小交点（排除入口点附近的交点）
                if (t > eps && t < minT) {
                    minT = t;
                    exitEdge = he;
                }

                he = he->next;
            } while (he != current->outerComponent);

            // ===== 判断是否继续行走 =====
            // 如果没有找到退出边，或者退出点超过终点，停止
            if (!exitEdge || minT > 1.0 + 1e-10) {
                break;
            }

            // 通过 twin 指针跳转到相邻三角形
            if (exitEdge->twin) {
                current = exitEdge->twin->incidentFace;
            } else {
                break;  // 到达边界，射线离开网格
            }
        }

        return result;
    }

    /**
     * 暴力查询（用于验证）
     *
     * 【用途】
     * 提供一个简单但正确的基准实现，用于：
     * - 验证优化算法的正确性
     * - 性能对比测试
     *
     * 【算法】
     * 对每个面进行两项检测：
     * 1. 线段相交测试：检查查询线段是否与面的任意边相交
     * 2. 端点包含测试：检查线段端点是否在面内部
     *
     * 【复杂度】
     * - 时间：O(n × p)，n 为面数，p 为平均边数
     * - 空间：O(k)，k 为结果数量
     *
     * @param p1 查询线段起点
     * @param p2 查询线段终点
     * @return 与线段相交的所有面
     */
    vector<Face*> bruteForceQuery(const Point2D& p1, const Point2D& p2) {
        vector<Face*> result;

        // 遍历所有面
        for (const auto& f : faces) {
            HalfEdge* he = f->outerComponent;
            bool intersects = false;

            // 检测 1：线段是否与面的任意边相交
            do {
                Point2D a = he->origin->position;
                Point2D b = he->next->origin->position;

                if (segmentsIntersectAdaptive(p1, p2, a, b)) {
                    intersects = true;
                    break;
                }

                he = he->next;
            } while (he != f->outerComponent);

            // 检测 2：线段端点是否在面内部
            // 这处理了线段完全在面内部的情况
            if (!intersects) {
                vector<Point2D> verts = f->getVertices();
                if (pointInConvexPolygon(p1, verts) || pointInConvexPolygon(p2, verts)) {
                    intersects = true;
                }
            }

            if (intersects) {
                result.push_back(f.get());
            }
        }

        return result;
    }

    int numFaces() const { return static_cast<int>(faces.size()); }
    int numVertices() const { return static_cast<int>(vertices.size()); }
    int numHalfEdges() const { return static_cast<int>(halfEdges.size()); }

    void printStatistics() const {
        cout << "\n=== 数据结构统计 ===" << endl;
        cout << "顶点数: " << vertices.size() << endl;
        cout << "半边数: " << halfEdges.size() << endl;
        cout << "面数: " << faces.size() << endl;

        int twinCount = 0;
        int boundaryCount = 0;

        for (const auto& he : halfEdges) {
            if (he->twin) twinCount++;
            else boundaryCount++;
        }

        cout << "内部边（有 twin）: " << twinCount / 2 << endl;
        cout << "边界边（无 twin）: " << boundaryCount << endl;
    }
};

// ============================================================================
// 第三部分：测试程序
// ============================================================================

void createTestMesh(PlanarSubdivision& subdivision) {
    cout << "=== 创建测试三角网格（带完整拓扑）===" << endl;

    vector<Point2D> points = {
        Point2D(0, 0), Point2D(1, 0), Point2D(2, 0),
        Point2D(0, 1), Point2D(1, 1), Point2D(2, 1),
        Point2D(0, 2), Point2D(1, 2), Point2D(2, 2)
    };

    subdivision.addVertices(points);

    // 第一行
    subdivision.addTriangle(3, 6, 7);
    subdivision.addTriangle(3, 7, 4);
    subdivision.addTriangle(4, 7, 8);
    subdivision.addTriangle(4, 8, 5);

    // 第二行
    subdivision.addTriangle(0, 3, 4);
    subdivision.addTriangle(0, 4, 1);
    subdivision.addTriangle(1, 4, 5);
    subdivision.addTriangle(1, 5, 2);

    subdivision.buildBoundaryIndex();

    cout << "创建了 " << subdivision.numFaces() << " 个三角形面" << endl;
    subdivision.printStatistics();
}

void runQueryTests(PlanarSubdivision& subdivision) {
    cout << "\n=== 运行直线相交查询测试（射线行走法）===" << endl;

    // 测试用例
    struct TestCase {
        Point2D p1, p2;
        const char* name;
    };

    TestCase tests[] = {
        {{0.2, 0.2}, {1.8, 1.8}, "对角线查询"},
        {{0.0, 1.0}, {2.0, 1.0}, "水平线查询（沿 y=1）"},
        {{1.0, 0.0}, {1.0, 2.0}, "垂直线查询（沿 x=1）"},
        {{0.3, 0.3}, {0.4, 0.4}, "短线段查询"}
    };

    for (int i = 0; i < 4; i++) {
        cout << "\n测试 " << (i + 1) << ": " << tests[i].name << endl;
        cout << "直线: " << tests[i].p1 << " -> " << tests[i].p2 << endl;

        auto start = chrono::high_resolution_clock::now();
        vector<Face*> result = subdivision.rayWalkQuery(tests[i].p1, tests[i].p2);
        auto end = chrono::high_resolution_clock::now();

        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (Face* f : result) cout << f->id << " ";
        cout << endl;
        cout << "查询耗时: " << duration.count() << " 微秒" << endl;

        vector<Face*> bruteResult = subdivision.bruteForceQuery(tests[i].p1, tests[i].p2);
        cout << "暴力验证结果: " << bruteResult.size() << " 个面" << endl;
    }
}

void runPerformanceTest() {
    cout << "\n=== 性能测试 ===" << endl;

    PlanarSubdivision subdivision;

    int gridSize = 10;
    cout << "生成 " << gridSize << "x" << gridSize << " 三角网格 ("
         << gridSize * gridSize * 2 << " 个三角形)" << endl;

    vector<Point2D> points;
    for (int j = 0; j <= gridSize; j++) {
        for (int i = 0; i <= gridSize; i++) {
            points.push_back(Point2D(i, j));
        }
    }
    subdivision.addVertices(points);

    for (int j = 0; j < gridSize; j++) {
        for (int i = 0; i < gridSize; i++) {
            int v00 = j * (gridSize + 1) + i;
            int v10 = j * (gridSize + 1) + i + 1;
            int v01 = (j + 1) * (gridSize + 1) + i;
            int v11 = (j + 1) * (gridSize + 1) + i + 1;

            subdivision.addTriangle(v00, v01, v11);
            subdivision.addTriangle(v00, v11, v10);
        }
    }

    subdivision.buildBoundaryIndex();
    subdivision.printStatistics();

    int numQueries = 1000;
    cout << "\n执行 " << numQueries << " 次随机查询..." << endl;

    srand(42);

    int totalIntersections = 0;
    auto queryStart = chrono::high_resolution_clock::now();

    for (int q = 0; q < numQueries; q++) {
        double x1 = static_cast<double>(rand()) / RAND_MAX * gridSize;
        double y1 = static_cast<double>(rand()) / RAND_MAX * gridSize;
        double x2 = static_cast<double>(rand()) / RAND_MAX * gridSize;
        double y2 = static_cast<double>(rand()) / RAND_MAX * gridSize;

        vector<Face*> result = subdivision.rayWalkQuery(Point2D(x1, y1), Point2D(x2, y2));
        totalIntersections += static_cast<int>(result.size());
    }

    auto queryEnd = chrono::high_resolution_clock::now();
    auto queryDuration = chrono::duration_cast<chrono::microseconds>(queryEnd - queryStart);

    cout << "总查询耗时: " << queryDuration.count() << " 微秒" << endl;
    cout << "平均每次查询: " << queryDuration.count() / numQueries << " 微秒" << endl;
    cout << "平均每次查询相交的多边形数: " << static_cast<double>(totalIntersections) / numQueries << endl;

    // 对比暴力方法
    cout << "\n对比暴力方法..." << endl;
    auto bruteStart = chrono::high_resolution_clock::now();

    srand(42);
    for (int q = 0; q < numQueries; q++) {
        double x1 = static_cast<double>(rand()) / RAND_MAX * gridSize;
        double y1 = static_cast<double>(rand()) / RAND_MAX * gridSize;
        double x2 = static_cast<double>(rand()) / RAND_MAX * gridSize;
        double y2 = static_cast<double>(rand()) / RAND_MAX * gridSize;

        subdivision.bruteForceQuery(Point2D(x1, y1), Point2D(x2, y2));
    }

    auto bruteEnd = chrono::high_resolution_clock::now();
    auto bruteDuration = chrono::duration_cast<chrono::microseconds>(bruteEnd - bruteStart);

    cout << "暴力方法总耗时: " << bruteDuration.count() << " 微秒" << endl;
    cout << "加速比: " << static_cast<double>(bruteDuration.count()) / queryDuration.count() << "x" << endl;
}

int main() {
    cout << "========================================================" << endl;
    cout << "  习题 1.3：平面分割数据结构与直线相交查询" << endl;
    cout << "  解法二（优化版）：射线行走法" << endl;
    cout << "========================================================" << endl;

    PlanarSubdivision subdivision;

    createTestMesh(subdivision);
    runQueryTests(subdivision);
    runPerformanceTest();

    cout << "\n=== 程序结束 ===" << endl;

    return 0;
}
