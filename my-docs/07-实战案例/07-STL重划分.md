# STL重划分

本案例演示STL文件的修复和重新网格化。

## 案例目标

- 导入STL文件
- 修复STL几何缺陷
- 重新生成高质量网格
- 优化网格分布

## 基本STL导入和重划分

```python
#!/usr/bin/env python3
"""
案例7: 基本STL重划分
"""
import gmsh
import sys

def remesh_stl_basic(input_file, output_file, mesh_size=1.0):
    """基本STL重划分"""
    gmsh.initialize(sys.argv)

    # 设置STL合并选项
    gmsh.option.setNumber("Geometry.ToleranceBoolean", 1e-3)
    gmsh.option.setNumber("Geometry.Tolerance", 1e-3)

    # 导入STL
    gmsh.merge(input_file)

    # 获取所有曲面
    surfaces = gmsh.model.getEntities(dim=2)
    print(f"导入曲面数: {len(surfaces)}")

    # 分类网格（将三角形分类到几何实体）
    # classifySurfaces(angle, boundary=True, forReparametrization=False, curveAngle=180)
    # angle: 两个三角形之间的角度阈值（度）
    gmsh.model.mesh.classifySurfaces(
        angle=40,           # 特征角度阈值
        boundary=True,      # 检测边界
        forReparametrization=False,
        curveAngle=180      # 曲线角度
    )

    # 从分类的曲面创建几何
    gmsh.model.mesh.createGeometry()

    # 同步
    gmsh.model.occ.synchronize()

    # 重新获取几何实体
    surfaces = gmsh.model.getEntities(dim=2)
    print(f"分类后曲面数: {len(surfaces)}")

    # 物理组
    surface_tags = [tag for dim, tag in surfaces]
    gmsh.model.addPhysicalGroup(2, surface_tags, name="Surface")

    # 设置网格尺寸
    gmsh.option.setNumber("Mesh.MeshSizeMax", mesh_size)
    gmsh.option.setNumber("Mesh.MeshSizeMin", mesh_size * 0.1)

    # 使用曲率自适应
    gmsh.option.setNumber("Mesh.MeshSizeFromCurvature", 20)

    # 生成新网格
    gmsh.model.mesh.generate(2)

    # 优化
    gmsh.model.mesh.optimize("Laplace2D")

    # 保存
    gmsh.write(output_file)

    # 统计
    node_tags, _, _ = gmsh.model.mesh.getNodes()
    elem_types, elem_tags, _ = gmsh.model.mesh.getElements(dim=2)
    num_elems = sum(len(t) for t in elem_tags)

    print(f"\n重划分结果:")
    print(f"  节点数: {len(node_tags)}")
    print(f"  元素数: {num_elems}")

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    # 创建测试STL（如果没有输入文件）
    import os
    if not os.path.exists("input.stl"):
        # 创建简单的测试STL
        gmsh.initialize()
        gmsh.model.add("test")
        gmsh.model.occ.addSphere(0, 0, 0, 1)
        gmsh.model.occ.synchronize()
        gmsh.model.mesh.generate(2)
        gmsh.write("input.stl")
        gmsh.finalize()

    remesh_stl_basic("input.stl", "output.msh", mesh_size=0.1)
```

## STL修复

```python
#!/usr/bin/env python3
"""
案例7: STL修复和重划分
"""
import gmsh
import sys

def repair_and_remesh_stl(input_file, output_file):
    """修复STL并重划分"""
    gmsh.initialize(sys.argv)

    # 设置修复选项
    # OCC修复选项
    gmsh.option.setNumber("Geometry.OCCFixDegenerated", 1)
    gmsh.option.setNumber("Geometry.OCCFixSmallEdges", 1)
    gmsh.option.setNumber("Geometry.OCCFixSmallFaces", 1)
    gmsh.option.setNumber("Geometry.OCCSewFaces", 1)
    gmsh.option.setNumber("Geometry.OCCMakeSolids", 1)

    # 容差设置
    gmsh.option.setNumber("Geometry.Tolerance", 1e-4)
    gmsh.option.setNumber("Geometry.ToleranceBoolean", 1e-4)

    # 导入STL
    print("导入STL...")
    gmsh.merge(input_file)

    # 获取初始信息
    surfaces = gmsh.model.getEntities(dim=2)
    print(f"初始曲面数: {len(surfaces)}")

    # 分类曲面
    print("分类曲面...")
    gmsh.model.mesh.classifySurfaces(
        angle=30,
        boundary=True,
        forReparametrization=True,  # 用于重参数化
        curveAngle=180
    )

    # 创建几何
    gmsh.model.mesh.createGeometry()

    # 获取并修复边
    curves = gmsh.model.getEntities(dim=1)
    print(f"检测到曲线数: {len(curves)}")

    gmsh.model.occ.synchronize()

    # 尝试创建体积（如果是闭合曲面）
    surfaces = gmsh.model.getEntities(dim=2)

    try:
        # 创建曲面环
        surface_tags = [tag for dim, tag in surfaces]
        sl = gmsh.model.occ.addSurfaceLoop(surface_tags)

        # 创建体积
        vol = gmsh.model.occ.addVolume([sl])
        gmsh.model.occ.synchronize()
        print("成功创建封闭体积")

        # 物理组
        gmsh.model.addPhysicalGroup(3, [vol], name="Volume")

        # 生成3D网格
        gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)
        gmsh.model.mesh.generate(3)

    except Exception as e:
        print(f"无法创建封闭体积: {e}")
        print("仅生成表面网格")

        # 物理组
        gmsh.model.addPhysicalGroup(2, surface_tags, name="Surface")

        # 生成2D网格
        gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)
        gmsh.model.mesh.generate(2)

    # 优化
    gmsh.model.mesh.optimize("Laplace2D")

    gmsh.write(output_file)

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    repair_and_remesh_stl("input.stl", "repaired.msh")
```

## 自适应STL重划分

```python
#!/usr/bin/env python3
"""
案例7: 自适应STL重划分
"""
import gmsh
import sys
import numpy as np

def adaptive_remesh_stl(input_file, output_file):
    """基于曲率的自适应重划分"""
    gmsh.initialize(sys.argv)

    # 导入并分类
    gmsh.merge(input_file)
    gmsh.model.mesh.classifySurfaces(angle=30, boundary=True)
    gmsh.model.mesh.createGeometry()
    gmsh.model.occ.synchronize()

    surfaces = gmsh.model.getEntities(dim=2)
    curves = gmsh.model.getEntities(dim=1)

    # 在高曲率区域加密
    # 使用曲线的曲率信息

    # 创建距离场（从所有曲线）
    curve_tags = [tag for dim, tag in curves]

    if curve_tags:
        dist = gmsh.model.mesh.field.add("Distance")
        gmsh.model.mesh.field.setNumbers(dist, "CurvesList", curve_tags)

        # 阈值场
        thresh = gmsh.model.mesh.field.add("Threshold")
        gmsh.model.mesh.field.setNumber(thresh, "InField", dist)
        gmsh.model.mesh.field.setNumber(thresh, "SizeMin", 0.02)
        gmsh.model.mesh.field.setNumber(thresh, "SizeMax", 0.2)
        gmsh.model.mesh.field.setNumber(thresh, "DistMin", 0.01)
        gmsh.model.mesh.field.setNumber(thresh, "DistMax", 0.1)

        gmsh.model.mesh.field.setAsBackgroundMesh(thresh)

    # 同时使用曲率
    gmsh.option.setNumber("Mesh.MeshSizeFromCurvature", 30)
    gmsh.option.setNumber("Mesh.MeshSizeExtendFromBoundary", 0)

    # 网格尺寸限制
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.2)
    gmsh.option.setNumber("Mesh.MeshSizeMin", 0.01)

    # 生成网格
    gmsh.model.mesh.generate(2)

    # 多次优化
    for _ in range(3):
        gmsh.model.mesh.optimize("Laplace2D")

    # 统计
    node_tags, coords, _ = gmsh.model.mesh.getNodes()
    elem_types, elem_tags, _ = gmsh.model.mesh.getElements(dim=2)
    num_elems = sum(len(t) for t in elem_tags)

    qualities = gmsh.model.mesh.getElementQualities([], "minSJ")
    q = np.array(qualities) if qualities else np.array([0])

    print(f"\n自适应重划分结果:")
    print(f"  节点数: {len(node_tags)}")
    print(f"  元素数: {num_elems}")
    print(f"  最小质量: {q.min():.4f}")
    print(f"  平均质量: {q.mean():.4f}")

    gmsh.write(output_file)

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    adaptive_remesh_stl("input.stl", "adaptive.msh")
```

## STL到实体转换

```python
#!/usr/bin/env python3
"""
案例7: STL转换为实体并生成体网格
"""
import gmsh
import sys

def stl_to_solid(input_file, output_file):
    """将STL转换为实体"""
    gmsh.initialize(sys.argv)

    # 设置严格的容差
    gmsh.option.setNumber("Geometry.Tolerance", 1e-6)
    gmsh.option.setNumber("Geometry.OCCSewFaces", 1)
    gmsh.option.setNumber("Geometry.OCCMakeSolids", 1)

    # 导入STL
    gmsh.merge(input_file)

    # 分类曲面（使用较小的角度以保留更多细节）
    gmsh.model.mesh.classifySurfaces(
        angle=20,
        boundary=True,
        forReparametrization=True
    )

    # 创建几何
    gmsh.model.mesh.createGeometry()
    gmsh.model.occ.synchronize()

    # 获取曲面
    surfaces = gmsh.model.getEntities(dim=2)
    surface_tags = [tag for dim, tag in surfaces]

    print(f"曲面数: {len(surfaces)}")

    # 尝试愈合并创建实体
    try:
        # 缝合所有曲面
        gmsh.model.occ.healShapes()

        # 创建曲面环
        sl = gmsh.model.occ.addSurfaceLoop(surface_tags)

        # 创建体积
        vol = gmsh.model.occ.addVolume([sl])

        gmsh.model.occ.synchronize()

        # 检查是否成功
        volumes = gmsh.model.getEntities(dim=3)
        if volumes:
            print(f"成功创建 {len(volumes)} 个体积")

            # 物理组
            gmsh.model.addPhysicalGroup(3, [v[1] for v in volumes], name="Solid")

            # 获取边界
            boundary = gmsh.model.getBoundary(volumes, oriented=False)
            boundary_tags = [tag for dim, tag in boundary]
            gmsh.model.addPhysicalGroup(2, boundary_tags, name="Surface")

            # 生成体网格
            gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)
            gmsh.option.setNumber("Mesh.Algorithm3D", 10)

            gmsh.model.mesh.generate(3)

            # 优化
            gmsh.model.mesh.optimize("Netgen")

        else:
            raise Exception("未能创建体积")

    except Exception as e:
        print(f"创建实体失败: {e}")
        print("仅生成表面网格")

        gmsh.model.addPhysicalGroup(2, surface_tags, name="Surface")
        gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)
        gmsh.model.mesh.generate(2)

    gmsh.write(output_file)

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    stl_to_solid("input.stl", "solid.msh")
```

## 批量STL处理

```python
#!/usr/bin/env python3
"""
案例7: 批量STL处理
"""
import gmsh
import sys
import os
import glob
import json

def process_stl_batch(input_dir, output_dir, mesh_size=0.1):
    """批量处理STL文件"""

    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)

    # 查找所有STL文件
    stl_files = glob.glob(os.path.join(input_dir, "*.stl"))
    print(f"找到 {len(stl_files)} 个STL文件")

    results = []

    for stl_file in stl_files:
        filename = os.path.basename(stl_file)
        output_file = os.path.join(output_dir, filename.replace(".stl", ".msh"))

        print(f"\n处理: {filename}")

        try:
            gmsh.initialize()

            # 导入
            gmsh.merge(stl_file)

            # 分类和创建几何
            gmsh.model.mesh.classifySurfaces(angle=30, boundary=True)
            gmsh.model.mesh.createGeometry()
            gmsh.model.occ.synchronize()

            # 获取统计
            surfaces = gmsh.model.getEntities(dim=2)

            # 网格设置
            gmsh.option.setNumber("Mesh.MeshSizeMax", mesh_size)
            gmsh.option.setNumber("Mesh.MeshSizeFromCurvature", 20)

            # 生成网格
            gmsh.model.mesh.generate(2)
            gmsh.model.mesh.optimize("Laplace2D")

            # 统计
            node_tags, _, _ = gmsh.model.mesh.getNodes()
            elem_types, elem_tags, _ = gmsh.model.mesh.getElements(dim=2)
            num_elems = sum(len(t) for t in elem_tags)

            # 保存
            gmsh.write(output_file)

            result = {
                "file": filename,
                "status": "success",
                "surfaces": len(surfaces),
                "nodes": len(node_tags),
                "elements": num_elems
            }

            print(f"  成功: {num_elems} 个元素")

        except Exception as e:
            result = {
                "file": filename,
                "status": "failed",
                "error": str(e)
            }
            print(f"  失败: {e}")

        finally:
            gmsh.finalize()

        results.append(result)

    # 保存报告
    report_file = os.path.join(output_dir, "processing_report.json")
    with open(report_file, "w") as f:
        json.dump(results, f, indent=2)

    # 统计
    success = sum(1 for r in results if r["status"] == "success")
    print(f"\n处理完成: {success}/{len(results)} 成功")
    print(f"报告保存到: {report_file}")

if __name__ == "__main__":
    process_stl_batch("stl_input", "stl_output", mesh_size=0.1)
```

## STL质量检查

```python
#!/usr/bin/env python3
"""
案例7: STL质量检查工具
"""
import gmsh
import sys
import numpy as np

def check_stl_quality(input_file):
    """检查STL文件质量"""
    gmsh.initialize()

    gmsh.merge(input_file)

    # 获取原始网格
    node_tags, coords, _ = gmsh.model.mesh.getNodes()
    elem_types, elem_tags, elem_nodes = gmsh.model.mesh.getElements(dim=2)

    coords = np.array(coords).reshape(-1, 3)
    num_nodes = len(node_tags)
    num_elems = sum(len(t) for t in elem_tags)

    print("=" * 50)
    print(f"STL文件: {input_file}")
    print("=" * 50)

    print(f"\n基本信息:")
    print(f"  三角形数: {num_elems}")
    print(f"  顶点数: {num_nodes}")

    # 边界框
    print(f"\n边界框:")
    print(f"  X: [{coords[:, 0].min():.4f}, {coords[:, 0].max():.4f}]")
    print(f"  Y: [{coords[:, 1].min():.4f}, {coords[:, 1].max():.4f}]")
    print(f"  Z: [{coords[:, 2].min():.4f}, {coords[:, 2].max():.4f}]")

    # 元素质量
    qualities = gmsh.model.mesh.getElementQualities([], "minSJ")
    if qualities:
        q = np.array(qualities)
        print(f"\n元素质量 (minSJ):")
        print(f"  最小: {q.min():.4f}")
        print(f"  最大: {q.max():.4f}")
        print(f"  平均: {q.mean():.4f}")
        print(f"  标准差: {q.std():.4f}")

        # 质量分布
        print(f"\n质量分布:")
        bins = [0, 0.1, 0.3, 0.5, 0.7, 0.9, 1.0]
        hist, _ = np.histogram(q, bins=bins)
        for i in range(len(bins) - 1):
            pct = hist[i] / len(q) * 100
            print(f"  [{bins[i]:.1f}, {bins[i+1]:.1f}): {hist[i]:5d} ({pct:5.1f}%)")

    # 检查退化三角形
    degenerate = np.sum(q < 0.01) if qualities else 0
    print(f"\n潜在问题:")
    print(f"  退化三角形 (质量<0.01): {degenerate}")

    # 检查重复节点
    gmsh.model.mesh.removeDuplicateNodes()
    node_tags_new, _, _ = gmsh.model.mesh.getNodes()
    duplicates = num_nodes - len(node_tags_new)
    print(f"  重复节点: {duplicates}")

    gmsh.finalize()

    return {
        "triangles": num_elems,
        "vertices": num_nodes,
        "quality_min": float(q.min()) if qualities else None,
        "quality_mean": float(q.mean()) if qualities else None,
        "degenerate": degenerate,
        "duplicates": duplicates
    }

if __name__ == "__main__":
    check_stl_quality("input.stl")
```

## 总结

本案例涵盖了：

1. **基本STL导入**: 使用classifySurfaces和createGeometry
2. **STL修复**: OCC修复选项
3. **自适应重划分**: 基于曲率的网格细化
4. **实体转换**: 从STL创建闭合体积
5. **批量处理**: 处理多个文件
6. **质量检查**: 诊断STL问题

## 下一步

- [08-周期性结构](./08-周期性结构.md) - 周期性网格生成
