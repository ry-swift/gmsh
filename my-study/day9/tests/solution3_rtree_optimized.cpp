/**
 * ============================================================================
 * 习题 1.3：平面分割数据结构与直线相交查询
 * 解法三（优化版）：R-Tree 版本
 * ============================================================================
 *
 * 【优化说明】
 *
 * 1. 内存管理：使用 std::unique_ptr
 * 2. R-Tree 分裂策略：实现二次分裂算法（Quadratic Split）
 * 3. union 安全性：使用两个独立数组替代 union
 * 4. 直线查询优化：实现直接的直线-节点相交测试
 * 5. 代码规范：引用公共头文件
 *
 * ============================================================================
 */

#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <limits>
#include <cassert>

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
// 第二部分：R-Tree 实现（优化版）
// ============================================================================

/**
 * R-Tree 节点类（优化版）
 *
 * 【R-Tree 简介】
 * R-Tree（Rectangle Tree）是一种平衡树数据结构，用于空间索引：
 * - 每个节点包含多个条目，每个条目有一个最小包围矩形（MBR）
 * - 叶子节点存储实际的空间对象
 * - 内部节点存储指向子节点的指针
 * - 查询时通过 MBR 重叠测试快速排除不相关的子树
 *
 * 【节点结构设计】
 * - MAXNODES: 节点最大容量，影响树高度
 *   - 值越大，树越矮，但节点内搜索时间增加
 *   - 典型值：4-16
 * - MINNODES: 节点最小填充率（MAXNODES/2）
 *   - 保证树的平衡性
 *   - 防止节点过于稀疏
 *
 * 【内存布局优化】
 * 原版 R-Tree 常用 union 存储子节点指针和数据指针
 * 本实现使用两个独立数组，原因：
 * 1. 避免 union + unique_ptr 的复杂内存管理问题
 * 2. 简化代码，提高可维护性
 * 3. 空间开销增加有限（每节点额外 8*sizeof(void*) 字节）
 */
class RTreeNode {
public:
    // ========== 节点容量常量 ==========
    static const int MAXNODES = 8;           // 节点最大条目数（影响树高度和查询效率）
    static const int MINNODES = MAXNODES / 2;// 节点最小条目数（保证平衡，分裂时使用）

    // ========== 节点属性 ==========
    bool isLeaf;           // 是否为叶子节点
    int count;             // 当前条目数量
    BoundingBox2D mbr;     // 本节点所有条目的最小包围矩形

    // ========== 条目存储 ==========
    // 每个条目的包围盒（内部节点为子树 MBR，叶子节点为数据 MBR）
    BoundingBox2D childMBR[MAXNODES];

    // 【优化】使用两个独立数组替代 union，避免内存管理问题
    unique_ptr<RTreeNode> children[MAXNODES];  // 内部节点：指向子节点
    Face* data[MAXNODES];                       // 叶子节点：指向数据对象

    /**
     * 构造函数
     * @param leaf 是否为叶子节点（默认 true）
     */
    explicit RTreeNode(bool leaf = true) : isLeaf(leaf), count(0) {
        for (int i = 0; i < MAXNODES; i++) {
            data[i] = nullptr;
        }
    }

    /**
     * 更新节点的 MBR
     * 遍历所有条目的 MBR，计算它们的并集
     */
    void updateMBR() {
        mbr = BoundingBox2D();  // 重置为空包围盒
        for (int i = 0; i < count; i++) {
            mbr.expand(childMBR[i]);
        }
    }
};

/**
 * R-Tree 类（优化版）- 空间索引数据结构
 *
 * 【R-Tree 核心操作】
 * 1. 插入（Insert）：将新元素插入到合适的叶子节点
 *    - 选择使 MBR 增长最小的子节点
 *    - 节点满时触发分裂
 *
 * 2. 查询（Search）：找出与给定范围相交的所有元素
 *    - 从根节点开始递归检查 MBR 重叠
 *    - 剪枝策略：跳过 MBR 不相交的子树
 *
 * 3. 分裂（Split）：节点满时将其分为两个节点
 *    - 本实现使用 Guttman 的二次分裂算法（Quadratic Split）
 *
 * 【分裂算法比较】
 * | 算法 | 复杂度 | 分裂质量 | 实现复杂度 |
 * |------|--------|----------|------------|
 * | 线性（Linear） | O(n) | 较低 | 简单 |
 * | 二次（Quadratic） | O(n²) | 中等 | 中等 |
 * | 指数（Exhaustive） | O(2^n) | 最优 | 复杂 |
 *
 * 本实现选择二次分裂，平衡了分裂质量和计算开销。
 *
 * 【复杂度分析】
 * - 插入：O(log n) 平均，O(n) 最坏（分裂传播）
 * - 范围查询：O(n) 最坏，但实际中远优于此
 * - 直线查询：O(k log n + m)，k 为访问节点数，m 为候选数
 */
class RTree {
private:
    unique_ptr<RTreeNode> root;  // 根节点
    int numItems;                 // 存储的元素总数

public:
    RTree() : root(nullptr), numItems(0) {}

    /**
     * 插入面到 R-Tree
     *
     * 【算法流程】
     * 1. 如果树为空，创建根节点
     * 2. 递归向下找到合适的叶子节点
     * 3. 如果叶子节点未满，直接插入
     * 4. 如果叶子节点已满，触发分裂并向上传播
     * 5. 如果根节点分裂，创建新的根节点
     *
     * @param face 待插入的面
     */
    void insert(Face* face) {
        BoundingBox2D rect = face->boundingBox;

        if (!root) {
            root = make_unique<RTreeNode>(true);
        }

        RTreeNode* newNode = insertInternal(root.get(), face, rect);

        if (newNode) {
            // 根节点分裂
            auto newRoot = make_unique<RTreeNode>(false);

            newRoot->childMBR[0] = root->mbr;
            newRoot->children[0] = std::move(root);

            newRoot->childMBR[1] = newNode->mbr;
            newRoot->children[1].reset(newNode);

            newRoot->count = 2;
            newRoot->updateMBR();

            root = std::move(newRoot);
        }

        numItems++;
    }

    /**
     * 范围查询
     */
    vector<Face*> search(const BoundingBox2D& rect) {
        vector<Face*> results;
        if (root) {
            searchInternal(root.get(), rect, results);
        }
        return results;
    }

    /**
     * 直线查询（优化版 - 直接测试直线与节点的相交）
     */
    vector<Face*> lineQuery(const Point2D& p1, const Point2D& p2) {
        vector<Face*> results;
        if (root) {
            lineQueryInternal(root.get(), p1, p2, results);
        }

        // 精确相交测试
        vector<Face*> finalResults;
        for (Face* f : results) {
            if (lineIntersectsPolygon(p1, p2, f)) {
                finalResults.push_back(f);
            }
        }

        return finalResults;
    }

    int size() const { return numItems; }

private:
    /**
     * 内部插入（递归）
     */
    RTreeNode* insertInternal(RTreeNode* node, Face* face, const BoundingBox2D& rect) {
        if (node->isLeaf) {
            if (node->count < RTreeNode::MAXNODES) {
                node->data[node->count] = face;
                node->childMBR[node->count] = rect;
                node->count++;
                node->updateMBR();
                return nullptr;
            } else {
                return splitLeafQuadratic(node, face, rect);
            }
        } else {
            int bestChild = chooseSubtree(node, rect);
            RTreeNode* newNode = insertInternal(node->children[bestChild].get(), face, rect);

            node->childMBR[bestChild] = node->children[bestChild]->mbr;
            node->updateMBR();

            if (newNode) {
                if (node->count < RTreeNode::MAXNODES) {
                    node->children[node->count].reset(newNode);
                    node->childMBR[node->count] = newNode->mbr;
                    node->count++;
                    node->updateMBR();
                    return nullptr;
                } else {
                    return splitInternalQuadratic(node, newNode);
                }
            }

            return nullptr;
        }
    }

    /**
     * 选择最佳子节点
     */
    int chooseSubtree(RTreeNode* node, const BoundingBox2D& rect) {
        int bestChild = 0;
        double minEnlargement = node->childMBR[0].enlargement(rect);

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
     * 二次分裂算法（叶子节点）
     *
     * 【Guttman 二次分裂算法】
     * 这是 R-Tree 原始论文中提出的分裂策略，目标是最小化分裂后两个节点
     * MBR 的总覆盖面积。
     *
     * 【算法步骤】
     *
     * 步骤 1：种子选择（PickSeeds）
     * - 遍历所有元素对 (i, j)
     * - 计算将 i 和 j 放入同一节点的"浪费面积"
     *   waste = combined_MBR(i, j).area() - i.area() - j.area()
     * - 选择浪费面积最大的一对作为两个种子
     * - 复杂度：O(n²)
     *
     * 步骤 2：元素分配（DistributeEntry）
     * - 重复直到所有元素分配完毕：
     *   a) 检查是否需要强制分配（保证 MINNODES）
     *   b) 对于每个未分配元素，计算加入两组的 MBR 增量差
     *   c) 选择差异最大的元素
     *   d) 将该元素分配到增量更小的组
     * - 复杂度：O(n²)
     *
     * 【总复杂度】O(n²)，其中 n = MAXNODES + 1
     *
     * @param node 待分裂的叶子节点
     * @param face 触发分裂的新插入元素
     * @param rect 新元素的包围盒
     * @return 新创建的节点（包含部分元素）
     */
    RTreeNode* splitLeafQuadratic(RTreeNode* node, Face* face, const BoundingBox2D& rect) {
        // ===== 准备工作：收集所有数据（原节点 + 新元素）=====
        Face* allData[RTreeNode::MAXNODES + 1];
        BoundingBox2D allRects[RTreeNode::MAXNODES + 1];

        for (int i = 0; i < node->count; i++) {
            allData[i] = node->data[i];
            allRects[i] = node->childMBR[i];
        }
        allData[node->count] = face;      // 添加触发分裂的新元素
        allRects[node->count] = rect;

        int total = node->count + 1;       // 总元素数 = MAXNODES + 1

        // ===== 步骤 1：种子选择（PickSeeds）=====
        // 目标：找到两个"最不应该放在一起"的元素作为两组的种子
        int seed1 = 0, seed2 = 1;
        double maxWaste = -numeric_limits<double>::max();

        for (int i = 0; i < total; i++) {
            for (int j = i + 1; j < total; j++) {
                // 计算将 i 和 j 放入同一节点时的"浪费面积"
                BoundingBox2D combined = allRects[i].unionWith(allRects[j]);
                double waste = combined.area() - allRects[i].area() - allRects[j].area();

                // 浪费面积越大，说明这两个元素越不适合放在一起
                if (waste > maxWaste) {
                    maxWaste = waste;
                    seed1 = i;
                    seed2 = j;
                }
            }
        }

        // ===== 初始化两个组 =====
        auto newNode = new RTreeNode(true);  // 新节点
        vector<bool> assigned(total, false); // 标记元素是否已分配

        // 将种子分配到两个组
        // 种子 1 留在原节点
        node->count = 0;
        node->data[node->count] = allData[seed1];
        node->childMBR[node->count] = allRects[seed1];
        node->count++;
        assigned[seed1] = true;

        // 种子 2 放入新节点
        newNode->data[newNode->count] = allData[seed2];
        newNode->childMBR[newNode->count] = allRects[seed2];
        newNode->count++;
        assigned[seed2] = true;

        // 记录两个组当前的 MBR（用于计算增量）
        BoundingBox2D mbr1 = allRects[seed1];
        BoundingBox2D mbr2 = allRects[seed2];

        // ===== 步骤 2：分配剩余元素（DistributeEntry）=====
        for (int remaining = total - 2; remaining > 0; remaining--) {

            // ----- 强制分配检查 -----
            // 确保两个节点都满足 MINNODES 要求
            if (node->count + remaining <= RTreeNode::MINNODES) {
                // 原节点需要所有剩余元素才能达到 MINNODES
                for (int i = 0; i < total; i++) {
                    if (!assigned[i]) {
                        node->data[node->count] = allData[i];
                        node->childMBR[node->count] = allRects[i];
                        node->count++;
                        assigned[i] = true;
                    }
                }
                break;
            }
            if (newNode->count + remaining <= RTreeNode::MINNODES) {
                // 新节点需要所有剩余元素才能达到 MINNODES
                for (int i = 0; i < total; i++) {
                    if (!assigned[i]) {
                        newNode->data[newNode->count] = allData[i];
                        newNode->childMBR[newNode->count] = allRects[i];
                        newNode->count++;
                        assigned[i] = true;
                    }
                }
                break;
            }

            // ----- 选择"偏好差异"最大的元素 -----
            // 偏好差异 = |加入组1的MBR增量 - 加入组2的MBR增量|
            int bestIdx = -1;
            double maxDiff = -numeric_limits<double>::max();

            for (int i = 0; i < total; i++) {
                if (assigned[i]) continue;

                double d1 = mbr1.enlargement(allRects[i]);  // 加入组1的增量
                double d2 = mbr2.enlargement(allRects[i]);  // 加入组2的增量
                double diff = abs(d1 - d2);

                if (diff > maxDiff) {
                    maxDiff = diff;
                    bestIdx = i;
                }
            }

            // ----- 将元素分配到"增量更小"的组 -----
            double d1 = mbr1.enlargement(allRects[bestIdx]);
            double d2 = mbr2.enlargement(allRects[bestIdx]);

            if (d1 < d2) {
                // 加入组1增量更小，分配到原节点
                node->data[node->count] = allData[bestIdx];
                node->childMBR[node->count] = allRects[bestIdx];
                node->count++;
                mbr1.expand(allRects[bestIdx]);
            } else {
                // 加入组2增量更小（或相等），分配到新节点
                newNode->data[newNode->count] = allData[bestIdx];
                newNode->childMBR[newNode->count] = allRects[bestIdx];
                newNode->count++;
                mbr2.expand(allRects[bestIdx]);
            }
            assigned[bestIdx] = true;
        }

        // ===== 更新两个节点的 MBR =====
        node->updateMBR();
        newNode->updateMBR();

        return newNode;  // 返回新创建的节点
    }

    /**
     * 二次分裂算法（内部节点）
     *
     * 【与叶子节点分裂的区别】
     * 1. 存储的是子节点指针（unique_ptr）而非数据指针
     * 2. 需要处理 unique_ptr 的所有权转移（std::move）
     * 3. 其余算法逻辑完全相同
     *
     * 【注意事项】
     * - 必须使用 std::move 转移 unique_ptr 所有权
     * - 原节点的 children 数组会被清空重建
     * - newChild 参数是裸指针，会被包装为 unique_ptr
     *
     * @param node 待分裂的内部节点
     * @param newChild 触发分裂的新子节点（来自下层分裂）
     * @return 新创建的内部节点
     */
    RTreeNode* splitInternalQuadratic(RTreeNode* node, RTreeNode* newChild) {
        // 收集所有子节点
        unique_ptr<RTreeNode> allChildren[RTreeNode::MAXNODES + 1];
        BoundingBox2D allRects[RTreeNode::MAXNODES + 1];

        for (int i = 0; i < node->count; i++) {
            allChildren[i] = std::move(node->children[i]);
            allRects[i] = node->childMBR[i];
        }
        allChildren[node->count].reset(newChild);
        allRects[node->count] = newChild->mbr;

        int total = node->count + 1;

        // 选择种子
        int seed1 = 0, seed2 = 1;
        double maxWaste = -numeric_limits<double>::max();

        for (int i = 0; i < total; i++) {
            for (int j = i + 1; j < total; j++) {
                BoundingBox2D combined = allRects[i].unionWith(allRects[j]);
                double waste = combined.area() - allRects[i].area() - allRects[j].area();
                if (waste > maxWaste) {
                    maxWaste = waste;
                    seed1 = i;
                    seed2 = j;
                }
            }
        }

        // 初始化两个组
        auto newNode = new RTreeNode(false);

        vector<bool> assigned(total, false);

        node->count = 0;
        node->children[node->count] = std::move(allChildren[seed1]);
        node->childMBR[node->count] = allRects[seed1];
        node->count++;
        assigned[seed1] = true;

        newNode->children[newNode->count] = std::move(allChildren[seed2]);
        newNode->childMBR[newNode->count] = allRects[seed2];
        newNode->count++;
        assigned[seed2] = true;

        BoundingBox2D mbr1 = allRects[seed1];
        BoundingBox2D mbr2 = allRects[seed2];

        // 分配剩余元素（与叶子节点类似）
        for (int remaining = total - 2; remaining > 0; remaining--) {
            if (node->count + remaining <= RTreeNode::MINNODES) {
                for (int i = 0; i < total; i++) {
                    if (!assigned[i]) {
                        node->children[node->count] = std::move(allChildren[i]);
                        node->childMBR[node->count] = allRects[i];
                        node->count++;
                        assigned[i] = true;
                    }
                }
                break;
            }
            if (newNode->count + remaining <= RTreeNode::MINNODES) {
                for (int i = 0; i < total; i++) {
                    if (!assigned[i]) {
                        newNode->children[newNode->count] = std::move(allChildren[i]);
                        newNode->childMBR[newNode->count] = allRects[i];
                        newNode->count++;
                        assigned[i] = true;
                    }
                }
                break;
            }

            int bestIdx = -1;
            double maxDiff = -numeric_limits<double>::max();

            for (int i = 0; i < total; i++) {
                if (assigned[i]) continue;

                double d1 = mbr1.enlargement(allRects[i]);
                double d2 = mbr2.enlargement(allRects[i]);
                double diff = abs(d1 - d2);

                if (diff > maxDiff) {
                    maxDiff = diff;
                    bestIdx = i;
                }
            }

            double d1 = mbr1.enlargement(allRects[bestIdx]);
            double d2 = mbr2.enlargement(allRects[bestIdx]);

            if (d1 < d2) {
                node->children[node->count] = std::move(allChildren[bestIdx]);
                node->childMBR[node->count] = allRects[bestIdx];
                node->count++;
                mbr1.expand(allRects[bestIdx]);
            } else {
                newNode->children[newNode->count] = std::move(allChildren[bestIdx]);
                newNode->childMBR[newNode->count] = allRects[bestIdx];
                newNode->count++;
                mbr2.expand(allRects[bestIdx]);
            }
            assigned[bestIdx] = true;
        }

        node->updateMBR();
        newNode->updateMBR();

        return newNode;
    }

    /**
     * 内部搜索（递归）
     */
    void searchInternal(RTreeNode* node, const BoundingBox2D& rect, vector<Face*>& results) {
        for (int i = 0; i < node->count; i++) {
            if (node->childMBR[i].intersects(rect)) {
                if (node->isLeaf) {
                    results.push_back(node->data[i]);
                } else {
                    searchInternal(node->children[i].get(), rect, results);
                }
            }
        }
    }

    /**
     * 直线查询（内部递归 - 直接测试直线与 MBR 的相交）
     *
     * 【算法原理】
     * R-Tree 直线查询的核心是 MBR 过滤：
     * 1. 对于每个节点的条目，测试直线是否与其 MBR 相交
     * 2. 如果不相交，跳过该子树（剪枝）
     * 3. 如果相交：
     *    - 叶子节点：将数据加入候选集
     *    - 内部节点：递归查询子节点
     *
     * 【优化点】
     * 使用 lineIntersectsBox 直接测试直线与 MBR 的相交，
     * 而不是将直线包围盒与 MBR 做矩形相交测试。
     * 这样可以更精确地过滤掉不相交的分支。
     *
     * 【复杂度】
     * - 最坏情况：O(n)，所有 MBR 都与直线相交
     * - 实际情况：远优于此，取决于数据分布和直线方向
     *
     * @param node 当前节点
     * @param p1 直线起点
     * @param p2 直线终点
     * @param results 输出参数，存储候选面
     */
    void lineQueryInternal(RTreeNode* node, const Point2D& p1, const Point2D& p2,
                           vector<Face*>& results) {
        // 遍历节点的所有条目
        for (int i = 0; i < node->count; i++) {
            // 直接测试直线与条目 MBR 的相交
            if (lineIntersectsBox(p1, p2, node->childMBR[i])) {
                if (node->isLeaf) {
                    // 叶子节点：将数据加入候选集
                    results.push_back(node->data[i]);
                } else {
                    // 内部节点：递归查询子节点
                    lineQueryInternal(node->children[i].get(), p1, p2, results);
                }
            }
            // 如果 MBR 不与直线相交，跳过该子树（剪枝）
        }
    }

    /**
     * 判断线段是否与多边形相交
     */
    bool lineIntersectsPolygon(const Point2D& p1, const Point2D& p2, Face* face) {
        const auto& bb = face->boundingBox;
        if (!lineIntersectsBox(p1, p2, bb)) return false;

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
};

// ============================================================================
// 第三部分：平面分割类
// ============================================================================

class PlanarSubdivision {
private:
    vector<unique_ptr<Vertex>> vertices;
    vector<unique_ptr<HalfEdge>> halfEdges;
    vector<unique_ptr<Face>> faces;

    RTree rtree;

public:
    ~PlanarSubdivision() = default;

    Face* addFace(const vector<Point2D>& polygon) {
        int n = static_cast<int>(polygon.size());
        if (n < 3) return nullptr;

        auto face = make_unique<Face>();
        face->id = static_cast<int>(faces.size());

        vector<Vertex*> faceVertices(n);
        for (int i = 0; i < n; i++) {
            auto v = make_unique<Vertex>(polygon[i], static_cast<int>(vertices.size()));
            faceVertices[i] = v.get();
            vertices.push_back(std::move(v));
        }

        vector<HalfEdge*> faceEdges(n);
        for (int i = 0; i < n; i++) {
            auto he = make_unique<HalfEdge>(faceVertices[i], static_cast<int>(halfEdges.size()));
            he->incidentFace = face.get();
            faceVertices[i]->incidentEdge = he.get();
            faceEdges[i] = he.get();
            halfEdges.push_back(std::move(he));
        }

        for (int i = 0; i < n; i++) {
            faceEdges[i]->next = faceEdges[(i + 1) % n];
            faceEdges[i]->prev = faceEdges[(i - 1 + n) % n];
        }

        face->outerComponent = faceEdges[0];
        face->computeBoundingBox();

        Face* facePtr = face.get();
        faces.push_back(std::move(face));

        // 插入 R-Tree
        rtree.insert(facePtr);

        return facePtr;
    }

    vector<Face*> queryLineIntersection(const Point2D& p1, const Point2D& p2) {
        return rtree.lineQuery(p1, p2);
    }

    vector<Face*> bruteForceQuery(const Point2D& p1, const Point2D& p2) {
        vector<Face*> result;
        for (const auto& f : faces) {
            HalfEdge* he = f->outerComponent;
            bool intersects = false;

            do {
                Point2D a = he->origin->position;
                Point2D b = he->next->origin->position;

                if (segmentsIntersect(p1, p2, a, b)) {
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

    void printStatistics() const {
        cout << "\n=== 数据结构统计 ===" << endl;
        cout << "顶点数: " << vertices.size() << endl;
        cout << "半边数: " << halfEdges.size() << endl;
        cout << "面数: " << faces.size() << endl;
        cout << "R-Tree 项数: " << rtree.size() << endl;
    }
};

// ============================================================================
// 第四部分：测试程序
// ============================================================================

void createTestMesh(PlanarSubdivision& subdivision) {
    cout << "=== 创建测试三角网格 ===" << endl;

    vector<vector<Point2D>> triangles = {
        {{0, 1}, {0, 2}, {1, 2}},
        {{0, 1}, {1, 2}, {1, 1}},
        {{1, 1}, {1, 2}, {2, 2}},
        {{1, 1}, {2, 2}, {2, 1}},
        {{0, 0}, {0, 1}, {1, 1}},
        {{0, 0}, {1, 1}, {1, 0}},
        {{1, 0}, {1, 1}, {2, 1}},
        {{1, 0}, {2, 1}, {2, 0}}
    };

    for (const auto& tri : triangles) {
        subdivision.addFace(tri);
    }

    cout << "创建了 " << subdivision.numFaces() << " 个三角形面" << endl;
}

void runQueryTests(PlanarSubdivision& subdivision) {
    cout << "\n=== 运行直线相交查询测试（R-Tree 版本）===" << endl;

    struct TestCase {
        Point2D p1, p2;
        const char* name;
    };

    TestCase tests[] = {
        {{0.2, 0.2}, {1.8, 1.8}, "对角线查询"},
        {{0.0, 1.0}, {2.0, 1.0}, "水平线查询"},
        {{1.0, 0.0}, {1.0, 2.0}, "垂直线查询"},
        {{0.3, 0.3}, {0.4, 0.4}, "短线段查询"}
    };

    for (int i = 0; i < 4; i++) {
        cout << "\n测试 " << (i + 1) << ": " << tests[i].name << endl;
        cout << "直线: " << tests[i].p1 << " -> " << tests[i].p2 << endl;

        auto start = chrono::high_resolution_clock::now();
        vector<Face*> result = subdivision.queryLineIntersection(tests[i].p1, tests[i].p2);
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

    for (int j = 0; j < gridSize; j++) {
        for (int i = 0; i < gridSize; i++) {
            double x0 = i, y0 = j;
            double x1 = i + 1, y1 = j + 1;

            subdivision.addFace({{x0, y0}, {x0, y1}, {x1, y1}});
            subdivision.addFace({{x0, y0}, {x1, y1}, {x1, y0}});
        }
    }

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

        vector<Face*> result = subdivision.queryLineIntersection(Point2D(x1, y1), Point2D(x2, y2));
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
    cout << "  解法三（优化版）：R-Tree 版本" << endl;
    cout << "========================================================" << endl;

    PlanarSubdivision subdivision;

    createTestMesh(subdivision);
    subdivision.printStatistics();
    runQueryTests(subdivision);
    runPerformanceTest();

    cout << "\n=== 程序结束 ===" << endl;

    return 0;
}
