# Day 48：Netgen集成分析

## 学习目标
理解Gmsh如何集成外部网格生成器（以Netgen为例），学习接口设计和数据交换模式。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 了解Netgen算法特点和适用场景 |
| 10:00-11:00 | 1h | 阅读meshGRegionNetgen.cpp接口代码 |
| 11:00-12:00 | 1h | 理解数据格式转换和参数传递 |
| 14:00-15:00 | 1h | 对比不同网格生成器的特点 |

---

## 上午学习内容

### 1. Netgen简介

**Netgen是什么**：
- 开源的3D网格生成器
- 采用前沿推进法（Advancing Front Method）
- 特别擅长处理复杂几何
- 有独立的CAD内核（OpenCASCADE集成）

**核心特点**：
| 特性 | 说明 |
|------|------|
| 算法 | 前沿推进 + 局部Delaunay |
| 输入 | 表面网格（STL/表面三角化） |
| 输出 | 四面体网格 |
| 优势 | 处理复杂几何、薄壁结构 |

### 2. Netgen算法原理

**前沿推进法概述**：
```
1. 从边界表面开始（初始前沿）
2. 选择一个前沿三角形
3. 找到合适的第四个点形成四面体
   - 理想点计算
   - 现有点查找
   - 新点创建
4. 更新前沿（删除旧面，添加新面）
5. 重复直到前沿闭合
```

```
初始状态：        步骤1：         步骤2：
  前沿面            添加点           继续填充
  _____           _____           _____
 |     |         |\ ⋅ /|         |\ ⋅ /|
 |     |   →     | \ / |   →     |\⋅ ⋅/|
 |_____|         |__⋅__|         |_⋅⋅⋅_|
   空腔           形成四面体        更多四面体
```

### 3. Gmsh-Netgen接口

**文件位置**：`src/mesh/meshGRegionNetgen.cpp`

```cpp
// Netgen集成的主入口
bool meshGRegionNetgen(GRegion *gr) {
    // 1. 准备Netgen输入
    NgMesh *ngMesh = prepareNetgenInput(gr);

    // 2. 调用Netgen
    NgResult result = generateMesh(ngMesh);

    // 3. 转换输出
    if (result.success) {
        convertNetgenOutput(ngMesh, gr);
    }

    // 4. 清理
    delete ngMesh;

    return result.success;
}
```

### 4. 数据转换 - 输入准备

**从GRegion到Netgen格式**：
```cpp
NgMesh* prepareNetgenInput(GRegion *gr) {
    NgMesh *ngMesh = new NgMesh();

    // 收集边界面
    std::vector<GFace*> faces = gr->faces();

    // 添加所有表面三角形
    for (GFace *gf : faces) {
        for (MTriangle *t : gf->triangles) {
            // 获取顶点
            MVertex *v0 = t->getVertex(0);
            MVertex *v1 = t->getVertex(1);
            MVertex *v2 = t->getVertex(2);

            // 添加到Netgen
            int idx0 = ngMesh->addPoint(v0->x(), v0->y(), v0->z());
            int idx1 = ngMesh->addPoint(v1->x(), v1->y(), v1->z());
            int idx2 = ngMesh->addPoint(v2->x(), v2->y(), v2->z());

            // 注意法向量方向
            int orientation = (gf->getNativeOrientation() > 0) ? 1 : -1;
            if (orientation > 0) {
                ngMesh->addSurfaceElement(idx0, idx1, idx2);
            } else {
                ngMesh->addSurfaceElement(idx0, idx2, idx1);
            }
        }
    }

    return ngMesh;
}
```

### 5. 顶点映射

**Gmsh顶点 ↔ Netgen顶点索引**：
```cpp
class VertexMapping {
    std::map<MVertex*, int> gmshToNetgen;
    std::map<int, MVertex*> netgenToGmsh;

public:
    int getNetgenIndex(MVertex *v) {
        auto it = gmshToNetgen.find(v);
        if (it != gmshToNetgen.end()) {
            return it->second;
        }
        return -1;  // 不存在
    }

    MVertex* getGmshVertex(int ngIdx) {
        auto it = netgenToGmsh.find(ngIdx);
        if (it != netgenToGmsh.end()) {
            return it->second;
        }
        return nullptr;
    }

    void addMapping(MVertex *v, int ngIdx) {
        gmshToNetgen[v] = ngIdx;
        netgenToGmsh[ngIdx] = v;
    }
};
```

---

## 下午学习内容

### 6. 参数传递

**Netgen参数设置**：
```cpp
void setNetgenParameters(NgMesh *ngMesh, GRegion *gr) {
    // 从Gmsh选项读取参数
    double maxh = CTX::instance()->mesh.lcMax;
    double minh = CTX::instance()->mesh.lcMin;
    double grading = CTX::instance()->mesh.gradingFactor;

    // 设置Netgen参数
    NgMeshingParameters mp;
    mp.maxh = maxh;
    mp.minh = minh;
    mp.grading = grading;
    mp.optsteps2d = 3;    // 2D优化步数
    mp.optsteps3d = 3;    // 3D优化步数

    // Netgen特有选项
    mp.secondorder = (CTX::instance()->mesh.order > 1);
    mp.optimize3d = true;
    mp.closeedgeenable = true;
    mp.closeedgefact = 2.0;

    ngMesh->setMeshingParameters(mp);
}
```

### 7. 网格生成调用

```cpp
NgResult generateMesh(NgMesh *ngMesh) {
    NgResult result;

    try {
        // 调用Netgen网格生成
        int status = ngMesh->GenerateMesh();

        if (status == 0) {
            result.success = true;
            result.numTets = ngMesh->getNumVolElements();
            result.numPoints = ngMesh->getNumPoints();
        } else {
            result.success = false;
            result.errorMsg = ngMesh->getErrorMessage();
        }
    }
    catch (const std::exception &e) {
        result.success = false;
        result.errorMsg = e.what();
    }

    return result;
}
```

### 8. 数据转换 - 输出处理

**从Netgen格式到GRegion**：
```cpp
void convertNetgenOutput(NgMesh *ngMesh, GRegion *gr,
                         VertexMapping &mapping) {
    // 1. 添加新的内部顶点
    int numSurfacePoints = mapping.size();  // 表面点数量
    int numTotalPoints = ngMesh->getNumPoints();

    for (int i = numSurfacePoints; i < numTotalPoints; i++) {
        double x, y, z;
        ngMesh->getPoint(i, x, y, z);

        MVertex *v = new MVertex(x, y, z, gr);
        gr->mesh_vertices.push_back(v);
        mapping.addMapping(v, i);
    }

    // 2. 创建四面体
    int numTets = ngMesh->getNumVolElements();
    for (int i = 0; i < numTets; i++) {
        int v0, v1, v2, v3;
        ngMesh->getVolElement(i, v0, v1, v2, v3);

        MVertex *mv0 = mapping.getGmshVertex(v0);
        MVertex *mv1 = mapping.getGmshVertex(v1);
        MVertex *mv2 = mapping.getGmshVertex(v2);
        MVertex *mv3 = mapping.getGmshVertex(v3);

        MTetrahedron *tet = new MTetrahedron(mv0, mv1, mv2, mv3);
        gr->tetrahedra.push_back(tet);
    }
}
```

### 9. 错误处理

```cpp
void handleNetgenError(GRegion *gr, const NgResult &result) {
    if (!result.success) {
        Msg::Error("Netgen meshing failed for region %d: %s",
                   gr->tag(), result.errorMsg.c_str());

        // 尝试诊断问题
        if (result.errorMsg.find("surface") != std::string::npos) {
            Msg::Info("Tip: Check surface mesh quality and orientation");

            // 检查表面网格
            for (GFace *gf : gr->faces()) {
                double minQ = 1e30;
                for (MTriangle *t : gf->triangles) {
                    double q = t->minSICNShapeMeasure();
                    minQ = std::min(minQ, q);
                }
                if (minQ < 0.1) {
                    Msg::Warning("Surface %d has poor quality triangles "
                                "(min SICN = %g)", gf->tag(), minQ);
                }
            }
        }

        if (result.errorMsg.find("closed") != std::string::npos) {
            Msg::Info("Tip: Surface mesh may not be watertight");
        }
    }
}
```

### 10. 与其他生成器对比

**Gmsh支持的3D网格生成器**：

| 生成器 | 算法 | 优势 | 劣势 |
|--------|------|------|------|
| Delaunay | 增量Delaunay | 快速，理论保证 | 复杂几何可能失败 |
| Netgen | 前沿推进 | 复杂几何 | 较慢 |
| HXT | 并行Delaunay | 大规模网格 | 需要良好输入 |
| TetGen | 约束Delaunay | 精确边界 | 单线程 |

### 11. HXT集成

**HXT简介**：
- Gmsh开发的高性能四面体网格生成器
- 利用并行计算
- 适合大规模网格

```cpp
// src/mesh/meshGRegionHxt.cpp
bool meshGRegionHxt(GRegion *gr) {
    // HXT接口类似Netgen
    // 主要区别在于并行支持

    HXTMesh *hxtMesh = prepareHxtInput(gr);

    // 设置并行参数
    HXTContext ctx;
    ctx.numThreads = CTX::instance()->numThreads;

    HXTStatus status = hxtMesh->generate(&ctx);

    if (status == HXT_STATUS_OK) {
        convertHxtOutput(hxtMesh, gr);
    }

    return status == HXT_STATUS_OK;
}
```

---

## 练习作业

### 练习1：理解接口设计（基础）
```cpp
// 分析以下接口设计的优缺点
class MeshGenerator {
public:
    virtual bool generateMesh(GRegion *gr) = 0;
    virtual std::string name() const = 0;
    virtual ~MeshGenerator() {}
};

class NetgenGenerator : public MeshGenerator {
public:
    bool generateMesh(GRegion *gr) override;
    std::string name() const override { return "Netgen"; }
};

class DelaunayGenerator : public MeshGenerator {
public:
    bool generateMesh(GRegion *gr) override;
    std::string name() const override { return "Delaunay"; }
};

// 问题：
// 1. 这种设计的优点是什么？
// 2. 如何添加新的生成器？
// 3. 如何实现生成器的自动选择？
```

### 练习2：数据转换实现（进阶）
```cpp
// 实现简化版的STL到内部格式转换
class STLToMesh {
public:
    struct STLTriangle {
        float normal[3];
        float v0[3], v1[3], v2[3];
    };

    // 读取STL文件并创建表面网格
    bool convert(const std::string &filename, GFace *gf) {
        std::vector<STLTriangle> stlTriangles;

        // TODO: 读取STL文件

        // TODO: 顶点去重
        // 相同位置的顶点应该合并

        // TODO: 创建MVertex和MTriangle

        return true;
    }

private:
    // 顶点去重辅助
    std::map<std::tuple<float,float,float>, MVertex*> vertexMap;
};
```

### 练习3：生成器对比测试（挑战）
```cpp
// 对比不同生成器的性能
class GeneratorBenchmark {
public:
    struct BenchResult {
        std::string generatorName;
        double time;
        int numTets;
        double minQuality;
        double avgQuality;
        bool success;
    };

    std::vector<BenchResult> runBenchmark(GRegion *gr) {
        std::vector<BenchResult> results;

        // TODO: 依次测试不同生成器
        // 1. Delaunay
        // 2. Netgen (如果可用)
        // 3. HXT (如果可用)

        // TODO: 记录时间、质量统计

        return results;
    }

    void printResults(const std::vector<BenchResult> &results) {
        // TODO: 打印对比表格
    }
};
```

---

## 源码导航

### 核心文件
| 文件 | 内容 | 行数 |
|------|------|------|
| `src/mesh/meshGRegionNetgen.cpp` | Netgen接口 | ~800行 |
| `src/mesh/meshGRegionHxt.cpp` | HXT接口 | ~1000行 |
| `src/mesh/meshGRegion.cpp` | 3D网格入口 | ~600行 |
| `contrib/Netgen/` | Netgen源码 | ~50000行 |

### 关键函数
```cpp
// meshGRegionNetgen.cpp
bool meshGRegionNetgen(GRegion *gr);
void TransferVolumeMesh(nglib::Ng_Mesh *ngmesh, GRegion *gr);

// meshGRegionHxt.cpp
bool meshGRegionHxt(GRegion *gr);

// meshGRegion.cpp
void meshGRegion(GRegion *gr);  // 入口函数，选择生成器
```

### 选项影响
```cpp
// 3D网格算法选择
// Mesh.Algorithm3D:
//   1 = Delaunay
//   4 = Frontal (Netgen)
//   7 = MMG3D
//   9 = R-tree
//  10 = HXT

int algo = CTX::instance()->mesh.algo3d;
switch (algo) {
    case 1: meshGRegionDelaunay(gr); break;
    case 4: meshGRegionNetgen(gr); break;
    case 10: meshGRegionHxt(gr); break;
    // ...
}
```

---

## 今日检查点

- [ ] 了解Netgen的算法特点（前沿推进法）
- [ ] 理解Gmsh如何封装外部生成器
- [ ] 能描述数据从Gmsh到Netgen再回来的转换流程
- [ ] 知道不同3D生成器的适用场景
- [ ] 理解错误处理和诊断方法

---

## 生成器选择指南

| 场景 | 推荐生成器 | 原因 |
|------|------------|------|
| 简单几何，快速网格 | Delaunay | 最快，质量好 |
| 复杂CAD几何 | Netgen | 鲁棒性强 |
| 大规模网格（>1M） | HXT | 并行加速 |
| 需要精确边界 | TetGen | 约束Delaunay |
| 自适应细化 | BAMG | 各向异性支持 |

---

## 导航

- 上一天：[Day 47 - 网格优化算法](day-47.md)
- 下一天：[Day 49 - 第七周复习](day-49.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
