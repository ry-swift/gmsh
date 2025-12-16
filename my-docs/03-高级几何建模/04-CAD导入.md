# CAD导入

Gmsh支持多种CAD格式的导入，这是处理复杂工程模型的关键功能。

## 支持的格式

| 格式 | 扩展名 | 说明 | 需要依赖 |
|-----|--------|------|---------|
| STEP | .step, .stp | ISO标准格式，推荐使用 | OpenCASCADE |
| IGES | .iges, .igs | 旧版通用格式 | OpenCASCADE |
| BREP | .brep | OpenCASCADE原生格式 | OpenCASCADE |
| STL | .stl | 三角网格格式 | 无 |
| OBJ | .obj | Wavefront格式 | 无 |
| PLY | .ply | 多边形文件格式 | 无 |

## STEP文件导入

### 基本导入

```python
import gmsh

gmsh.initialize()
gmsh.model.add("step_import")

# 导入STEP文件
gmsh.merge("model.step")

# 或者使用OCC导入（更多控制）
gmsh.model.occ.importShapes("model.step")
gmsh.model.occ.synchronize()

gmsh.finalize()
```

### 导入选项

```python
# STEP导入选项
# 设置导入时的公差
gmsh.option.setNumber("Geometry.Tolerance", 1e-6)

# 是否愈合几何
gmsh.option.setNumber("Geometry.OCCFixDegenerated", 1)
gmsh.option.setNumber("Geometry.OCCFixSmallEdges", 1)
gmsh.option.setNumber("Geometry.OCCFixSmallFaces", 1)
gmsh.option.setNumber("Geometry.OCCSewFaces", 1)

# 导入
gmsh.model.occ.importShapes("model.step")
gmsh.model.occ.synchronize()
```

### 处理导入结果

```python
#!/usr/bin/env python3
"""
导入STEP文件并分析
"""
import gmsh

gmsh.initialize()
gmsh.model.add("step_analysis")

# 导入
gmsh.model.occ.importShapes("model.step")
gmsh.model.occ.synchronize()

# 获取实体信息
points = gmsh.model.getEntities(0)
curves = gmsh.model.getEntities(1)
surfaces = gmsh.model.getEntities(2)
volumes = gmsh.model.getEntities(3)

print(f"点数: {len(points)}")
print(f"曲线数: {len(curves)}")
print(f"曲面数: {len(surfaces)}")
print(f"体积数: {len(volumes)}")

# 获取边界框
for vol in volumes:
    bbox = gmsh.model.occ.getBoundingBox(3, vol[1])
    print(f"体积 {vol[1]} 边界框: "
          f"({bbox[0]:.2f}, {bbox[1]:.2f}, {bbox[2]:.2f}) - "
          f"({bbox[3]:.2f}, {bbox[4]:.2f}, {bbox[5]:.2f})")

# 网格生成
gmsh.option.setNumber("Mesh.MeshSizeMax", 1.0)
gmsh.model.mesh.generate(3)

gmsh.write("from_step.msh")
gmsh.finalize()
```

## IGES文件导入

```python
# IGES导入（类似STEP）
gmsh.model.occ.importShapes("model.iges")
gmsh.model.occ.synchronize()

# IGES特定选项
gmsh.option.setNumber("Geometry.OCCImportLabels", 1)  # 导入标签
gmsh.option.setNumber("Geometry.OCCBooleanPreserveNumbering", 0)
```

## BREP文件导入

```python
# BREP是OpenCASCADE的原生格式
# 最精确，推荐用于中间文件交换

gmsh.model.occ.importShapes("model.brep")
gmsh.model.occ.synchronize()

# 导出为BREP
gmsh.write("output.brep")
```

## STL文件导入

### 基本导入

```python
# STL导入（三角网格）
gmsh.merge("model.stl")

# STL导入后是离散几何，不是精确CAD
# 可以用于网格重划分
```

### STL重网格化

```python
#!/usr/bin/env python3
"""
导入STL并重新网格化
"""
import gmsh

gmsh.initialize()
gmsh.model.add("stl_remesh")

# 导入STL
gmsh.merge("model.stl")

# 从离散网格创建几何
gmsh.model.mesh.createGeometry()

# 重新分类网格元素到几何实体
gmsh.model.mesh.classifySurfaces(math.pi / 4,  # 角度阈值
                                  curveAngle=math.pi / 4,
                                  splitAngle=math.pi / 2)

# 创建几何拓扑
gmsh.model.mesh.createTopology()

gmsh.model.geo.synchronize()

# 设置新的网格尺寸
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.5)

# 删除旧网格，生成新网格
gmsh.model.mesh.clear()
gmsh.model.mesh.generate(2)

gmsh.write("remeshed.msh")
gmsh.finalize()
```

### STL修复

```python
# STL常见问题及处理

# 1. 合并接近的节点
gmsh.option.setNumber("Geometry.Tolerance", 1e-6)

# 2. 修复法向一致性
gmsh.option.setNumber("Mesh.OrientedEdges", 1)

# 3. 填充孔洞
# 需要手动识别和填充
```

## 导入后处理

### 几何愈合

```python
# 愈合（修复）几何
gmsh.model.occ.healShapes()
gmsh.model.occ.synchronize()

# 详细愈合选项
gmsh.model.occ.healShapes(
    dimTags=[],           # 空=所有实体
    tolerance=1e-6,       # 公差
    fixDegenerated=True,  # 修复退化面
    fixSmallEdges=True,   # 修复小边
    fixSmallFaces=True,   # 修复小面
    sewFaces=True,        # 缝合面
    makeSolids=True       # 尝试创建实体
)
```

### 去除小特征

```python
#!/usr/bin/env python3
"""
去除CAD模型中的小特征
"""
import gmsh

gmsh.initialize()
gmsh.model.add("defeaturing")

# 导入模型
gmsh.model.occ.importShapes("model_with_small_features.step")
gmsh.model.occ.synchronize()

# 获取所有曲面
surfaces = gmsh.model.getEntities(2)

# 识别小曲面
small_surfaces = []
threshold_area = 0.01  # 面积阈值

for surf in surfaces:
    area = gmsh.model.occ.getMass(2, surf[1])
    if area < threshold_area:
        small_surfaces.append(surf)
        print(f"小曲面: {surf[1]}, 面积: {area:.6f}")

# 可以尝试移除小曲面（需要谨慎操作）
# 通常通过布尔运算或使用CAD软件预处理

gmsh.finalize()
```

### 拆分和合并

```python
# 拆分实体
# fragment可以在实体之间创建共享边界
volumes = gmsh.model.getEntities(3)
if len(volumes) > 1:
    gmsh.model.occ.fragment(volumes, [])
    gmsh.model.occ.synchronize()

# 合并相邻实体
# 使用布尔并集
volume_tags = [v[1] for v in volumes]
if len(volume_tags) > 1:
    gmsh.model.occ.fuse([(3, volume_tags[0])],
                        [(3, v) for v in volume_tags[1:]])
    gmsh.model.occ.synchronize()
```

## 多文件导入

```python
#!/usr/bin/env python3
"""
导入多个CAD文件并组装
"""
import gmsh

gmsh.initialize()
gmsh.model.add("assembly")

# 导入第一个零件
gmsh.model.occ.importShapes("part1.step")
part1_volumes = gmsh.model.getEntities(3)

# 平移第一个零件
gmsh.model.occ.translate(part1_volumes, 0, 0, 0)

# 导入第二个零件
gmsh.model.occ.importShapes("part2.step")
# 新导入的实体会有新的tag

# 获取所有实体
all_volumes = gmsh.model.getEntities(3)

# 识别新导入的零件（tag较大的）
part2_volumes = [v for v in all_volumes if v not in part1_volumes]

# 平移第二个零件
gmsh.model.occ.translate(part2_volumes, 5, 0, 0)

# 同步
gmsh.model.occ.synchronize()

# 创建共形网格（如果零件接触）
gmsh.model.occ.fragment(all_volumes, [])
gmsh.model.occ.synchronize()

gmsh.model.mesh.generate(3)
gmsh.write("assembly.msh")
gmsh.finalize()
```

## 物理组自动创建

```python
#!/usr/bin/env python3
"""
导入CAD并自动创建物理组
"""
import gmsh

gmsh.initialize()
gmsh.model.add("auto_physicals")

# 导入
gmsh.model.occ.importShapes("model.step")
gmsh.model.occ.synchronize()

# 为每个体积创建物理组
volumes = gmsh.model.getEntities(3)
for i, vol in enumerate(volumes):
    gmsh.model.addPhysicalGroup(3, [vol[1]], tag=100 + i,
                                 name=f"Volume_{vol[1]}")

# 为每个曲面创建物理组（边界条件）
surfaces = gmsh.model.getEntities(2)

# 按位置分类曲面
for surf in surfaces:
    com = gmsh.model.occ.getCenterOfMass(2, surf[1])
    bbox = gmsh.model.occ.getBoundingBox(2, surf[1])

    # 根据位置命名
    name = f"Surface_{surf[1]}"
    if abs(bbox[2] - bbox[5]) < 1e-6:  # 平面（z相同）
        if bbox[2] < 0.1:
            name = f"Bottom_{surf[1]}"
        elif bbox[5] > 9.9:
            name = f"Top_{surf[1]}"

    gmsh.model.addPhysicalGroup(2, [surf[1]], name=name)

gmsh.model.mesh.generate(3)
gmsh.write("with_physicals.msh")
gmsh.finalize()
```

## CAD导出

```python
# 导出为不同格式
gmsh.write("output.step")   # STEP
gmsh.write("output.brep")   # BREP
gmsh.write("output.iges")   # IGES
gmsh.write("output.stl")    # STL（仅网格）

# 或者使用明确的方法
# gmsh.model.occ.exportShapes(filename)
```

## 完整示例：工业零件处理

```python
#!/usr/bin/env python3
"""
完整的CAD导入和网格生成流程
"""
import gmsh
import sys

def process_cad(input_file, output_file, mesh_size=1.0):
    """处理CAD文件"""

    gmsh.initialize()
    gmsh.model.add("cad_processing")

    # 1. 设置导入选项
    gmsh.option.setNumber("Geometry.OCCFixDegenerated", 1)
    gmsh.option.setNumber("Geometry.OCCFixSmallEdges", 1)
    gmsh.option.setNumber("Geometry.OCCFixSmallFaces", 1)
    gmsh.option.setNumber("Geometry.OCCSewFaces", 1)
    gmsh.option.setNumber("Geometry.Tolerance", 1e-6)

    # 2. 导入CAD
    print(f"导入: {input_file}")
    try:
        gmsh.model.occ.importShapes(input_file)
    except Exception as e:
        print(f"导入失败: {e}")
        gmsh.finalize()
        return False

    # 3. 愈合几何
    print("愈合几何...")
    gmsh.model.occ.healShapes()
    gmsh.model.occ.synchronize()

    # 4. 分析模型
    volumes = gmsh.model.getEntities(3)
    surfaces = gmsh.model.getEntities(2)
    curves = gmsh.model.getEntities(1)
    points = gmsh.model.getEntities(0)

    print(f"几何统计:")
    print(f"  体积数: {len(volumes)}")
    print(f"  曲面数: {len(surfaces)}")
    print(f"  曲线数: {len(curves)}")
    print(f"  点数: {len(points)}")

    # 5. 获取整体边界框
    if volumes:
        xmin, ymin, zmin = float('inf'), float('inf'), float('inf')
        xmax, ymax, zmax = float('-inf'), float('-inf'), float('-inf')
        for vol in volumes:
            bbox = gmsh.model.occ.getBoundingBox(3, vol[1])
            xmin = min(xmin, bbox[0])
            ymin = min(ymin, bbox[1])
            zmin = min(zmin, bbox[2])
            xmax = max(xmax, bbox[3])
            ymax = max(ymax, bbox[4])
            zmax = max(zmax, bbox[5])

        model_size = max(xmax - xmin, ymax - ymin, zmax - zmin)
        print(f"模型尺寸: {model_size:.2f}")

        # 自动调整网格尺寸
        if mesh_size <= 0:
            mesh_size = model_size / 20

    # 6. 创建物理组
    # 所有体积
    if volumes:
        vol_tags = [v[1] for v in volumes]
        gmsh.model.addPhysicalGroup(3, vol_tags, name="AllVolumes")

    # 所有边界面
    if surfaces:
        surf_tags = [s[1] for s in surfaces]
        gmsh.model.addPhysicalGroup(2, surf_tags, name="AllSurfaces")

    # 7. 网格设置
    gmsh.option.setNumber("Mesh.MeshSizeMin", mesh_size * 0.1)
    gmsh.option.setNumber("Mesh.MeshSizeMax", mesh_size)
    gmsh.option.setNumber("Mesh.MeshSizeFromCurvature", 20)
    gmsh.option.setNumber("Mesh.Algorithm", 6)
    gmsh.option.setNumber("Mesh.Algorithm3D", 10)

    # 8. 生成网格
    print("生成网格...")
    try:
        gmsh.model.mesh.generate(3)
    except Exception as e:
        print(f"3D网格生成失败: {e}")
        print("尝试2D网格...")
        gmsh.model.mesh.generate(2)

    # 9. 优化
    print("优化网格...")
    try:
        gmsh.model.mesh.optimize("Netgen")
    except:
        print("优化跳过")

    # 10. 质量统计
    qualities = gmsh.model.mesh.getElementQualities([], "minSJ")
    if qualities:
        print(f"网格质量:")
        print(f"  最小: {min(qualities):.4f}")
        print(f"  平均: {sum(qualities)/len(qualities):.4f}")
        print(f"  元素数: {len(qualities)}")

    # 11. 保存
    gmsh.write(output_file)
    print(f"保存: {output_file}")

    gmsh.finalize()
    return True

if __name__ == "__main__":
    # 示例用法
    process_cad("input.step", "output.msh", mesh_size=0.5)
```

## 常见问题

### Q1: 导入后没有体积

**原因**: CAD文件可能只包含曲面，或曲面未闭合

**解决**:
```python
# 尝试缝合曲面
surfaces = gmsh.model.getEntities(2)
gmsh.model.occ.sewFaces([s[1] for s in surfaces], tolerance=1e-4)
gmsh.model.occ.synchronize()
```

### Q2: 网格生成失败

**原因**: 几何可能有问题（小特征、自交等）

**解决**:
```python
# 增加公差
gmsh.option.setNumber("Geometry.ToleranceBoolean", 1e-3)

# 愈合几何
gmsh.model.occ.healShapes()

# 使用更大的网格尺寸
gmsh.option.setNumber("Mesh.MeshSizeMin", 0.5)
```

### Q3: 导入后实体位置错误

**原因**: 单位不一致

**解决**:
```python
# 缩放几何
entities = gmsh.model.getEntities()
gmsh.model.occ.dilate(entities, 0, 0, 0, 0.001, 0.001, 0.001)  # mm to m
```

## 下一步

- [05-离散几何](./05-离散几何.md) - 学习从网格创建几何
