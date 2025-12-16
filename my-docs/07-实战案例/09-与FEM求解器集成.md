# 与FEM求解器集成

本案例演示将Gmsh网格与有限元求解器集成的完整工作流。

## 案例目标

- 创建适合FEM分析的网格
- 导出多种求解器格式
- 提取边界条件信息
- 实现完整的预处理工作流

## 通用FEM网格准备

```python
#!/usr/bin/env python3
"""
案例9: 通用FEM网格准备
"""
import gmsh
import numpy as np
import json
import sys

class FEMMeshGenerator:
    """FEM网格生成器"""

    def __init__(self, name="fem_model"):
        gmsh.initialize(sys.argv)
        gmsh.model.add(name)
        self.name = name
        self.physical_groups = {}

    def create_geometry(self, geometry_type="plate_with_hole"):
        """创建几何模型"""
        if geometry_type == "plate_with_hole":
            self._create_plate_with_hole()
        elif geometry_type == "beam":
            self._create_beam()
        elif geometry_type == "bracket":
            self._create_bracket()

        gmsh.model.occ.synchronize()
        return self

    def _create_plate_with_hole(self):
        """带孔平板"""
        Lx, Ly, t = 100.0, 50.0, 10.0
        r = 10.0

        plate = gmsh.model.occ.addBox(0, 0, 0, Lx, Ly, t)
        hole = gmsh.model.occ.addCylinder(Lx/2, Ly/2, -1, 0, 0, t+2, r)
        gmsh.model.occ.cut([(3, plate)], [(3, hole)])

    def _create_beam(self):
        """简支梁"""
        L, H, B = 200.0, 20.0, 10.0
        gmsh.model.occ.addBox(0, 0, 0, L, H, B)

    def _create_bracket(self):
        """L型支架"""
        L1, L2 = 100.0, 80.0
        H, t = 50.0, 10.0

        part1 = gmsh.model.occ.addBox(0, 0, 0, L1, t, H)
        part2 = gmsh.model.occ.addBox(0, 0, 0, t, L2, H)
        gmsh.model.occ.fuse([(3, part1)], [(3, part2)])

    def setup_boundary_conditions(self):
        """设置边界条件物理组"""
        volumes = gmsh.model.getEntities(dim=3)
        if not volumes:
            volumes = gmsh.model.getEntities(dim=2)

        boundary = gmsh.model.getBoundary(volumes, oriented=False)

        # 分类边界
        boundaries = {"fixed": [], "loaded": [], "free": []}

        for dim, tag in boundary:
            com = gmsh.model.occ.getCenterOfMass(dim, tag)
            bbox = gmsh.model.getBoundingBox(dim, tag)

            # 根据位置分类（示例：左端固定，右端加载）
            if abs(bbox[0]) < 1e-6:  # x_min接近0
                boundaries["fixed"].append(tag)
            elif abs(bbox[3] - bbox[0]) < 1e-6:  # 薄面
                # 可能是侧面
                boundaries["free"].append(tag)
            else:
                boundaries["free"].append(tag)

        # 特殊处理：最右端面为加载面
        max_x = max(gmsh.model.getBoundingBox(dim, tag)[3]
                    for dim, tag in boundary)

        for dim, tag in boundary:
            bbox = gmsh.model.getBoundingBox(dim, tag)
            if abs(bbox[0] - max_x) < 1e-6 and abs(bbox[3] - max_x) < 1e-6:
                if tag in boundaries["free"]:
                    boundaries["free"].remove(tag)
                boundaries["loaded"].append(tag)

        # 创建物理组
        for name, tags in boundaries.items():
            if tags:
                dim = 2 if volumes[0][0] == 3 else 1
                pg = gmsh.model.addPhysicalGroup(dim, tags, name=name.capitalize())
                self.physical_groups[name] = {"tag": pg, "entities": tags}

        # 体积物理组
        vol_tags = [v[1] for v in volumes]
        pg = gmsh.model.addPhysicalGroup(volumes[0][0], vol_tags, name="Domain")
        self.physical_groups["domain"] = {"tag": pg, "entities": vol_tags}

        return self

    def generate_mesh(self, element_order=1, mesh_size=5.0, optimize=True):
        """生成网格"""
        gmsh.option.setNumber("Mesh.MeshSizeMax", mesh_size)
        gmsh.option.setNumber("Mesh.MeshSizeMin", mesh_size * 0.2)
        gmsh.option.setNumber("Mesh.ElementOrder", element_order)

        # 选择算法
        gmsh.option.setNumber("Mesh.Algorithm", 6)  # Frontal-Delaunay
        gmsh.option.setNumber("Mesh.Algorithm3D", 10)  # HXT

        # 生成网格
        dim = 3 if gmsh.model.getEntities(dim=3) else 2
        gmsh.model.mesh.generate(dim)

        # 优化
        if optimize:
            if dim == 3:
                gmsh.model.mesh.optimize("Netgen")
            gmsh.model.mesh.optimize("Laplace2D")

        return self

    def export_mesh(self, base_filename):
        """导出网格到多种格式"""
        formats = {
            "msh": f"{base_filename}.msh",
            "vtk": f"{base_filename}.vtk",
            "inp": f"{base_filename}.inp",    # Abaqus
            "bdf": f"{base_filename}.bdf",    # Nastran
            "unv": f"{base_filename}.unv",    # I-DEAS
        }

        exported = {}
        for fmt, filename in formats.items():
            try:
                gmsh.write(filename)
                exported[fmt] = filename
                print(f"  导出 {fmt}: {filename}")
            except Exception as e:
                print(f"  {fmt} 导出失败: {e}")

        return exported

    def get_mesh_info(self):
        """获取网格信息"""
        node_tags, coords, _ = gmsh.model.mesh.getNodes()
        coords = np.array(coords).reshape(-1, 3)

        elem_types, elem_tags, elem_nodes = gmsh.model.mesh.getElements()

        info = {
            "name": self.name,
            "num_nodes": len(node_tags),
            "bounding_box": {
                "xmin": float(coords[:, 0].min()),
                "xmax": float(coords[:, 0].max()),
                "ymin": float(coords[:, 1].min()),
                "ymax": float(coords[:, 1].max()),
                "zmin": float(coords[:, 2].min()),
                "zmax": float(coords[:, 2].max()),
            },
            "elements": {},
            "physical_groups": {}
        }

        for etype, etags in zip(elem_types, elem_tags):
            props = gmsh.model.mesh.getElementProperties(etype)
            info["elements"][props[0]] = {
                "count": len(etags),
                "dimension": props[1],
                "order": props[2],
                "nodes_per_element": props[3]
            }

        for name, data in self.physical_groups.items():
            info["physical_groups"][name] = {
                "tag": data["tag"],
                "num_entities": len(data["entities"])
            }

        return info

    def save_info(self, filename):
        """保存网格信息到JSON"""
        info = self.get_mesh_info()
        with open(filename, "w") as f:
            json.dump(info, f, indent=2)
        print(f"  信息保存: {filename}")

    def show(self):
        """显示GUI"""
        if "-nopopup" not in sys.argv:
            gmsh.fltk.run()

    def finalize(self):
        """清理"""
        gmsh.finalize()

def main():
    gen = FEMMeshGenerator("structural_analysis")

    print("1. 创建几何...")
    gen.create_geometry("plate_with_hole")

    print("2. 设置边界条件...")
    gen.setup_boundary_conditions()

    print("3. 生成网格...")
    gen.generate_mesh(element_order=2, mesh_size=3.0)

    print("4. 导出网格...")
    gen.export_mesh("fem_model")

    print("5. 保存信息...")
    gen.save_info("fem_model_info.json")

    info = gen.get_mesh_info()
    print(f"\n网格统计:")
    print(f"  节点数: {info['num_nodes']}")
    for name, data in info['elements'].items():
        print(f"  {name}: {data['count']} 个")

    gen.show()
    gen.finalize()

if __name__ == "__main__":
    main()
```

## 与开源求解器集成

### 与FEniCS集成

```python
#!/usr/bin/env python3
"""
案例9: 与FEniCS/DOLFINx集成
"""
import gmsh
import numpy as np

def create_mesh_for_fenics():
    """创建适合FEniCS的网格"""
    gmsh.initialize()
    gmsh.model.add("fenics_mesh")

    # 创建几何
    Lx, Ly = 1.0, 0.1
    rect = gmsh.model.occ.addRectangle(0, 0, 0, Lx, Ly)
    gmsh.model.occ.synchronize()

    # 获取边界
    boundary = gmsh.model.getBoundary([(2, rect)], oriented=False)

    left = []
    right = []

    for dim, tag in boundary:
        com = gmsh.model.occ.getCenterOfMass(dim, tag)
        if abs(com[0]) < 1e-6:
            left.append(tag)
        elif abs(com[0] - Lx) < 1e-6:
            right.append(tag)

    # 物理组（FEniCS需要）
    gmsh.model.addPhysicalGroup(1, left, tag=1, name="left")
    gmsh.model.addPhysicalGroup(1, right, tag=2, name="right")
    gmsh.model.addPhysicalGroup(2, [rect], tag=10, name="domain")

    # 生成网格
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.02)
    gmsh.model.mesh.generate(2)

    # 保存为MSH格式（FEniCS支持）
    gmsh.write("fenics_mesh.msh")

    gmsh.finalize()

    # 示例：使用meshio转换（如果需要XDMF格式）
    try:
        import meshio
        msh = meshio.read("fenics_mesh.msh")

        # 提取三角形单元
        cells = {"triangle": msh.cells_dict["triangle"]}
        mesh = meshio.Mesh(msh.points[:, :2], cells)
        meshio.write("fenics_mesh.xdmf", mesh)
        print("已转换为XDMF格式")
    except ImportError:
        print("meshio未安装，跳过XDMF转换")

if __name__ == "__main__":
    create_mesh_for_fenics()
```

### 与deal.II集成

```python
#!/usr/bin/env python3
"""
案例9: 与deal.II集成
"""
import gmsh

def create_mesh_for_dealii():
    """创建适合deal.II的网格"""
    gmsh.initialize()
    gmsh.model.add("dealii_mesh")

    # deal.II偏好六面体网格
    L = 1.0
    N = 10

    # 创建结构化六面体网格
    # 使用geo内核以便设置transfinite

    # 创建点
    lc = L / N
    points = []
    for k in range(2):
        for j in range(2):
            for i in range(2):
                p = gmsh.model.geo.addPoint(i * L, j * L, k * L, lc)
                points.append(p)

    # 创建线
    lines = [
        gmsh.model.geo.addLine(points[0], points[1]),  # 底面
        gmsh.model.geo.addLine(points[1], points[3]),
        gmsh.model.geo.addLine(points[3], points[2]),
        gmsh.model.geo.addLine(points[2], points[0]),
        gmsh.model.geo.addLine(points[4], points[5]),  # 顶面
        gmsh.model.geo.addLine(points[5], points[7]),
        gmsh.model.geo.addLine(points[7], points[6]),
        gmsh.model.geo.addLine(points[6], points[4]),
        gmsh.model.geo.addLine(points[0], points[4]),  # 竖边
        gmsh.model.geo.addLine(points[1], points[5]),
        gmsh.model.geo.addLine(points[2], points[6]),
        gmsh.model.geo.addLine(points[3], points[7]),
    ]

    # 创建面
    loops = [
        gmsh.model.geo.addCurveLoop([lines[0], lines[1], lines[2], lines[3]]),
        gmsh.model.geo.addCurveLoop([lines[4], lines[5], lines[6], lines[7]]),
        gmsh.model.geo.addCurveLoop([lines[0], lines[9], -lines[4], -lines[8]]),
        gmsh.model.geo.addCurveLoop([-lines[2], lines[10], lines[6], -lines[11]]),
        gmsh.model.geo.addCurveLoop([-lines[3], lines[10], -lines[7], -lines[8]]),
        gmsh.model.geo.addCurveLoop([lines[1], lines[11], -lines[5], -lines[9]]),
    ]

    surfaces = [gmsh.model.geo.addPlaneSurface([l]) for l in loops]

    # 创建体积
    sl = gmsh.model.geo.addSurfaceLoop(surfaces)
    vol = gmsh.model.geo.addVolume([sl])

    gmsh.model.geo.synchronize()

    # 设置Transfinite
    for l in lines:
        gmsh.model.mesh.setTransfiniteCurve(l, N + 1)

    for s in surfaces:
        gmsh.model.mesh.setTransfiniteSurface(s)
        gmsh.model.mesh.setRecombine(2, s)

    gmsh.model.mesh.setTransfiniteVolume(vol)

    # 物理组
    gmsh.model.addPhysicalGroup(3, [vol], name="domain")

    # 生成网格
    gmsh.model.mesh.generate(3)

    # deal.II支持的格式
    gmsh.write("dealii_mesh.msh")

    gmsh.finalize()

if __name__ == "__main__":
    create_mesh_for_dealii()
```

## 边界条件提取

```python
#!/usr/bin/env python3
"""
案例9: 提取边界条件信息
"""
import gmsh
import numpy as np
import json

def extract_boundary_info(mesh_file):
    """从网格提取边界条件信息"""
    gmsh.initialize()
    gmsh.merge(mesh_file)

    # 获取物理组
    groups = gmsh.model.getPhysicalGroups()

    bc_info = {}

    for dim, tag in groups:
        name = gmsh.model.getPhysicalName(dim, tag)
        entities = gmsh.model.getEntitiesForPhysicalGroup(dim, tag)

        # 获取该物理组的节点
        all_nodes = []
        for entity_tag in entities:
            node_tags, coords, _ = gmsh.model.mesh.getNodes(dim, entity_tag)
            all_nodes.extend(node_tags)

        all_nodes = list(set(all_nodes))

        # 获取节点坐标
        if all_nodes:
            coords = []
            for node in all_nodes:
                _, c, _ = gmsh.model.mesh.getNodes(dim, -1)
                # 简化：获取所有节点并筛选

            node_tags, node_coords, _ = gmsh.model.mesh.getNodes()
            node_coords = np.array(node_coords).reshape(-1, 3)
            node_dict = {t: c for t, c in zip(node_tags, node_coords)}

            coords = np.array([node_dict[n] for n in all_nodes if n in node_dict])

            bc_info[name] = {
                "dimension": dim,
                "tag": tag,
                "num_nodes": len(all_nodes),
                "node_tags": all_nodes,
                "centroid": coords.mean(axis=0).tolist() if len(coords) > 0 else None,
                "bbox": {
                    "min": coords.min(axis=0).tolist() if len(coords) > 0 else None,
                    "max": coords.max(axis=0).tolist() if len(coords) > 0 else None,
                }
            }

    gmsh.finalize()
    return bc_info

def generate_bc_file(bc_info, output_file):
    """生成边界条件输入文件"""
    with open(output_file, "w") as f:
        f.write("# 边界条件定义\n")
        f.write("# 由Gmsh自动生成\n\n")

        for name, info in bc_info.items():
            f.write(f"# {name}\n")
            f.write(f"# 维度: {info['dimension']}\n")
            f.write(f"# 节点数: {info['num_nodes']}\n")

            if info['centroid']:
                f.write(f"# 中心: ({info['centroid'][0]:.4f}, "
                        f"{info['centroid'][1]:.4f}, {info['centroid'][2]:.4f})\n")

            f.write(f"*BOUNDARY, NAME={name}\n")
            for node in info['node_tags'][:10]:  # 仅显示前10个
                f.write(f"  {node}\n")
            if len(info['node_tags']) > 10:
                f.write(f"  ... (共{len(info['node_tags'])}个节点)\n")
            f.write("\n")

    print(f"边界条件文件已生成: {output_file}")

if __name__ == "__main__":
    # 首先创建示例网格
    gmsh.initialize()
    gmsh.model.add("test")
    box = gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
    gmsh.model.occ.synchronize()

    boundary = gmsh.model.getBoundary([(3, box)], oriented=False)
    gmsh.model.addPhysicalGroup(2, [boundary[0][1]], name="fixed")
    gmsh.model.addPhysicalGroup(2, [boundary[1][1]], name="loaded")
    gmsh.model.addPhysicalGroup(3, [box], name="domain")

    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.2)
    gmsh.model.mesh.generate(3)
    gmsh.write("test_bc.msh")
    gmsh.finalize()

    # 提取边界信息
    bc_info = extract_boundary_info("test_bc.msh")

    # 保存JSON
    with open("boundary_conditions.json", "w") as f:
        json.dump(bc_info, f, indent=2, default=str)

    # 生成边界条件文件
    generate_bc_file(bc_info, "boundary_conditions.inp")
```

## 完整预处理工作流

```python
#!/usr/bin/env python3
"""
案例9: 完整FEM预处理工作流
"""
import gmsh
import numpy as np
import json
import sys
import os

def complete_fem_workflow(output_dir="fem_output"):
    """完整FEM预处理工作流"""

    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)

    gmsh.initialize(sys.argv)
    gmsh.model.add("complete_workflow")

    print("=" * 60)
    print("FEM预处理工作流")
    print("=" * 60)

    # 1. 几何创建
    print("\n1. 创建几何...")
    L, H, t = 100.0, 20.0, 10.0
    r = 5.0

    beam = gmsh.model.occ.addBox(0, -H/2, -t/2, L, H, t)
    hole = gmsh.model.occ.addCylinder(L/2, 0, -t/2-1, 0, 0, t+2, r)
    gmsh.model.occ.cut([(3, beam)], [(3, hole)])
    gmsh.model.occ.synchronize()

    print(f"  梁尺寸: {L} x {H} x {t}")
    print(f"  孔半径: {r}")

    # 2. 物理组定义
    print("\n2. 定义物理组...")
    volumes = gmsh.model.getEntities(dim=3)
    boundary = gmsh.model.getBoundary(volumes, oriented=False)

    groups = {
        "fixed": [],
        "loaded": [],
        "hole": [],
        "free": []
    }

    for dim, tag in boundary:
        bbox = gmsh.model.getBoundingBox(dim, tag)
        com = gmsh.model.occ.getCenterOfMass(dim, tag)

        if abs(bbox[0]) < 1e-6 and abs(bbox[3]) < 1e-6:
            groups["fixed"].append(tag)
        elif abs(bbox[0] - L) < 1e-6 and abs(bbox[3] - L) < 1e-6:
            groups["loaded"].append(tag)
        elif abs(com[0] - L/2) < r + 1 and abs(com[1]) < r + 1:
            groups["hole"].append(tag)
        else:
            groups["free"].append(tag)

    for name, tags in groups.items():
        if tags:
            gmsh.model.addPhysicalGroup(2, tags, name=name.capitalize())
            print(f"  {name.capitalize()}: {len(tags)} 个面")

    vol_tags = [v[1] for v in volumes]
    gmsh.model.addPhysicalGroup(3, vol_tags, name="Solid")

    # 3. 网格控制
    print("\n3. 设置网格控制...")

    # 孔边缘加密
    if groups["hole"]:
        dist = gmsh.model.mesh.field.add("Distance")
        gmsh.model.mesh.field.setNumbers(dist, "SurfacesList", groups["hole"])

        thresh = gmsh.model.mesh.field.add("Threshold")
        gmsh.model.mesh.field.setNumber(thresh, "InField", dist)
        gmsh.model.mesh.field.setNumber(thresh, "SizeMin", 0.5)
        gmsh.model.mesh.field.setNumber(thresh, "SizeMax", 3.0)
        gmsh.model.mesh.field.setNumber(thresh, "DistMin", 1.0)
        gmsh.model.mesh.field.setNumber(thresh, "DistMax", 10.0)

        gmsh.model.mesh.field.setAsBackgroundMesh(thresh)

    gmsh.option.setNumber("Mesh.MeshSizeExtendFromBoundary", 0)
    gmsh.option.setNumber("Mesh.MeshSizeFromPoints", 0)

    # 4. 网格生成
    print("\n4. 生成网格...")
    gmsh.option.setNumber("Mesh.ElementOrder", 2)
    gmsh.option.setNumber("Mesh.Algorithm3D", 10)
    gmsh.model.mesh.generate(3)
    gmsh.model.mesh.optimize("Netgen")

    # 5. 网格统计
    print("\n5. 网格统计...")
    node_tags, coords, _ = gmsh.model.mesh.getNodes()
    coords = np.array(coords).reshape(-1, 3)

    elem_types, elem_tags, _ = gmsh.model.mesh.getElements(dim=3)

    print(f"  节点数: {len(node_tags)}")
    for etype, etags in zip(elem_types, elem_tags):
        props = gmsh.model.mesh.getElementProperties(etype)
        print(f"  {props[0]}: {len(etags)} 个")

    # 网格质量
    qualities = gmsh.model.mesh.getElementQualities([], "minSJ")
    q = np.array(qualities)
    print(f"  质量范围: [{q.min():.4f}, {q.max():.4f}]")
    print(f"  平均质量: {q.mean():.4f}")

    # 6. 导出文件
    print("\n6. 导出文件...")
    base = os.path.join(output_dir, "model")

    gmsh.write(f"{base}.msh")
    print(f"  Gmsh MSH: {base}.msh")

    gmsh.write(f"{base}.vtk")
    print(f"  VTK: {base}.vtk")

    try:
        gmsh.write(f"{base}.inp")
        print(f"  Abaqus INP: {base}.inp")
    except:
        pass

    # 7. 保存元数据
    print("\n7. 保存元数据...")
    metadata = {
        "geometry": {
            "length": L,
            "height": H,
            "thickness": t,
            "hole_radius": r
        },
        "mesh": {
            "num_nodes": len(node_tags),
            "num_elements": sum(len(t) for t in elem_tags),
            "element_order": 2,
            "quality": {
                "min": float(q.min()),
                "max": float(q.max()),
                "mean": float(q.mean())
            }
        },
        "physical_groups": {
            name: len(tags) for name, tags in groups.items() if tags
        },
        "bounding_box": {
            "xmin": float(coords[:, 0].min()),
            "xmax": float(coords[:, 0].max()),
            "ymin": float(coords[:, 1].min()),
            "ymax": float(coords[:, 1].max()),
            "zmin": float(coords[:, 2].min()),
            "zmax": float(coords[:, 2].max()),
        }
    }

    meta_file = os.path.join(output_dir, "metadata.json")
    with open(meta_file, "w") as f:
        json.dump(metadata, f, indent=2)
    print(f"  元数据: {meta_file}")

    print("\n" + "=" * 60)
    print("预处理完成!")
    print("=" * 60)

    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    complete_fem_workflow()
```

## 总结

本案例涵盖了：

1. **通用网格准备**: 参数化几何和网格生成
2. **求解器集成**: FEniCS、deal.II等
3. **边界条件提取**: 自动识别和分类
4. **多格式导出**: MSH、VTK、INP等
5. **完整工作流**: 从几何到分析准备

## 下一步

- [10-波导结构网格](./10-波导结构网格.md) - 电磁应用
