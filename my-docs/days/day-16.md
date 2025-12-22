# Day 16：选项系统与上下文

**所属周次**：第3周 - 通用工具模块与核心数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解Gmsh配置体系

---

## 学习文件

- `src/common/Options.h`
- `src/common/Context.h`

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 阅读Options.h头文件 |
| 09:45-10:30 | 45min | 理解选项注册机制 |
| 10:30-11:15 | 45min | 阅读opt_*函数实现 |
| 11:15-12:00 | 45min | 追踪一个选项的完整流程 |
| 14:00-14:45 | 45min | 阅读Context.h |
| 14:45-15:30 | 45min | 理解contextMeshOptions |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### 阅读 Options.h

**文件路径**：`src/common/Options.h`

关注内容：
- [ ] 选项类型定义（Number、String、Color）
- [ ] 选项注册宏
- [ ] 选项回调机制

```cpp
// 选项系统的核心结构
struct StringXNumber {
  const char *str;  // 选项名称
  int id;           // 选项ID
  double def;       // 默认值
  double *val;      // 当前值指针
};

// 选项获取/设置函数签名
double opt_geometry_tolerance(OPT_ARGS_NUM);
double opt_mesh_algorithm(OPT_ARGS_NUM);
```

### 阅读 Options.cpp（部分）

找到并理解以下函数：
- [ ] `opt_mesh_algorithm` - 网格算法选项
- [ ] `opt_mesh_characteristic_length_max` - 最大网格尺寸
- [ ] `opt_general_verbosity` - 日志级别

---

## 下午任务（2小时）

### 阅读 Context.h

**文件路径**：`src/common/Context.h`

关注内容：
- [ ] CTX类的单例模式
- [ ] 全局上下文数据存储
- [ ] 各子系统的配置结构

```cpp
// Context.h 核心结构
class CTX {
public:
  static CTX *instance();  // 单例获取

  // 几何选项
  struct contextGeometryOptions geom;

  // 网格选项
  struct contextMeshOptions mesh;

  // 后处理选项
  struct contextPostOptions post;

  // 求解器选项
  struct contextSolverOptions solver;

  // ...
};
```

### 理解 contextMeshOptions

```cpp
struct contextMeshOptions {
  int algorithm;           // 网格算法
  int algorithm3D;         // 3D网格算法
  double lcMin, lcMax;     // 网格尺寸范围
  int optimize;            // 优化级别
  int order;               // 单元阶数
  // ...
};
```

---

## 练习作业

### 1. 【基础】追踪选项设置流程

追踪`Mesh.Algorithm`选项从API到生效的流程：

```cpp
// API调用
gmsh::option::setNumber("Mesh.Algorithm", 6);

// 问题：这个调用最终如何影响到网格生成？
```

搜索路径：
```bash
# 1. 找到API入口
grep -rn "setNumber.*Mesh.Algorithm" api/

# 2. 找到选项处理函数
grep -rn "opt_mesh_algorithm" src/common/

# 3. 找到选项被使用的位置
grep -rn "CTX::instance()->mesh.algorithm" src/mesh/
```

### 2. 【进阶】列出所有网格相关选项

```bash
# 查找所有mesh相关的选项函数
grep -n "^double opt_mesh" src/common/Options.cpp | head -30
```

记录至少10个重要的网格选项及其作用。

### 3. 【挑战】添加自定义选项（概念练习）

假设要添加一个新选项`Mesh.MyCustomOption`，需要修改哪些文件？

写出修改步骤（不需要实际修改）：
1. 在哪里声明选项？
2. 在哪里实现opt函数？
3. 在哪里注册到选项表？
4. 如何在代码中使用？

---

## 今日检查点

- [ ] 理解Options.h中的选项注册机制
- [ ] 理解Context.h的单例模式设计
- [ ] 能追踪一个选项从设置到生效的完整流程
- [ ] 了解contextMeshOptions中的主要配置项

---

## 核心概念

### 选项系统架构

```
┌─────────────────────────────────────────┐
│              API / GUI / 命令行          │
│   gmsh::option::setNumber("Mesh.XX", v) │
└────────────────────┬────────────────────┘
                     │
┌────────────────────▼────────────────────┐
│            Options.cpp                   │
│   opt_mesh_xx(action, val) 函数         │
│   - ACTION_GET: 获取值                   │
│   - ACTION_SET: 设置值                   │
└────────────────────┬────────────────────┘
                     │
┌────────────────────▼────────────────────┐
│             Context.h                    │
│   CTX::instance()->mesh.xx = val        │
└────────────────────┬────────────────────┘
                     │
┌────────────────────▼────────────────────┐
│            网格生成模块                  │
│   使用 CTX::instance()->mesh.xx         │
└─────────────────────────────────────────┘
```

### 选项命名规范

| 前缀 | 对应模块 | 示例 |
|------|----------|------|
| General. | 通用 | General.Verbosity |
| Geometry. | 几何 | Geometry.Tolerance |
| Mesh. | 网格 | Mesh.Algorithm |
| PostProcessing. | 后处理 | PostProcessing.Smoothing |
| Solver. | 求解器 | Solver.Name |
| View. | 视图 | View.IntervalsType |

### 常用网格选项

```cpp
// src/common/Context.h - contextMeshOptions

// 算法选择
mesh.algorithm     // 2D: 1=MeshAdapt, 2=Auto, 5=Delaunay, 6=Frontal...
mesh.algorithm3D   // 3D: 1=Delaunay, 4=Frontal, 7=MMG3D, 10=HXT

// 尺寸控制
mesh.lcMin         // 最小网格尺寸
mesh.lcMax         // 最大网格尺寸
mesh.lcFactor      // 尺寸因子

// 优化
mesh.optimize      // 优化级别
mesh.optimizeNetgen // Netgen优化
mesh.smoothRatio   // 平滑比

// 单元类型
mesh.order         // 单元阶数（1=线性，2=二次...）
mesh.recombineAll  // 全部重组为四边形/六面体
```

---

## 源码导航

### 关键文件位置

```
src/common/
├── Options.h          # 选项声明
├── Options.cpp        # 选项实现（很长，约10000行）
├── Context.h          # 全局上下文
├── Context.cpp        # 上下文实现
├── StringOption.h     # 字符串选项
├── NumberOption.h     # 数值选项
└── ColorOption.h      # 颜色选项
```

### 搜索技巧

```bash
# 查找特定选项的定义
grep -n "opt_mesh_algorithm" src/common/Options.cpp

# 查找选项的使用位置
grep -rn "CTX::instance()->mesh.algorithm" src/

# 查看Context结构定义
grep -A 50 "struct contextMeshOptions" src/common/Context.h
```

---

## 产出

- 理解Gmsh配置机制
- 记录10个重要的网格选项

---

## 导航

- **上一天**：[Day 15 - 项目架构全览](day-15.md)
- **下一天**：[Day 17 - 空间索引结构](day-17.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
