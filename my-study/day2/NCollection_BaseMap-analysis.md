# NCollection_BaseMap 详细解析

> 本文档详细分析 OpenCASCADE 中 `NCollection_BaseMap` 类的设计、实现和使用方式。

## 一、类概述

### 1.1 文件信息

- **文件路径**: `opencascade/NCollection_BaseMap.hxx`
- **创建时间**: 2002-04-18
- **作者**: Alexander KARTOMIN (akm)
- **许可证**: LGPL 2.1

### 1.2 类的作用

`NCollection_BaseMap` 是 OpenCASCADE 中**所有映射类（Map）的基类**，提供哈希表的基础设施：

```cpp
/**
 * Purpose:     This is a base class for all Maps:
 *                Map           - 存储键的集合
 *                DataMap       - 键值对映射
 *                DoubleMap     - 双向映射
 *                IndexedMap    - 带索引的集合
 *                IndexedDataMap- 带索引的键值对映射
 *              Provides utilities for managing the buckets.
 */
```

### 1.3 继承关系

```
NCollection_BaseMap (基类)
        │
        ├── NCollection_Map<Key>              // 键集合
        ├── NCollection_DataMap<Key, Value>   // 键值映射
        ├── NCollection_DoubleMap<K1, K2>     // 双向映射
        ├── NCollection_IndexedMap<Key>       // 带索引的集合
        └── NCollection_IndexedDataMap<K, V>  // 带索引的键值映射
```

---

## 二、数据结构设计

### 2.1 哈希表结构

`NCollection_BaseMap` 实现了一个**链地址法**的哈希表：

```
┌─────────────────────────────────────────────────────────────────────┐
│                         哈希表结构图                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  myData1 (桶数组)                                                    │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │ [0]   │ [1]   │ [2]   │ [3]   │ ... │ [myNbBuckets]        │    │
│  └───┬───┴───┬───┴───┬───┴───┬───┴─────┴───────────────────────┘    │
│      │       │       │       │                                       │
│      ▼       ▼       ▼       ▼                                       │
│   ┌─────┐  null  ┌─────┐ ┌─────┐                                    │
│   │Node1│        │Node3│ │Node4│                                    │
│   └──┬──┘        └──┬──┘ └──┬──┘                                    │
│      ▼              ▼       ▼                                        │
│   ┌─────┐        ┌─────┐  null                                      │
│   │Node2│        │Node5│                                            │
│   └──┬──┘        └──┬──┘                                            │
│      ▼              ▼                                                │
│    null           null                                               │
│                                                                      │
│  mySize = 5 (总共5个节点)                                            │
│  myNbBuckets = 桶的数量                                              │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.2 双映射结构 (DoubleMap/IndexedMap)

当 `isDouble = true` 时，使用 `myData2` 存储第二个哈希表：

```
┌─────────────────────────────────────────────────────────────────────┐
│                       双向映射结构 (isDouble = true)                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  myData1 (主键 → 节点)          myData2 (副键/索引 → 节点)          │
│  ┌──────────────────┐           ┌──────────────────┐                │
│  │ [hash1(key)]     │           │ [hash2(key2)]    │                │
│  └────────┬─────────┘           └────────┬─────────┘                │
│           │                              │                           │
│           └──────────┬───────────────────┘                          │
│                      ▼                                               │
│                 ┌─────────┐                                          │
│                 │  Node   │  ← 同一个节点被两个哈希表引用             │
│                 │ key1    │                                          │
│                 │ key2    │                                          │
│                 └─────────┘                                          │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 三、成员变量详解

### 3.1 protected 成员变量

```cpp
protected:
  Handle(NCollection_BaseAllocator) myAllocator;  // 内存分配器
  NCollection_ListNode**            myData1;      // 主哈希表（桶数组）
  NCollection_ListNode**            myData2;      // 副哈希表（双向映射用）
```

| 变量 | 类型 | 说明 |
|-----|------|------|
| `myAllocator` | 智能指针 | 内存分配器，负责所有节点的内存管理 |
| `myData1` | 指针数组 | 主哈希表的桶数组，每个桶是一个链表头 |
| `myData2` | 指针数组 | 副哈希表（仅双向映射使用） |

### 3.2 private 成员变量

```cpp
private:
  Standard_Integer       myNbBuckets;  // 桶的数量
  Standard_Integer       mySize;       // 当前元素数量
  const Standard_Boolean isDouble;     // 是否是双向映射
```

| 变量 | 类型 | 说明 |
|-----|------|------|
| `myNbBuckets` | int | 哈希表的桶数量，影响哈希分布 |
| `mySize` | int | 当前存储的元素总数 |
| `isDouble` | bool | 是否使用双哈希表（`!single` 参数决定） |

---

## 四、构造函数详解

### 4.1 主构造函数

```cpp
//! Constructor
NCollection_BaseMap(const Standard_Integer                   NbBuckets,
                    const Standard_Boolean                   single,
                    const Handle(NCollection_BaseAllocator)& theAllocator)
    : myAllocator(theAllocator.IsNull() ? NCollection_BaseAllocator::CommonBaseAllocator()
                                        : theAllocator),
      myData1(nullptr),
      myData2(nullptr),
      myNbBuckets(NbBuckets),
      mySize(0),
      isDouble(!single)
{
}
```

#### 参数说明

| 参数 | 类型 | 说明 |
|-----|------|------|
| `NbBuckets` | int | 初始桶数量，通常从小值开始（如1），后续动态扩展 |
| `single` | bool | `true`=单向映射，`false`=双向映射 |
| `theAllocator` | Handle | 内存分配器，空则使用默认分配器 |

#### 初始化列表逐行解析

```cpp
: myAllocator(theAllocator.IsNull() ? NCollection_BaseAllocator::CommonBaseAllocator()
                                    : theAllocator),
```
- **作用**: 设置内存分配器
- **逻辑**: 如果传入空分配器，使用全局默认分配器

```cpp
  myData1(nullptr),
  myData2(nullptr),
```
- **作用**: 初始化哈希表为空
- **说明**: 延迟分配，只有在真正添加元素时才分配内存

```cpp
  myNbBuckets(NbBuckets),
```
- **作用**: 记录桶数量
- **典型值**: 初始值通常很小（如1），随元素增加动态扩展

```cpp
  mySize(0),
```
- **作用**: 元素计数器初始化为0

```cpp
  isDouble(!single)
```
- **作用**: 设置是否使用双哈希表
- **注意**: 参数是 `single`，取反后存储

### 4.2 移动构造函数

```cpp
//! Move Constructor
NCollection_BaseMap(NCollection_BaseMap&& theOther) noexcept
    : myAllocator(theOther.myAllocator),
      myData1(theOther.myData1),
      myData2(theOther.myData2),
      myNbBuckets(theOther.myNbBuckets),
      mySize(theOther.mySize),
      isDouble(theOther.isDouble)
{
  // 将源对象置空，防止重复释放
  theOther.myData1     = nullptr;
  theOther.myData2     = nullptr;
  theOther.mySize      = 0;
  theOther.myNbBuckets = 0;
}
```

#### 移动语义图解

```
移动前:
┌─────────────────┐      ┌─────────────────┐
│   theOther      │      │    新对象        │
├─────────────────┤      ├─────────────────┤
│ myData1 ────────┼──→ [哈希表数据]        │ (空)               │
│ mySize = 100    │                        │                    │
└─────────────────┘                        └────────────────────┘

移动后:
┌─────────────────┐      ┌─────────────────┐
│   theOther      │      │    新对象        │
├─────────────────┤      ├─────────────────┤
│ myData1 = null  │      │ myData1 ────────┼──→ [哈希表数据]
│ mySize = 0      │      │ mySize = 100    │
└─────────────────┘      └─────────────────┘
```

### 4.3 禁用的构造函数和操作符

```cpp
//! Copy Constructor - 禁用
NCollection_BaseMap(const NCollection_BaseMap&) = delete;

//! Assign operator - 禁用
NCollection_BaseMap& operator=(const NCollection_BaseMap&) = delete;

//! Move operator - 禁用
NCollection_BaseMap& operator=(NCollection_BaseMap&&) noexcept = delete;
```

**原因**: 防止意外的深拷贝，哈希表拷贝开销大，应显式使用派生类的拷贝方法。

---

## 五、公共方法详解

### 5.1 查询方法

```cpp
//! 返回桶的数量
Standard_Integer NbBuckets() const noexcept { return myNbBuckets; }
```

```cpp
//! 返回元素数量（等同于 Size()）
Standard_Integer Extent() const noexcept { return mySize; }
```

```cpp
//! 判断是否为空
Standard_Boolean IsEmpty() const noexcept { return mySize == 0; }
```

```cpp
//! 返回内存分配器
const Handle(NCollection_BaseAllocator)& Allocator() const noexcept { return myAllocator; }
```

```cpp
//! 输出统计信息到流
Standard_EXPORT void Statistics(Standard_OStream& S) const;
```

### 5.2 使用示例

```cpp
NCollection_IndexedMap<TopoDS_Shape> map;
map.Add(shape1);
map.Add(shape2);

std::cout << "元素数量: " << map.Extent() << std::endl;      // 输出: 2
std::cout << "桶数量: " << map.NbBuckets() << std::endl;     // 输出: 取决于实现
std::cout << "是否为空: " << map.IsEmpty() << std::endl;     // 输出: 0 (false)
```

---

## 六、保护方法详解

### 6.1 大小调整方法

```cpp
//! 判断是否需要调整大小
Standard_Boolean Resizable() const noexcept { return IsEmpty() || (mySize > myNbBuckets); }
```

**调整条件**: 当元素数量 > 桶数量时，哈希冲突增加，需要扩容。

```cpp
//! 开始调整大小
Standard_EXPORT Standard_Boolean BeginResize(
    const Standard_Integer  NbBuckets,      // 当前桶数
    Standard_Integer&       NewBuckets,     // 输出：新桶数
    NCollection_ListNode**& data1,          // 输出：新哈希表1
    NCollection_ListNode**& data2           // 输出：新哈希表2
) const;
```

```cpp
//! 结束调整大小
Standard_EXPORT void EndResize(
    const Standard_Integer NbBuckets,       // 旧桶数
    const Standard_Integer NewBuckets,      // 新桶数
    NCollection_ListNode** data1,           // 新哈希表1
    NCollection_ListNode** data2            // 新哈希表2
) noexcept;
```

#### 扩容流程图

```
┌─────────────────────────────────────────────────────────────────────┐
│                         哈希表扩容流程                               │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  1. 检查是否需要扩容                                                 │
│     if (Resizable()) { ... }                                        │
│                                                                      │
│  2. 计算新桶数量（通常选择下一个质数）                               │
│     NewBuckets = NextPrimeForMap(mySize)                            │
│                                                                      │
│  3. 分配新的桶数组                                                   │
│     BeginResize(myNbBuckets, NewBuckets, newData1, newData2)        │
│                                                                      │
│  4. 重新哈希所有元素                                                 │
│     for each node in oldTable:                                      │
│         newBucket = hash(node.key) % NewBuckets                     │
│         insert node into newTable[newBucket]                        │
│                                                                      │
│  5. 释放旧桶数组，更新指针                                           │
│     EndResize(...)                                                  │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 6.2 计数方法

```cpp
//! 元素计数+1
Standard_Integer Increment() noexcept { return ++mySize; }

//! 元素计数-1
Standard_Integer Decrement() noexcept { return --mySize; }
```

### 6.3 销毁方法

```cpp
//! 销毁所有节点
Standard_EXPORT void Destroy(
    NCollection_DelMapNode fDel,              // 节点删除函数
    Standard_Boolean doReleaseMemory = true   // 是否释放桶数组内存
);
```

**参数说明**:
- `fDel`: 函数指针，用于删除特定类型的节点
- `doReleaseMemory`: 是否释放桶数组本身的内存

### 6.4 质数计算

```cpp
//! 返回大于等于 N 的下一个质数
Standard_EXPORT Standard_Integer NextPrimeForMap(const Standard_Integer N) const noexcept;
```

**用途**: 桶数量选择质数可以减少哈希冲突。

### 6.5 交换方法

```cpp
//! 交换两个 Map 的内部数据（无数据拷贝）
void exchangeMapsData(NCollection_BaseMap& theOther) noexcept
{
  std::swap(myAllocator, theOther.myAllocator);
  std::swap(myData1, theOther.myData1);
  std::swap(myData2, theOther.myData2);
  std::swap(myNbBuckets, theOther.myNbBuckets);
  std::swap(mySize, theOther.mySize);
}
```

**用途**: 高效交换两个 Map 的内容，O(1) 复杂度。

---

## 七、Iterator 内部类详解

### 7.1 迭代器成员变量

```cpp
protected:
  Standard_Integer       myNbBuckets;  // 总桶数
  NCollection_ListNode** myBuckets;    // 桶数组指针
  Standard_Integer       myBucket;     // 当前桶索引
  NCollection_ListNode*  myNode;       // 当前节点指针
```

### 7.2 迭代器工作原理

```
┌─────────────────────────────────────────────────────────────────────┐
│                      迭代器遍历过程                                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  桶数组: [bucket0] [bucket1] [bucket2] [bucket3] [bucket4]          │
│              │        │         │         │         │               │
│              ▼        ▼         ▼         ▼         ▼               │
│           NodeA    nullptr    NodeC    nullptr    NodeE             │
│              │                  │                   │               │
│              ▼                  ▼                   ▼               │
│           NodeB              NodeD              nullptr             │
│              │                  │                                   │
│              ▼                  ▼                                   │
│           nullptr            nullptr                                │
│                                                                      │
│  遍历顺序: NodeA → NodeB → NodeC → NodeD → NodeE                   │
│                                                                      │
│  1. 从 bucket0 开始，找到 NodeA                                     │
│  2. 沿链表前进到 NodeB                                              │
│  3. NodeB.Next() = nullptr，跳到下一个非空桶 bucket2                │
│  4. 找到 NodeC，继续...                                             │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### 7.3 迭代器核心方法

```cpp
//! 检查是否还有更多元素
Standard_Boolean PMore(void) const noexcept { return (myNode != nullptr); }

//! 移动到下一个元素
void PNext(void) noexcept
{
  if (!myBuckets)
    return;

  // 先尝试链表中的下一个节点
  if (myNode)
  {
    myNode = myNode->Next();
    if (myNode)
      return;  // 找到了，直接返回
  }

  // 链表结束，寻找下一个非空桶
  while (!myNode)
  {
    myBucket++;
    if (myBucket > myNbBuckets)
      return;  // 遍历结束
    myNode = myBuckets[myBucket];
  }
}
```

---

## 八、在 Gmsh 中的使用

### 8.1 Gmsh 使用的派生类

```cpp
// GModelIO_OCC.h 中的定义
TopTools_IndexedMapOfShape _vmap, _emap, _wmap, _fmap, _shmap, _somap;
TopTools_DataMapOfShapeInteger _vertexTag, _edgeTag, _faceTag, _solidTag;
TopTools_DataMapOfIntegerShape _tagVertex, _tagEdge, _tagFace, _tagSolid;
```

### 8.2 类型映射关系

| Gmsh 使用的类型 | 实际基类 | 用途 |
|----------------|---------|------|
| `TopTools_IndexedMapOfShape` | `NCollection_IndexedMap` | 存储形状集合（带索引） |
| `TopTools_DataMapOfShapeInteger` | `NCollection_DataMap` | Shape → Tag 映射 |
| `TopTools_DataMapOfIntegerShape` | `NCollection_DataMap` | Tag → Shape 映射 |

### 8.3 构造过程

当创建 `OCC_Internals` 对象时：

```cpp
OCC_Internals::OCC_Internals()
{
  // 成员变量自动构造：
  // _vmap → TopTools_IndexedMapOfShape()
  //      → NCollection_IndexedMap<TopoDS_Shape>()
  //      → NCollection_BaseMap(1, true, Handle())  ← 这里！
}
```

---

## 九、性能特性

### 9.1 时间复杂度

| 操作 | 平均复杂度 | 最坏复杂度 |
|-----|----------|----------|
| 插入 | O(1) | O(n) |
| 查找 | O(1) | O(n) |
| 删除 | O(1) | O(n) |
| 遍历 | O(n) | O(n) |

### 9.2 空间复杂度

```
总内存 = 桶数组 + 所有节点
       = O(myNbBuckets) + O(mySize)
       ≈ O(n)  (当 myNbBuckets ∝ mySize)
```

### 9.3 负载因子

```cpp
负载因子 = mySize / myNbBuckets

当 负载因子 > 1 时:
  - 平均每个桶有多个节点
  - 查找效率下降
  - 触发 Resizable() = true
  - 需要扩容
```

---

## 十、总结

### 10.1 核心设计要点

1. **链地址法哈希表**: 使用链表解决哈希冲突
2. **动态扩容**: 元素数量超过桶数量时自动扩容
3. **质数桶数**: 桶数量选择质数减少冲突
4. **内存分配器**: 支持自定义内存管理
5. **双哈希支持**: 可选的第二哈希表用于双向映射

### 10.2 在调试中的意义

当你在调试中看到 `NCollection_BaseMap` 的构造：

```
这意味着正在创建一个哈希表，用于存储和快速查找数据。
在 Gmsh + OCC 场景中，这些哈希表用于：
  - 存储几何形状集合
  - 维护 Shape ↔ Tag 的双向映射
  - 支持快速的几何查询操作
```

---

*文档生成时间：2024年12月24日*
*基于 OpenCASCADE 源代码分析*
