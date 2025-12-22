# Day 15：项目架构全览

**所属周次**：第3周 - 通用工具模块与核心数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解Gmsh整体架构

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 浏览src/目录结构，了解模块划分 |
| 09:45-10:30 | 45min | 阅读各模块的主要头文件 |
| 10:30-11:15 | 45min | 理解模块间的依赖关系 |
| 11:15-12:00 | 45min | 绘制架构图 |
| 14:00-14:45 | 45min | 阅读GmshDefines.h |
| 14:45-15:30 | 45min | 阅读GmshMessage.h |
| 15:30-16:00 | 30min | 完成练习和笔记 |

---

## 上午任务（2小时）

- [ ] 阅读`src/`目录结构，理解11个模块的职责：

```
src/
├── common/   - 通用工具（日志、选项、数学基础）
├── geo/      - 几何模块（GModel、GEntity系列）
├── mesh/     - 网格模块（网格生成算法）
├── post/     - 后处理（PView、数据可视化）
├── solver/   - 求解器（FEM框架）
├── numeric/  - 数值计算（基函数、积分）
├── parser/   - 脚本解析（.geo文件解析）
├── graphics/ - 渲染（OpenGL绑定）
├── fltk/     - GUI（FLTK界面）
└── plugin/   - 插件（可扩展功能）
```

- [ ] 使用以下命令统计各模块代码量：

```bash
cd src
for dir in */; do
    echo -n "$dir: "
    find "$dir" -name "*.cpp" -o -name "*.h" | xargs wc -l 2>/dev/null | tail -1
done
```

---

## 下午任务（2小时）

### 阅读 GmshDefines.h

**文件路径**：`src/common/GmshDefines.h`

关注内容：
- [ ] 数学常量定义（PI、tolerance等）
- [ ] 元素类型枚举（TYPE_PNT、TYPE_LIN等）
- [ ] 几何实体维度定义

```cpp
// 示例：元素类型定义
#define TYPE_PNT 1   // 点
#define TYPE_LIN 2   // 线段
#define TYPE_TRI 3   // 三角形
#define TYPE_QUA 4   // 四边形
#define TYPE_TET 5   // 四面体
#define TYPE_HEX 6   // 六面体
// ...
```

### 阅读 GmshMessage.h

**文件路径**：`src/common/GmshMessage.h`

关注内容：
- [ ] Msg类的日志级别（Info、Warning、Error、Debug）
- [ ] 日志输出机制
- [ ] 进度回调设计

---

## 练习作业

### 1. 【基础】模块依赖分析

使用grep分析模块间的依赖关系：

```bash
# 查看geo模块依赖了哪些其他模块
grep -r "#include.*src/" src/geo/*.h | head -20

# 查看mesh模块依赖了哪些其他模块
grep -r "#include.*src/" src/mesh/*.h | head -20
```

记录你的发现：哪些模块是核心依赖？

### 2. 【进阶】追踪日志输出

在源码中搜索日志调用：

```bash
# 查看Info日志的使用
grep -r "Msg::Info" src/ | head -10

# 查看Error日志的使用
grep -r "Msg::Error" src/ | head -10
```

理解日志在哪些场景下被调用。

### 3. 【挑战】绘制模块架构图

用文本或工具绘制Gmsh的模块架构图，标注：
- 核心模块（geo、mesh）
- 辅助模块（common、numeric）
- 接口模块（parser、fltk）
- 模块间的调用关系

```
参考架构图：

┌─────────────────────────────────────────────────┐
│                     API层                        │
│  (api/gmsh.h - C++/Python/Julia/Fortran绑定)    │
└─────────────────────────────────────────────────┘
                        │
┌─────────────────────────────────────────────────┐
│                   应用层                         │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐         │
│  │  fltk   │  │ parser  │  │ plugin  │         │
│  │  (GUI)  │  │ (.geo)  │  │ (扩展)  │         │
│  └─────────┘  └─────────┘  └─────────┘         │
└─────────────────────────────────────────────────┘
                        │
┌─────────────────────────────────────────────────┐
│                   核心层                         │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐         │
│  │   geo   │──│  mesh   │──│  post   │         │
│  │ (几何)  │  │ (网格)  │  │(后处理) │         │
│  └─────────┘  └─────────┘  └─────────┘         │
│        │           │                            │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐         │
│  │ solver  │  │graphics │  │ numeric │         │
│  │(求解器) │  │ (渲染)  │  │ (数值)  │         │
│  └─────────┘  └─────────┘  └─────────┘         │
└─────────────────────────────────────────────────┘
                        │
┌─────────────────────────────────────────────────┐
│                   基础层                         │
│  ┌─────────────────────────────────────────┐   │
│  │                common                    │   │
│  │  (选项、消息、数据结构、数学工具)        │   │
│  └─────────────────────────────────────────┘   │
└─────────────────────────────────────────────────┘
                        │
┌─────────────────────────────────────────────────┐
│                  第三方库                        │
│  OpenCASCADE, FLTK, Eigen, CGNS, MED...        │
└─────────────────────────────────────────────────┘
```

---

## 今日检查点

- [ ] 能说出Gmsh的11个主要模块及其职责
- [ ] 理解common模块作为基础层的作用
- [ ] 理解geo和mesh模块是核心计算模块
- [ ] 能在源码中找到日志输出的位置

---

## 核心概念

### 模块职责总结

| 模块 | 主要职责 | 核心文件 |
|------|----------|----------|
| common | 通用工具、选项、日志 | GmshDefines.h, Context.h |
| geo | 几何模型、实体管理 | GModel.h, GEntity.h |
| mesh | 网格生成算法 | meshGFace.cpp, Generator.cpp |
| post | 后处理数据、视图 | PView.h, PViewData.h |
| solver | 有限元求解框架 | dofManager.h |
| numeric | 数值计算、基函数 | fullMatrix.h, Gauss.h |
| parser | GEO脚本解析 | Gmsh.y, Gmsh.l |
| graphics | OpenGL渲染 | drawContext.h |
| fltk | GUI界面 | FlGui.h |
| plugin | 插件系统 | Plugin.h |

### 关键头文件

```cpp
// 每个模块的入口头文件
#include "GmshDefines.h"  // 基础定义
#include "GmshMessage.h"  // 日志系统
#include "Context.h"      // 全局上下文
#include "GModel.h"       // 几何模型
#include "MElement.h"     // 网格元素
#include "PView.h"        // 后处理视图
```

---

## 源码阅读技巧

### 使用IDE的跳转功能

1. 在VSCode中安装C/C++扩展
2. 配置`compile_commands.json`（CMake生成）
3. 使用`Ctrl+Click`跳转到定义
4. 使用`Ctrl+Shift+O`查看文件大纲

### 使用命令行工具

```bash
# 查找类定义
grep -rn "class GModel" src/

# 查找函数实现
grep -rn "void GModel::mesh" src/

# 统计头文件被包含的次数
grep -rh "#include.*GModel.h" src/ | wc -l
```

---

## 产出

- 对项目架构有整体认知
- 模块架构图（手绘或工具绘制）

---

## 导航

- **上一天**：[Day 14 - 第二周复习与API练习](day-14.md)
- **下一天**：[Day 16 - 选项系统与上下文](day-16.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
