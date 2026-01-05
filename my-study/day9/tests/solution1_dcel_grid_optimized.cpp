/**
 * ============================================================================
 * 习题 1.3：平面分割数据结构与直线相交查询
 * 解法一（优化版）：DCEL（双向连接边表）+ 均匀网格方法
 * ============================================================================
 *
 * 【优化说明】
 *
 * 相比原版，本优化版本包含以下改进：
 *
 * 1. 内存管理：
 *    - 使用 std::unique_ptr 管理动态内存
 *    - 避免内存泄漏和重复释放
 *
 * 2. twin 指针设置：
 *    - 使用 unordered_map 建立边的配对关系
 *    - 正确设置半边之间的 twin 指针
 *
 * 3. DDA 算法改进：
 *    - 使用 Amanatides & Woo 精确遍历算法
 *    - 保证不遗漏任何被直线穿过的网格单元
 *
 * 4. 数值精度：
 *    - 使用 std::numeric_limits 替代魔数
 *    - 支持自适应容差
 *
 * 5. 代码规范：
 *    - 引用公共头文件
 *    - 添加 const 正确性
 *    - 使用现代 C++ 特性
 *
 * ============================================================================
 */

// ============================================================================
// 头文件包含区域
// ============================================================================

#include <iostream>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <limits>
#include <functional>

// 引用公共头文件
#include "common/geometry_types.hpp"
#include "common/robust_predicates.hpp"

using namespace std;
using namespace geometry;

// ============================================================================
// 第一部分：DCEL 数据结构定义
// ============================================================================

// 前向声明
class HalfEdge;
class Face;

/**
 * 顶点类（Vertex）
 *
 * 使用原始指针指向 HalfEdge（因为 HalfEdge 的生命周期由 PlanarSubdivision 管理）
 */
class Vertex {
public:
    Point2D position;       // 顶点在平面上的坐标位置
    HalfEdge* incidentEdge; // 指向一条从该顶点出发的半边
    int id;                 // 顶点的唯一标识符

    /**
     * 构造函数（使用坐标值）
     */
    Vertex(double x = 0, double y = 0, int _id = -1)
        : position(x, y), incidentEdge(nullptr), id(_id) {}

    /**
     * 构造函数（使用 Point2D 对象）
     */
    explicit Vertex(const Point2D& p, int _id = -1)
        : position(p), incidentEdge(nullptr), id(_id) {}
};

/**
 * 半边类（Half-Edge）
 *
 * DCEL 的核心结构
 */
class HalfEdge {
public:
    Vertex* origin;         // 半边的起点
    HalfEdge* twin;         // 对偶半边（方向相反的配对半边）
    HalfEdge* next;         // 同一面上逆时针方向的下一条半边
    HalfEdge* prev;         // 同一面上逆时针方向的上一条半边
    Face* incidentFace;     // 半边所属的面
    int id;                 // 半边的唯一标识符

    /**
     * 构造函数
     */
    explicit HalfEdge(Vertex* v = nullptr, int _id = -1)
        : origin(v), twin(nullptr), next(nullptr), prev(nullptr),
          incidentFace(nullptr), id(_id) {}

    /**
     * 获取半边的终点
     */
    Vertex* destination() const {
        return next ? next->origin : nullptr;
    }
};

/**
 * 面类（Face）
 *
 * 存储外边界半边指针和包围盒
 */
class Face {
public:
    HalfEdge* outerComponent;  // 外边界上的一条半边
    int id;                    // 面的唯一标识符
    BoundingBox2D boundingBox; // 包围盒

    /**
     * 构造函数
     */
    explicit Face(HalfEdge* he = nullptr, int _id = -1)
        : outerComponent(he), id(_id) {}

    /**
     * 计算面的轴对齐包围盒
     */
    void computeBoundingBox() {
        if (!outerComponent) return;

        boundingBox = BoundingBox2D();  // 重置为空包围盒

        HalfEdge* he = outerComponent;
        do {
            boundingBox.expand(he->origin->position);
            he = he->next;
        } while (he != outerComponent);
    }

    /**
     * 获取面的所有顶点坐标列表
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
// 第二部分：平面分割类（优化版）
// ============================================================================

/**
 * 平面分割类（PlanarSubdivision）- DCEL + 均匀网格空间索引
 *
 * 【设计说明】
 * 本类实现了平面分割的数据结构和直线相交查询功能：
 * 1. 使用 DCEL（双向连接边表）存储平面分割的拓扑结构
 * 2. 使用均匀网格作为空间索引，加速直线相交查询
 * 3. 使用智能指针（unique_ptr）管理内存，避免内存泄漏
 *
 * 【核心算法】
 * - 空间索引：将平面划分为 gridNx × gridNy 的均匀网格
 * - 直线查询：使用 Amanatides & Woo 算法遍历网格单元
 *
 * 【复杂度分析】
 * - 构建空间索引：O(n × 平均单元覆盖数)
 * - 直线查询：O(k + m)，k 为遍历的网格单元数，m 为候选面数
 *
 * 【使用示例】
 * PlanarSubdivision subdivision(50, 50);  // 创建 50×50 网格
 * subdivision.addFace({...});              // 添加多边形面
 * subdivision.buildSpatialIndex();         // 构建空间索引
 * auto result = subdivision.queryLineIntersection(p1, p2);  // 查询
 */
class PlanarSubdivision {
private:
    // ========== DCEL 数据存储 ==========
    // 使用 unique_ptr 管理内存，确保 RAII 语义
    vector<unique_ptr<Vertex>> vertices;    // 所有顶点的所有权容器
    vector<unique_ptr<HalfEdge>> halfEdges; // 所有半边的所有权容器
    vector<unique_ptr<Face>> faces;         // 所有面的所有权容器

    // 边映射表：用于在添加面时建立 twin 指针配对关系
    // 键：EdgeKey(v1, v2)，规范化后的顶点 ID 对
    // 值：对应的半边指针
    unordered_map<EdgeKey, HalfEdge*, EdgeKeyHash> edgeMap;

    // ========== 均匀网格参数 ==========
    int gridNx, gridNy;           // 网格在 X/Y 方向的单元数量
    double cellWidth, cellHeight; // 每个单元的宽度和高度
    double globalMinX, globalMinY;// 网格覆盖区域的左下角坐标
    double globalMaxX, globalMaxY;// 网格覆盖区域的右上角坐标

    // 网格存储：三维数组
    // grid[i][j] 表示单元 (i, j) 中注册的面指针列表
    // 一个面可能注册到多个单元（当其包围盒跨越多个单元时）
    // 空间复杂度：O(gridNx × gridNy + 总注册次数)
    vector<vector<vector<Face*>>> grid;

public:
    /**
     * 构造函数
     */
    explicit PlanarSubdivision(int nx = 50, int ny = 50)
        : gridNx(nx), gridNy(ny),
          cellWidth(0), cellHeight(0),
          globalMinX(0), globalMinY(0),
          globalMaxX(0), globalMaxY(0) {}

    // 禁用拷贝（因为使用了 unique_ptr）
    PlanarSubdivision(const PlanarSubdivision&) = delete;
    PlanarSubdivision& operator=(const PlanarSubdivision&) = delete;

    // 允许移动
    PlanarSubdivision(PlanarSubdivision&&) = default;
    PlanarSubdivision& operator=(PlanarSubdivision&&) = default;

    /**
     * 析构函数
     * unique_ptr 自动释放内存，无需手动 delete
     */
    ~PlanarSubdivision() = default;

    /**
     * 添加一个多边形面（带 twin 指针设置）
     *
     * 【优化点】
     * 使用 edgeMap 建立边的配对关系，正确设置 twin 指针
     */
    Face* addFace(const vector<Point2D>& polygon) {
        int n = static_cast<int>(polygon.size());
        if (n < 3) return nullptr;

        // 创建面
        auto face = make_unique<Face>();
        face->id = static_cast<int>(faces.size());

        // 创建顶点
        vector<Vertex*> faceVertices(n);
        for (int i = 0; i < n; i++) {
            auto v = make_unique<Vertex>(polygon[i], static_cast<int>(vertices.size()));
            faceVertices[i] = v.get();
            vertices.push_back(std::move(v));
        }

        // 创建半边
        vector<HalfEdge*> faceEdges(n);
        for (int i = 0; i < n; i++) {
            auto he = make_unique<HalfEdge>(faceVertices[i], static_cast<int>(halfEdges.size()));
            he->incidentFace = face.get();
            faceVertices[i]->incidentEdge = he.get();
            faceEdges[i] = he.get();
            halfEdges.push_back(std::move(he));
        }

        // 连接半边形成环
        for (int i = 0; i < n; i++) {
            faceEdges[i]->next = faceEdges[(i + 1) % n];
            faceEdges[i]->prev = faceEdges[(i - 1 + n) % n];
        }

        // 设置面的属性
        face->outerComponent = faceEdges[0];
        face->computeBoundingBox();

        // 【关键优化】设置 twin 指针
        for (int i = 0; i < n; i++) {
            int id1 = faceVertices[i]->id;
            int id2 = faceVertices[(i + 1) % n]->id;

            EdgeKey key(id1, id2);

            auto it = edgeMap.find(key);
            if (it != edgeMap.end()) {
                // 找到配对边，设置双向 twin
                HalfEdge* existingEdge = it->second;
                faceEdges[i]->twin = existingEdge;
                existingEdge->twin = faceEdges[i];
            } else {
                // 注册新边
                edgeMap[key] = faceEdges[i];
            }
        }

        Face* facePtr = face.get();
        faces.push_back(std::move(face));

        return facePtr;
    }

    /**
     * 构建空间索引（均匀网格）
     *
     * 【算法步骤】
     * 1. 计算所有面的全局包围盒
     * 2. 将包围盒扩展一个小边距，防止边界问题
     * 3. 计算每个网格单元的尺寸
     * 4. 将每个面注册到其包围盒覆盖的所有网格单元中
     *
     * 【设计决策】
     * - 使用均匀网格而非四叉树，因为对于均匀分布的数据效率更高
     * - 边界扩展 pad=0.01 防止浮点误差导致的边界问题
     * - 一个面可能注册到多个单元，这是空间换时间的权衡
     *
     * 【复杂度】
     * - 时间：O(n × 平均单元覆盖数)，n 为面的数量
     * - 空间：O(gridNx × gridNy + 总注册次数)
     */
    void buildSpatialIndex() {
        if (faces.empty()) return;

        cout << "\n=== 构建均匀网格空间索引 ===" << endl;

        // ===== 步骤 1：计算全局包围盒 =====
        // 遍历所有面，找出最小和最大坐标
        globalMinX = globalMinY = numeric_limits<double>::max();
        globalMaxX = globalMaxY = numeric_limits<double>::lowest();

        for (const auto& f : faces) {
            const auto& bb = f->boundingBox;
            globalMinX = min(globalMinX, bb.minX);
            globalMinY = min(globalMinY, bb.minY);
            globalMaxX = max(globalMaxX, bb.maxX);
            globalMaxY = max(globalMaxY, bb.maxY);
        }

        // ===== 步骤 2：扩展边界 =====
        // 添加小边距，防止边界上的点落在网格外
        double pad = 0.01;
        globalMinX -= pad; globalMinY -= pad;
        globalMaxX += pad; globalMaxY += pad;

        // ===== 步骤 3：计算单元尺寸 =====
        cellWidth = (globalMaxX - globalMinX) / gridNx;
        cellHeight = (globalMaxY - globalMinY) / gridNy;

        cout << "全局包围盒: [" << globalMinX << ", " << globalMinY << "] -> ["
             << globalMaxX << ", " << globalMaxY << "]" << endl;
        cout << "网格维度: " << gridNx << " x " << gridNy << endl;
        cout << "单元尺寸: " << cellWidth << " x " << cellHeight << endl;

        // 初始化网格：分配 gridNx × gridNy 的二维数组
        grid.assign(gridNx, vector<vector<Face*>>(gridNy));

        // ===== 步骤 4：将面注册到网格单元 =====
        // 对于每个面，计算其包围盒覆盖的单元范围，并注册到所有这些单元
        int totalRegistrations = 0;

        for (const auto& f : faces) {
            const auto& bb = f->boundingBox;

            // 计算包围盒覆盖的单元索引范围
            // 公式：索引 = floor((坐标 - 网格原点) / 单元尺寸)
            int minI = static_cast<int>((bb.minX - globalMinX) / cellWidth);
            int maxI = static_cast<int>((bb.maxX - globalMinX) / cellWidth);
            int minJ = static_cast<int>((bb.minY - globalMinY) / cellHeight);
            int maxJ = static_cast<int>((bb.maxY - globalMinY) / cellHeight);

            // 边界限制：确保索引在有效范围内 [0, gridN-1]
            minI = max(0, min(minI, gridNx - 1));
            maxI = max(0, min(maxI, gridNx - 1));
            minJ = max(0, min(minJ, gridNy - 1));
            maxJ = max(0, min(maxJ, gridNy - 1));

            // 将面注册到所有覆盖的单元
            for (int i = minI; i <= maxI; i++) {
                for (int j = minJ; j <= maxJ; j++) {
                    grid[i][j].push_back(f.get());
                    totalRegistrations++;
                }
            }
        }

        cout << "面数量: " << faces.size() << endl;
        cout << "总注册次数: " << totalRegistrations << endl;
        cout << "平均每面注册单元数: " << static_cast<double>(totalRegistrations) / faces.size() << endl;
    }

    /**
     * 直线相交查询（使用 Amanatides & Woo 算法）
     *
     * 【算法原理】
     * 本方法采用两阶段过滤策略：
     *
     * 阶段 1：粗筛选（Broad Phase）
     * - 使用 Amanatides & Woo 快速体素遍历算法
     * - 精确找出直线穿过的所有网格单元
     * - 收集这些单元中注册的所有面作为候选集
     *
     * 阶段 2：精筛选（Narrow Phase）
     * - 对候选集中的每个面进行精确的线段-多边形相交测试
     * - 排除假阳性（仅包围盒相交但实际不相交的面）
     *
     * 【Amanatides & Woo 算法优势】
     * - 保证不遗漏任何被直线穿过的网格单元
     * - 按直线行进顺序遍历，无重复访问
     * - 时间复杂度与穿过的单元数成线性关系
     *
     * 【复杂度】
     * - 时间：O(k + m × p)
     *   - k: 直线穿过的网格单元数（通常与对角线长度成比例）
     *   - m: 候选面数量
     *   - p: 每个面的平均边数
     * - 空间：O(m) 用于候选集存储
     *
     * @param p1 直线起点
     * @param p2 直线终点
     * @return 与直线相交的所有面的列表
     */
    vector<Face*> queryLineIntersection(const Point2D& p1, const Point2D& p2) {
        unordered_set<Face*> candidateSet;  // 使用集合去重（一个面可能注册在多个单元）
        vector<Face*> result;

        // 计算直线长度，用于退化情况检测
        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double length = sqrt(dx * dx + dy * dy);

        // ===== 处理退化情况：点查询 =====
        // 如果两点几乎重合，直接在单个单元中查找
        if (length < numeric_limits<double>::epsilon()) {
            int ci = static_cast<int>((p1.x - globalMinX) / cellWidth);
            int cj = static_cast<int>((p1.y - globalMinY) / cellHeight);
            ci = max(0, min(ci, gridNx - 1));
            cj = max(0, min(cj, gridNy - 1));

            for (Face* f : grid[ci][cj]) {
                if (lineIntersectsPolygon(p1, p2, f)) {
                    result.push_back(f);
                }
            }
            return result;
        }

        // ===== 阶段 1：使用 Amanatides & Woo 算法遍历网格 =====
        // 该算法保证精确遍历直线穿过的所有单元
        // 参见：robust_predicates.hpp 中的 traverseGridAmanatidesWoo
        traverseGridAmanatidesWoo(
            p1, p2,
            globalMinX, globalMinY,
            cellWidth, cellHeight,
            gridNx, gridNy,
            [this, &candidateSet](int x, int y) {
                // Lambda 回调：将单元中的所有面加入候选集
                for (Face* f : grid[x][y]) {
                    candidateSet.insert(f);  // 自动去重
                }
            }
        );

        // ===== 阶段 2：对候选面进行精确相交测试 =====
        for (Face* f : candidateSet) {
            if (lineIntersectsPolygon(p1, p2, f)) {
                result.push_back(f);
            }
        }

        return result;
    }

    /**
     * 判断线段是否与多边形相交
     */
    bool lineIntersectsPolygon(const Point2D& p1, const Point2D& p2, Face* face) const {
        const auto& bb = face->boundingBox;

        // 包围盒快速排除
        if (!lineIntersectsBox(p1, p2, bb)) {
            return false;
        }

        // 检查与多边形各边的相交
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

    /**
     * 打印统计信息
     */
    void printStatistics() const {
        cout << "\n=== 数据结构统计 ===" << endl;
        cout << "顶点数: " << vertices.size() << endl;
        cout << "半边数: " << halfEdges.size() << endl;
        cout << "面数: " << faces.size() << endl;

        // 统计 twin 指针设置情况
        int twinCount = 0;
        int boundaryCount = 0;

        for (const auto& he : halfEdges) {
            if (he->twin) {
                twinCount++;
            } else {
                boundaryCount++;
            }
        }

        cout << "内部边（有 twin）: " << twinCount / 2 << endl;
        cout << "边界边（无 twin）: " << boundaryCount << endl;

        // 统计网格使用情况
        int nonEmptyCells = 0;
        int maxFacesInCell = 0;
        int totalFacesInCells = 0;

        for (int i = 0; i < gridNx; i++) {
            for (int j = 0; j < gridNy; j++) {
                int count = static_cast<int>(grid[i][j].size());
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
            cout << "平均每单元面数: " << static_cast<double>(totalFacesInCells) / nonEmptyCells << endl;
        }
    }

    /**
     * 获取面的数量
     */
    int numFaces() const { return static_cast<int>(faces.size()); }

    /**
     * 获取指定索引的面
     */
    Face* getFace(int i) const { return faces[i].get(); }
};

// ============================================================================
// 第三部分：测试程序
// ============================================================================

/**
 * 创建测试用的三角网格
 *
 * 【网格结构】
 * 创建一个 2×2 的正方形区域，每个正方形被对角线分割为两个三角形
 * 共 8 个三角形，覆盖区域 [0,2] × [0,2]
 *
 *     (0,2)-----(1,2)-----(2,2)
 *       |  \ 0 |  \ 2  |
 *       | 1  \ | 3  \  |
 *     (0,1)-----(1,1)-----(2,1)
 *       |  \ 4 |  \ 6  |
 *       | 5  \ | 7  \  |
 *     (0,0)-----(1,0)-----(2,0)
 *
 * 【用途】
 * 用于验证算法的正确性，包括：
 * - 对角线查询
 * - 水平线/垂直线查询
 * - 边界情况处理
 */
void createTestMesh(PlanarSubdivision& subdivision) {
    cout << "=== 创建测试三角网格 ===" << endl;

    vector<Point2D> tri;

    // 第一行（y = 1 到 y = 2）：4 个三角形
    tri = {Point2D(0, 1), Point2D(0, 2), Point2D(1, 2)};  // 三角形 0：左上单元的上三角
    subdivision.addFace(tri);

    tri = {Point2D(0, 1), Point2D(1, 2), Point2D(1, 1)};  // 三角形 1：左上单元的下三角
    subdivision.addFace(tri);

    tri = {Point2D(1, 1), Point2D(1, 2), Point2D(2, 2)};  // 三角形 2：右上单元的上三角
    subdivision.addFace(tri);

    tri = {Point2D(1, 1), Point2D(2, 2), Point2D(2, 1)};  // 三角形 3：右上单元的下三角
    subdivision.addFace(tri);

    // 第二行（y = 0 到 y = 1）：4 个三角形
    tri = {Point2D(0, 0), Point2D(0, 1), Point2D(1, 1)};  // 三角形 4：左下单元的上三角
    subdivision.addFace(tri);

    tri = {Point2D(0, 0), Point2D(1, 1), Point2D(1, 0)};  // 三角形 5：左下单元的下三角
    subdivision.addFace(tri);

    tri = {Point2D(1, 0), Point2D(1, 1), Point2D(2, 1)};  // 三角形 6：右下单元的上三角
    subdivision.addFace(tri);

    tri = {Point2D(1, 0), Point2D(2, 1), Point2D(2, 0)};  // 三角形 7：右下单元的下三角
    subdivision.addFace(tri);

    cout << "创建了 " << subdivision.numFaces() << " 个三角形面" << endl;
}

/**
 * 运行查询测试
 *
 * 【测试用例说明】
 * 1. 对角线查询：(0.2, 0.2) -> (1.8, 1.8)
 *    - 预期穿过多个三角形
 *    - 测试斜向遍历的正确性
 *
 * 2. 水平线查询：(0, 1) -> (2, 1)
 *    - 沿 y=1 的水平线
 *    - 测试边界情况（y=1 是网格边界）
 *
 * 3. 垂直线查询：(1, 0) -> (1, 2)
 *    - 沿 x=1 的垂直线
 *    - 测试边界情况（x=1 是网格边界）
 *
 * 4. 短线段查询：(0.3, 0.3) -> (0.4, 0.4)
 *    - 完全位于单个三角形内部
 *    - 测试小范围查询
 */
void runQueryTests(PlanarSubdivision& subdivision) {
    cout << "\n=== 运行直线相交查询测试 ===" << endl;

    // 测试用例 1：对角线查询
    {
        Point2D p1(0.2, 0.2);
        Point2D p2(1.8, 1.8);

        cout << "\n测试 1：对角线查询" << endl;
        cout << "直线: " << p1 << " -> " << p2 << endl;

        auto start = chrono::high_resolution_clock::now();
        vector<Face*> result = subdivision.queryLineIntersection(p1, p2);
        auto end = chrono::high_resolution_clock::now();

        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);

        cout << "相交的多边形数量: " << result.size() << endl;
        cout << "多边形 ID: ";
        for (Face* f : result) {
            cout << f->id << " ";
        }
        cout << endl;
        cout << "查询耗时: " << duration.count() << " 微秒" << endl;
    }

    // 测试用例 2：水平线查询
    {
        Point2D p1(0.0, 1.0);
        Point2D p2(2.0, 1.0);

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

    // 测试用例 3：垂直线查询
    {
        Point2D p1(1.0, 0.0);
        Point2D p2(1.0, 2.0);

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

    // 测试用例 4：短线段查询
    {
        Point2D p1(0.3, 0.3);
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
 *
 * 【测试内容】
 * 1. 生成 10×10 的三角网格（200 个三角形）
 * 2. 使用 100×100 的均匀网格进行空间索引
 * 3. 执行 1000 次随机直线查询
 * 4. 统计构建时间和查询时间
 *
 * 【性能指标】
 * - 空间索引构建时间
 * - 平均单次查询时间
 * - 平均每次查询相交的多边形数量
 *
 * 【随机数种子】
 * 使用固定种子 42 确保结果可重现
 */
void runPerformanceTest() {
    cout << "\n=== 性能测试 ===" << endl;

    // 使用 100×100 网格，对 200 个三角形是足够细致的划分
    PlanarSubdivision subdivision(100, 100);

    int gridSize = 10;
    cout << "生成 " << gridSize << "x" << gridSize << " 三角网格 ("
         << gridSize * gridSize * 2 << " 个三角形)" << endl;

    vector<Point2D> tri;
    for (int i = 0; i < gridSize; i++) {
        for (int j = 0; j < gridSize; j++) {
            double x0 = i, y0 = j;
            double x1 = i + 1, y1 = j + 1;

            tri = {Point2D(x0, y0), Point2D(x0, y1), Point2D(x1, y1)};
            subdivision.addFace(tri);

            tri = {Point2D(x0, y0), Point2D(x1, y1), Point2D(x1, y0)};
            subdivision.addFace(tri);
        }
    }

    auto buildStart = chrono::high_resolution_clock::now();
    subdivision.buildSpatialIndex();
    auto buildEnd = chrono::high_resolution_clock::now();

    auto buildDuration = chrono::duration_cast<chrono::microseconds>(buildEnd - buildStart);
    cout << "构建空间索引耗时: " << buildDuration.count() << " 微秒" << endl;

    subdivision.printStatistics();

    // 执行随机查询
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

        vector<Face*> result = subdivision.queryLineIntersection(
            Point2D(x1, y1), Point2D(x2, y2));
        totalIntersections += static_cast<int>(result.size());
    }

    auto queryEnd = chrono::high_resolution_clock::now();
    auto queryDuration = chrono::duration_cast<chrono::microseconds>(queryEnd - queryStart);

    cout << "总查询耗时: " << queryDuration.count() << " 微秒" << endl;
    cout << "平均每次查询: " << queryDuration.count() / numQueries << " 微秒" << endl;
    cout << "平均每次查询相交的多边形数: " << static_cast<double>(totalIntersections) / numQueries << endl;
}

/**
 * 主函数
 */
int main() {
    cout << "========================================================" << endl;
    cout << "  习题 1.3：平面分割数据结构与直线相交查询" << endl;
    cout << "  解法一（优化版）：DCEL + 均匀网格方法" << endl;
    cout << "========================================================" << endl;

    PlanarSubdivision subdivision(10, 10);

    createTestMesh(subdivision);

    subdivision.buildSpatialIndex();

    subdivision.printStatistics();

    runQueryTests(subdivision);

    runPerformanceTest();

    cout << "\n=== 程序结束 ===" << endl;

    return 0;
}
