/**
 * ============================================================================
 * 习题 1.3：平面分割数据结构与直线相交查询
 * 解法二（高性能版）：射线行走法（Ray Walking）
 * ============================================================================
 *
 * 【优化说明】
 *
 * 相比"优化版"，本高性能版本修复了以下性能问题：
 *
 * 1. 移除每次查询创建 set 的开销：
 *    - 使用全局递增的"版本号"代替 set<Face*>
 *    - 每个 Face 记录最后访问的版本号
 *    - 比较版本号即可判断是否已访问，O(1) 时间
 *
 * 2. 简化精度控制：
 *    - 使用固定的 EPS = 1e-10
 *    - 对于规则网格不需要自适应精度计算
 *
 * 3. 内联关键函数：
 *    - orient2d 和 rayEdgeIntersection 直接内联
 *    - 减少函数调用开销
 *
 * 4. 使用 unordered_map 替代 map：
 *    - O(1) 平均查找时间 vs O(log n)
 *
 * ============================================================================
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <limits>

using namespace std;

// ============================================================================
// 常量定义
// ============================================================================

// 固定容差常量 - 对于规则网格足够使用
static constexpr double EPS = 1e-10;

// ============================================================================
// 第一部分：基础数据结构
// ============================================================================

struct Point2D {
    double x, y;

    Point2D(double _x = 0, double _y = 0) : x(_x), y(_y) {}

    Point2D operator+(const Point2D& p) const { return Point2D(x + p.x, y + p.y); }
    Point2D operator-(const Point2D& p) const { return Point2D(x - p.x, y - p.y); }
    Point2D operator*(double s) const { return Point2D(x * s, y * s); }

    friend ostream& operator<<(ostream& os, const Point2D& p) {
        os << "(" << p.x << ", " << p.y << ")";
        return os;
    }
};

// 前向声明
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

    // 包围盒
    double minX, minY, maxX, maxY;

    // 【关键优化】访问版本号 - 用于高效的循环检测
    // 每次查询开始时递增全局版本号，
    // 如果 face->visitedVersion == currentVersion，说明已访问
    mutable uint32_t visitedVersion;

    explicit Face(HalfEdge* he = nullptr, int _id = -1)
        : outerComponent(he), id(_id),
          minX(1e18), minY(1e18), maxX(-1e18), maxY(-1e18),
          visitedVersion(0) {}

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
// 第二部分：内联几何函数（避免函数调用开销）
// ============================================================================

/**
 * 内联 orient2d - 方向判定
 */
inline double orient2d(const Point2D& a, const Point2D& b, const Point2D& c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

/**
 * 内联射线-边交点计算
 *
 * 直接返回 t 值，避免额外的函数调用
 */
inline double rayEdgeIntersection(
    double p1x, double p1y, double p2x, double p2y,
    double ax, double ay, double bx, double by) {

    double dx = p2x - p1x;
    double dy = p2y - p1y;
    double ex = bx - ax;
    double ey = by - ay;

    double denom = dx * ey - dy * ex;

    if (denom > -EPS && denom < EPS) return -1.0;  // 平行

    double t = ((ax - p1x) * ey - (ay - p1y) * ex) / denom;
    double s = ((ax - p1x) * dy - (ay - p1y) * dx) / denom;

    if (t > EPS && t <= 1.0 + EPS && s >= -EPS && s <= 1.0 + EPS) {
        return t;
    }

    return -1.0;
}

// ============================================================================
// 第三部分：边键哈希（用于 unordered_map）
// ============================================================================

struct EdgeKey {
    int v1, v2;

    EdgeKey(int a, int b) {
        if (a < b) { v1 = a; v2 = b; }
        else { v1 = b; v2 = a; }
    }

    bool operator==(const EdgeKey& other) const {
        return v1 == other.v1 && v2 == other.v2;
    }
};

struct EdgeKeyHash {
    size_t operator()(const EdgeKey& key) const {
        // 简单高效的哈希组合
        return static_cast<size_t>(key.v1) * 1000003ULL + static_cast<size_t>(key.v2);
    }
};

// ============================================================================
// 第四部分：平面分割类（高性能版）
// ============================================================================

class PlanarSubdivision {
private:
    vector<unique_ptr<Vertex>> vertices;
    vector<unique_ptr<HalfEdge>> halfEdges;
    vector<unique_ptr<Face>> faces;

    unordered_map<EdgeKey, HalfEdge*, EdgeKeyHash> edgeMap;

    // 边界边缓存
    vector<HalfEdge*> boundaryEdges;

    // 【关键优化】全局版本号
    // 每次查询前递增，用于高效的访问检测
    uint32_t currentVersion;

    // 点定位缓存
    Face* lastLocatedFace;

public:
    PlanarSubdivision() : currentVersion(0), lastLocatedFace(nullptr) {}

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
     * 添加三角形（使用顶点索引）
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
            EdgeKey key(vertIds[i], vertIds[(i + 1) % 3]);
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
     * 点定位（Walk 算法 - 高性能版）
     *
     * 【优化点】
     * 1. 使用版本号代替 set，避免每次创建 set 的开销
     * 2. 使用简单的 double 比较代替 adaptiveOrient2d
     * 3. 减少函数调用
     */
    Face* locatePoint(const Point2D& p) {
        if (faces.empty()) return nullptr;

        Face* current = lastLocatedFace ? lastLocatedFace : faces[0].get();

        int maxIterations = static_cast<int>(faces.size()) + 100;

        while (maxIterations-- > 0) {
            // 【关键优化】使用版本号检测循环，O(1) 时间
            if (current->visitedVersion == currentVersion) {
                // 检测到循环，尝试从其他面开始
                bool found = false;
                for (const auto& f : faces) {
                    if (f->visitedVersion != currentVersion) {
                        current = f.get();
                        found = true;
                        break;
                    }
                }
                if (!found) return nullptr;
            }
            current->visitedVersion = currentVersion;

            // 获取三角形顶点
            HalfEdge* he = current->outerComponent;
            const Point2D& v0 = he->origin->position;
            const Point2D& v1 = he->next->origin->position;
            const Point2D& v2 = he->next->next->origin->position;

            // 【优化】直接内联 orient2d 计算
            double s0 = -((v1.x - v0.x) * (p.y - v0.y) - (v1.y - v0.y) * (p.x - v0.x));
            double s1 = -((v2.x - v1.x) * (p.y - v1.y) - (v2.y - v1.y) * (p.x - v1.x));
            double s2 = -((v0.x - v2.x) * (p.y - v2.y) - (v0.y - v2.y) * (p.x - v2.x));

            // 判断是否在三角形内
            if (s0 >= -EPS && s1 >= -EPS && s2 >= -EPS) {
                lastLocatedFace = current;
                return current;
            }

            // 选择跳转方向
            HalfEdge* nextEdge = nullptr;

            if (s0 < -EPS && (s0 <= s1 || s1 >= -EPS) && (s0 <= s2 || s2 >= -EPS)) {
                nextEdge = he;
            } else if (s1 < -EPS && (s1 <= s0 || s0 >= -EPS) && (s1 <= s2 || s2 >= -EPS)) {
                nextEdge = he->next;
            } else if (s2 < -EPS) {
                nextEdge = he->next->next;
            }

            if (nextEdge && nextEdge->twin) {
                current = nextEdge->twin->incidentFace;
            } else {
                return nullptr;  // 到达边界
            }
        }

        return nullptr;
    }

    /**
     * 射线行走查询（高性能版）
     *
     * 【优化点】
     * 1. 使用版本号代替 set
     * 2. 内联 rayEdgeIntersection 计算
     * 3. 使用固定 EPS
     */
    vector<Face*> rayWalkQuery(const Point2D& p1, const Point2D& p2) {
        vector<Face*> result;

        // 【关键】递增版本号，开始新查询
        // 注意：需要在 locatePoint 之前递增，locatePoint 也会使用版本号
        ++currentVersion;

        // 定位起点
        Face* startFace = locatePoint(p1);

        // 【修复】locatePoint 会标记访问的面，需要再次递增版本号
        // 否则 startFace 会被误认为已访问
        ++currentVersion;

        if (!startFace) {
            // 起点在网格外，找入口
            for (HalfEdge* he : boundaryEdges) {
                const Point2D& a = he->origin->position;
                const Point2D& b = he->next->origin->position;

                double t = rayEdgeIntersection(
                    p1.x, p1.y, p2.x, p2.y,
                    a.x, a.y, b.x, b.y);

                if (t > 0 && t <= 1) {
                    startFace = he->incidentFace;
                    break;
                }
            }

            if (!startFace) return result;
        }

        // 射线行走主循环
        Face* current = startFace;
        int maxIterations = static_cast<int>(faces.size()) + 100;

        // 预计算射线参数
        double p1x = p1.x, p1y = p1.y;
        double p2x = p2.x, p2y = p2.y;

        while (current && maxIterations-- > 0) {
            // 【关键优化】版本号检测，O(1)
            if (current->visitedVersion == currentVersion) break;
            current->visitedVersion = currentVersion;

            result.push_back(current);

            // 找退出边
            HalfEdge* exitEdge = nullptr;
            double minT = numeric_limits<double>::max();

            HalfEdge* he = current->outerComponent;
            do {
                const Point2D& a = he->origin->position;
                const Point2D& b = he->next->origin->position;

                // 【优化】内联交点计算
                double t = rayEdgeIntersection(
                    p1x, p1y, p2x, p2y,
                    a.x, a.y, b.x, b.y);

                if (t > EPS && t < minT) {
                    minT = t;
                    exitEdge = he;
                }

                he = he->next;
            } while (he != current->outerComponent);

            // 判断是否继续
            if (!exitEdge || minT > 1.0 + EPS) {
                break;
            }

            // 跳转
            if (exitEdge->twin) {
                current = exitEdge->twin->incidentFace;
            } else {
                break;
            }
        }

        return result;
    }

    /**
     * 暴力查询（用于验证）
     */
    vector<Face*> bruteForceQuery(const Point2D& p1, const Point2D& p2) {
        vector<Face*> result;

        for (const auto& f : faces) {
            HalfEdge* he = f->outerComponent;
            bool intersects = false;

            do {
                const Point2D& a = he->origin->position;
                const Point2D& b = he->next->origin->position;

                double d1 = orient2d(a, b, p1);
                double d2 = orient2d(a, b, p2);
                double d3 = orient2d(p1, p2, a);
                double d4 = orient2d(p1, p2, b);

                if (((d1 > EPS && d2 < -EPS) || (d1 < -EPS && d2 > EPS)) &&
                    ((d3 > EPS && d4 < -EPS) || (d3 < -EPS && d4 > EPS))) {
                    intersects = true;
                    break;
                }

                he = he->next;
            } while (he != f->outerComponent);

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
// 第五部分：测试程序
// ============================================================================

void createTestMesh(PlanarSubdivision& subdivision) {
    cout << "=== 创建测试三角网格（带完整拓扑）===" << endl;

    vector<Point2D> points = {
        Point2D(0, 0), Point2D(1, 0), Point2D(2, 0),
        Point2D(0, 1), Point2D(1, 1), Point2D(2, 1),
        Point2D(0, 2), Point2D(1, 2), Point2D(2, 2)
    };

    subdivision.addVertices(points);

    subdivision.addTriangle(3, 6, 7);
    subdivision.addTriangle(3, 7, 4);
    subdivision.addTriangle(4, 7, 8);
    subdivision.addTriangle(4, 8, 5);

    subdivision.addTriangle(0, 3, 4);
    subdivision.addTriangle(0, 4, 1);
    subdivision.addTriangle(1, 4, 5);
    subdivision.addTriangle(1, 5, 2);

    subdivision.buildBoundaryIndex();

    cout << "创建了 " << subdivision.numFaces() << " 个三角形面" << endl;
    subdivision.printStatistics();
}

void runQueryTests(PlanarSubdivision& subdivision) {
    cout << "\n=== 运行直线相交查询测试（高性能版）===" << endl;

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
    cout << "  解法二（高性能版）：射线行走法" << endl;
    cout << "========================================================" << endl;

    PlanarSubdivision subdivision;

    createTestMesh(subdivision);
    runQueryTests(subdivision);
    runPerformanceTest();

    cout << "\n=== 程序结束 ===" << endl;

    return 0;
}
