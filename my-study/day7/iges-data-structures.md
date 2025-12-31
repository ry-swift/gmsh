# OCCT IGES 读取模块数据结构详解

> 本文档详细分析 OpenCASCADE Technology (OCCT) 中 IGES 文件读取模块使用的核心数据结构。
> 源码位置: `OCCT/src/DataExchange/TKDEIGES/IGESFile/igesread.h` 和 `structiges.c`

## 目录

1. [数据结构总览](#1-数据结构总览)
2. [struct dirpart - 目录条目结构](#2-struct-dirpart---目录条目结构)
3. [struct parlist - 参数列表结构](#3-struct-parlist---参数列表结构)
4. [struct oneparam - 单参数结构](#4-struct-oneparam---单参数结构)
5. [内存页管理机制](#5-内存页管理机制)
6. [参数类型枚举](#6-参数类型枚举)
7. [全局状态变量](#7-全局状态变量)

---

## 1. 数据结构总览

### 1.1 结构关系图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     IGES 内存数据结构层次关系                            │
└─────────────────────────────────────────────────────────────────────────┘

                         ┌─────────────────┐
                         │   全局指针       │
                         └────────┬────────┘
                                  │
          ┌───────────────────────┼───────────────────────┐
          │                       │                       │
          ▼                       ▼                       ▼
   ┌─────────────┐         ┌─────────────┐         ┌─────────────┐
   │   starts    │         │   header    │         │  firstpage  │
   │ (parlist*)  │         │ (parlist*)  │         │ (dirpage*)  │
   │  S段参数    │         │  G段参数    │         │  实体目录    │
   └──────┬──────┘         └──────┬──────┘         └──────┬──────┘
          │                       │                       │
          │                       │            ┌──────────┴──────────┐
          │                       │            │                     │
          ▼                       ▼            ▼                     ▼
   ┌─────────────┐         ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
   │  oneparam   │         │  oneparam   │   │  dirpart[0] │   │  dirpart[1] │ ...
   │  链表       │         │  链表       │   │             │   │             │
   └─────────────┘         └─────────────┘   │ ┌─────────┐ │   │ ┌─────────┐ │
                                             │ │ parlist │ │   │ │ parlist │ │
                                             │ └────┬────┘ │   │ └────┬────┘ │
                                             └──────┼──────┘   └──────┼──────┘
                                                    │                 │
                                                    ▼                 ▼
                                             ┌─────────────┐   ┌─────────────┐
                                             │  oneparam   │   │  oneparam   │
                                             │  链表 (P段) │   │  链表 (P段) │
                                             └─────────────┘   └─────────────┘


┌─────────────────────────────────────────────────────────────────────────┐
│                        内存页管理结构                                    │
└─────────────────────────────────────────────────────────────────────────┘

     字符页链表                   参数页链表                  目录页链表
     (onecarpage)                (oneparpage)               (firstpage)
          │                           │                          │
          ▼                           ▼                          ▼
   ┌───────────┐              ┌───────────┐              ┌───────────┐
   │  carpage  │──next──▶     │  parpage  │──next──▶     │  dirpage  │──next──▶
   ├───────────┤              ├───────────┤              ├───────────┤
   │ used: int │              │ used: int │              │ used: int │
   │ cars[10K] │              │params[20K]│              │parts[1000]│
   └───────────┘              └───────────┘              └───────────┘
```

### 1.2 结构大小汇总

| 结构名 | 大小(估算) | 数组容量 | 说明 |
|-------|-----------|---------|------|
| `struct carpage` | ~10KB | 10000 字符 | 字符存储页 |
| `struct parpage` | ~500KB | 20000 参数 | 参数对象页 |
| `struct dirpage` | ~600KB | 1000 实体 | 目录条目页 |
| `struct dirpart` | ~100 字节 | - | 单个实体目录 |
| `struct oneparam` | ~16 字节 | - | 单个参数 |
| `struct parlist` | ~20 字节 | - | 参数列表头 |

---

## 2. struct dirpart - 目录条目结构

### 2.1 结构定义

```c
/*======================================================================
 * 结构: dirpart
 * 描述: 表示 IGES 文件 D 段中的一个实体条目
 *       每个 IGES 实体在 D 段占用两行，共包含 17 个数值字段和 4 个字符串字段
 * 定义位置: igesread.h
 *======================================================================*/
struct dirpart
{
  /*------------------------------------------------------------------
   * 第一行字段 (D段奇数行)
   * 每字段 8 字符宽，最后 4 个字段各 2 字符宽
   *------------------------------------------------------------------*/
  int typ;    /* [字段1] 实体类型号
               * 例: 110=线, 116=点, 120=旋转曲面, 126=B样条曲线
               *     128=B样条曲面, 144=裁剪曲面, 186=ManifoldSolid
               * 完整类型列表参见 IGES 规范 */

  int poi;    /* [字段2] 参数数据指针
               * 指向 P 段的起始行号 (奇数)
               * 用于定位该实体的详细参数数据 */

  int pdef;   /* [字段3] 结构定义指针
               * 通常为 0 (无关联结构定义)
               * 非零时指向定义此实体结构的另一实体的 D 段行号 */

  int tra;    /* [字段4] 线型模式/字体
               * 0 = 默认 (实线)
               * 1 = 实线
               * 2 = 虚线
               * 3 = 中心线
               * 4 = 点划线
               * 负值 = 指向线型定义实体的 D 段指针 */

  int niv;    /* [字段5] 级别/层次指针
               * 正值 = 直接层号 (如 1, 2, 3...)
               * 负值 = 指向 Property 实体的 D 段指针，该实体包含多层定义
               * 0 = 默认层 */

  int vue;    /* [字段6] 视图指针
               * 0 = 所有视图可见
               * 正值 = 指向 View 实体 (Type 410) 的 D 段指针
               * 负值 = 指向 View Visible Associativity (Type 402) */

  int trf;    /* [字段7] 变换矩阵指针
               * 0 = 无变换 (单位矩阵)
               * 正值 = 指向 Transformation Matrix (Type 124) 的 D 段指针 */

  int aff;    /* [字段8] 标签显示关联指针
               * 0 = 无标签
               * 正值 = 指向 Label Display Associativity (Type 402, Form 5) */

  int blk;    /* [字段9] 空白状态 (2位)
               * 0 = 可见
               * 1 = 空白/隐藏 */

  int sub;    /* [字段10] 从属实体开关 (2位)
               * 0 = 独立实体
               * 1 = 物理从属
               * 2 = 逻辑从属
               * 3 = 物理+逻辑从属 */

  int use;    /* [字段11] 实体使用标志 (2位)
               * 0 = 几何 (Geometry)
               * 1 = 注释 (Annotation)
               * 2 = 定义 (Definition)
               * 3 = 其他
               * 4 = 逻辑/位置
               * 5 = 2D参数化
               * 6 = 构造几何 */

  int her;    /* [字段12] 层次结构标志 (2位)
               * 0 = 全局置顶 (Global top)
               * 1 = 全局延迟 (Global defer)
               * 2 = 使用层次属性 (Use hierarchy property) */

  /*------------------------------------------------------------------
   * 第二行字段 (D段偶数行)
   *------------------------------------------------------------------*/
  int typ2;   /* [字段13] 实体类型号 (重复)
               * 应与 typ 相同，用于验证 */

  int epa;    /* [字段14] 线粗号
               * 0 = 默认
               * 正值 = 线粗等级 (1-n)
               * 负值 = 指向线粗定义实体 */

  int col;    /* [字段15] 颜色号
               * 0 = 无颜色 (使用默认)
               * 1 = 黑色, 2 = 红色, 3 = 绿色, 4 = 蓝色
               * 5 = 黄色, 6 = 品红, 7 = 青色, 8 = 白色
               * 负值 = 指向 Color Definition (Type 314) 的 D 段指针 */

  int nbl;    /* [字段16] 参数数据行数
               * 该实体在 P 段占用的总行数 */

  int form;   /* [字段17] 形式号
               * 同一实体类型的不同变体
               * 例: Type 126 (B样条曲线)
               *     Form 0 = 基本形式
               *     Form 1 = 线段
               *     Form 2 = 圆弧
               *     Form 3 = 椭圆弧
               *     Form 4 = 抛物线
               *     Form 5 = 双曲线 */

  /*------------------------------------------------------------------
   * 字符串字段 (第二行后 32 字符)
   *------------------------------------------------------------------*/
  char res1[10];  /* [字段18] 保留字段1 (8字符+终止符)
                   * 供将来扩展使用 */

  char res2[10];  /* [字段19] 保留字段2 (8字符+终止符)
                   * 供将来扩展使用 */

  char nom[10];   /* [字段20] 实体标签 (8字符+终止符)
                   * 用户定义的实体名称
                   * 例: "SURFACE1", "LINE_A" */

  char num[10];   /* [字段21] 实体下标 (8字符+终止符)
                   * 数字序号，用于区分同名实体 */

  /*------------------------------------------------------------------
   * 参数数据链接
   *------------------------------------------------------------------*/
  struct parlist list;  /* P 段参数列表
                         * 存储该实体的所有参数数据 */

  int numpart;          /* D 段序号
                         * 奇数: 1, 3, 5, 7, ...
                         * (偶数行与奇数行配对) */
};
```

### 2.2 D 段两行对应图

```
D段格式 (每实体占2行，每行72个数据字符 + 8个控制字符)

第1行 (奇数行号): 位置      0    8   16   24   32   40   48   56   64 66 68 70
                          ┌────┬────┬────┬────┬────┬────┬────┬────┬──┬──┬──┬──┐
                字段:     │typ │poi │pdef│tra │niv │vue │trf │aff │bl│su│us│he│
                宽度:     │ 8  │ 8  │ 8  │ 8  │ 8  │ 8  │ 8  │ 8  │2 │2 │2 │2 │
                          └────┴────┴────┴────┴────┴────┴────┴────┴──┴──┴──┴──┘

第2行 (偶数行号): 位置      0    8   16   24   32   40   48   56   64
                          ┌────┬────┬────┬────┬────┬────┬────┬────┬────┐
                字段:     │typ2│epa │col │nbl │form│res1│res2│nom │num │
                宽度:     │ 8  │ 8  │ 8  │ 8  │ 8  │ 8  │ 8  │ 8  │ 8  │
                          └────┴────┴────┴────┴────┴────┴────┴────┴────┘

示例: 一条直线实体

       110       1       0       0       0       0       0       0 0 0 0 0D      1
       110       0       0       1       0                        LINE    D      2
       ▲        ▲       ▲       ▲       ▲       ▲       ▲       ▲
       │        │       │       │       │       │       │       │
       typ=110  poi=1   ...    nbl=1   form=0  res1="" nom="LINE"

       110 = Line Entity Type
       poi=1 表示参数在 P 段第 1 行
       nbl=1 表示参数占 1 行
```

---

## 3. struct parlist - 参数列表结构

### 3.1 结构定义

```c
/*======================================================================
 * 结构: parlist
 * 描述: 参数链表的头结构
 *       用于管理 S段注释、G段全局参数、或某实体的 P段参数
 * 定义位置: igesread.h
 *======================================================================*/
struct parlist
{
  struct oneparam *first;  /* 链表头指针
                            * 指向第一个参数节点
                            * NULL 表示空列表 */

  struct oneparam *last;   /* 链表尾指针
                            * 指向最后一个参数节点
                            * 用于快速追加新参数 (O(1) 复杂度)
                            * NULL 表示空列表 */

  int nbparam;             /* 参数计数
                            * 链表中的参数总数 */
};
```

### 3.2 链表操作图示

```
参数链表结构:

        parlist
       ┌────────────────────┐
       │ first ─────────────┼────┐
       │ last ──────────────┼────┼───────────────────────────┐
       │ nbparam: 3         │    │                           │
       └────────────────────┘    │                           │
                                 ▼                           ▼
                          ┌───────────┐     ┌───────────┐     ┌───────────┐
                          │ oneparam  │     │ oneparam  │     │ oneparam  │
                          ├───────────┤     ├───────────┤     ├───────────┤
                          │ typarg: 3 │     │ typarg: 5 │     │ typarg: 5 │
                          │ parval:"1"│     │parval:"0.0"│    │parval:"1.0"│
                          │ next ─────┼────▶│ next ─────┼────▶│ next:NULL │
                          └───────────┘     └───────────┘     └───────────┘


新增参数操作 (iges_newparam):

Step 1: 创建新节点
        ┌───────────┐
        │ newparam  │
        │ next:NULL │
        └───────────┘

Step 2: 链接到尾部
        parlist.last->next = newparam
        parlist.last = newparam
        parlist.nbparam++

删除操作: 无 (整个链表在 iges_finfile 中一次性释放)
```

---

## 4. struct oneparam - 单参数结构

### 4.1 结构定义

```c
/*======================================================================
 * 结构: oneparam
 * 描述: 单个 IGES 参数的存储结构
 *       构成参数链表的节点
 * 定义位置: structiges.c (静态定义)
 *======================================================================*/
static struct oneparam {
  struct oneparam *next;  /* 下一个参数的指针
                           * NULL 表示链表结束 */

  int typarg;             /* 参数类型
                           * 使用 ArgXxx 枚举值
                           * 见第6节详细说明 */

  char *parval;           /* 参数值字符串
                           * 指向字符页 (carpage) 中的位置
                           * 以 '\0' 结尾 */
} *curparam;             /* 当前正在操作的参数指针 */
```

### 4.2 参数值存储示意

```
参数值存储机制:

parval 指针指向 carpage 中的位置，而非独立分配内存

              oneparam                           carpage
            ┌───────────┐                    ┌──────────────────────────────┐
            │ typarg: 5 │                    │ ... ▼                        │
            │ parval ───┼───────────────────▶│    "123.456\0"               │
            │ next      │                    │                   ▼          │
            └───────────┘                    │    "HELLO\0"                 │
            ┌───────────┐                    │                        ▼     │
            │ typarg: 2 │                    │    "POINT_A\0"               │
            │ parval ───┼───────────────────▶│                              │
            │ next      │                    │                              │
            └───────────┘                    └──────────────────────────────┘

优点:
- 避免大量小内存块分配 (减少 malloc 调用)
- 内存连续，缓存友好
- 整体释放简单高效
```

---

## 5. 内存页管理机制

### 5.1 字符页 (carpage)

```c
/*======================================================================
 * 结构: carpage
 * 描述: 字符存储页，用于批量管理字符串内存
 *       避免频繁的小内存块 malloc/free
 * 容量: Maxcar = 10000 字符
 *======================================================================*/
#define Maxcar 10000

static struct carpage {
  struct carpage* next;        /* 下一页指针
                                * 当前页用满后链接新页
                                * NULL 表示最后一页 */

  int used;                    /* 已使用字符数
                                * 范围: 0 ~ Maxcar */

  char cars[Maxcar+1];         /* 字符缓冲区
                                * +1 用于终止符 */
} *onecarpage;                 /* 当前活动字符页指针 */


/*======================================================================
 * 函数: iges_newchar
 * 描述: 在字符页中分配指定长度的字符空间
 *======================================================================*/
static char* iges_newchar (int lentext)
{
  int lnt = onecarpage->used;

  /* 检查当前页是否有足够空间 */
  if (lnt > Maxcar - lentext - 1) {
    /* 空间不足，分配新页 */
    struct carpage *newpage;
    unsigned int sizepage = sizeof(struct carpage);

    /* 如果请求长度超过标准页大小，分配更大的页 */
    if (lentext >= Maxcar)
      sizepage += (lentext + 1 - Maxcar);

    newpage = (struct carpage*) malloc(sizepage);
    newpage->next = onecarpage;  /* 链接到旧页 */
    onecarpage = newpage;        /* 更新当前页指针 */
    lnt = onecarpage->used = 0;
  }

  /* 返回可用位置的指针 */
  restext = onecarpage->cars + lnt;
  onecarpage->used = (lnt + lentext + 1);  /* 更新使用量 */
  restext[lentext] = '\0';                  /* 设置终止符 */

  return restext;
}
```

### 5.2 参数页 (parpage)

```c
/*======================================================================
 * 结构: parpage
 * 描述: 参数对象页，批量管理 oneparam 结构的内存
 * 容量: Maxpar = 20000 个参数
 *======================================================================*/
#define Maxpar 20000

static struct parpage {
  struct parpage* next;           /* 下一页指针 */
  int             used;           /* 已使用参数数 */
  struct oneparam params[Maxpar+1];  /* 参数对象数组 */
} *oneparpage;                    /* 当前活动参数页指针 */


/* 参数分配逻辑 (在 iges_newparam 中) */
if (oneparpage->used > Maxpar) {
  /* 页已满，分配新页 */
  struct parpage* newparpage;
  newparpage = (struct parpage*) malloc(sizeof(struct parpage));
  newparpage->next = oneparpage;  /* 链接到旧页 */
  newparpage->used = 0;
  oneparpage = newparpage;
}

curparam = &(oneparpage->params[oneparpage->used]);
oneparpage->used++;
```

### 5.3 目录页 (dirpage)

```c
/*======================================================================
 * 结构: dirpage
 * 描述: 目录条目页，批量管理 dirpart 结构的内存
 * 容量: Maxparts = 1000 个实体
 *======================================================================*/
#define Maxparts 1000

static struct dirpage {
  int              used;           /* 已使用实体数 */
  struct dirpage  *next;           /* 下一页指针 */
  struct dirpart   parts[Maxparts];/* 实体目录数组 */
} *firstpage;                      /* 目录页链表头 */

static struct dirpage *curpage;    /* 当前活动目录页 */
static int curnumpart;             /* 当前实体在页内的索引 */


/* 新实体分配逻辑 (在 iges_newpart 中) */
void iges_newpart(int numsec)
{
  if (curpage->used >= Maxparts) {
    /* 页已满，分配新页 */
    struct dirpage* newpage;
    newpage = (struct dirpage*) malloc(sizeof(struct dirpage));
    newpage->next = NULL;
    newpage->used = 0;
    curpage->next = newpage;
    curpage = newpage;
  }

  curnumpart = curpage->used;
  curp = &(curpage->parts[curnumpart]);  /* 获取新实体槽位 */
  curlist = &(curp->list);               /* 获取其参数列表 */

  curp->numpart = numsec;
  curlist->nbparam = 0;
  curlist->first = curlist->last = NULL;

  curpage->used++;
  nbparts++;
}
```

### 5.4 内存页链表图示

```
内存页链表结构 (以 carpage 为例):

               onecarpage (当前页，最新)
                    │
                    ▼
            ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
            │   carpage    │      │   carpage    │      │   carpage    │
            ├──────────────┤      ├──────────────┤      ├──────────────┤
            │ used: 5432   │      │ used: 10000  │      │ used: 10000  │
            │ next ────────┼─────▶│ next ────────┼─────▶│ next: NULL   │
            │              │      │              │      │              │
            │ cars[10000]  │      │ cars[10000]  │      │ cars[10000]  │
            │ [部分使用]    │      │ [全满]       │      │ [全满]       │
            └──────────────┘      └──────────────┘      └──────────────┘
                  ▲                      ▲                     ▲
                最新页               第2旧页               最早页
              (写入中)              (已满)                (已满)


释放顺序 (iges_finfile):
onecarpage → oldcarpage = onecarpage->next → free(onecarpage) → onecarpage = oldcarpage → 循环
```

---

## 6. 参数类型枚举

### 6.1 枚举定义

```c
/*======================================================================
 * 参数类型枚举 (定义于 igesread.h)
 * 描述: 用于标识 IGES 参数的数据类型
 *       由 iges_param() 函数在解析时确定
 *======================================================================*/

#define ArgVide 0   /* 空参数
                     * 两个分隔符之间无内容
                     * 例: "1,,3" 中的第二个参数 */

#define ArgQuid 1   /* 未知/无效类型
                     * 无法识别的格式
                     * 需要上层处理 */

#define ArgChar 2   /* Hollerith 字符串
                     * 格式: nH<n个字符>
                     * 例: 4HLINE 表示 "LINE" */

#define ArgInt  3   /* 无符号整数
                     * 纯数字，无符号无小数点
                     * 可演化为 Real 或 Char
                     * 例: 123, 42 */

#define ArgSign 4   /* 有符号整数
                     * 以 +/- 开头的数字
                     * 可演化为 Real
                     * 例: -5, +100 */

#define ArgReal 5   /* 实数
                     * 包含小数点的数字
                     * 例: 3.14, 0.5, 100.0 */

#define ArgExp  6   /* 实数 + E (待确认)
                     * 遇到 E/e/D/d 但指数部分未完成
                     * 例: "1.0E" (等待后续字符) */

#define ArgRexp 7   /* 实数 + 完整指数
                     * 科学计数法完整形式
                     * 例: 1.0E5, 3.14D-2
                     * 最终归类为 Real */

#define ArgMexp 8   /* 实数 + 不完整指数
                     * 整数后跟 E/D (缺少小数点)
                     * 例: 10D3 (表示 10×10³)
                     * 特殊处理 */
```

### 6.2 类型识别状态机

```
参数类型识别流程 (iges_param 状态机):

                         开始
                           │
                           ▼
                     ┌─────────┐
                     │ ArgVide │  初始状态
                     └────┬────┘
                          │
            ┌─────────────┼─────────────┬─────────────┐
            │             │             │             │
         数字0-9        +/-            .          其他字符
            │             │             │             │
            ▼             ▼             ▼             ▼
      ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐
      │ ArgInt  │   │ ArgSign │   │ ArgReal │   │ ArgQuid │
      └────┬────┘   └────┬────┘   └────┬────┘   └─────────┘
           │             │             │
           │      ┌──────┴──────┐      │
           │      │             │      │
           │     .           数字0-9   │
           │      │             │      │
           │      ▼             │      │
           │ ┌─────────┐        │      │
           │ │ ArgReal │◀───────┘      │
           │ └────┬────┘               │
           │      │                    │
           │   E/e/D/d              E/e/D/d
           │      │                    │
           │      ▼                    │
           │ ┌─────────┐          ┌─────────┐
           │ │ ArgExp  │          │ ArgMexp │
           │ └────┬────┘          └────┬────┘
           │      │                    │
           │   数字0-9                 │
           │      │                    │
           │      ▼                    │
           │ ┌─────────┐               │
           │ │ ArgRexp │◀──────────────┘
           │ └─────────┘
           │
           H
           │
           ▼
      ┌─────────┐
      │ ArgChar │  Hollerith 字符串
      └─────────┘
```

### 6.3 C++ 层类型映射

```c
/* IGESFile_Read.cxx 中的类型映射 */
static Interface_ParamType LesTypes[10];

LesTypes[ArgVide] = Interface_ParamVoid;      /* 空 → Void */
LesTypes[ArgQuid] = Interface_ParamMisc;      /* 未知 → Misc */
LesTypes[ArgChar] = Interface_ParamText;      /* 字符串 → Text */
LesTypes[ArgInt]  = Interface_ParamInteger;   /* 整数 → Integer */
LesTypes[ArgSign] = Interface_ParamInteger;   /* 有符号整数 → Integer */
LesTypes[ArgReal] = Interface_ParamReal;      /* 实数 → Real */
LesTypes[ArgExp]  = Interface_ParamMisc;      /* 不完整指数 → Misc */
LesTypes[ArgRexp] = Interface_ParamReal;      /* 完整指数 → Real */
LesTypes[ArgMexp] = Interface_ParamEnum;      /* 特殊指数 → Enum */
```

---

## 7. 全局状态变量

### 7.1 变量汇总

```c
/*======================================================================
 * structiges.c 中的全局状态变量
 *======================================================================*/

/* 统计计数器 */
static int nbparts;              /* 已读取的实体总数 */
static int nbparams;             /* 已读取的参数总数 */

/* 段参数列表 */
static struct parlist *curlist;  /* 当前活动参数列表
                                  * 可能指向 starts、header 或某 dirpart.list */
static struct parlist *starts;   /* S 段 (Start Section) 参数列表 */
static struct parlist *header;   /* G 段 (Global Section) 参数列表 */

/* 目录管理 */
static struct dirpart *curp;     /* 当前操作的实体目录指针 */
static int curnumpart;           /* 当前实体在页内的索引 */
static struct dirpage *curpage;  /* 当前活动目录页 */

/* 当前参数 */
static struct oneparam *curparam;/* 当前正在操作的参数节点 */

/* 字符管理 */
static char* restext;            /* 当前分配的字符串指针 */


/*======================================================================
 * liriges.c 中的状态变量
 *======================================================================*/
static int iges_fautrelire = 0;  /* "重读"标志
                                  * 1 = 下次调用 iges_lire 时重用当前行
                                  * 用于处理段切换 */


/*======================================================================
 * analiges.c 中的状态变量 (iges_param 内部)
 *======================================================================*/
static int nbcarH = 0;           /* Hollerith 剩余字符数 (跨行时) */
static int numcar = 0;           /* 当前行内的字符位置 */
static int reste = 0;            /* 0=正常, 1=追加参数, -1=跳过 */
static int typarg;               /* 当前参数的类型 */
```

### 7.2 状态变量作用域图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         全局状态变量作用域                               │
└─────────────────────────────────────────────────────────────────────────┘

     igesread.c                structiges.c              analiges.c
    ┌───────────┐             ┌───────────┐             ┌───────────┐
    │           │             │           │             │           │
    │ 调用各函数 │────────────▶│ curp      │◀────────────│ Dstat     │
    │           │             │ curlist   │             │ nbcarH    │
    │           │             │ starts    │             │ numcar    │
    │           │             │ header    │             │ reste     │
    │           │             │ nbparts   │             │ typarg    │
    │           │             │ nbparams  │             │           │
    └───────────┘             └───────────┘             └───────────┘
         │                          │                         │
         │                          │                         │
         ▼                          ▼                         ▼
    ┌───────────────────────────────────────────────────────────────┐
    │                      liriges.c                                │
    │                   iges_fautrelire                             │
    │                   (控制重读行为)                               │
    └───────────────────────────────────────────────────────────────┘


初始化调用链:
  igesread() → iges_initfile()
                    │
                    ├── 分配 onecarpage
                    ├── 分配 oneparpage
                    ├── 分配 starts
                    ├── 分配 header
                    ├── 分配 firstpage
                    └── 初始化 curlist = starts

清理调用链:
  IGESFile_Read() → iges_finfile(1)  // 释放目录页和参数页
                  → iges_finfile(2)  // 释放字符页和列表头
```

---

## 附录: 结构体内存布局

```
struct dirpart 内存布局 (估算):

偏移    字段          大小(字节)   说明
────────────────────────────────────────
0       typ           4           int
4       poi           4           int
8       pdef          4           int
12      tra           4           int
16      niv           4           int
20      vue           4           int
24      trf           4           int
28      aff           4           int
32      blk           4           int
36      sub           4           int
40      use           4           int
44      her           4           int
48      typ2          4           int
52      epa           4           int
56      col           4           int
60      nbl           4           int
64      form          4           int
68      res1[10]      10          char[]
78      res2[10]      10          char[]
88      nom[10]       10          char[]
98      num[10]       10          char[]
108     list          ~20         struct parlist
128     numpart       4           int
────────────────────────────────────────
总计:   ~132 字节 (含对齐填充可能更大)
```

---

*文档生成时间: 2024-12-31*
*源码版本: OpenCASCADE Technology*
