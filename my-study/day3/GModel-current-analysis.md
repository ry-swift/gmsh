# GModel::current(int index) 方法深度解析

> Day 3 学习笔记 - 2025-12-25
>
> 本文档详细分析 Gmsh 源码中 `GModel::current()` 方法的设计思想、实现机制和调用场景。

---

## 一、方法定位

### 1.1 源码位置

| 项目 | 值 |
|------|-----|
| **头文件** | `src/geo/GModel.h:207` |
| **实现文件** | `src/geo/GModel.cpp:136-145` |
| **所属类** | `GModel` |
| **方法类型** | `static` 静态方法 |

### 1.2 方法签名

```cpp
// 头文件声明 (GModel.h:207)
static GModel *current(int index = -1);
```

### 1.3 完整实现

```cpp
// 实现 (GModel.cpp:136-145)
GModel *GModel::current(int index)
{
  // 步骤1：如果模型列表为空，自动创建一个新模型
  if(list.empty()) {
    Msg::Debug("No current model available: creating one");
    new GModel();
  }

  // 步骤2：如果传入了有效索引，更新当前模型索引
  if(index >= 0) _current = index;

  // 步骤3：边界检查，返回当前模型
  if(_current < 0 || _current >= (int)list.size()) return list.back();
  return list[_current];
}
```

---

## 二、关键静态成员

### 2.1 静态变量声明

```cpp
// GModel.h:202 - 所有加载模型的列表
static std::vector<GModel *> list;

// GModel.h:139 - 当前模型在list中的索引
static int _current;
```

### 2.2 静态变量初始化

```cpp
// GModel.cpp:69-70
std::vector<GModel *> GModel::list;
int GModel::_current = -1;
```

### 2.3 变量关系图

```
┌─────────────────────────────────────────────────────────────┐
│                    GModel::list (静态向量)                   │
├─────────────────────────────────────────────────────────────┤
│  index:    [0]         [1]         [2]         [3]          │
│            ↓           ↓           ↓           ↓            │
│         GModel*     GModel*     GModel*     GModel*         │
│         "model1"    "model2"    "hhh.igs"   "test.geo"      │
│                        ↑                                    │
│                        │                                    │
│            GModel::_current = 1  (指向当前活跃模型)          │
└─────────────────────────────────────────────────────────────┘
```

---

## 三、设计模式分析

### 3.1 多例模式 (Multiton Pattern)

`GModel::current()` 实现的是一种**多例模式**，与传统单例模式的区别：

| 特性 | 单例模式 | 多例模式 (GModel) |
|------|---------|------------------|
| 实例数量 | 固定1个 | 可以有多个 |
| 访问方式 | `getInstance()` | `current(index)` 切换 |
| 全局访问 | 是 | 是 |
| 适用场景 | 唯一资源 | 需要管理多个同类对象 |

### 3.2 为什么需要多例模式？

在 Gmsh 中，用户可能同时处理多个几何模型：

```
┌─────────────────────────────────────────────────────────┐
│                    使用场景示例                          │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  场景1：加载多个CAD文件                                  │
│  ┌─────────┐   ┌─────────┐   ┌─────────┐               │
│  │ box.igs │   │ cyl.stp │   │ sph.brep│               │
│  └────┬────┘   └────┬────┘   └────┬────┘               │
│       ↓             ↓             ↓                     │
│    GModel*       GModel*       GModel*                  │
│    list[0]       list[1]       list[2]                  │
│                                                         │
│  场景2：布尔运算                                         │
│  ┌─────────┐         ┌─────────┐                       │
│  │ 模型A   │  Union  │ 模型B   │  →  结果存入 list[2]  │
│  └─────────┘         └─────────┘                       │
│                                                         │
│  场景3：比较分析                                         │
│  ┌─────────┐         ┌─────────┐                       │
│  │ 原始网格 │   vs   │ 优化网格 │                       │
│  └─────────┘         └─────────┘                       │
└─────────────────────────────────────────────────────────┘
```

---

## 四、执行流程详解

### 4.1 流程图

```
                    ┌─────────────────────┐
                    │ GModel::current(n)  │
                    │     被调用          │
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │   list.empty()?     │
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              │ 是             │                │ 否
              ▼                │                ▼
    ┌──────────────────┐       │      ┌──────────────────┐
    │ new GModel()     │       │      │ (跳过创建步骤)   │
    │ 自动创建新模型   │       │      └────────┬─────────┘
    └────────┬─────────┘       │               │
             │                 │               │
             └─────────────────┼───────────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │   index >= 0 ?      │
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              │ 是             │                │ 否
              ▼                │                ▼
    ┌──────────────────┐       │      ┌──────────────────┐
    │ _current = index │       │      │ (保持_current)   │
    │ 更新当前索引     │       │      └────────┬─────────┘
    └────────┬─────────┘       │               │
             │                 │               │
             └─────────────────┼───────────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │ _current 有效?      │
                    │ (0 <= _current      │
                    │  < list.size())     │
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              │ 否             │                │ 是
              ▼                │                ▼
    ┌──────────────────┐       │      ┌──────────────────┐
    │ return list.back()│      │      │ return list      │
    │ 返回最后一个模型 │       │      │      [_current]  │
    └──────────────────┘       │      │ 返回当前模型     │
                               │      └──────────────────┘
```

### 4.2 典型调用场景

#### 场景A：首次调用（list为空）

```cpp
// 程序启动时，list是空的
GModel *m = GModel::current();  // index默认为-1

// 执行流程：
// 1. list.empty() == true → 执行 new GModel()
// 2. GModel构造函数中：list.push_back(this) → list = [新模型]
// 3. _current = -1（未改变）
// 4. _current < 0 → 返回 list.back() = list[0]
```

#### 场景B：获取当前模型（不切换）

```cpp
// 已有多个模型，_current = 1
GModel *m = GModel::current();  // 不传参数

// 执行流程：
// 1. list.empty() == false → 跳过创建
// 2. index = -1 < 0 → 不更新_current
// 3. _current = 1 有效 → 返回 list[1]
```

#### 场景C：切换到指定模型

```cpp
// 切换到第3个模型
GModel *m = GModel::current(2);  // 传入索引2

// 执行流程：
// 1. list.empty() == false → 跳过创建
// 2. index = 2 >= 0 → _current = 2
// 3. _current = 2 有效 → 返回 list[2]
```

---

## 五、与构造函数的协作

### 5.1 GModel 构造函数

```cpp
// GModel.cpp:72-97
GModel::GModel(const std::string &name)
  : _name(name), _visible(1), ...
{
  // 关键：隐藏所有其他模型
  for(std::size_t i = 0; i < list.size(); i++)
    list[i]->setVisibility(0);

  // 将新模型加入列表
  list.push_back(this);

  // 创建内部GEO模型
  createGEOInternals();

#if defined(HAVE_MESH)
  _fields = new FieldManager();
#endif
}
```

### 5.2 构造与current的关系图

```
┌─────────────────────────────────────────────────────────────────┐
│                   new GModel() 的连锁反应                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  调用前：                                                        │
│  ┌─────────────────────────────────────────┐                    │
│  │ list = [m0, m1]    _current = 1         │                    │
│  │ visibility: [1, 1]                      │                    │
│  └─────────────────────────────────────────┘                    │
│                         │                                       │
│                         ▼ new GModel("new")                     │
│                                                                 │
│  调用后：                                                        │
│  ┌─────────────────────────────────────────┐                    │
│  │ list = [m0, m1, m2]  _current = 1       │ (索引未变)         │
│  │ visibility: [0, 0, 1] ← 只有新模型可见   │                    │
│  └─────────────────────────────────────────┘                    │
│                                                                 │
│  注意：_current 仍然指向 m1，但 m1 已被隐藏                       │
│  调用 GModel::current() 仍返回 m1                                │
└─────────────────────────────────────────────────────────────────┘
```

---

## 六、关联方法

### 6.1 setCurrent(GModel *m)

```cpp
// GModel.cpp:147-156
int GModel::setCurrent(GModel *m)
{
  for(std::size_t i = 0; i < list.size(); i++) {
    if(list[i] == m) {
      _current = i;
      break;
    }
  }
  return _current;
}
```

用途：通过模型指针设置当前模型

### 6.2 findByName()

```cpp
// GModel.cpp:158-166
GModel *GModel::findByName(const std::string &name,
                           const std::string &fileName)
{
  // 从后向前查找（返回最后一个匹配的模型）
  for(int i = list.size() - 1; i >= 0; i--)
    if(list[i]->getName() == name && ...)
      return list[i];
  return nullptr;
}
```

### 6.3 三种获取模型的方式对比

| 方法 | 用途 | 示例 |
|------|------|------|
| `current()` | 获取/切换当前模型 | `GModel::current(2)` |
| `setCurrent(m)` | 通过指针设置当前 | `GModel::setCurrent(myModel)` |
| `findByName()` | 按名称查找模型 | `GModel::findByName("box")` |

---

## 七、实际调用场景分析

### 7.1 文件解析时

```cpp
// src/geo/GModel.cpp:3567-3575
int GModel::readGEO(const std::string &name)
{
  ParseFile(name, true);

  // 同步OCC几何
  if(GModel::current()->getOCCInternals())
    GModel::current()->getOCCInternals()->synchronize(GModel::current());

  // 同步GEO几何
  GModel::current()->getGEOInternals()->synchronize(GModel::current());
  return 1;
}
```

### 7.2 网格生成时

```cpp
// src/common/GmshGlobal.cpp:334
GModel::current()->mesh(CTX::instance()->batch);
```

### 7.3 GUI交互时

```cpp
// src/graphics/drawMesh.cpp:363
CTX::instance()->pickElements && e->model() == GModel::current()
```

### 7.4 文件加载/保存时

```cpp
// src/geo/GModel.cpp:3550-3556
void GModel::load(const std::string &fileName)
{
  GModel *temp = GModel::current();  // 保存当前模型
  GModel::setCurrent(this);          // 切换到目标模型
  MergeFile(fileName, true);         // 执行加载
  GModel::setCurrent(temp);          // 恢复原来的当前模型
}
```

---

## 八、调用链追踪

### 8.1 从加载IGES文件到使用current()

```
用户命令: gmsh hhh.igs
    │
    ▼
main() [src/common/GmshGlobal.cpp]
    │
    ▼
GmshInitialize()
    │ 创建默认GModel
    ▼
GmshBatch() 或 GmshFLTK()
    │
    ▼
MergeFile("hhh.igs") [src/common/OpenFile.cpp:380]
    │ 识别.igs扩展名
    ▼
GModel::current()->readOCCIGES()  ← 这里调用 current()
    │
    ▼
OCC_Internals::importShapes()
    │
    ▼
OCC_Internals::synchronize(GModel::current())  ← 再次调用
```

### 8.2 调用频率统计

在 Gmsh 源码中，`GModel::current()` 被调用超过 **1000次**，主要分布：

| 模块 | 调用次数 | 典型用途 |
|------|---------|---------|
| `src/parser/` | 400+ | .geo脚本解析 |
| `src/geo/` | 200+ | 几何操作 |
| `src/mesh/` | 150+ | 网格生成 |
| `src/common/` | 100+ | 全局操作 |
| `src/fltk/` | 100+ | GUI交互 |
| `src/post/` | 50+ | 后处理 |

---

## 九、调试建议

### 9.1 LLDB断点设置

```lldb
# 在current()方法入口设置断点
breakpoint set -f GModel.cpp -l 136 -N gmodel_current

# 监视_current变化
watchpoint set variable GModel::_current

# 监视list大小变化
expr GModel::list.size()
```

### 9.2 关键观察变量

| 变量 | LLDB命令 | 说明 |
|------|---------|------|
| 当前索引 | `expr GModel::_current` | 当前模型索引 |
| 模型数量 | `expr GModel::list.size()` | 已加载模型数 |
| 模型名称 | `expr GModel::list[i]->getName()` | 各模型名称 |
| 传入参数 | `frame variable index` | 调用时传入的索引 |

### 9.3 典型调试场景

```lldb
# 查看所有已加载的模型
(lldb) expr for(int i=0; i<GModel::list.size(); i++) \
       printf("[%d] %s\n", i, GModel::list[i]->getName().c_str())

# 查看当前模型的详细信息
(lldb) expr GModel::current()->getName()
(lldb) expr GModel::current()->getNumVertices()
(lldb) expr GModel::current()->getNumElements()
```

---

## 十、总结

### 10.1 核心要点

1. **设计目的**：提供对当前活跃GModel的全局访问点
2. **多例管理**：通过静态list管理多个模型实例
3. **惰性创建**：首次调用时自动创建模型
4. **索引切换**：支持通过index参数切换当前模型
5. **安全返回**：边界检查确保总能返回有效模型

### 10.2 设计优点

- 简化了全局模型访问
- 支持多模型并存
- 自动初始化避免空指针
- 灵活的模型切换机制

### 10.3 注意事项

- 新建GModel会隐藏其他模型（visibility设为0）
- _current索引可能与可见性不一致
- 多线程环境需要注意同步

---

## 参考资料

- **源码路径**: `src/geo/GModel.h`, `src/geo/GModel.cpp`
- **Gmsh文档**: https://gmsh.info/doc/texinfo/gmsh.html
- **设计模式**: Multiton Pattern (多例模式)
