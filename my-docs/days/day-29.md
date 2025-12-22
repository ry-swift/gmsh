# Day 29：GModel几何模型类

**所属周次**：第5周 - 几何模块深入
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

深入理解GModel作为几何数据的中央容器

---

## 学习文件

- `src/geo/GModel.h`（约800行）
- `src/geo/GModel.cpp`（约3000行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读GModel.h类定义 |
| 09:45-10:30 | 45min | 理解实体容器（vertices, edges, faces, regions） |
| 10:30-11:15 | 45min | 理解GModel的生命周期管理 |
| 11:15-12:00 | 45min | 分析实体的添加和删除 |
| 14:00-14:45 | 45min | 理解多模型管理 |
| 14:45-15:30 | 45min | 分析GModel与GEntity的关系 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 阅读 GModel.h

**文件路径**：`src/geo/GModel.h`

GModel是Gmsh中几何数据的中央容器类。

```cpp
class GModel {
private:
  // 模型名称
  std::string _name;

  // 文件名
  std::string _fileName;

  // 几何实体容器（按tag索引）
  std::map<int, GVertex*> vertices;
  std::map<int, GEdge*> edges;
  std::map<int, GFace*> faces;
  std::map<int, GRegion*> regions;

  // 物理群组名称
  std::map<std::pair<int, int>, std::string> _physicalNames;
  // key: (dim, tag), value: name

  // 几何内核工厂
  GeoFactory *_factory;

  // 当前模型（全局指针）
  static GModel *_current;

public:
  GModel(const std::string &name = "");
  ~GModel();

  // 获取当前模型
  static GModel *current(int index = -1);
  static int setCurrent(GModel *m);

  // 实体访问
  GVertex *getVertexByTag(int tag) const;
  GEdge *getEdgeByTag(int tag) const;
  GFace *getFaceByTag(int tag) const;
  GRegion *getRegionByTag(int tag) const;

  // 实体迭代器
  std::vector<GVertex*> getVertices() const;
  std::vector<GEdge*> getEdges() const;
  std::vector<GFace*> getFaces() const;
  std::vector<GRegion*> getRegions() const;
};
```

### 实体容器结构

```
GModel实体组织：
┌─────────────────────────────────────────────────────────────┐
│                        GModel                               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ vertices: std::map<int, GVertex*>                   │   │
│  │   tag=1 → GVertex_1                                 │   │
│  │   tag=2 → GVertex_2                                 │   │
│  │   ...                                               │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ edges: std::map<int, GEdge*>                        │   │
│  │   tag=1 → GEdge_1 (v0=GVertex_1, v1=GVertex_2)     │   │
│  │   tag=2 → GEdge_2                                   │   │
│  │   ...                                               │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ faces: std::map<int, GFace*>                        │   │
│  │   tag=1 → GFace_1 (edges=[GEdge_1, GEdge_2, ...])  │   │
│  │   ...                                               │   │
│  └─────────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────────┐   │
│  │ regions: std::map<int, GRegion*>                    │   │
│  │   tag=1 → GRegion_1 (faces=[GFace_1, ...])         │   │
│  │   ...                                               │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### GModel生命周期

```cpp
// 创建模型
GModel *model = new GModel("myModel");

// 设置为当前模型
GModel::setCurrent(model);

// 添加几何实体
GVertex *v = model->addVertex(...);
GEdge *e = model->addEdge(...);
GFace *f = model->addFace(...);

// 生成网格
model->mesh(3);  // 3D网格

// 保存
model->writeMSH("output.msh");

// 清理
delete model;
```

---

## 下午任务（2小时）

### 实体的添加和删除

```cpp
class GModel {
public:
  // 添加实体
  GVertex *addVertex(GVertex *v);
  GEdge *addEdge(GEdge *e);
  GFace *addFace(GFace *f);
  GRegion *addRegion(GRegion *r);

  // 按tag添加
  bool add(GVertex *v) {
    if(vertices.count(v->tag())) return false;
    vertices[v->tag()] = v;
    v->setModel(this);
    return true;
  }

  // 删除实体
  bool remove(GVertex *v);
  bool remove(GEdge *e);
  bool remove(GFace *f);
  bool remove(GRegion *r);

  // 获取下一个可用tag
  int getMaxVertexTag() const;
  int getMaxEdgeTag() const;
  int getMaxFaceTag() const;
  int getMaxRegionTag() const;

  // 通过维度和tag获取实体
  GEntity *getEntityByTag(int dim, int tag) const {
    switch(dim) {
      case 0: return getVertexByTag(tag);
      case 1: return getEdgeByTag(tag);
      case 2: return getFaceByTag(tag);
      case 3: return getRegionByTag(tag);
    }
    return nullptr;
  }
};
```

### 多模型管理

```cpp
// Gmsh支持同时存在多个模型
// 通过静态变量管理

class GModel {
private:
  static std::vector<GModel*> _list;  // 所有模型列表
  static GModel *_current;            // 当前活动模型

public:
  // 获取所有模型
  static std::vector<GModel*> &list() { return _list; }

  // 获取当前模型
  static GModel *current(int index = -1) {
    if(index >= 0 && index < (int)_list.size())
      return _list[index];
    return _current;
  }

  // 设置当前模型
  static int setCurrent(GModel *m) {
    _current = m;
    for(int i = 0; i < (int)_list.size(); i++) {
      if(_list[i] == m) return i;
    }
    _list.push_back(m);
    return _list.size() - 1;
  }

  // 按名称查找模型
  static GModel *findByName(const std::string &name) {
    for(auto m : _list) {
      if(m->getName() == name) return m;
    }
    return nullptr;
  }
};

// API使用示例
// gmsh::model::add("model1")  → 创建新模型并设为当前
// gmsh::model::setCurrent("model2") → 切换当前模型
// gmsh::model::list() → 获取所有模型名称
```

### GModel与GEntity的关系

```cpp
// 双向关联
class GModel {
  std::map<int, GVertex*> vertices;  // 模型拥有实体
};

class GEntity {
  GModel *_model;  // 实体知道所属模型
};

// 使用场景
GEntity *entity = ...;
GModel *model = entity->model();  // 从实体获取模型

GModel *model = ...;
for(auto v : model->getVertices()) {
  // 遍历模型中的所有顶点
}
```

---

## 练习作业

### 1. 【基础】理解实体容器

查找GModel中的实体存储：

```bash
# 查找实体容器定义
grep -n "std::map.*GVertex\|std::map.*GEdge" src/geo/GModel.h

# 查找实体添加函数
grep -n "GModel::add" src/geo/GModel.cpp | head -20
```

理解为什么使用map而不是vector：
- 按tag快速查找 O(log n)
- 支持非连续tag
- 删除后不影响其他tag

### 2. 【进阶】追踪模型创建流程

分析从API到GModel的创建过程：

```bash
# 查找gmsh::model::add的实现
grep -rn "model::add" api/gmsh.cpp

# 查找GModel构造函数
grep -n "GModel::GModel" src/geo/GModel.cpp
```

绘制调用链：
```
gmsh::model::add("name")
    ↓
api/gmsh.cpp: gmsh::model::add()
    ↓
new GModel("name")
    ↓
GModel::setCurrent(model)
```

### 3. 【挑战】实现模型合并

设计将两个GModel合并的算法：

```cpp
// 伪代码
void mergeModels(GModel *target, GModel *source) {
  // 1. 计算tag偏移量，避免冲突
  int vOffset = target->getMaxVertexTag();
  int eOffset = target->getMaxEdgeTag();
  int fOffset = target->getMaxFaceTag();
  int rOffset = target->getMaxRegionTag();

  // 2. 复制顶点（更新tag）
  std::map<GVertex*, GVertex*> vMap;
  for(auto v : source->getVertices()) {
    GVertex *newV = copyVertex(v, vOffset);
    target->add(newV);
    vMap[v] = newV;
  }

  // 3. 复制边（更新tag和端点引用）
  for(auto e : source->getEdges()) {
    GEdge *newE = copyEdge(e, eOffset, vMap);
    target->add(newE);
  }

  // 4. 复制面和体...
}
```

---

## 今日检查点

- [ ] 理解GModel作为几何数据中央容器的角色
- [ ] 理解实体容器（vertices, edges, faces, regions）的组织
- [ ] 理解多模型管理机制
- [ ] 能解释GModel与GEntity的双向关联

---

## 核心概念

### GModel的核心职责

```
GModel职责：
┌─────────────────────────────────────────────────────────┐
│ 1. 几何数据容器                                         │
│    - 存储所有GEntity（顶点、边、面、体）                │
│    - 按tag索引，支持快速查找                            │
│                                                         │
│ 2. 拓扑关系管理                                         │
│    - 维护实体间的边界/邻接关系                          │
│    - 确保拓扑一致性                                     │
│                                                         │
│ 3. 网格数据关联                                         │
│    - 每个GEntity包含其网格数据                          │
│    - GModel协调整体网格生成                             │
│                                                         │
│ 4. 文件IO                                               │
│    - 支持多种几何和网格格式                             │
│    - 统一的读写接口                                     │
│                                                         │
│ 5. 物理群组                                             │
│    - 管理用户定义的物理群组                             │
│    - 支持边界条件和材料属性定义                         │
└─────────────────────────────────────────────────────────┘
```

### 关键方法分类

| 类别 | 方法 | 说明 |
|------|------|------|
| 实体访问 | getVertexByTag() | 按tag获取顶点 |
| | getEdgeByTag() | 按tag获取边 |
| | getFaceByTag() | 按tag获取面 |
| | getRegionByTag() | 按tag获取体 |
| 实体管理 | add(GEntity*) | 添加实体 |
| | remove(GEntity*) | 删除实体 |
| | getMaxTag() | 获取最大tag |
| 网格生成 | mesh(int dim) | 生成指定维度网格 |
| | deleteMesh() | 删除网格 |
| 文件IO | readGEO() | 读取GEO文件 |
| | writeMSH() | 写入MSH文件 |
| | readSTEP() | 读取STEP文件 |
| 查询 | getBoundingBox() | 获取边界盒 |
| | getNumVertices() | 获取顶点数 |

### 物理群组

```cpp
// 物理群组用于定义边界条件和材料属性
class GModel {
  // (维度, 物理tag) → 名称
  std::map<std::pair<int, int>, std::string> _physicalNames;

  // 每个GEntity可以属于多个物理群组
  // GEntity::_physicalEntities 存储物理tag列表

public:
  // 设置物理群组名称
  void setPhysicalName(int dim, int tag, const std::string &name) {
    _physicalNames[{dim, tag}] = name;
  }

  // 获取物理群组名称
  std::string getPhysicalName(int dim, int tag) const {
    auto it = _physicalNames.find({dim, tag});
    return (it != _physicalNames.end()) ? it->second : "";
  }

  // 获取属于某物理群组的所有实体
  std::vector<GEntity*> getEntitiesForPhysicalGroup(int dim, int tag) const;
};

// 使用示例
// Physical Surface("inlet") = {1, 2, 3};
// 这会将面1, 2, 3添加到名为"inlet"的物理群组
```

---

## 源码导航

### 关键文件

```
src/geo/
├── GModel.h/cpp          # 模型类（核心）
├── GModelFactory.h/cpp   # 几何创建工厂
├── GModelIO_GEO.cpp      # GEO文件IO
├── GModelIO_MSH.cpp      # MSH文件IO
├── GModelIO_OCC.cpp      # OCC文件IO
├── GModelCreateTopologyFromMesh.cpp  # 从网格创建拓扑
└── GModelVertexArrays.cpp # 顶点数组（渲染用）
```

### 搜索命令

```bash
# 查找GModel类定义
grep -n "class GModel" src/geo/GModel.h

# 查找实体添加
grep -n "GModel::add\|GModel::remove" src/geo/GModel.cpp

# 查找多模型管理
grep -n "_current\|_list" src/geo/GModel.cpp
```

---

## 产出

- 理解GModel的核心设计
- 掌握模型-实体关系

---

## 导航

- **上一天**：[Day 28 - 第四周复习](day-28.md)
- **下一天**：[Day 30 - 内置几何引擎](day-30.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
