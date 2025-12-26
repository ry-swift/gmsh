# VS Code 配置文件详解

本文档详细记录 Gmsh 项目中 `.vscode` 目录下的配置文件，用于 VS Code 开发环境的配置。

## 目录结构

```
.vscode/
├── c_cpp_properties.json    # C/C++ 扩展配置（IntelliSense）
└── launch.json              # 调试配置
```

---

## 一、c_cpp_properties.json - C/C++ 智能提示配置

### 1.1 文件作用

这是 **VS Code C/C++ 扩展**（由 Microsoft 提供）的配置文件，主要用于：

- 配置 IntelliSense（代码智能提示、自动补全）
- 设置头文件搜索路径
- 定义预处理宏
- 指定 C/C++ 语言标准
- 配置编译器路径

### 1.2 完整配置内容

```json
{
  "configurations": [
    {
      "name": "Mac",
      "includePath": [
        "${workspaceFolder}/**",
        "${workspaceFolder}/src/**",
        "${workspaceFolder}/contrib/**",
        "/opt/homebrew/include/**"
      ],
      "defines": [
        "HAVE_OCC",
        "HAVE_OCC_CAF",
        "HAVE_FLTK"
      ],
      "macFrameworkPath": [
        "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks"
      ],
      "compilerPath": "/usr/bin/clang++",
      "cStandard": "c99",
      "cppStandard": "c++14",
      "intelliSenseMode": "macos-clang-arm64",
      "compileCommands": "${workspaceFolder}/build-debug/compile_commands.json"
    }
  ],
  "version": 4
}
```

### 1.3 各字段详细说明

#### 1.3.1 `name` - 配置名称

```json
"name": "Mac"
```

- **作用**：配置的显示名称
- **说明**：可以创建多个配置（如 Mac、Linux、Windows），通过 VS Code 状态栏切换
- **位置**：显示在 VS Code 右下角状态栏

#### 1.3.2 `includePath` - 头文件搜索路径

```json
"includePath": [
  "${workspaceFolder}/**",        // 项目根目录及所有子目录
  "${workspaceFolder}/src/**",    // 源码目录
  "${workspaceFolder}/contrib/**", // 第三方库目录
  "/opt/homebrew/include/**"      // Homebrew 安装的库头文件
]
```

| 路径 | 说明 |
|------|------|
| `${workspaceFolder}/**` | 项目根目录，`**` 表示递归搜索所有子目录 |
| `${workspaceFolder}/src/**` | Gmsh 源码目录 |
| `${workspaceFolder}/contrib/**` | 第三方依赖库（Eigen、CGNS、MED 等） |
| `/opt/homebrew/include/**` | Homebrew 安装的库（如 OpenCASCADE、FLTK） |

**特殊变量**：
- `${workspaceFolder}` - 当前工作区根目录
- `**` - 递归匹配所有子目录

#### 1.3.3 `defines` - 预定义宏

```json
"defines": [
  "HAVE_OCC",      // 启用 OpenCASCADE 几何内核
  "HAVE_OCC_CAF",  // 启用 OCCT CAF（应用框架）
  "HAVE_FLTK"      // 启用 FLTK GUI 库
]
```

这些宏告诉 IntelliSense 哪些条件编译分支是激活的：

| 宏名称 | 含义 | 影响的代码 |
|--------|------|-----------|
| `HAVE_OCC` | 启用 OpenCASCADE 支持 | `src/geo/OCC*.cpp` 中的 OCC 相关代码 |
| `HAVE_OCC_CAF` | 启用 OCCT 应用框架 | CAF 相关的高级功能 |
| `HAVE_FLTK` | 启用 FLTK GUI | `src/fltk/` 目录下的 GUI 代码 |

**示例**：在代码中的使用

```cpp
#if defined(HAVE_OCC)
  #include <BRepBuilderAPI_MakeVertex.hxx>
  // OpenCASCADE 相关代码
#endif
```

#### 1.3.4 `macFrameworkPath` - macOS Framework 路径

```json
"macFrameworkPath": [
  "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks"
]
```

- **作用**：指定 macOS Framework 的搜索路径
- **说明**：用于查找系统框架（如 OpenGL.framework、Cocoa.framework）
- **仅 macOS 有效**

#### 1.3.5 `compilerPath` - 编译器路径

```json
"compilerPath": "/usr/bin/clang++"
```

- **作用**：指定编译器路径
- **用途**：
  - IntelliSense 通过查询编译器获取系统头文件路径
  - 自动推断系统 include 路径
  - 确定编译器特定的预定义宏

#### 1.3.6 `cStandard` / `cppStandard` - 语言标准

```json
"cStandard": "c99",
"cppStandard": "c++14"
```

| 字段 | 值 | 说明 |
|------|-----|------|
| `cStandard` | `c99` | C 语言使用 C99 标准 |
| `cppStandard` | `c++14` | C++ 使用 C++14 标准（Gmsh 要求） |

**可选值**：
- C: `c89`, `c99`, `c11`, `c17`, `c23`
- C++: `c++98`, `c++11`, `c++14`, `c++17`, `c++20`, `c++23`

#### 1.3.7 `intelliSenseMode` - IntelliSense 模式

```json
"intelliSenseMode": "macos-clang-arm64"
```

格式：`<platform>-<compiler>-<architecture>`

| 组成部分 | 当前值 | 说明 |
|---------|--------|------|
| platform | `macos` | 操作系统 |
| compiler | `clang` | 编译器类型 |
| architecture | `arm64` | CPU 架构（Apple Silicon） |

**常见模式**：
- `macos-clang-arm64` - macOS Apple Silicon
- `macos-clang-x64` - macOS Intel
- `linux-gcc-x64` - Linux GCC
- `windows-msvc-x64` - Windows MSVC

#### 1.3.8 `compileCommands` - 编译数据库

```json
"compileCommands": "${workspaceFolder}/build-debug/compile_commands.json"
```

- **作用**：指定 CMake 生成的编译数据库路径
- **优先级**：此配置优先于 `includePath` 和 `defines`
- **生成方式**：CMake 配置时自动生成

**`compile_commands.json` 示例内容**：

```json
[
  {
    "directory": "/path/to/build-debug",
    "command": "/usr/bin/clang++ -DHAVE_OCC -I/path/to/include -o file.o -c file.cpp",
    "file": "/path/to/src/geo/GModel.cpp"
  }
]
```

**为什么重要**：提供每个源文件的精确编译参数，让 IntelliSense 最准确。

### 1.4 如何创建/修改此文件

1. **命令面板方式**：
   - `Cmd+Shift+P` → 输入 `C/C++: Edit Configurations (JSON)`

2. **UI 方式**：
   - `Cmd+Shift+P` → 输入 `C/C++: Edit Configurations (UI)`

3. **手动创建**：
   - 在 `.vscode/` 目录下创建 `c_cpp_properties.json`

---

## 二、launch.json - 调试配置

### 2.1 文件作用

这是 VS Code 的**调试配置文件**，定义了如何启动和调试程序，包括：

- 调试器类型
- 程序路径和参数
- 环境变量
- 源码映射
- 启动前任务

### 2.2 完整配置内容

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug Gmsh + OCC (IGES)",
      "type": "lldb",
      "request": "launch",
      "program": "${workspaceFolder}/build-debug/gmsh",
      "args": [
        "${workspaceFolder}/tutorials/hhh.igs"
      ],
      "cwd": "${workspaceFolder}",
      "env": {
        "DYLD_LIBRARY_PATH": "/Users/renyuxiao/Documents/gmsh/occt-debug-install/lib"
      },
      "sourceMap": {
        "/Users/renyuxiao/Documents/gmsh/OCCT": "/Users/renyuxiao/Documents/gmsh/OCCT",
        "/Users/renyuxiao/Documents/gmsh/OCCT/src": "/Users/renyuxiao/Documents/gmsh/OCCT/src",
        "/Users/renyuxiao/Documents/gmsh/OCCT/build-debug": "/Users/renyuxiao/Documents/gmsh/OCCT/build-debug"
      },
      "initCommands": [
        "settings set target.source-map /Users/renyuxiao/Documents/gmsh/OCCT /Users/renyuxiao/Documents/gmsh/OCCT"
      ],
      "stopOnEntry": false
    },
    {
      "name": "Debug Gmsh + OCC (当前文件)",
      "type": "lldb",
      "request": "launch",
      "program": "${workspaceFolder}/build-debug/gmsh",
      "args": [
        "${file}"
      ],
      "cwd": "${workspaceFolder}",
      "env": {
        "DYLD_LIBRARY_PATH": "/Users/renyuxiao/Documents/gmsh/occt-debug-install/lib"
      },
      "sourceMap": {
        "/Users/renyuxiao/Documents/gmsh/OCCT/src": "/Users/renyuxiao/Documents/gmsh/OCCT/src"
      },
      "stopOnEntry": false
    },
    {
      "name": "Debug Gmsh + OCC (生成2D网格)",
      "type": "lldb",
      "request": "launch",
      "program": "${workspaceFolder}/build-debug/gmsh",
      "args": [
        "${file}",
        "-2"
      ],
      "cwd": "${workspaceFolder}",
      "env": {
        "DYLD_LIBRARY_PATH": "/Users/renyuxiao/Documents/gmsh/occt-debug-install/lib"
      },
      "sourceMap": {
        "/Users/renyuxiao/Documents/gmsh/OCCT/src": "/Users/renyuxiao/Documents/gmsh/OCCT/src"
      }
    },
    {
      "name": "Debug Gmsh + OCC (cppdbg)",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build-debug/gmsh",
      "args": [
        "${workspaceFolder}/tutorials/hhh.igs"
      ],
      "cwd": "${workspaceFolder}",
      "environment": [
        {
          "name": "DYLD_LIBRARY_PATH",
          "value": "/Users/renyuxiao/Documents/gmsh/occt-debug-install/lib"
        }
      ],
      "MIMode": "lldb",
      "sourceFileMap": {
        "/Users/renyuxiao/Documents/gmsh/OCCT/src": "/Users/renyuxiao/Documents/gmsh/OCCT/src"
      },
      "setupCommands": [
        {
          "description": "Enable pretty-printing",
          "text": "-enable-pretty-printing",
          "ignoreFailures": true
        }
      ]
    },
    {
      "name": "Attach to Gmsh Process",
      "type": "lldb",
      "request": "attach",
      "pid": "${command:pickProcess}",
      "sourceMap": {
        "/Users/renyuxiao/Documents/gmsh/OCCT/src": "/Users/renyuxiao/Documents/gmsh/OCCT/src"
      }
    }
  ]
}
```

### 2.3 调试配置概览

| 配置名称 | 类型 | 用途 |
|---------|------|------|
| Debug Gmsh + OCC (IGES) | lldb/launch | 调试加载固定 IGES 文件 |
| Debug Gmsh + OCC (当前文件) | lldb/launch | 调试加载当前编辑的文件 |
| Debug Gmsh + OCC (生成2D网格) | lldb/launch | 调试 2D 网格生成 |
| Debug Gmsh + OCC (cppdbg) | cppdbg/launch | 使用 cppdbg 调试器 |
| Attach to Gmsh Process | lldb/attach | 附加到已运行的进程 |

### 2.4 通用字段说明

#### 2.4.1 基础字段

| 字段 | 说明 | 示例值 |
|------|------|--------|
| `name` | 配置名称，显示在调试下拉菜单 | `"Debug Gmsh + OCC (IGES)"` |
| `type` | 调试器类型 | `"lldb"` 或 `"cppdbg"` |
| `request` | 请求类型 | `"launch"` 启动 / `"attach"` 附加 |
| `program` | 要调试的程序路径 | `"${workspaceFolder}/build-debug/gmsh"` |
| `args` | 程序参数（数组） | `["${file}", "-2"]` |
| `cwd` | 工作目录 | `"${workspaceFolder}"` |
| `stopOnEntry` | 是否在入口点暂停 | `false` |

#### 2.4.2 调试器类型对比

| 类型 | 扩展 | 优势 | 劣势 |
|------|------|------|------|
| `lldb` | CodeLLDB | 原生 LLDB，macOS 最佳 | 仅限 macOS/Linux |
| `cppdbg` | C/C++ (Microsoft) | 跨平台，功能丰富 | macOS 上稍慢 |

### 2.5 各配置详解

#### 2.5.1 配置一：Debug Gmsh + OCC (IGES)

```json
{
  "name": "Debug Gmsh + OCC (IGES)",
  "type": "lldb",
  "request": "launch",
  "program": "${workspaceFolder}/build-debug/gmsh",
  "args": [
    "${workspaceFolder}/tutorials/hhh.igs"
  ],
  "cwd": "${workspaceFolder}",
  "env": {
    "DYLD_LIBRARY_PATH": "/Users/renyuxiao/Documents/gmsh/occt-debug-install/lib"
  },
  "sourceMap": {
    "/Users/renyuxiao/Documents/gmsh/OCCT": "/Users/renyuxiao/Documents/gmsh/OCCT",
    "/Users/renyuxiao/Documents/gmsh/OCCT/src": "/Users/renyuxiao/Documents/gmsh/OCCT/src"
  },
  "initCommands": [
    "settings set target.source-map /Users/renyuxiao/Documents/gmsh/OCCT /Users/renyuxiao/Documents/gmsh/OCCT"
  ],
  "stopOnEntry": false
}
```

**用途**：调试 Gmsh 加载固定的 IGES 文件（`hhh.igs`）

**关键配置**：

1. **`env.DYLD_LIBRARY_PATH`** - 动态库搜索路径
   ```json
   "env": {
     "DYLD_LIBRARY_PATH": "/Users/renyuxiao/Documents/gmsh/occt-debug-install/lib"
   }
   ```
   - macOS 上用于查找 OpenCASCADE 的 Debug 版动态库
   - 确保 Gmsh 能找到带调试符号的 OCCT 库

2. **`sourceMap`** - 源码映射
   ```json
   "sourceMap": {
     "/Users/renyuxiao/Documents/gmsh/OCCT/src": "/Users/renyuxiao/Documents/gmsh/OCCT/src"
   }
   ```
   - 作用：将编译时的源码路径映射到当前路径
   - 用途：当 OCCT 库在不同位置编译时，确保调试器能找到源码
   - 格式：`"编译时路径": "当前路径"`

3. **`initCommands`** - LLDB 初始化命令
   ```json
   "initCommands": [
     "settings set target.source-map /path/from /path/to"
   ]
   ```
   - 作用：在调试开始前执行 LLDB 命令
   - 此处设置 LLDB 的源码映射（与 `sourceMap` 功能类似）

#### 2.5.2 配置二：Debug Gmsh + OCC (当前文件)

```json
{
  "name": "Debug Gmsh + OCC (当前文件)",
  "type": "lldb",
  "request": "launch",
  "program": "${workspaceFolder}/build-debug/gmsh",
  "args": [
    "${file}"
  ],
  ...
}
```

**用途**：调试 Gmsh 加载当前编辑器中打开的文件

**特殊变量**：
- `${file}` - 当前编辑器中活动文件的完整路径

**使用场景**：
1. 打开一个 `.geo` 或 `.igs` 文件
2. 按 F5 启动调试
3. Gmsh 会自动加载该文件

#### 2.5.3 配置三：Debug Gmsh + OCC (生成2D网格)

```json
{
  "name": "Debug Gmsh + OCC (生成2D网格)",
  "args": [
    "${file}",
    "-2"
  ],
  ...
}
```

**用途**：调试 2D 网格生成过程

**参数说明**：
- `-2` - Gmsh 命令行参数，表示生成 2D 网格

**Gmsh 常用命令行参数**：

| 参数 | 说明 |
|------|------|
| `-1` | 生成 1D 网格 |
| `-2` | 生成 2D 网格 |
| `-3` | 生成 3D 网格 |
| `-o file.msh` | 输出网格文件 |
| `-format msh2` | 指定输出格式 |

#### 2.5.4 配置四：Debug Gmsh + OCC (cppdbg)

```json
{
  "name": "Debug Gmsh + OCC (cppdbg)",
  "type": "cppdbg",
  "request": "launch",
  "MIMode": "lldb",
  "environment": [
    {
      "name": "DYLD_LIBRARY_PATH",
      "value": "/Users/renyuxiao/Documents/gmsh/occt-debug-install/lib"
    }
  ],
  "sourceFileMap": {
    "/Users/renyuxiao/Documents/gmsh/OCCT/src": "/Users/renyuxiao/Documents/gmsh/OCCT/src"
  },
  "setupCommands": [
    {
      "description": "Enable pretty-printing",
      "text": "-enable-pretty-printing",
      "ignoreFailures": true
    }
  ]
}
```

**用途**：使用 Microsoft C/C++ 扩展的 cppdbg 调试器

**与 lldb 类型的区别**：

| 字段 | lldb 类型 | cppdbg 类型 |
|------|----------|-------------|
| 环境变量 | `env` | `environment` (数组格式) |
| 源码映射 | `sourceMap` | `sourceFileMap` |
| 初始化命令 | `initCommands` | `setupCommands` |
| 调试器模式 | 不需要 | `MIMode: "lldb"` |

**`setupCommands`**：
```json
"setupCommands": [
  {
    "description": "Enable pretty-printing",
    "text": "-enable-pretty-printing",
    "ignoreFailures": true
  }
]
```
- 启用 pretty-printing，美化 STL 容器的显示

#### 2.5.5 配置五：Attach to Gmsh Process

```json
{
  "name": "Attach to Gmsh Process",
  "type": "lldb",
  "request": "attach",
  "pid": "${command:pickProcess}",
  "sourceMap": {
    "/Users/renyuxiao/Documents/gmsh/OCCT/src": "/Users/renyuxiao/Documents/gmsh/OCCT/src"
  }
}
```

**用途**：附加到已经运行的 Gmsh 进程

**关键字段**：
- `request: "attach"` - 附加模式（而非启动模式）
- `pid: "${command:pickProcess}"` - 弹出进程选择器

**使用场景**：
1. 先在终端手动启动 Gmsh
2. 使用此配置附加调试器
3. 适用于调试启动后才出现的问题

### 2.6 常用 VS Code 变量

| 变量 | 说明 |
|------|------|
| `${workspaceFolder}` | 工作区根目录 |
| `${file}` | 当前打开文件的完整路径 |
| `${fileBasename}` | 当前文件名（含扩展名） |
| `${fileBasenameNoExtension}` | 当前文件名（不含扩展名） |
| `${fileDirname}` | 当前文件所在目录 |
| `${command:pickProcess}` | 显示进程选择器 |

### 2.7 如何创建/修改此文件

1. **命令面板方式**：
   - `Cmd+Shift+P` → `Debug: Open launch.json`

2. **调试面板方式**：
   - 点击左侧调试图标 → 点击齿轮图标

3. **手动创建**：
   - 在 `.vscode/` 目录下创建 `launch.json`

---

## 三、调试 Gmsh + OpenCASCADE 的完整流程

### 3.1 前置条件

1. **Debug 版 OpenCASCADE**
   ```bash
   cd OCCT
   mkdir build-debug && cd build-debug
   cmake .. -DCMAKE_BUILD_TYPE=Debug \
            -DCMAKE_INSTALL_PREFIX=xxx/occt-debug-install \
            -DBUILD_LIBRARY_TYPE=Shared
   make -j8 && make install
   ```

2. **Debug 版 Gmsh**
   ```bash
   cd gmsh-4.14.0-source
   mkdir build-debug && cd build-debug
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   make -j8
   ```

3. **生成 compile_commands.json**
   ```bash
   # CMake 自动生成，确保 build-debug 目录下存在此文件
   ls build-debug/compile_commands.json
   ```

### 3.2 调试步骤

1. **设置断点**
   - 在源码中点击行号左侧设置断点
   - 可以在 Gmsh 或 OpenCASCADE 源码中设置

2. **选择调试配置**
   - 按 `Cmd+Shift+D` 打开调试面板
   - 从下拉菜单选择配置

3. **启动调试**
   - 按 `F5` 或点击绿色播放按钮

4. **调试操作**
   - `F10` - 单步跳过
   - `F11` - 单步进入
   - `Shift+F11` - 单步跳出
   - `F5` - 继续执行

### 3.3 常见问题

#### Q1: 无法跳转到 OpenCASCADE 源码

**原因**：源码路径映射不正确

**解决**：检查 `sourceMap` 配置，确保路径正确：
```json
"sourceMap": {
  "/编译时的OCCT路径": "/当前OCCT源码路径"
}
```

#### Q2: 找不到动态库

**错误**：`dyld: Library not loaded: libTKernel.dylib`

**解决**：确保 `DYLD_LIBRARY_PATH` 设置正确：
```json
"env": {
  "DYLD_LIBRARY_PATH": "/path/to/occt-debug-install/lib"
}
```

#### Q3: IntelliSense 报错找不到头文件

**解决**：
1. 确保 `compile_commands.json` 存在
2. 检查 `c_cpp_properties.json` 中的路径
3. 执行 `Cmd+Shift+P` → `C/C++: Reset IntelliSense Database`

---

## 四、总结

| 文件 | 作用 | 关键配置 |
|------|------|----------|
| `c_cpp_properties.json` | IntelliSense 配置 | `compileCommands`, `includePath`, `defines` |
| `launch.json` | 调试配置 | `program`, `args`, `env`, `sourceMap` |

这两个文件是 VS Code 开发 C/C++ 项目的核心配置，正确配置后可以获得：
- 精准的代码智能提示
- 正确的代码跳转
- 完整的调试支持（包括第三方库源码调试）

---

*文档创建日期：2024-12-25*
*适用项目：Gmsh 4.14.0 + OpenCASCADE*
