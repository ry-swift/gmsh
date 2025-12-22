# Day 18：几何实体基类

**所属周次**：第3周 - 通用工具模块与核心数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解GEntity继承体系

---

## 学习文件

- `src/geo/GEntity.h`（约600行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读GEntity.h前半部分 |
| 09:45-10:30 | 45min | 理解维度和标签系统 |
| 10:30-11:15 | 45min | 理解可见性和颜色属性 |
| 11:15-12:00 | 45min | 阅读GEntity.h后半部分 |
| 14:00-14:45 | 45min | 理解关键虚函数 |
| 14:45-15:30 | 45min | 绘制继承关系图 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 阅读 GEntity.h

**文件路径**：`src/geo/GEntity.h`

这是Gmsh几何系统的核心基类，所有几何实体都继承自它。

### 关键成员变量

```cpp
class GEntity {
protected:
  GModel *_model;       // 所属模型
  int _tag;             // 唯一标识符
  int _meshMaster;      // 网格主实体（用于周期性）
  char _visible;        // 可见性
  unsigned int _color;  // 颜色（RGBA）

  // 物理群组
  std::vector<int> _physicalEntities;

  // 网格元素存储
  std::vector<MVertex*> mesh_vertices;

public:
  // 维度：0=点, 1=边, 2=面, 3=体
  virtual int dim() const = 0;

  // 获取标签
  int tag() const { return _tag; }

  // ...
};
```

### 维度系统

| 维度 | 实体类型 | 派生类 |
|------|----------|--------|
| 0 | 点/顶点 | GVertex |
| 1 | 边/曲线 | GEdge |
| 2 | 面/曲面 | GFace |
| 3 | 体/区域 | GRegion |

---

## 下午任务（2小时）

### 关键虚函数

```cpp
class GEntity {
public:
  // 几何信息
  virtual int dim() const = 0;                    // 维度
  virtual SBoundingBox3d bounds() const;          // 边界盒
  virtual double getMeshSize() const;             // 网格尺寸

  // 参数化
  virtual Range<double> parBounds(int i) const;   // 参数范围
  virtual bool haveParametrization();             // 是否有参数化
  virtual GPoint point(double par) const;         // 参数到点（1D）
  virtual GPoint point(double u, double v) const; // 参数到点（2D）

  // 网格相关
  virtual std::size_t getNumMeshElements() const; // 网格元素数量
  virtual MElement *getMeshElement(std::size_t i);// 获取元素
  virtual void addMeshVertex(MVertex *v);         // 添加顶点

  // 拓扑关系
  virtual std::vector<GVertex*> vertices() const; // 边界顶点
  virtual std::vector<GEdge*> edges() const;      // 边界边
  virtual std::vector<GFace*> faces() const;      // 边界面
};
```

### 绘制继承关系图

```
                    GEntity（抽象基类）
                        │
        ┌───────────────┼───────────────┐
        │               │               │
    GVertex(0D)     GEdge(1D)      GFace(2D)      GRegion(3D)
        │               │               │               │
    ┌───┴───┐      ┌───┴───┐      ┌───┴───┐      ┌───┴───┐
    │       │      │       │      │       │      │       │
 gmshVertex OCCVertex  gmshEdge OCCEdge  gmshFace OCCFace gmshRegion OCCRegion
 (内置)    (OCC)    (内置)   (OCC)    (内置)  (OCC)    (内置)    (OCC)

discreteVertex  discreteEdge  discreteFace  discreteRegion
 (离散)          (离散)        (离散)         (离散)
```

---

## 练习作业

### 1. 【基础】统计虚函数

计算GEntity类中有多少个纯虚函数和普通虚函数：

```bash
# 统计纯虚函数（= 0）
grep -c "= 0;" src/geo/GEntity.h

# 统计所有虚函数
grep -c "virtual" src/geo/GEntity.h
```

### 2. 【进阶】追踪继承关系

找到GVertex、GEdge、GFace、GRegion的定义：

```bash
# 查找各派生类
grep -rn "class GVertex.*GEntity" src/geo/
grep -rn "class GEdge.*GEntity" src/geo/
grep -rn "class GFace.*GEntity" src/geo/
grep -rn "class GRegion.*GEntity" src/geo/
```

记录每个派生类的文件位置和主要职责。

### 3. 【挑战】理解参数化接口

分析`point()`函数在不同维度实体中的行为：

```cpp
// GVertex: 直接返回坐标
GPoint GVertex::point() const;

// GEdge: t ∈ [t_min, t_max] 映射到曲线上的点
GPoint GEdge::point(double t) const;

// GFace: (u,v) ∈ [u_min,u_max] × [v_min,v_max] 映射到曲面上的点
GPoint GFace::point(double u, double v) const;
```

思考：
- 参数化的数学意义是什么？
- 为什么需要参数化？

---

## 今日检查点

- [ ] 理解GEntity作为基类的设计目的
- [ ] 能说出dim()返回值与实体类型的对应关系
- [ ] 理解虚函数机制在几何系统中的应用
- [ ] 能绘制完整的GEntity继承树

---

## 核心概念

### GEntity设计模式

**模板方法模式**：
- GEntity定义算法骨架
- 派生类实现具体步骤

**多态性**：
```cpp
// 统一处理不同维度的实体
void processEntity(GEntity *entity) {
    int d = entity->dim();           // 多态调用
    SBoundingBox3d bb = entity->bounds();
    // ...
}

// 遍历模型中的所有实体
for (auto v : model->getVertices()) processEntity(v);
for (auto e : model->getEdges()) processEntity(e);
for (auto f : model->getFaces()) processEntity(f);
for (auto r : model->getRegions()) processEntity(r);
```

### 物理群组

```cpp
// 物理群组用于定义边界条件和材料属性
// 一个实体可以属于多个物理群组
std::vector<int> _physicalEntities;

// API使用示例
gmsh::model::addPhysicalGroup(2, {1, 2, 3}, 100, "inlet");
// dim=2, tags=[1,2,3], physicalTag=100, name="inlet"
```

### 参数化的数学意义

**曲线参数化（1D）**：
```
C(t) = (x(t), y(t), z(t)), t ∈ [t_min, t_max]
```

**曲面参数化（2D）**：
```
S(u,v) = (x(u,v), y(u,v), z(u,v)), (u,v) ∈ [u_min,u_max] × [v_min,v_max]
```

参数化的作用：
1. 统一的几何表示
2. 便于网格生成（参数空间划分）
3. 支持各种曲线/曲面类型

---

## 源码导航

### GEntity家族文件

```
src/geo/
├── GEntity.h          # 基类定义
├── GEntity.cpp        # 基类实现
├── GVertex.h/cpp      # 0D顶点
├── GEdge.h/cpp        # 1D边
├── GFace.h/cpp        # 2D面
├── GRegion.h/cpp      # 3D体
│
├── gmshVertex.h/cpp   # 内置几何顶点
├── gmshEdge.h/cpp     # 内置几何边
├── gmshFace.h/cpp     # 内置几何面
├── gmshRegion.h/cpp   # 内置几何体
│
├── OCCVertex.h/cpp    # OCC顶点
├── OCCEdge.h/cpp      # OCC边
├── OCCFace.h/cpp      # OCC面
├── OCCRegion.h/cpp    # OCC体
│
├── discreteVertex.h/cpp  # 离散顶点
├── discreteEdge.h/cpp    # 离散边
├── discreteFace.h/cpp    # 离散面
└── discreteRegion.h/cpp  # 离散体
```

### 搜索命令

```bash
# 查找GEntity的所有派生类
grep -rn "class.*: public GEntity" src/geo/
grep -rn "class.*: public GVertex" src/geo/
grep -rn "class.*: public GEdge" src/geo/
grep -rn "class.*: public GFace" src/geo/
grep -rn "class.*: public GRegion" src/geo/
```

---

## 产出

- 掌握几何实体抽象设计
- GEntity继承关系图

---

## 导航

- **上一天**：[Day 17 - 空间索引结构](day-17.md)
- **下一天**：[Day 19 - 顶点与边实体](day-19.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
