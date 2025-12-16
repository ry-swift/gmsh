# 附录B: 命令行选项

本附录提供Gmsh命令行选项的完整参考。

## 基本用法

```bash
gmsh [options] [file(s)]
```

## 通用选项

| 选项 | 描述 |
|------|------|
| `-help` | 显示帮助信息 |
| `-version` | 显示版本信息 |
| `-info` | 显示详细版本和配置信息 |
| `-v int` | 设置详细程度 (0-99) |
| `-nopopup` | 不显示弹出对话框 |

## 几何选项

| 选项 | 描述 |
|------|------|
| `-0` | 解析输入文件，输出未修改的几何 |
| `-tol float` | 设置几何容差 |

## 网格选项

| 选项 | 描述 |
|------|------|
| `-1` | 生成1D网格 |
| `-2` | 生成2D网格 |
| `-3` | 生成3D网格 |
| `-refine` | 细化网格 |
| `-barycentric_refine` | 重心细化 |
| `-part int` | 分区网格为n个部分 |
| `-part_weight [tri,quad,tet,...]` | 设置分区权重 |
| `-part_split` | 将分区保存到单独文件 |
| `-preserve_numbering_msh2` | 保留MSH2编号 |

### 网格算法

| 选项 | 描述 |
|------|------|
| `-algo string` | 2D算法 (auto, meshadapt, del2d, front2d, delquad, pack) |
| `-algo3d string` | 3D算法 (auto, del3d, front3d, mmg3d, hxt) |
| `-smooth int` | 光滑迭代次数 |
| `-order int` | 元素阶次 (1, 2, ...) |
| `-optimize[_netgen]` | 优化网格 |
| `-optimize_threshold float` | 优化阈值 |
| `-optimize_ho` | 优化高阶网格 |
| `-ho_min float` | 高阶网格最小雅可比 |
| `-ho_max float` | 高阶网格最大雅可比 |

### 网格尺寸

| 选项 | 描述 |
|------|------|
| `-clscale float` | 全局网格尺寸因子 |
| `-clmin float` | 最小网格尺寸 |
| `-clmax float` | 最大网格尺寸 |
| `-anisoMax float` | 最大各向异性比 |
| `-clcurv` | 根据曲率自动计算尺寸 |

### 网格格式

| 选项 | 描述 |
|------|------|
| `-format string` | 输出格式 (auto, msh, msh1, msh2, msh22, msh3, msh4, msh41, unv, vtk, nas, p3d, stl, mesh, bdf, cgns, med, ir3, inp, ply2, celum, su2, x3d, dat, neu, m) |
| `-bin` | 二进制输出 |
| `-ascii` | ASCII输出 |
| `-save_all` | 保存所有元素 |
| `-save_parametric` | 保存参数坐标 |
| `-save_topology` | 保存拓扑信息 |

## 后处理选项

| 选项 | 描述 |
|------|------|
| `-link int` | 链接模型视图 (0=no, 1=yes) |
| `-combine` | 合并视图 |

## 显示选项

| 选项 | 描述 |
|------|------|
| `-n` | 隐藏所有网格 |
| `-display string` | 设置X显示 |
| `-fontsize int` | 字体大小 |

## 输出选项

| 选项 | 描述 |
|------|------|
| `-o file` | 指定输出文件 |
| `-new` | 创建新模型 |
| `-merge` | 合并到当前模型 |

## 其他选项

| 选项 | 描述 |
|------|------|
| `-string "string"` | 解析命令字符串 |
| `-setnumber name value` | 设置数值常量 |
| `-setstring name value` | 设置字符串常量 |
| `-nthreads int` | 设置线程数 |
| `-nt int` | 同 -nthreads |
| `-cpu` | 报告CPU时间 |
| `-run` | 运行脚本 |
| `-bg file` | 加载背景网格 |
| `-pid` | 输出进程ID |
| `-check` | 只检查交互式输入 |
| `-watch pattern` | 监视文件变化 |
| `-socket name` | 从套接字请求选项 |

## 常用示例

### 基本网格生成

```bash
# 生成2D网格
gmsh model.geo -2

# 生成3D网格
gmsh model.geo -3

# 生成并保存
gmsh model.geo -3 -o output.msh
```

### 网格尺寸控制

```bash
# 设置全局尺寸因子
gmsh model.geo -3 -clscale 0.5

# 设置尺寸范围
gmsh model.geo -3 -clmin 0.1 -clmax 1.0

# 启用曲率自适应
gmsh model.geo -3 -clcurv
```

### 网格算法选择

```bash
# 2D Frontal-Delaunay
gmsh model.geo -2 -algo front2d

# 3D HXT并行
gmsh model.geo -3 -algo3d hxt -nt 8
```

### 网格优化

```bash
# 基本优化
gmsh model.geo -3 -optimize

# Netgen优化
gmsh model.geo -3 -optimize_netgen

# 高阶网格优化
gmsh model.geo -3 -order 2 -optimize_ho
```

### 格式转换

```bash
# MSH转VTK
gmsh input.msh -o output.vtk

# MSH转Abaqus INP
gmsh input.msh -o output.inp -format inp

# 设置MSH版本
gmsh model.geo -3 -format msh22 -o output.msh

# 二进制输出
gmsh model.geo -3 -bin -o output.msh
```

### 网格分区

```bash
# 分区为4个部分
gmsh model.msh -part 4 -o partitioned.msh

# 分区并保存到单独文件
gmsh model.msh -part 4 -part_split
```

### 批量处理

```bash
# 合并多个文件
gmsh file1.msh file2.msh -merge -o merged.msh

# 从STEP生成网格
gmsh model.step -3 -o mesh.msh

# 处理多个文件
for f in *.geo; do
    gmsh "$f" -3 -o "${f%.geo}.msh"
done
```

### 参数化脚本

```bash
# 设置脚本参数
gmsh model.geo -3 -setnumber L 10 -setnumber H 5

# 执行字符串命令
gmsh model.geo -3 -string "Mesh.CharacteristicLengthMax = 0.5;"
```

### 后处理

```bash
# 加载网格和视图
gmsh mesh.msh view.pos

# 合并视图
gmsh view1.pos view2.pos -combine -o combined.pos
```

### 调试和信息

```bash
# 详细输出
gmsh model.geo -3 -v 99

# 显示CPU时间
gmsh model.geo -3 -cpu

# 只检查语法
gmsh model.geo -check
```

## 环境变量

| 变量 | 描述 |
|------|------|
| `GMSH_OPTIONS` | 默认命令行选项 |
| `GMSH_HOME` | Gmsh主目录 |
| `HOME` | 用户主目录（配置文件位置） |

## 配置文件

Gmsh按以下顺序读取配置：

1. `$GMSH_HOME/.gmshrc` 或 `$HOME/.gmshrc`
2. 当前目录的 `.gmshrc`
3. 命令行选项

### 示例.gmshrc

```
// 默认设置
Mesh.Algorithm = 6;
Mesh.Algorithm3D = 1;
Mesh.OptimizeNetgen = 1;
General.NumThreads = 4;
General.Verbosity = 3;
```

## Python调用命令行

```python
import subprocess
import os

def run_gmsh(input_file, output_file, dimension=3, options=None):
    """
    运行Gmsh命令行

    参数:
        input_file: 输入文件
        output_file: 输出文件
        dimension: 网格维度 (1, 2, 3)
        options: 额外选项列表
    """
    cmd = ['gmsh', input_file, f'-{dimension}', '-o', output_file]

    if options:
        cmd.extend(options)

    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        print(f"Error: {result.stderr}")
    else:
        print(f"Success: {output_file}")

    return result.returncode

# 使用示例
run_gmsh('model.geo', 'mesh.msh', 3, ['-clscale', '0.5', '-optimize'])
```

## 小结

Gmsh命令行工具适用于：

1. **批量处理**：脚本化网格生成
2. **格式转换**：不同格式间转换
3. **参数化研究**：通过参数生成不同网格
4. **服务器部署**：无GUI环境

常用组合：
- `-3 -o output.msh`：基本3D网格生成
- `-3 -clscale 0.5 -optimize`：细化并优化
- `-3 -algo3d hxt -nt 8`：并行网格生成
- `-format msh22 -bin`：兼容格式输出
