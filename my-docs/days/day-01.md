# Day 1：环境准备与第一次运行

**所属周次**：第1周 - 环境搭建与初识Gmsh
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

成功编译Gmsh并运行第一个示例

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:30 | 30min | 阅读README.md和CREDITS.txt |
| 09:30-10:00 | 30min | 阅读CLAUDE.md了解架构概览 |
| 10:00-11:00 | 1h | 浏览官方文档，了解Gmsh功能全貌 |
| 11:00-12:00 | 1h | 编译Gmsh（cmake + make） |
| 14:00-14:30 | 30min | 运行GUI和t1.geo教程 |
| 14:30-15:00 | 30min | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 阅读项目README.md和CREDITS.txt，了解项目历史
- [ ] 阅读CLAUDE.md了解项目架构概览
- [ ] 浏览官方文档：https://gmsh.info/doc/texinfo/gmsh.html

---

## 下午任务（2小时）

- [ ] 编译Gmsh：

```bash
cd /Users/renyuxiao/Documents/gmsh/gmsh-4.14.0-source
mkdir build && cd build
cmake -DENABLE_BUILD_DYNAMIC=1 ..
make -j4
```

- [ ] 运行GUI验证：`./gmsh`
- [ ] 运行第一个教程：`./gmsh ../tutorials/t1.geo`

---

## 练习作业

### 1. 【基础】GUI操作练习

在GUI中打开t1.geo，观察几何和网格的生成过程

**操作步骤**：
1. 启动 `./gmsh`
2. File -> Open -> 选择 `tutorials/t1.geo`
3. 点击左侧 Modules -> Mesh -> 2D 生成网格
4. 观察网格生成过程

### 2. 【进阶】编译选项实验

修改cmake命令，尝试使用`-DENABLE_FLTK=0`编译无GUI版本，对比差异

```bash
mkdir build-nogui && cd build-nogui
cmake -DENABLE_BUILD_DYNAMIC=1 -DENABLE_FLTK=0 ..
make -j4
# 对比两个版本的可执行文件大小
ls -lh gmsh ../build/gmsh
```

### 3. 【挑战】命令行参数学习

查看`gmsh --help`输出，记录至少10个有用的命令行参数

**参考答案**：
| 参数 | 作用 |
|------|------|
| `-1` | 生成1D网格 |
| `-2` | 生成2D网格 |
| `-3` | 生成3D网格 |
| `-format msh` | 指定输出格式 |
| `-o filename` | 指定输出文件名 |
| `-clmax val` | 设置最大网格尺寸 |
| `-clmin val` | 设置最小网格尺寸 |
| `-v n` | 设置详细程度（0-99） |
| `-nopopup` | 不显示弹出窗口 |
| `-setnumber name val` | 设置参数值 |

---

## 今日检查点

- [ ] 能成功执行`./gmsh --version`显示版本号
- [ ] 能在GUI中打开t1.geo并生成2D网格
- [ ] 理解Gmsh的四大模块（几何、网格、求解器、后处理）

---

## 核心概念

### Gmsh四大模块

1. **几何模块（Geometry）**：定义CAD几何形状
2. **网格模块（Mesh）**：生成有限元网格
3. **求解器模块（Solver）**：求解物理方程（可选）
4. **后处理模块（Post-processing）**：可视化结果

### 项目架构

```
src/
├── common/   - 通用工具
├── geo/      - 几何模块
├── mesh/     - 网格模块
├── post/     - 后处理
├── solver/   - 求解器
├── numeric/  - 数值计算
├── parser/   - 脚本解析
├── graphics/ - 渲染
├── fltk/     - GUI
└── plugin/   - 插件
```

---

## 产出

- 编译成功，能打开GUI界面

---

## 相关文件

- `README.md` - 项目说明
- `CREDITS.txt` - 致谢名单
- `CLAUDE.md` - 架构概览
- `tutorials/t1.geo` - 第一个教程

---

## 导航

- **上一天**：无（第一天）
- **下一天**：[Day 2 - GEO脚本语言基础（一）](day-02.md)
- **返回目录**：[学习计划总览](../study.md)
