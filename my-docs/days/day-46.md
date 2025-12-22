# Day 46：网格质量度量

## 学习目标
掌握网格质量评估的各种指标，理解有限元分析对网格质量的要求，学习Gmsh中的质量度量实现。

## 时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-10:00 | 1h | 学习常用网格质量指标定义 |
| 10:00-11:00 | 1h | 阅读qualityMeasures.h/cpp |
| 11:00-12:00 | 1h | 理解Jacobian和条件数的关系 |
| 14:00-15:00 | 1h | 了解不同物理问题对网格质量的要求 |

---

## 上午学习内容

### 1. 网格质量概述

**为什么网格质量重要**：
- 影响有限元分析的精度和收敛性
- 影响求解器的数值稳定性
- 影响计算效率

**质量差的网格表现**：
```
好的三角形          差的三角形（瘦长）      退化三角形
    △                    /\                    /
   /|\                  /  \                  /_______
  / | \                /    \
 /__|__\              /______\
```

### 2. 常用质量指标

| 指标 | 公式 | 理想值 | 说明 |
|------|------|--------|------|
| 形状比 (Aspect Ratio) | 最长边/最短边 | 1.0 | 越小越好 |
| 最小角 | min(θ₁, θ₂, θ₃) | 60° | 越大越好 |
| 面积比 | A / A_ideal | 1.0 | 接近1为好 |
| SICN | 见下文 | 1.0 | 越大越好 |
| Gamma | 见下文 | 1.0 | 越大越好 |

### 3. SICN - Scaled Jacobian

**定义**：
对于三角形，SICN是归一化的Jacobian值。

```cpp
// 三角形SICN计算
double triangleSICN(const MTriangle *t) {
    // 获取顶点坐标
    SPoint3 p0 = t->getVertex(0)->point();
    SPoint3 p1 = t->getVertex(1)->point();
    SPoint3 p2 = t->getVertex(2)->point();

    // 边向量
    SVector3 e0 = p1 - p0;
    SVector3 e1 = p2 - p0;
    SVector3 e2 = p2 - p1;

    // 面积（2倍）
    SVector3 normal = crossprod(e0, e1);
    double area2 = normal.norm();

    // 理想等边三角形的面积（以最长边为基准）
    double maxEdge = std::max({e0.norm(), e1.norm(), e2.norm()});
    double idealArea2 = sqrt(3.0) / 2.0 * maxEdge * maxEdge;

    // SICN = actual_area / ideal_area
    return area2 / idealArea2;
}
```

### 4. Gamma质量指标

**定义**：
```
γ = (4 * √3 * A) / (l₁² + l₂² + l₃²)

其中：
- A 是三角形面积
- l₁, l₂, l₃ 是三条边长

等边三角形时 γ = 1
退化三角形时 γ → 0
```

```cpp
double triangleGamma(const MTriangle *t) {
    SPoint3 p0 = t->getVertex(0)->point();
    SPoint3 p1 = t->getVertex(1)->point();
    SPoint3 p2 = t->getVertex(2)->point();

    double l0 = p0.distance(p1);
    double l1 = p1.distance(p2);
    double l2 = p2.distance(p0);

    // 面积（海伦公式）
    double s = (l0 + l1 + l2) / 2.0;
    double area = sqrt(s * (s-l0) * (s-l1) * (s-l2));

    // Gamma
    double sumL2 = l0*l0 + l1*l1 + l2*l2;
    return 4.0 * sqrt(3.0) * area / sumL2;
}
```

### 5. 四面体质量指标

```cpp
// 四面体SICN
double tetrahedronSICN(const MTetrahedron *tet) {
    // Jacobian矩阵
    // J = [x1-x0, x2-x0, x3-x0]
    //     [y1-y0, y2-y0, y3-y0]
    //     [z1-z0, z2-z0, z3-z0]

    SPoint3 p0 = tet->getVertex(0)->point();
    SPoint3 p1 = tet->getVertex(1)->point();
    SPoint3 p2 = tet->getVertex(2)->point();
    SPoint3 p3 = tet->getVertex(3)->point();

    SVector3 e0 = p1 - p0;
    SVector3 e1 = p2 - p0;
    SVector3 e2 = p3 - p0;

    // 体积（6倍）
    double vol6 = dot(e0, crossprod(e1, e2));

    // 归一化
    double maxEdge = 0;
    // 计算所有6条边的最大长度...

    double idealVol6 = /* 基于maxEdge的理想四面体体积 */;

    return vol6 / idealVol6;
}
```

---

## 下午学习内容

### 6. qualityMeasures.h/cpp 源码分析

**文件位置**：`src/mesh/qualityMeasures.h`

```cpp
// 主要质量计算函数

namespace qualityMeasures {

// 三角形质量
double qmTriangle(MTriangle *t, int measure);

// 四面体质量
double qmTetrahedron(MTetrahedron *t, int measure);

// 质量类型枚举
enum QualityMeasure {
    QM_SICN = 0,           // Scaled Jacobian (Inscribed Circle Normalized)
    QM_SICN_MIN = 1,       // 最小SICN
    QM_SIGE = 2,           // Scaled Jacobian (Gradient Error)
    QM_GAMMA = 3,          // Gamma
    QM_MINANGLE = 4,       // 最小角
    QM_MAXANGLE = 5,       // 最大角
    QM_ASPECTRATIO = 6,    // 形状比
    QM_VOLUME = 7          // 体积/面积
};

// 获取单元质量
double getQuality(MElement *e, int measure);

// 质量统计
void computeStatistics(GEntity *ge, int measure,
                       double &min, double &max, double &avg);

}  // namespace qualityMeasures
```

### 7. Jacobian与有限元

**Jacobian的意义**：
```
在有限元分析中，Jacobian矩阵将参考单元映射到实际单元。

参考单元 (ξ, η) → 实际单元 (x, y)

J = | ∂x/∂ξ  ∂x/∂η |
    | ∂y/∂ξ  ∂y/∂η |

det(J) 表示面积/体积的放大比例
- det(J) > 0: 有效单元
- det(J) = 0: 退化单元
- det(J) < 0: 翻转单元（无效）
```

```cpp
// 计算Jacobian
double MElement::getJacobian(double u, double v, double w,
                             fullMatrix<double> &jac) const {
    // 获取形函数梯度
    const nodalBasis *nb = getNodalBasis();
    fullMatrix<double> gradShape;
    nb->df(u, v, w, gradShape);

    // 初始化Jacobian矩阵
    jac.setAll(0);

    // J = sum_i (gradShape_i * x_i)
    for (int i = 0; i < getNumVertices(); i++) {
        MVertex *v = getVertex(i);
        for (int j = 0; j < 3; j++) {
            jac(0, j) += gradShape(i, 0) * v->x();
            jac(1, j) += gradShape(i, 1) * v->y();
            jac(2, j) += gradShape(i, 2) * v->z();
        }
    }

    return det(jac);
}
```

### 8. 条件数

**定义**：
矩阵A的条件数 κ(A) = ||A|| * ||A⁻¹||

**在网格质量中的应用**：
```cpp
// 条件数计算
double conditionNumber(const fullMatrix<double> &J) {
    // 奇异值分解 J = U * S * V^T
    std::vector<double> sigma;
    svd(J, sigma);

    // 条件数 = 最大奇异值 / 最小奇异值
    double sigmaMax = *std::max_element(sigma.begin(), sigma.end());
    double sigmaMin = *std::min_element(sigma.begin(), sigma.end());

    if (sigmaMin < 1e-15) {
        return 1e30;  // 退化
    }

    return sigmaMax / sigmaMin;
}

// 理想单元条件数 = 1
// 条件数越大，单元越差
```

### 9. 不同物理问题的质量要求

| 物理问题 | 关键质量指标 | 要求 |
|----------|--------------|------|
| 结构力学 | 形状比 | < 3-5 |
| 流体力学 | 正交性、边界层分辨率 | 边界层高宽比可高 |
| 热传导 | 各向同性 | γ > 0.3 |
| 电磁场 | 最小角 | > 15° |

### 10. 质量可视化

```geo
// 在Gmsh中可视化网格质量
// 创建质量视图

// 方法1：使用Plugin
Plugin(AnalyseMeshQuality).JacobianDeterminant = 1;
Plugin(AnalyseMeshQuality).Run;

// 方法2：使用命令行
// gmsh mesh.msh -e -o quality.msh
// 会输出质量统计信息
```

```cpp
// 创建质量视图的代码
PView *createQualityView(GModel *model, int measure) {
    PView *v = new PView("Mesh Quality");
    PViewDataGModel *data = new PViewDataGModel(PViewDataGModel::ElementData);

    for (auto it = model->firstFace(); it != model->lastFace(); ++it) {
        GFace *gf = *it;
        for (MTriangle *t : gf->triangles) {
            double q = qualityMeasures::qmTriangle(t, measure);
            data->addData(t, q);
        }
    }

    v->setData(data);
    return v;
}
```

---

## 练习作业

### 练习1：质量统计（基础）
```cpp
// 实现网格质量统计函数
struct QualityStats {
    double min;
    double max;
    double avg;
    int numBad;     // 质量低于阈值的单元数
    int numTotal;
};

QualityStats computeTriangleQualityStats(GFace *gf,
                                          double badThreshold = 0.3) {
    QualityStats stats = {1e30, -1e30, 0, 0, 0};

    for (MTriangle *t : gf->triangles) {
        double q = /* TODO: 计算质量 */;

        stats.min = std::min(stats.min, q);
        stats.max = std::max(stats.max, q);
        stats.avg += q;
        stats.numTotal++;

        if (q < badThreshold) {
            stats.numBad++;
        }
    }

    stats.avg /= stats.numTotal;
    return stats;
}
```

### 练习2：质量直方图（进阶）
```cpp
// 创建质量直方图
class QualityHistogram {
    std::vector<int> bins;
    double minQ, maxQ;
    int numBins;

public:
    QualityHistogram(int nbins = 10) : numBins(nbins) {
        bins.resize(nbins, 0);
        minQ = 0.0;
        maxQ = 1.0;
    }

    void add(double quality) {
        int bin = (int)((quality - minQ) / (maxQ - minQ) * numBins);
        bin = std::min(std::max(bin, 0), numBins - 1);
        bins[bin]++;
    }

    void print() {
        // TODO: 打印ASCII直方图
        // 示例输出：
        // [0.0-0.1]: ███ (15)
        // [0.1-0.2]: █████ (25)
        // ...
    }
};
```

### 练习3：差单元定位（挑战）
```cpp
// 找出并输出所有质量差的单元
void exportBadElements(GModel *model, double threshold,
                       const std::string &filename) {
    // TODO:
    // 1. 遍历所有单元
    // 2. 计算质量
    // 3. 将质量低于threshold的单元输出到.geo文件
    // 4. 可以用不同颜色标记不同质量范围

    std::ofstream file(filename);
    file << "// Bad elements (quality < " << threshold << ")\n";

    // 输出点和单元...
}
```

---

## 源码导航

### 核心文件
| 文件 | 内容 | 行数 |
|------|------|------|
| `src/mesh/qualityMeasures.h` | 质量度量头文件 | ~50行 |
| `src/mesh/qualityMeasures.cpp` | 质量度量实现 | ~600行 |
| `src/mesh/meshGFaceOptimize.cpp` | 质量优化 | ~2000行 |

### 关键函数
```cpp
// qualityMeasures.cpp
double qmTriangle(MTriangle *t, int measure);
double qmTetrahedron(MTetrahedron *t, int measure);

// MElement.h
double MElement::getJacobian(...);
double MElement::minSICNShapeMeasure();

// 辅助函数
double computeAspectRatio(MTriangle *t);
double computeMinAngle(MTriangle *t);
double computeGamma(MTriangle *t);
```

### 质量度量常量
```cpp
// 各种度量的理想值
const double IDEAL_SICN = 1.0;
const double IDEAL_GAMMA = 1.0;
const double IDEAL_ASPECT_RATIO = 1.0;  // 等边
const double IDEAL_MIN_ANGLE = 60.0;    // 等边三角形
```

---

## 今日检查点

- [ ] 理解SICN质量指标的含义和计算方法
- [ ] 理解Gamma质量指标的定义
- [ ] 能解释Jacobian在有限元中的作用
- [ ] 了解条件数与网格质量的关系
- [ ] 知道不同物理问题对网格质量的不同要求

---

## 扩展阅读

### 参考文献
1. Knupp, "Algebraic Mesh Quality Metrics"
2. Shewchuk, "What Is a Good Linear Finite Element?"
3. Frey & George, "Mesh Generation: Application to Finite Elements"

### 工具
- Gmsh内置：`Plugin(AnalyseMeshQuality)`
- ParaView质量过滤器
- VisIt网格质量模块

---

## 导航

- 上一天：[Day 45 - 几何谓词与数值稳定性](day-45.md)
- 下一天：[Day 47 - 网格优化算法](day-47.md)
- 返回：[学习计划索引](../STUDY-INDEX.md)
