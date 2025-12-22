# Day 4：循环与控制结构

**所属周次**：第1周 - 环境搭建与初识Gmsh
**所属阶段**：第一阶段 - 基础入门

---

## 学习目标

学习For循环和条件判断

---

## 学习文件

- `tutorials/t4.geo`（103行）
- `tutorials/t5.geo`（119行）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 精读t4.geo，学习For循环语法 |
| 09:45-10:30 | 45min | 学习数学函数和表达式 |
| 10:30-11:15 | 45min | 学习If-Else条件判断 |
| 11:15-12:00 | 45min | 学习t5.geo的其他模式 |
| 14:00-14:45 | 45min | 用循环创建规则点阵 |
| 14:45-15:30 | 45min | 完成练习作业 |

---

## 上午任务（2小时）

- [ ] 学习For循环语法：
  - `For i In {start:end}`
  - `For i In {start:end:step}`
- [ ] 学习数学函数（Sin, Cos, Sqrt等）
- [ ] 学习If-Else条件判断

---

## 下午任务（2小时）

- [ ] 学习`tutorials/t5.geo`（119行）
- [ ] 理解结构化网格的创建方式
- [ ] 用循环创建规则点阵
- [ ] 练习：创建参数化的几何

---

## 练习作业

### 1. 【基础】创建5x5的点阵

使用双层For循环创建一个5x5的规则点阵

```geo
// grid_points.geo - 5x5点阵
lc = 0.1;
n = 5;      // 行数
m = 5;      // 列数
spacing = 0.5;  // 点间距

point_id = 1;
For i In {0:n-1}
    For j In {0:m-1}
        Point(point_id) = {i*spacing, j*spacing, 0, lc};
        point_id += 1;
    EndFor
EndFor

// 可选：创建网格连接
// TODO: 用For循环创建线和面
```

### 2. 【进阶】参数化圆形点列

创建一个圆周上的等距点

```geo
// circle_points.geo - 圆周上的点
lc = 0.1;
n = 12;          // 点数
radius = 1.0;    // 半径

// 圆心
Point(1) = {0, 0, 0, lc};

// 圆周上的点
For i In {0:n-1}
    angle = 2*Pi*i/n;
    x = radius * Cos(angle);
    y = radius * Sin(angle);
    Point(i+2) = {x, y, 0, lc};
EndFor

// 创建圆弧连接
For i In {0:n-1}
    next = (i+1) % n;
    Circle(i+1) = {i+2, 1, next+2};
EndFor

// 创建曲线环和面
line_list[] = {};
For i In {1:n}
    line_list[] += i;
EndFor
Curve Loop(1) = {line_list[]};
Plane Surface(1) = {1};
```

### 3. 【挑战】条件控制的网格密度

创建一个矩形，根据位置使用不同的网格密度

```geo
// adaptive_grid.geo - 条件控制网格密度
lc_coarse = 0.2;
lc_fine = 0.05;

// 创建10x10的点阵，中心区域使用细网格
n = 10;
spacing = 0.1;

point_id = 1;
For i In {0:n}
    For j In {0:n}
        x = i * spacing;
        y = j * spacing;

        // 判断是否在中心区域（0.3 < x < 0.7, 0.3 < y < 0.7）
        If (x > 0.3 && x < 0.7 && y > 0.3 && y < 0.7)
            lc = lc_fine;
        Else
            lc = lc_coarse;
        EndIf

        Point(point_id) = {x, y, 0, lc};
        point_id += 1;
    EndFor
EndFor

// 注意：完整实现需要添加线和面的创建
// 这里主要展示条件判断的使用
```

---

## 今日检查点

- [ ] 能写出正确的For循环语法
- [ ] 理解GEO脚本中的数组操作
- [ ] 能使用If-Else进行条件判断

---

## 核心概念

### For 循环语法

```geo
// 基本语法
For i In {start:end}
    // 循环体
EndFor

// 带步长
For i In {0:10:2}  // 0, 2, 4, 6, 8, 10
    // 循环体
EndFor

// 嵌套循环
For i In {0:n-1}
    For j In {0:m-1}
        // 操作
    EndFor
EndFor
```

### If-Else 条件判断

```geo
If (condition)
    // 条件为真时执行
ElseIf (another_condition)
    // 另一个条件
Else
    // 其他情况
EndIf
```

### 数组操作

```geo
// 声明数组
arr[] = {1, 2, 3, 4, 5};

// 追加元素
arr[] += 6;

// 获取长度
n = #arr[];

// 访问元素
val = arr[0];
```

### 常用数学函数

| 函数 | 说明 |
|------|------|
| `Sin(x)` | 正弦（x为弧度） |
| `Cos(x)` | 余弦 |
| `Tan(x)` | 正切 |
| `Sqrt(x)` | 平方根 |
| `Exp(x)` | e^x |
| `Log(x)` | 自然对数 |
| `Log10(x)` | 常用对数 |
| `Abs(x)` | 绝对值 |
| `Pi` | 圆周率常量 |

### 字符串操作

```geo
// 字符串拼接
name = Sprintf("point_%g", i);

// 打印信息
Printf("Creating point %g at (%g, %g)", id, x, y);
```

---

## 产出

- 能使用循环创建重复几何
- 能使用条件判断控制参数

---

## 导航

- **上一天**：[Day 3 - GEO脚本语言基础（二）](day-03.md)
- **下一天**：[Day 5 - 结构化网格](day-05.md)
- **返回目录**：[学习计划总览](../study.md)
