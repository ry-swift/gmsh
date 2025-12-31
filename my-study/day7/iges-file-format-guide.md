# IGES 文件格式完整指南

> 本文档详细介绍 IGES (Initial Graphics Exchange Specification) 文件格式标准，
> 帮助理解 OCCT igesread 模块的解析逻辑。

## 目录

1. [IGES 格式概述](#1-iges-格式概述)
2. [文件结构详解](#2-文件结构详解)
3. [行格式规范](#3-行格式规范)
4. [实体类型编码表](#4-实体类型编码表)
5. [实例文件解析](#5-实例文件解析)
6. [常见问题与处理](#6-常见问题与处理)

---

## 1. IGES 格式概述

### 1.1 什么是 IGES

IGES (Initial Graphics Exchange Specification) 是一种中性文件格式，用于在不同 CAD/CAM/CAE 系统之间交换产品数据。

**历史发展:**
- 1980年: IGES 1.0 发布
- 1988年: IGES 4.0 (最广泛使用的版本)
- 1996年: IGES 5.3 (最终版本)
- 现状: 虽然 STEP 格式逐渐取代 IGES，但 IGES 仍广泛用于遗留系统和2D数据交换

**主要特点:**
- ASCII 文本格式，人类可读
- 固定行宽 (80 字符)
- 支持 2D/3D 几何、注释、结构关系
- 广泛的 CAD 系统支持

### 1.2 IGES 文件的五段结构

```
┌────────────────────────────────────────────────────────────────────────┐
│                         IGES 文件结构                                  │
├────────────────────────────────────────────────────────────────────────┤
│                                                                        │
│   ┌────────────────────────────────────────────────────────────────┐  │
│   │ S段 (Start Section)                                            │  │
│   │ ─────────────────────                                          │  │
│   │ 描述性文本，通常包含:                                           │  │
│   │ · 创建系统/软件信息                                             │  │
│   │ · 创建者/公司信息                                               │  │
│   │ · 文件描述                                                     │  │
│   │ 可选段，可以为空                                                │  │
│   └────────────────────────────────────────────────────────────────┘  │
│                                                                        │
│   ┌────────────────────────────────────────────────────────────────┐  │
│   │ G段 (Global Section)                                           │  │
│   │ ─────────────────────                                          │  │
│   │ 25个参数，定义文件级别信息:                                      │  │
│   │ · 分隔符和字符串定界符                                           │  │
│   │ · 单位、精度、尺度                                               │  │
│   │ · 模型空间范围                                                  │  │
│   │ · 创建日期/时间                                                 │  │
│   │ 必选段，至少1行                                                 │  │
│   └────────────────────────────────────────────────────────────────┘  │
│                                                                        │
│   ┌────────────────────────────────────────────────────────────────┐  │
│   │ D段 (Directory Entry Section)                                  │  │
│   │ ──────────────────────────────                                 │  │
│   │ 实体目录，每个实体占 2 行:                                       │  │
│   │ · 奇数行: 实体类型、参数指针、属性等                             │  │
│   │ · 偶数行: 补充属性、标签                                        │  │
│   │ 必选段，行数为偶数                                              │  │
│   └────────────────────────────────────────────────────────────────┘  │
│                                                                        │
│   ┌────────────────────────────────────────────────────────────────┐  │
│   │ P段 (Parameter Data Section)                                   │  │
│   │ ───────────────────────────────                                │  │
│   │ 实体参数数据:                                                   │  │
│   │ · 以实体类型号开头                                               │  │
│   │ · 逗号分隔的参数列表                                             │  │
│   │ · 分号结束                                                      │  │
│   │ 必选段                                                          │  │
│   └────────────────────────────────────────────────────────────────┘  │
│                                                                        │
│   ┌────────────────────────────────────────────────────────────────┐  │
│   │ T段 (Terminate Section)                                        │  │
│   │ ──────────────────────                                         │  │
│   │ 单行，统计各段行数:                                              │  │
│   │ S<nnn>G<nnn>D<nnn>P<nnn>                                       │  │
│   │ 必选段，仅1行                                                   │  │
│   └────────────────────────────────────────────────────────────────┘  │
│                                                                        │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 文件结构详解

### 2.1 Start Section (S段)

**用途:** 存储文件描述性注释文本

**格式:**
```
列位置:  1-72           73    74-80
内容:    自由文本        S     行号
```

**示例:**
```
                                                                        S      1
This file was created by Example CAD System v3.2                       S      2
Part: Assembly_001                                                      S      3
Creator: John Doe                                                       S      4
```

**特点:**
- 列 1-72: 自由格式文本，无特定结构要求
- 可以包含多行
- 可选（某些系统生成的文件可能没有 S 段）

### 2.2 Global Section (G段)

**用途:** 定义文件级参数

**格式:**
```
列位置:  1-72                      73    74-80
内容:    用分隔符分开的参数序列     G     行号
```

**25个全局参数详解:**

| 序号 | 名称 | 类型 | 说明 | 示例 |
|------|------|------|------|------|
| 1 | Parameter Delimiter | 字符 | 参数分隔符 | `1H,` (逗号) |
| 2 | Record Delimiter | 字符 | 记录结束符 | `1H;` (分号) |
| 3 | Product ID Sending System | 字符串 | 发送系统产品ID | `8HMyModel` |
| 4 | File Name | 字符串 | 文件名 | `12Hmyfile.igs` |
| 5 | Native System ID | 字符串 | 原生系统标识 | `10HCadSystem` |
| 6 | Preprocessor Version | 字符串 | 预处理器版本 | `5Hv1.0` |
| 7 | Integer Bits | 整数 | 整数位数 | `32` |
| 8 | Single Precision Max Power | 整数 | 单精度最大幂 | `38` |
| 9 | Single Precision Significant Digits | 整数 | 单精度有效位数 | `6` |
| 10 | Double Precision Max Power | 整数 | 双精度最大幂 | `308` |
| 11 | Double Precision Significant Digits | 整数 | 双精度有效位数 | `15` |
| 12 | Product ID Receiving System | 字符串 | 接收系统产品ID | `8HMyModel` |
| 13 | Model Space Scale | 实数 | 模型空间比例 | `1.0` |
| 14 | Units Flag | 整数 | 单位标志 | `1` (英寸) |
| 15 | Units Name | 字符串 | 单位名称 | `2HIN` (英寸) |
| 16 | Maximum Number of Line Weight Gradations | 整数 | 线宽等级数 | `1` |
| 17 | Width of Maximum Line Weight | 实数 | 最大线宽 | `0.02` |
| 18 | Date and Time of Exchange File Generation | 字符串 | 创建日期时间 | `15H20231201.120000` |
| 19 | Minimum User-Intended Resolution | 实数 | 最小分辨率 | `1.0E-6` |
| 20 | Approximate Maximum Coordinate Value | 实数 | 最大坐标近似值 | `1000.0` |
| 21 | Name of Author | 字符串 | 作者姓名 | `8HJohnDoe` |
| 22 | Author's Organization | 字符串 | 组织名称 | `10HMyCompany` |
| 23 | Version Flag | 整数 | IGES 版本标志 | `11` (IGES 5.3) |
| 24 | Drafting Standard Flag | 整数 | 制图标准标志 | `0` |
| 25 | Date and Time Model was Created or Modified | 字符串 | 模型修改日期 | `15H20231201.100000` |

**单位标志 (参数14) 取值:**

| 值 | 单位 | 说明 |
|----|------|------|
| 1 | Inch | 英寸 |
| 2 | Millimeter | 毫米 |
| 3 | (Reserved) | 保留 |
| 4 | Feet | 英尺 |
| 5 | Miles | 英里 |
| 6 | Meters | 米 |
| 7 | Kilometers | 千米 |
| 8 | Mils | 密耳 (千分之一英寸) |
| 9 | Microns | 微米 |
| 10 | Centimeters | 厘米 |
| 11 | Microinches | 微英寸 |

### 2.3 Directory Entry Section (D段)

**用途:** 为每个实体建立索引/目录

**格式:** 每个实体占用两行，共18个字段 (17个数值 + 1个标签组)

**第一行 (奇数行号):**
```
列位置:  1-8   9-16  17-24  25-32  33-40  41-48  49-56  57-64  65-66 67-68 69-70 71-72  73   74-80
字段名:  Type  Para  Struct Line   Level  View   Trans  Label  Status Status Status Status  D   行号
字段号:  1     2     3      4      5      6      7      8      9a    9b    9c    9d
```

**第二行 (偶数行号):**
```
列位置:  1-8   9-16  17-24  25-32  33-40  41-48  49-56  57-64  65-72  73   74-80
字段名:  Type  Line  Color  Para   Form   Res1   Res2   Label  Subsc   D    行号
字段号:  10    11    12     13     14     15     16     17     18
```

**字段详细说明:**

| 字段号 | 名称 | 列位置 | 说明 |
|--------|------|--------|------|
| 1 | Entity Type | 1-8 | 实体类型号 (见第4节) |
| 2 | Parameter Data | 9-16 | P段参数起始行号 |
| 3 | Structure | 17-24 | 结构定义指针 (通常0) |
| 4 | Line Font | 25-32 | 线型: 0=默认, 1=实线, 2=虚线... |
| 5 | Level | 33-40 | 层号或层属性指针 |
| 6 | View | 41-48 | 视图指针 |
| 7 | Transformation Matrix | 49-56 | 变换矩阵指针 |
| 8 | Label Display | 57-64 | 标签显示关联指针 |
| 9a | Status: Blank | 65-66 | 空白状态: 0=可见, 1=隐藏 |
| 9b | Status: Subordinate | 67-68 | 从属性: 0=独立, 1=物理, 2=逻辑 |
| 9c | Status: Use | 69-70 | 使用标志: 0=几何, 1=注释... |
| 9d | Status: Hierarchy | 71-72 | 层次: 0=全局顶, 1=延迟... |
| 10 | Entity Type (repeat) | 1-8 | 实体类型号 (验证用) |
| 11 | Line Weight | 9-16 | 线粗等级 |
| 12 | Color | 17-24 | 颜色号或颜色定义指针 |
| 13 | Parameter Line Count | 25-32 | 参数占用的P段行数 |
| 14 | Form | 33-40 | 形式号 (类型的变体) |
| 15 | Reserved | 41-48 | 保留字段 |
| 16 | Reserved | 49-56 | 保留字段 |
| 17 | Entity Label | 57-64 | 实体标签 (用户名称) |
| 18 | Entity Subscript | 65-72 | 实体下标 |

### 2.4 Parameter Data Section (P段)

**用途:** 存储每个实体的详细参数数据

**格式:**
```
列位置:  1-64                      65-72    73   74-80
内容:    参数数据 (逗号分隔)        D段指针   P    行号
```

**参数格式规则:**
- 以实体类型号开头
- 参数用逗号分隔
- 记录用分号结束
- 字符串使用 Hollerith 格式: `nH<n个字符>`

**示例:**
```
110,0.0,0.0,0.0,1.0,1.0,1.0;                                           1P      1
```
解读: Type 110 (直线), 起点 (0,0,0), 终点 (1,1,1)

### 2.5 Terminate Section (T段)

**用途:** 文件结束标志，包含各段行数统计

**格式:**
```
S      1G      1D     10P      5                                        T      1
```
解读: S段1行, G段1行, D段10行, P段5行

---

## 3. 行格式规范

### 3.1 固定行宽

所有 IGES 行严格固定为 **80 字符**:

```
┌──────────────────────────────────────────────────────────────────────────────┐
│          数据区 (72字符)                │段│  行号 (7字符)                    │
│  列 1-72                               │73│  列 74-80                        │
└──────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Hollerith 字符串格式

IGES 使用 Hollerith 格式表示字符串:

```
格式: nH<n个字符>

示例:
  4HLINE    → "LINE"
  10HHelloWorld → "HelloWorld"
  0H        → "" (空字符串)

跨行处理:
  如果字符串超过一行末尾，继续到下一行开头
  计数包含实际字符，不包含行末空格
```

### 3.3 数值格式

```
整数:      右对齐，前导空格
           示例: "     110" (类型号)

实数:      支持多种格式
           · 定点: 3.14159
           · 科学计数法 (E): 1.0E-6
           · FORTRAN 格式 (D): 1.0D+3

空参数:    两个连续分隔符之间无内容
           示例: "1,,3" (第2个参数为空)
```

### 3.4 特殊字符

| 字符 | 默认用途 | 可自定义 |
|------|---------|---------|
| `,` | 参数分隔符 | 是 (G段参数1) |
| `;` | 记录结束符 | 是 (G段参数2) |
| `H` | Hollerith 标记 | 否 |
| `S/G/D/P/T` | 段标识符 | 否 |

---

## 4. 实体类型编码表

### 4.1 曲线实体 (100-199)

| 类型号 | 名称 | 说明 |
|--------|------|------|
| 100 | Circular Arc | 圆弧 |
| 102 | Composite Curve | 复合曲线 |
| 104 | Conic Arc | 圆锥曲线弧 |
| 106 | Copious Data | 大量数据 (点、向量等) |
| 108 | Plane | 平面 |
| 110 | Line | 直线 |
| 112 | Parametric Spline Curve | 参数样条曲线 |
| 114 | Parametric Spline Surface | 参数样条曲面 |
| 116 | Point | 点 |
| 118 | Ruled Surface | 直纹面 |
| 120 | Surface of Revolution | 旋转曲面 |
| 122 | Tabulated Cylinder | 平移曲面 |
| 124 | Transformation Matrix | 变换矩阵 |
| 126 | Rational B-Spline Curve | 有理 B 样条曲线 |
| 128 | Rational B-Spline Surface | 有理 B 样条曲面 |
| 130 | Offset Curve | 偏移曲线 |
| 140 | Offset Surface | 偏移曲面 |
| 141 | Boundary | 边界 |
| 142 | Curve on a Parametric Surface | 参数曲面上的曲线 |
| 143 | Bounded Surface | 有界曲面 |
| 144 | Trimmed (Parametric) Surface | 裁剪曲面 |

### 4.2 CSG 和 B-Rep 实体 (150-199, 180-199)

| 类型号 | 名称 | 说明 |
|--------|------|------|
| 150 | Block | 块体 |
| 152 | Right Angular Wedge | 直角楔 |
| 154 | Right Circular Cylinder | 正圆柱 |
| 156 | Right Circular Cone Frustum | 正圆锥台 |
| 158 | Sphere | 球体 |
| 160 | Torus | 环面 |
| 162 | Solid of Revolution | 旋转体 |
| 164 | Solid of Linear Extrusion | 拉伸体 |
| 168 | Ellipsoid | 椭球体 |
| 180 | Boolean Tree | 布尔树 |
| 182 | Selected Component | 选择组件 |
| 184 | Solid Assembly | 实体装配 |
| 186 | Manifold Solid B-Rep | 流形实体 B-Rep |

### 4.3 注释实体 (200-299)

| 类型号 | 名称 | 说明 |
|--------|------|------|
| 202 | Angular Dimension | 角度标注 |
| 206 | Diameter Dimension | 直径标注 |
| 208 | Flag Note | 标志注释 |
| 210 | General Label | 通用标签 |
| 212 | General Note | 通用注释 |
| 214 | Leader (Arrow) | 引线 (箭头) |
| 216 | Linear Dimension | 线性标注 |
| 218 | Ordinate Dimension | 坐标标注 |
| 220 | Point Dimension | 点标注 |
| 222 | Radius Dimension | 半径标注 |
| 228 | General Symbol | 通用符号 |
| 230 | Sectioned Area | 剖面区域 |

### 4.4 结构实体 (300-399, 400-499)

| 类型号 | 名称 | 说明 |
|--------|------|------|
| 302 | Associativity Definition | 关联定义 |
| 304 | Line Font Definition | 线型定义 |
| 306 | Macro Definition | 宏定义 |
| 308 | Subfigure Definition | 子图定义 |
| 310 | Text Font Definition | 文本字体定义 |
| 312 | Text Display Template | 文本显示模板 |
| 314 | Color Definition | 颜色定义 |
| 316 | Units Data | 单位数据 |
| 320 | Network Subfigure Definition | 网络子图定义 |
| 322 | Attribute Table Definition | 属性表定义 |

### 4.5 常用实体参数说明

**Type 110: Line (直线)**
```
参数: 110, X1, Y1, Z1, X2, Y2, Z2;
       │   │   │   │   │   │   │
       │   └───┴───┴───┘   └───┴───┘
       │      起点坐标        终点坐标
       类型号
```

**Type 116: Point (点)**
```
参数: 116, X, Y, Z, PTR;
       │   │  │  │  │
       │   └──┴──┴──┘
       │   点坐标 (X,Y,Z)
       类型号

PTR: 子图定义指针 (通常为0)
```

**Type 126: Rational B-Spline Curve (有理B样条曲线)**
```
参数: 126, K, M, PROP1, PROP2, PROP3, PROP4,
          T(0), T(1), ..., T(N),           节点向量
          W(0), W(1), ..., W(K),           权重
          X(0), Y(0), Z(0),                控制点
          X(1), Y(1), Z(1),
          ...,
          X(K), Y(K), Z(K),
          V(0), V(1),                      参数范围
          XNORM, YNORM, ZNORM;             法向量

K: 上界索引 (控制点数-1)
M: 次数
N: K + M + 1 (节点数-1)
PROP1-4: 属性标志
```

**Type 144: Trimmed Surface (裁剪曲面)**
```
参数: 144, PTS, N1, N2, PTO, PTI(1), ..., PTI(N2);
       │    │   │   │   │    │
       │    │   │   │   │    内部裁剪曲线指针数组
       │    │   │   │   外边界曲线指针 (0=整个曲面)
       │    │   │   内部边界数量
       │    │   外边界标志: 0=无, 1=有
       │    被裁剪曲面指针
       类型号
```

---

## 5. 实例文件解析

### 5.1 最简单的 IGES 文件 (一条直线)

```
                                                                        S      1
1H,,1H;,7Hexample,11Hexample.igs,32HExample CAD System Version 1.0,     G      1
16HVersion 1.0,32,38,6,308,15,7Hexample,1.0,2,2HMM,1,0.1,               G      2
15H20231201.120000,0.001,1000.0,6HAuthor,10HMyCompany,11,0,            G      3
15H20231201.100000;                                                     G      4
     110       1       0       0       0       0       0       0 0 0 0 0D      1
     110       0       0       1       0                        LINE    D      2
110,0.0,0.0,0.0,10.0,10.0,0.0;                                         1P      1
S      1G      4D      2P      1                                        T      1
```

**逐段解析:**

**S段 (第1行):**
- 空描述，仅保留段标识

**G段 (第2-5行):**
- `1H,` - 参数分隔符为逗号
- `1H;` - 记录分隔符为分号
- `7Hexample` - 产品ID
- `11Hexample.igs` - 文件名
- ... (其余全局参数)

**D段 (第6-7行):**
- `110` - 实体类型 (直线)
- `1` - P段起始行号
- 其他属性均为默认值
- `LINE` - 实体标签

**P段 (第8行):**
- `110` - 类型号
- `0.0,0.0,0.0` - 起点 (0,0,0)
- `10.0,10.0,0.0` - 终点 (10,10,0)
- `1` - 对应D段行号

**T段 (第9行):**
- `S1 G4 D2 P1` - 各段行数统计

### 5.2 包含多个实体的文件

```
This file contains multiple geometric entities                          S      1
1H,,1H;,6Hmulti,10Hmulti.igs,...                                       G      1
(G段其他参数省略)                                                        G      2
     110       1       0       0       0       0       0       0 0 0 0 0D      1
     110       0       0       1       0                        LINE1   D      2
     116       2       0       0       0       0       0       0 0 0 0 0D      3
     116       0       0       1       0                        POINT1  D      4
     100       3       0       0       0       0       0       0 0 0 0 0D      5
     100       0       0       1       0                        ARC1    D      6
110,0.0,0.0,0.0,10.0,0.0,0.0;                                          1P      1
116,5.0,5.0,0.0,0;                                                      3P      2
100,0.0,0.0,0.0,5.0,5.0,0.0,0.0;                                        5P      3
S      1G      2D      6P      3                                        T      1
```

**实体列表:**
1. LINE1 (Type 110): 从 (0,0,0) 到 (10,0,0) 的直线
2. POINT1 (Type 116): 位于 (5,5,0) 的点
3. ARC1 (Type 100): 圆弧 (圆心在原点，半径5)

---

## 6. 常见问题与处理

### 6.1 格式变体

| 变体 | 特征 | 处理方法 |
|------|------|---------|
| 标准 ASCII | 固定80字符行 | 正常解析 |
| FNES 加密 | 高位设置+XOR | modefnes=1 |
| 压缩格式 | 非固定行宽 | 需要预处理 |
| DOS 行尾 | CR+LF | 跳过 CR |
| Mac 行尾 | 仅 CR | 特殊处理 |
| Unicode BOM | 文件头有 BOM | 跳过前3字节 |

### 6.2 常见错误

| 错误 | 原因 | igesread 处理 |
|------|------|---------------|
| 第73列非段标识 | 格式错误 | 尝试相邻列 |
| 行号不连续 | 生成器问题 | 警告并继续 |
| 缺少T段 | 文件不完整 | 警告 (XSTEP_20) |
| 分隔符异常 | G段解析问题 | 从第一行推断 |

### 6.3 调试技巧

```bash
# 查看文件基本信息
head -20 file.igs

# 检查行宽
awk '{ print length }' file.igs | sort -n | uniq -c

# 统计各段行数
grep -c 'S[[:space:]]*[0-9]*$' file.igs
grep -c 'G[[:space:]]*[0-9]*$' file.igs
grep -c 'D[[:space:]]*[0-9]*$' file.igs
grep -c 'P[[:space:]]*[0-9]*$' file.igs
grep -c 'T[[:space:]]*[0-9]*$' file.igs

# 查看实体类型分布
grep '^[[:space:]]*[0-9]' file.igs | grep 'D[[:space:]]*[0-9]*$' | \
  awk '{ print substr($0,1,8) }' | sort | uniq -c | sort -rn
```

---

## 附录: 参考资料

1. **IGES 5.3 规范** (NIST)
   - 官方完整规范文档
   - 包含所有实体类型的详细定义

2. **OpenCASCADE 文档**
   - IGES 读取模块 API 说明
   - 实体映射关系

3. **相关 OCCT 源码**
   - `TKDEIGES/IGESFile/` - 底层读取
   - `TKIGESData/` - 数据模型
   - `TKIGESToBRep/` - 转换到 B-Rep

---

*文档生成时间: 2024-12-31*
*基于 IGES 5.3 规范和 OpenCASCADE Technology 源码*
