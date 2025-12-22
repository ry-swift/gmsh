# Day 27：网格数据IO

**所属周次**：第4周 - 网格数据结构
**所属阶段**：第二阶段 - 源码阅读基础

---

## 学习目标

理解网格文件格式和读写流程

---

## 学习文件

- `src/geo/GModelIO_MSH.cpp`（约2000行）
- `doc/gmsh.html`（MSH格式说明）

---

## 每日时间分配（4小时）

| 时段 | 时长 | 任务 |
|------|------|------|
| 09:00-09:45 | 45min | 学习MSH文件格式 |
| 09:45-10:30 | 45min | 阅读MSH4格式详细规范 |
| 10:30-11:15 | 45min | 理解MSH文件读取流程 |
| 11:15-12:00 | 45min | 理解MSH文件写入流程 |
| 14:00-14:45 | 45min | 学习其他格式（VTK、STL） |
| 14:45-15:30 | 45min | 了解格式转换方法 |
| 15:30-16:00 | 30min | 完成练习 |

---

## 上午任务（2小时）

### MSH文件格式（版本4）

MSH是Gmsh的原生网格格式，当前版本为4.1。

```
MSH4文件结构：
┌─────────────────────────────────────────────────────┐
│ $MeshFormat                                         │
│ 4.1 0 8                                             │
│ $EndMeshFormat                                      │
├─────────────────────────────────────────────────────┤
│ $PhysicalNames (可选)                               │
│ numPhysicalNames                                    │
│ dim tag "name"                                      │
│ ...                                                 │
│ $EndPhysicalNames                                   │
├─────────────────────────────────────────────────────┤
│ $Entities (可选)                                    │
│ numPoints numCurves numSurfaces numVolumes          │
│ 点实体...                                           │
│ 曲线实体...                                         │
│ 曲面实体...                                         │
│ 体实体...                                           │
│ $EndEntities                                        │
├─────────────────────────────────────────────────────┤
│ $Nodes                                              │
│ numEntityBlocks numNodes minNodeTag maxNodeTag      │
│ entityDim entityTag parametric numNodesInBlock      │
│ nodeTag                                             │
│ x y z                                               │
│ ...                                                 │
│ $EndNodes                                           │
├─────────────────────────────────────────────────────┤
│ $Elements                                           │
│ numEntityBlocks numElements minElemTag maxElemTag   │
│ entityDim entityTag elementType numElementsInBlock  │
│ elementTag nodeTag1 nodeTag2 ...                    │
│ ...                                                 │
│ $EndElements                                        │
└─────────────────────────────────────────────────────┘
```

### MSH格式详解

```
$MeshFormat
version fileType dataSize
$EndMeshFormat

- version: 4.1
- fileType: 0=ASCII, 1=二进制
- dataSize: 浮点数字节数（通常为8，即double）

$Nodes
numEntityBlocks numNodes minNodeTag maxNodeTag
entityDim entityTag parametric numNodesInBlock
nodeTag
x y z [u] [v] [w]  # 如果parametric=1则包含参数坐标
...
$EndNodes

$Elements
numEntityBlocks numElements minElementTag maxElementTag
entityDim entityTag elementType numElementsInBlock
elementTag nodeTag1 nodeTag2 ...
...
$EndElements

元素类型编号（部分）：
  1: 2节点线
  2: 3节点三角形
  3: 4节点四边形
  4: 4节点四面体
  5: 8节点六面体
  6: 6节点三棱柱
  7: 5节点金字塔
  8: 3节点线（二次）
  9: 6节点三角形（二次）
  ...
```

### MSH文件读取流程

```cpp
// src/geo/GModelIO_MSH.cpp

int GModel::readMSH(const std::string &name) {
  FILE *fp = fopen(name.c_str(), "r");

  while(1) {
    char str[256];
    if(!fgets(str, sizeof(str), fp)) break;

    if(!strncmp(str, "$MeshFormat", 11)) {
      // 读取格式信息
      readMSH4Format(fp);
    }
    else if(!strncmp(str, "$PhysicalNames", 14)) {
      // 读取物理群组名称
      readMSH4PhysicalNames(fp);
    }
    else if(!strncmp(str, "$Entities", 9)) {
      // 读取几何实体信息
      readMSH4Entities(fp);
    }
    else if(!strncmp(str, "$Nodes", 6)) {
      // 读取节点
      readMSH4Nodes(fp);
    }
    else if(!strncmp(str, "$Elements", 9)) {
      // 读取元素
      readMSH4Elements(fp);
    }
    // ... 其他块
  }

  fclose(fp);
  return 1;
}

// 读取节点
void readMSH4Nodes(FILE *fp, GModel *model) {
  int numEntityBlocks, numNodes, minTag, maxTag;
  fscanf(fp, "%d %d %d %d", &numEntityBlocks, &numNodes, &minTag, &maxTag);

  for(int i = 0; i < numEntityBlocks; i++) {
    int entityDim, entityTag, parametric, numNodesInBlock;
    fscanf(fp, "%d %d %d %d", &entityDim, &entityTag,
           &parametric, &numNodesInBlock);

    // 获取几何实体
    GEntity *ge = model->getEntityByTag(entityDim, entityTag);

    for(int j = 0; j < numNodesInBlock; j++) {
      int tag;
      double x, y, z;
      fscanf(fp, "%d", &tag);
      fscanf(fp, "%lf %lf %lf", &x, &y, &z);

      // 创建顶点
      MVertex *v = new MVertex(x, y, z, ge, tag);
      ge->mesh_vertices.push_back(v);
    }
  }
}
```

---

## 下午任务（2小时）

### MSH文件写入流程

```cpp
// src/geo/GModelIO_MSH.cpp

int GModel::writeMSH(const std::string &name, double version,
                      bool binary, bool saveAll, ...) {
  FILE *fp = fopen(name.c_str(), binary ? "wb" : "w");

  // 写入格式头
  fprintf(fp, "$MeshFormat\n");
  fprintf(fp, "%g %d %d\n", version, binary ? 1 : 0, (int)sizeof(double));
  fprintf(fp, "$EndMeshFormat\n");

  // 写入物理群组
  if(physicalNames.size()) {
    fprintf(fp, "$PhysicalNames\n");
    // ...
    fprintf(fp, "$EndPhysicalNames\n");
  }

  // 写入实体
  fprintf(fp, "$Entities\n");
  // ...
  fprintf(fp, "$EndEntities\n");

  // 写入节点
  fprintf(fp, "$Nodes\n");
  writeMSH4Nodes(fp, ...);
  fprintf(fp, "$EndNodes\n");

  // 写入元素
  fprintf(fp, "$Elements\n");
  writeMSH4Elements(fp, ...);
  fprintf(fp, "$EndElements\n");

  fclose(fp);
  return 1;
}
```

### 其他网格格式

#### VTK格式

```cpp
// src/geo/GModelIO_VTK.cpp

// VTK Legacy格式
// # vtk DataFile Version 2.0
// description
// ASCII
// DATASET UNSTRUCTURED_GRID
// POINTS n dataType
// x0 y0 z0
// x1 y1 z1
// ...
// CELLS n size
// numPoints p0 p1 p2 ...
// ...
// CELL_TYPES n
// type0
// type1
// ...

int GModel::writeVTK(const std::string &name, bool binary, ...) {
  // VTK元素类型映射
  // 1: VTK_VERTEX
  // 3: VTK_LINE
  // 5: VTK_TRIANGLE
  // 9: VTK_QUAD
  // 10: VTK_TETRA
  // 12: VTK_HEXAHEDRON
  // ...
}
```

#### STL格式

```cpp
// src/geo/GModelIO_STL.cpp

// STL ASCII格式
// solid name
//   facet normal ni nj nk
//     outer loop
//       vertex v1x v1y v1z
//       vertex v2x v2y v2z
//       vertex v3x v3y v3z
//     endloop
//   endfacet
//   ...
// endsolid name

// STL只支持三角形面，不包含拓扑信息
int GModel::writeSTL(const std::string &name, bool binary, ...) {
  // 遍历所有面的三角形
  for(auto gf : getFaces()) {
    for(auto t : gf->triangles) {
      // 计算法向量
      SVector3 n = t->normal();
      // 写入三角形
    }
  }
}
```

### 格式转换

```cpp
// 格式转换的基本流程
// 1. 读取源格式
model->readMSH("input.msh");
// 或
model->readSTL("input.stl");
model->readVTK("input.vtk");

// 2. 写入目标格式
model->writeMSH("output.msh", 4.1);
// 或
model->writeVTK("output.vtk");
model->writeSTL("output.stl");

// 注意事项：
// - STL → MSH: 需要重建拓扑（合并重复顶点）
// - MSH → STL: 丢失拓扑和高阶元素信息
// - 二进制格式通常更快、更小
```

### 支持的格式列表

| 格式 | 扩展名 | 特点 |
|------|--------|------|
| MSH | .msh | Gmsh原生，完整信息 |
| VTK | .vtk | ParaView兼容 |
| STL | .stl | 三角面片，无拓扑 |
| PLY | .ply | 3D扫描常用 |
| CGNS | .cgns | CFD标准格式 |
| MED | .med | Salome兼容 |
| UNV | .unv | I-deas通用格式 |
| INP | .inp | Abaqus输入格式 |
| BDF | .bdf | Nastran格式 |

---

## 练习作业

### 1. 【基础】分析MSH文件

手动创建一个简单的MSH文件：

```
$MeshFormat
4.1 0 8
$EndMeshFormat
$Nodes
1 4 1 4
2 1 0 4
1
2
3
4
0 0 0
1 0 0
1 1 0
0 1 0
$EndNodes
$Elements
1 2 1 2
2 1 2 2
1 1 2 3
2 1 3 4
$EndElements
```

用Gmsh打开验证：
```bash
gmsh test.msh
```

### 2. 【进阶】追踪读取流程

分析MSH文件读取的完整代码路径：

```bash
# 查找readMSH入口
grep -rn "int GModel::readMSH" src/geo/GModelIO_MSH.cpp

# 查找节点读取
grep -rn "readMSH4Nodes" src/geo/GModelIO_MSH.cpp

# 查找元素读取
grep -rn "readMSH4Elements" src/geo/GModelIO_MSH.cpp
```

绘制读取流程图：
```
readMSH()
├── readMSH4Format()
├── readMSH4PhysicalNames()
├── readMSH4Entities()
├── readMSH4Nodes()
│   └── 为每个几何实体创建MVertex
└── readMSH4Elements()
    └── 为每个几何实体创建MElement
```

### 3. 【挑战】实现简单的网格导出

编写代码将网格导出为简化格式：

```cpp
// 伪代码：导出为简单的节点-元素格式
void exportSimpleFormat(GModel *model, const std::string &filename) {
  FILE *fp = fopen(filename.c_str(), "w");

  // 收集所有顶点
  std::vector<MVertex*> vertices;
  std::map<MVertex*, int> vertex2id;
  int id = 0;
  for(auto gf : model->getFaces()) {
    for(auto v : gf->mesh_vertices) {
      vertices.push_back(v);
      vertex2id[v] = id++;
    }
  }

  // 写入顶点
  fprintf(fp, "VERTICES %d\n", (int)vertices.size());
  for(auto v : vertices) {
    fprintf(fp, "%g %g %g\n", v->x(), v->y(), v->z());
  }

  // 收集所有三角形
  std::vector<MTriangle*> triangles;
  for(auto gf : model->getFaces()) {
    for(auto t : gf->triangles) {
      triangles.push_back(t);
    }
  }

  // 写入三角形
  fprintf(fp, "TRIANGLES %d\n", (int)triangles.size());
  for(auto t : triangles) {
    fprintf(fp, "%d %d %d\n",
            vertex2id[t->getVertex(0)],
            vertex2id[t->getVertex(1)],
            vertex2id[t->getVertex(2)]);
  }

  fclose(fp);
}
```

---

## 今日检查点

- [ ] 理解MSH4文件格式的结构
- [ ] 理解节点和元素块的组织方式
- [ ] 能追踪MSH文件的读取流程
- [ ] 了解其他常见网格格式（VTK、STL）

---

## 核心概念

### MSH4格式特点

```
MSH4相比MSH2的改进：
┌────────────────────────────────────────────────────────┐
│ 1. 按几何实体分组存储                                  │
│    - 便于并行读写                                      │
│    - 保持几何-网格关联                                 │
│                                                        │
│ 2. 支持参数坐标                                        │
│    - 边上顶点存储 t                                    │
│    - 面上顶点存储 (u, v)                               │
│                                                        │
│ 3. 支持高阶元素                                        │
│    - 二次、三次等高阶元素                              │
│    - 节点编号遵循规范                                  │
│                                                        │
│ 4. 支持大规模网格                                      │
│    - 64位标签                                          │
│    - 高效二进制格式                                    │
└────────────────────────────────────────────────────────┘
```

### 文件格式比较

| 格式 | 拓扑 | 高阶 | 物理组 | 二进制 | 大小 |
|------|------|------|--------|--------|------|
| MSH4 | ✓ | ✓ | ✓ | ✓ | 中 |
| VTK | 部分 | ✓ | 部分 | ✓ | 中 |
| STL | ✗ | ✗ | ✗ | ✓ | 大 |
| CGNS | ✓ | ✓ | ✓ | ✓ | 小 |
| MED | ✓ | ✓ | ✓ | ✓ | 小 |

### IO性能优化

```cpp
// 二进制vs ASCII
// - 二进制：读写快，文件小
// - ASCII：可读，便于调试

// 并行IO（MSH4支持）
// - 按实体块划分
// - 多线程读写

// 内存映射（大文件）
// - 避免全部加载到内存
// - 按需读取

// 压缩
// - gzip压缩的MSH文件
// - gmsh可以直接读取 .msh.gz
```

---

## 源码导航

### 关键文件

```
src/geo/
├── GModelIO_MSH.cpp     # MSH格式（主要）
├── GModelIO_MSH2.cpp    # MSH2旧格式
├── GModelIO_MSH3.cpp    # MSH3格式
├── GModelIO_VTK.cpp     # VTK格式
├── GModelIO_STL.cpp     # STL格式
├── GModelIO_MED.cpp     # MED格式
├── GModelIO_CGNS.cpp    # CGNS格式
├── GModelIO_UNV.cpp     # UNV格式
├── GModelIO_INP.cpp     # Abaqus格式
└── GModelIO_BDF.cpp     # Nastran格式
```

### 搜索命令

```bash
# 查找格式读取函数
grep -rn "GModel::read" src/geo/GModelIO*.cpp | grep "const std::string"

# 查找格式写入函数
grep -rn "GModel::write" src/geo/GModelIO*.cpp | grep "const std::string"

# 查找元素类型映射
grep -rn "getTypeForMSH\|TYPE_" src/geo/
```

---

## 产出

- 理解MSH文件格式
- 掌握网格IO的基本流程

---

## 导航

- **上一天**：[Day 26 - 网格质量度量](day-26.md)
- **下一天**：[Day 28 - 第四周复习](day-28.md)
- **返回目录**：[学习计划总览](../STUDY-INDEX.md)
