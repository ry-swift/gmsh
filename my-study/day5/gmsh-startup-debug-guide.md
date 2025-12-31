# Gmsh 启动到 GUI 显示完整流程调试指南

## 概述

本文档记录了执行 `./gmsh ../tutorials/hhh.igs` 命令后，从程序入口到 GUI 窗口显示几何模型的完整流程，以及推荐的调试断点位置。

---

## 核心断点位置（按执行顺序）

### 1. 程序入口
**文件**: `src/common/Main.cpp`
**行号**: 42
**函数**: `main()`
**说明**: 程序的最初入口点

```cpp
int main(int argc, char *argv[])
{
#if defined(HAVE_FLTK)
  return GmshMainFLTK(argc, argv);  // 第48行
#else
  return GmshMainBatch(argc, argv);
#endif
}
```

---

### 2. FLTK 主入口
**文件**: `src/common/GmshGlobal.cpp`
**行号**: 512
**函数**: `GmshMainFLTK()`
**说明**: FLTK GUI 模式的主函数入口

```cpp
GMSH_API int GmshMainFLTK(int argc, char **argv)
{
  new GModel();                          // 第515行: 创建几何模型
  GmshInitialize(argc, argv, true);      // 第529行: 初始化
  if(CTX::instance()->batch) {           // 第532行: 检查批处理模式
    GmshBatch();
    return;
  }
  return GmshFLTK(argc, argv);           // 第540行: 启动GUI
}
```

---

### 3. 命令行参数解析
**文件**: `src/common/CommandLine.cpp`
**行号**: 1544
**函数**: `GetOptions()`
**说明**: 将文件参数（如 hhh.igs）存入 CTX::instance()->files 数组

```cpp
else {
  CTX::instance()->files.push_back(argv[i++]);  // 非'-'开头的参数被当作文件
}
```

---

### 4. GUI 类创建
**文件**: `src/fltk/FlGui.cpp`
**行号**: 463
**函数**: `FlGui::FlGui()` 构造函数
**说明**: 创建 FlGui 单例，初始化 GUI 组件

关键步骤：
- 第507行: 设置绘图上下文 `drawContext::setGlobal(new drawContextFltk)`
- 第559行: 创建图形窗口 `new graphicWindow(...)`

---

### 5. 图形窗口显示
**文件**: `src/fltk/FlGui.cpp`
**行号**: 559-564
**说明**: 创建并显示主图形窗口

```cpp
graph.push_back(
  new graphicWindow(true, CTX::instance()->numTiles,
                    CTX::instance()->detachedMenu ? true : false));
graph[0]->getWindow()->show(argc > 0 ? 1 : 0, argv);
```

---

### 6. 文件加载调用
**文件**: `src/common/GmshGlobal.cpp`
**行号**: 426
**函数**: `MergeFile()`
**说明**: 加载命令行指定的 IGES 文件

```cpp
for(std::size_t i = 0; i < CTX::instance()->files.size(); i++) {
  if(CTX::instance()->files[i] == "-merge")
    open = false;
  else if(CTX::instance()->files[i] == "-open")
    open = true;
  else if(open)
    OpenProject(CTX::instance()->files[i]);
  else
    MergeFile(CTX::instance()->files[i]);    // 第426行 - 默认执行这里
}
```

---

### 7. IGES 文件读取
**文件**: `src/geo/GModelIO_OCC.cpp`
**行号**: 6783
**函数**: `GModel::readOCCIGES()`
**说明**: 调用 OpenCASCADE 读取 IGES 文件

```cpp
int GModel::readOCCIGES(const std::string &fn)
{
  if(!_occ_internals) _occ_internals = new OCC_Internals;
  std::vector<std::pair<int, int>> outDimTags;
  _occ_internals->importShapes(fn, false, outDimTags, "iges");
  _occ_internals->synchronize(this);
  return 1;
}
```

---

### 8. 几何模型绘制
**文件**: `src/graphics/drawGeom.cpp`
**行号**: 521
**函数**: `drawContext::drawGeom()`
**说明**: 使用 OpenGL 绘制几何模型

```cpp
void drawContext::drawGeom()
{
  if(!CTX::instance()->geom.draw) return;

  for(std::size_t i = 0; i < GModel::list.size(); i++) {
    GModel *m = GModel::list[i];
    if(m->getVisibility() && isVisible(m)) {
      std::for_each(m->firstVertex(), m->lastVertex(), drawGVertex(this));
      std::for_each(m->firstEdge(), m->lastEdge(), drawGEdge(this));
      std::for_each(m->firstFace(), m->lastFace(), drawGFace(this));
      std::for_each(m->firstRegion(), m->lastRegion(), drawGRegion(this));
    }
  }
}
```

---

### 9. 主事件循环
**文件**: `src/fltk/FlGui.cpp`
**行号**: 702
**函数**: `FlGui::run()` → `Fl::run()`
**说明**: 启动 FLTK 主事件循环，持续处理用户交互

```cpp
int FlGui::run()
{
  drawContext::global()->draw(false);  // 第696行: 首次绘制
  return Fl::run();                     // 第702行: 事件循环
}
```

---

## 完整调用链

```
./gmsh ../tutorials/hhh.igs
    ↓
main() [src/common/Main.cpp:42]
    ↓
GmshMainFLTK(argc, argv) [src/common/GmshGlobal.cpp:512]
    ├→ new GModel()
    ├→ GmshInitialize(argc, argv, true) [第529行]
    │   └→ GetOptions() [src/common/CommandLine.cpp:1486]
    │       └→ CTX::instance()->files.push_back("hhh.igs") [第1544行]
    ↓
GmshFLTK(argc, argv) [src/common/GmshGlobal.cpp:395]
    ├→ FlGui::instance(argc, argv) [src/fltk/FlGui.cpp:651]
    │   └→ new FlGui(...) [第463行]
    │       ├→ drawContext::setGlobal(new drawContextFltk) [第507行]
    │       └→ new graphicWindow(...) [第559行]
    ├→ MergeFile("../tutorials/hhh.igs") [第426行]
    │   └→ GModel::current()->readOCCIGES() [src/common/OpenFile.cpp:380]
    │       └→ OCC_Internals::importShapes() + synchronize()
    └→ FlGui::instance()->run() [第484行]
        ├→ drawContext::global()->draw(false) [src/fltk/FlGui.cpp:696]
        │   └→ drawContext::draw3d() [src/graphics/drawContext.cpp:277]
        │       └→ drawGeom() [src/graphics/drawGeom.cpp:521]
        └→ Fl::run() [第702行] ← 持续事件循环
```

---

## 断点汇总表

| 序号 | 阶段 | 文件 | 行号 | 函数 |
|:---:|------|------|:----:|------|
| 1 | 程序入口 | `src/common/Main.cpp` | 42 | `main()` |
| 2 | FLTK入口 | `src/common/GmshGlobal.cpp` | 512 | `GmshMainFLTK()` |
| 3 | 参数解析 | `src/common/CommandLine.cpp` | 1544 | `GetOptions()` |
| 4 | GUI创建 | `src/fltk/FlGui.cpp` | 463 | `FlGui::FlGui()` |
| 5 | 窗口显示 | `src/fltk/FlGui.cpp` | 559 | `graphicWindow` 创建 |
| 6 | 文件加载 | `src/common/GmshGlobal.cpp` | 426 | `MergeFile()` |
| 7 | IGES读取 | `src/geo/GModelIO_OCC.cpp` | 6783 | `readOCCIGES()` |
| 8 | 几何绘制 | `src/graphics/drawGeom.cpp` | 521 | `drawGeom()` |
| 9 | 事件循环 | `src/fltk/FlGui.cpp` | 702 | `Fl::run()` |

---

## VS Code / Cursor 断点设置方法

### 方法一：直接跳转设置

1. 打开对应文件（如 `src/common/GmshGlobal.cpp`）
2. 按 `Ctrl+G` (macOS: `Cmd+G`)
3. 输入行号（如 `512`），回车
4. 点击行号左侧空白处，出现红点即为断点

### 方法二：使用 launch.json 配置

在 `.vscode/launch.json` 中添加预设断点：

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug Gmsh Startup",
      "type": "lldb",
      "request": "launch",
      "program": "${workspaceFolder}/build/gmsh",
      "args": ["../tutorials/hhh.igs"],
      "cwd": "${workspaceFolder}/build",
      "stopOnEntry": false
    }
  ]
}
```

---

## LLDB 命令行调试

```bash
cd /Users/renyuxiao/Documents/gmsh/gmsh-4.14.0-source/build

# 启动 lldb
lldb ./gmsh

# 设置断点
(lldb) breakpoint set -f Main.cpp -l 42
(lldb) breakpoint set -f GmshGlobal.cpp -l 512
(lldb) breakpoint set -f GmshGlobal.cpp -l 426
(lldb) breakpoint set -f CommandLine.cpp -l 1544
(lldb) breakpoint set -f FlGui.cpp -l 463
(lldb) breakpoint set -f GModelIO_OCC.cpp -l 6783
(lldb) breakpoint set -f drawGeom.cpp -l 521
(lldb) breakpoint set -f FlGui.cpp -l 702

# 运行程序
(lldb) run ../tutorials/hhh.igs

# 调试命令
(lldb) n          # 单步跳过 (next)
(lldb) s          # 单步进入 (step)
(lldb) c          # 继续执行 (continue)
(lldb) bt         # 查看调用栈 (backtrace)
(lldb) p <var>    # 打印变量值
(lldb) fr v       # 显示当前帧所有局部变量
```

---

## 快速调试方案（3个关键断点）

如果只想快速了解流程，设置以下3个断点即可：

| 断点 | 文件 | 行号 | 说明 |
|:---:|------|:----:|------|
| 1 | `src/common/GmshGlobal.cpp` | 512 | 程序启动入口 |
| 2 | `src/common/GmshGlobal.cpp` | 426 | IGES文件加载 |
| 3 | `src/graphics/drawGeom.cpp` | 521 | 几何模型绘制 |

```bash
# LLDB 快速设置
(lldb) breakpoint set -f GmshGlobal.cpp -l 512
(lldb) breakpoint set -f GmshGlobal.cpp -l 426
(lldb) breakpoint set -f drawGeom.cpp -l 521
(lldb) run ../tutorials/hhh.igs
```

---

## 相关文件路径汇总

| 模块 | 文件路径 |
|------|---------|
| 程序入口 | `src/common/Main.cpp` |
| 全局初始化 | `src/common/GmshGlobal.cpp` |
| 命令行解析 | `src/common/CommandLine.cpp` |
| 文件加载 | `src/common/OpenFile.cpp` |
| FLTK GUI | `src/fltk/FlGui.cpp` |
| 图形窗口 | `src/fltk/graphicWindow.cpp` |
| OpenGL窗口 | `src/fltk/openglWindow.cpp` |
| 绘图上下文 | `src/graphics/drawContext.cpp` |
| 几何绘制 | `src/graphics/drawGeom.cpp` |
| OCC 几何IO | `src/geo/GModelIO_OCC.cpp` |

---

## 调试技巧

1. **查看变量**: 鼠标悬停在变量上或使用 `p <变量名>`
2. **监视表达式**: 添加监视如 `CTX::instance()->files.size()`
3. **条件断点**: 右键断点 → 编辑条件（如 `fileName.find(".igs") != std::string::npos`）
4. **调用堆栈**: 使用 `bt` 命令或查看"调用堆栈"面板
5. **跳过循环**: 在循环体外设置断点，使用 `c` 跳过

---

## 流程图

```
┌─────────────────────────────────────────────────────────────┐
│                    ./gmsh ../tutorials/hhh.igs              │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  main() [Main.cpp:42]                                       │
│  └→ GmshMainFLTK(argc, argv)                               │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  GmshMainFLTK() [GmshGlobal.cpp:512]                       │
│  ├→ new GModel()                                           │
│  ├→ GmshInitialize() → GetOptions() → 解析"hhh.igs"        │
│  └→ GmshFLTK()                                             │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  GmshFLTK() [GmshGlobal.cpp:395]                           │
│  ├→ FlGui::instance() → 创建GUI窗口                        │
│  ├→ MergeFile("hhh.igs") → readOCCIGES() → 加载几何        │
│  └→ FlGui::run() → 启动事件循环                            │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  Fl::run() [FLTK事件循环]                                   │
│  └→ 持续调用 openglWindow::draw()                          │
│      └→ drawContext::draw3d()                              │
│          └→ drawGeom() → 绘制几何实体                      │
└─────────────────────────────────────────────────────────────┘
                              ↓
                    ┌─────────────────┐
                    │   GUI 窗口显示   │
                    │   几何模型渲染   │
                    └─────────────────┘
```
