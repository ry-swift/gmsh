# Day 26：网格质量度量

**所属周次**：第4周 - 网格数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解网格质量评估方法

---

## 学习文件

- `src/geo/MElement.cpp`（质量计算部分）
- `src/mesh/qualityMeasures.h`

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 理解质量度量的概念 |
| 09:45-10:30 | 45min | 学习三角形质量度量 |
| 10:30-11:15 | 45min | 学习四面体质量度量 |
| 11:15-12:00 | 45min | 阅读qualityMeasures代码 |
| 14:00-14:45 | 45min | 理解雅可比质量 |
| 14:45-15:30 | 45min | 学习质量统计和可视化 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 质量度量概念

网格质量影响有限元分析的精度和收敛性。

```
好的网格元素特征：
┌────────────────────────────────────────────────────┐
│ • 形状接近正则（等边三角形、正四面体）             │
│ • 没有过大或过小的角度                             │
│ • 边长比例适中                                     │
│ • 没有翻转或退化                                   │
└────────────────────────────────────────────────────┘

差的网格元素：
┌────────────────────────────────────────────────────┐
│ • 细长的三角形（sliver）                           │
│ • 扁平的四面体                                     │
│ • 极端的角度（接近0°或180°）                       │
│ • 翻转的元素（负雅可比）                           │
└────────────────────────────────────────────────────┘
```

### 三角形质量度量

```cpp
// 1. Gamma质量（形状度量）
// γ = 4√3 * Area / (Σ edge²)
// 范围 [0, 1]，1表示等边三角形
double gammaTriangle(MVertex *v0, MVertex *v1, MVertex *v2) {
  double a = distance(v0, v1);
  double b = distance(v1, v2);
  double c = distance(v2, v0);
  double s = 0.5 * (a + b + c);
  double area = sqrt(s * (s-a) * (s-b) * (s-c));  // 海伦公式
  double sumSq = a*a + b*b + c*c;
  return 4.0 * sqrt(3.0) * area / sumSq;
}

// 2. 内外接圆比
// ρ = 2 * r_in / r_circ
// 范围 [0, 1]，1表示等边三角形
double rhoTriangle(MVertex *v0, MVertex *v1, MVertex *v2) {
  double a = distance(v0, v1);
  double b = distance(v1, v2);
  double c = distance(v2, v0);
  double s = 0.5 * (a + b + c);
  double area = sqrt(s * (s-a) * (s-b) * (s-c));
  double r_in = area / s;                    // 内切圆半径
  double r_circ = a * b * c / (4.0 * area);  // 外接圆半径
  return 2.0 * r_in / r_circ;
}

// 3. 最小角度
// 范围 [0°, 60°]，60°表示等边三角形
double minAngleTriangle(MVertex *v0, MVertex *v1, MVertex *v2);

// 4. 边长比
// aspectRatio = max_edge / min_edge
// 范围 [1, ∞)，1表示等边三角形
double aspectRatioTriangle(MVertex *v0, MVertex *v1, MVertex *v2);
```

### 四面体质量度量

```cpp
// 1. Gamma质量
// γ = 12 * (3V)^(2/3) / (Σ face_area)
// 范围 [0, 1]，1表示正四面体
double gammaTetrahedron(MVertex *v0, MVertex *v1, MVertex *v2, MVertex *v3);

// 2. 内外接球比
// ρ = 3 * r_in / r_circ
// 范围 [0, 1]，1表示正四面体
double rhoTetrahedron(MVertex *v0, MVertex *v1, MVertex *v2, MVertex *v3) {
  double volume = volumeTetrahedron(v0, v1, v2, v3);
  double surfaceArea = ...; // 四个面面积之和
  double r_in = 3.0 * volume / surfaceArea;  // 内切球半径
  double r_circ = circumRadiusTetrahedron(v0, v1, v2, v3);
  return 3.0 * r_in / r_circ;
}

// 3. SICN（Scaled Inverse Condition Number）
// 基于雅可比矩阵条件数
// 范围 [-1, 1]，负值表示翻转
double sicnTetrahedron(MVertex *v0, MVertex *v1, MVertex *v2, MVertex *v3);

// 4. 边长比
double aspectRatioTetrahedron(...);

// 5. 二面角
// 检查相邻面之间的夹角
double dihedralAngle(...);
```

---

## 下午任务（2小时）

### 雅可比质量

```cpp
// 雅可比行列式用于检测元素质量
class MElement {
public:
  // 获取归一化最小雅可比
  // 范围 [-1, 1]
  // > 0: 有效元素
  // = 0: 退化元素
  // < 0: 翻转元素
  double minScaledJacobian() const;

  // 获取所有积分点处的雅可比
  void getScaledJacobian(std::vector<double> &jacobians) const;
};

// 雅可比计算（以四面体为例）
double jacobianTetrahedron(MVertex *v0, MVertex *v1,
                            MVertex *v2, MVertex *v3) {
  // J = [v1-v0, v2-v0, v3-v0]^T
  double ax = v1->x() - v0->x();
  double ay = v1->y() - v0->y();
  double az = v1->z() - v0->z();

  double bx = v2->x() - v0->x();
  double by = v2->y() - v0->y();
  double bz = v2->z() - v0->z();

  double cx = v3->x() - v0->x();
  double cy = v3->y() - v0->y();
  double cz = v3->z() - v0->z();

  // det(J) = (v1-v0) · ((v2-v0) × (v3-v0))
  return ax * (by * cz - bz * cy)
       + ay * (bz * cx - bx * cz)
       + az * (bx * cy - by * cx);
}

// 归一化：除以理想元素的雅可比值
double scaledJacobian = jacobian / idealJacobian;
```

### 质量统计

```cpp
// 计算网格质量统计
struct MeshQualityStats {
  double minQuality;
  double maxQuality;
  double avgQuality;
  double stdDev;

  int numInverted;   // 翻转元素数
  int numDegenerate; // 退化元素数
  int numPoor;       // 质量低于阈值的元素数

  std::vector<int> histogram;  // 质量直方图
};

MeshQualityStats computeQualityStats(
    const std::vector<MElement*> &elements,
    double (*qualityFunc)(MElement*)) {

  MeshQualityStats stats;
  stats.minQuality = 1.0;
  stats.maxQuality = 0.0;

  double sum = 0.0;
  for(auto e : elements) {
    double q = qualityFunc(e);
    stats.minQuality = std::min(stats.minQuality, q);
    stats.maxQuality = std::max(stats.maxQuality, q);
    sum += q;

    if(q < 0) stats.numInverted++;
    else if(q < 1e-6) stats.numDegenerate++;
    else if(q < 0.1) stats.numPoor++;

    // 更新直方图
    int bin = std::min(9, (int)(q * 10));
    stats.histogram[bin]++;
  }

  stats.avgQuality = sum / elements.size();
  return stats;
}
```

### 质量可视化

```cpp
// Gmsh中的质量可视化
// 1. 通过GUI查看：Tools -> Statistics
// 2. 通过API获取：

// Python API示例
// gmsh.option.setNumber("Mesh.QualityType", 2)  # SICN
// gmsh.fltk.run()  # 颜色显示质量

// 质量类型：
// 0: SICN (Scaled Inverse Condition Number)
// 1: Gamma
// 2: minDetJac (最小雅可比)
// 3: Inner/Outer radius ratio
```

---

## 练习作业

### 1. 【基础】计算三角形质量

查找Gmsh中的质量计算函数：

```bash
# 查找gamma质量
grep -rn "gammaShapeMeasure" src/geo/

# 查找质量度量定义
grep -rn "QualityMeasure" src/mesh/
```

手动计算以下三角形的gamma质量：
```
等边三角形：边长 = 1.0
- 顶点：(0, 0), (1, 0), (0.5, √3/2)
- Area = √3/4
- Σ edge² = 3
- γ = 4√3 * (√3/4) / 3 = 1.0

直角三角形：
- 顶点：(0, 0), (1, 0), (0, 1)
- Area = 0.5
- Σ edge² = 1 + 1 + 2 = 4
- γ = 4√3 * 0.5 / 4 = √3/2 ≈ 0.866
```

### 2. 【进阶】分析质量统计

使用Gmsh查看网格质量：

```python
# test_quality.py
import gmsh
gmsh.initialize()

# 创建简单几何并生成网格
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 获取质量统计
elementTypes, elementTags, nodeTags = gmsh.model.mesh.getElements(3)
for i, etype in enumerate(elementTypes):
    qualities = gmsh.model.mesh.getElementQualities(elementTags[i])
    print(f"Type {etype}: min={min(qualities):.3f}, max={max(qualities):.3f}, avg={sum(qualities)/len(qualities):.3f}")

gmsh.finalize()
```

### 3. 【挑战】实现自定义质量检查

编写检测低质量元素的函数：

```cpp
// 伪代码
std::vector<MElement*> findPoorQualityElements(
    const std::vector<MElement*> &elements,
    double threshold = 0.1) {

  std::vector<MElement*> poor;
  for(auto e : elements) {
    double q = e->minScaledJacobian();
    if(q < threshold) {
      poor.push_back(e);
    }
  }
  return poor;
}

// 输出低质量元素信息
void reportPoorElements(const std::vector<MElement*> &poor) {
  for(auto e : poor) {
    std::cout << "Element " << e->getNum()
              << " quality = " << e->minScaledJacobian()
              << " at (" << e->barycenter().x() << ", "
              << e->barycenter().y() << ", "
              << e->barycenter().z() << ")" << std::endl;
  }
}
```

---

## 今日检查点

- [ ] 理解常用的质量度量（gamma、rho、SICN）
- [ ] 能计算简单三角形的质量
- [ ] 理解雅可比行列式的几何意义
- [ ] 了解如何在Gmsh中查看网格质量

---

## 核心概念

### 质量度量汇总

| 度量 | 范围 | 最优值 | 适用元素 |
|------|------|--------|----------|
| Gamma (γ) | [0, 1] | 1 | 三角形、四面体 |
| Rho (ρ) | [0, 1] | 1 | 三角形、四面体 |
| SICN | [-1, 1] | 1 | 所有元素 |
| minDetJac | [-1, 1] | 1 | 所有元素 |
| Aspect Ratio | [1, ∞) | 1 | 所有元素 |
| Min Angle | [0°, optimal] | 60°/90° | 三角形/四边形 |

### 质量阈值建议

```
SICN质量分级：
┌─────────────────────────────────────────────┐
│  > 0.9    极好（Excellent）                 │
│  0.7-0.9  好（Good）                        │
│  0.5-0.7  可接受（Acceptable）              │
│  0.3-0.5  差（Poor）                        │
│  0.1-0.3  很差（Very Poor）                 │
│  < 0.1    不可接受（Unacceptable）          │
│  < 0      翻转（Inverted）                  │
└─────────────────────────────────────────────┘
```

### 质量与数值精度

```
质量对有限元的影响：

1. 条件数
   - 差的元素 → 大的条件数 → 求解不稳定

2. 插值误差
   - 细长元素 → 各向异性误差 → 精度下降

3. 积分精度
   - 翻转元素 → 负积分权重 → 错误结果

4. 收敛性
   - 整体质量差 → 迭代收敛慢
```

---

## 源码导航

### 关键文件

```
src/geo/
├── MElement.cpp         # 元素质量计算
├── MTriangle.cpp        # 三角形特定质量
├── MTetrahedron.cpp     # 四面体特定质量
└── qualityMeasures.h    # 质量度量接口

src/mesh/
├── meshGFaceOptimize.cpp # 面网格优化（使用质量）
├── meshGRegionDelaunay.cpp # 体网格（Delaunay质量）
└── meshMetric.h         # 度量张量
```

### 搜索命令

```bash
# 查找质量计算
grep -rn "getQuality\|gammaShapeMeasure\|SICN" src/geo/

# 查找质量优化
grep -rn "optimize.*quality\|quality.*optimize" src/mesh/

# 查找质量阈值
grep -rn "qualityThreshold\|minQuality" src/
```

---

## 产出

- 理解网格质量度量方法
- 掌握质量检查的基本方法

---

## 导航

- **上一天**：[Day 25 - 网格拓扑关系](day-25.md)
- **下一天**：[Day 27 - 网格数据IO](day-27.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
