# GUI交互

Gmsh提供了基于FLTK的GUI界面，可以通过API进行控制。

## gmsh.fltk模块概览

```python
import gmsh

gmsh.fltk.initialize()      # 初始化GUI
gmsh.fltk.run()             # 运行GUI事件循环
gmsh.fltk.update()          # 更新显示
gmsh.fltk.wait()            # 等待事件
gmsh.fltk.lock()            # 锁定GUI
gmsh.fltk.unlock()          # 解锁GUI
gmsh.fltk.awake()           # 唤醒GUI线程
gmsh.fltk.isAvailable()     # 检查GUI是否可用
gmsh.fltk.selectEntities()  # 交互选择实体
gmsh.fltk.selectElements()  # 交互选择元素
gmsh.fltk.selectViews()     # 交互选择视图
```

## 基本GUI操作

### 运行GUI

```python
#!/usr/bin/env python3
"""
基本GUI运行示例
"""
import gmsh
import sys

gmsh.initialize(sys.argv)
gmsh.model.add("gui_example")

# 创建几何
gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 检查GUI是否可用
if gmsh.fltk.isAvailable():
    # 运行GUI
    gmsh.fltk.run()
else:
    print("GUI不可用")

gmsh.finalize()
```

### 非阻塞GUI更新

```python
#!/usr/bin/env python3
"""
非阻塞GUI更新示例
"""
import gmsh
import time

gmsh.initialize()
gmsh.model.add("non_blocking")

# 创建初始几何
gmsh.model.occ.addSphere(0, 0, 0, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 初始化GUI（但不阻塞）
gmsh.fltk.initialize()

# 动态更新
for i in range(10):
    gmsh.clear()
    gmsh.model.add(f"step_{i}")

    # 创建变化的几何
    radius = 0.5 + 0.05 * i
    gmsh.model.occ.addSphere(0, 0, 0, radius)
    gmsh.model.occ.synchronize()
    gmsh.model.mesh.generate(3)

    # 更新显示
    gmsh.fltk.update()

    # 处理事件（非阻塞）
    gmsh.fltk.wait(0.1)  # 等待0.1秒

    time.sleep(0.5)

# 最终运行GUI
gmsh.fltk.run()
gmsh.finalize()
```

## 交互选择

### 选择实体

```python
#!/usr/bin/env python3
"""
交互选择几何实体
"""
import gmsh

gmsh.initialize()
gmsh.model.add("select_entities")

# 创建多个几何
for i in range(3):
    for j in range(3):
        gmsh.model.occ.addBox(i * 2, j * 2, 0, 1, 1, 1)

gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 初始化GUI
gmsh.fltk.initialize()

print("请在GUI中选择实体，然后按'e'确认或'q'退出")

while True:
    # selectEntities(dim=-1)
    # 返回: dimTags列表
    # dim=-1: 选择任意维度
    # dim=0: 选择点
    # dim=1: 选择曲线
    # dim=2: 选择曲面
    # dim=3: 选择体积

    result = gmsh.fltk.selectEntities(dim=-1)

    if result:
        print(f"选择的实体: {result}")

        # 高亮选中的实体
        for dim, tag in result:
            # 获取实体的边界框
            xmin, ymin, zmin, xmax, ymax, zmax = gmsh.model.getBoundingBox(dim, tag)
            print(f"  ({dim}, {tag}): 边界框 = "
                  f"({xmin:.2f}, {ymin:.2f}, {zmin:.2f}) - "
                  f"({xmax:.2f}, {ymax:.2f}, {zmax:.2f})")
    else:
        print("没有选择或用户取消")
        break

gmsh.finalize()
```

### 选择元素

```python
#!/usr/bin/env python3
"""
交互选择网格元素
"""
import gmsh

gmsh.initialize()
gmsh.model.add("select_elements")

gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

gmsh.fltk.initialize()

print("请在GUI中选择元素，然后按'e'确认或'q'退出")

while True:
    # selectElements()
    # 返回: 元素tag列表
    result = gmsh.fltk.selectElements()

    if result:
        print(f"选择的元素数: {len(result)}")

        for elem_tag in result[:5]:  # 只显示前5个
            # 获取元素类型
            elem_type, elem_tags, node_tags = gmsh.model.mesh.getElements()

            print(f"  元素 {elem_tag}")
    else:
        print("没有选择或用户取消")
        break

gmsh.finalize()
```

### 选择视图

```python
#!/usr/bin/env python3
"""
交互选择后处理视图
"""
import gmsh
import math

gmsh.initialize()
gmsh.model.add("select_views")

gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 创建多个视图
node_tags, coords, _ = gmsh.model.mesh.getNodes()

for view_name in ["Temperature", "Pressure", "Velocity"]:
    view = gmsh.view.add(view_name)

    # 生成随机数据
    data = []
    for i in range(len(node_tags)):
        val = math.sin(coords[i * 3]) * math.cos(coords[i * 3 + 1])
        data.append([val])

    gmsh.view.addModelData(view, 0, "select_views", "NodeData",
                           list(node_tags), data)

gmsh.fltk.initialize()

print("请在GUI中选择视图，然后按'e'确认或'q'退出")

while True:
    # selectViews()
    # 返回: 视图tag列表
    result = gmsh.fltk.selectViews()

    if result:
        print(f"选择的视图: {result}")

        for view_tag in result:
            name = gmsh.view.option.getString(view_tag, "Name")
            print(f"  视图 {view_tag}: {name}")
    else:
        print("没有选择或用户取消")
        break

gmsh.finalize()
```

## GUI事件处理

### 等待事件

```python
#!/usr/bin/env python3
"""
GUI事件等待示例
"""
import gmsh

gmsh.initialize()
gmsh.model.add("events")

gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

gmsh.fltk.initialize()

print("GUI已启动，按Escape退出")

# 持续等待事件
while True:
    # wait(time=-1)
    # time=-1: 无限等待
    # time=0: 不等待
    # time>0: 等待指定秒数
    gmsh.fltk.wait(0.1)  # 每0.1秒检查一次

    # 检查是否应该退出
    # 这里可以添加自定义逻辑
    # 例如检查某个条件或用户输入

gmsh.finalize()
```

### 线程安全操作

```python
#!/usr/bin/env python3
"""
多线程GUI操作示例
"""
import gmsh
import threading
import time

def background_task():
    """后台任务"""
    for i in range(10):
        # 锁定GUI
        gmsh.fltk.lock()

        try:
            # 修改几何
            gmsh.clear()
            gmsh.model.add(f"step_{i}")
            gmsh.model.occ.addSphere(0, 0, 0, 0.5 + 0.05 * i)
            gmsh.model.occ.synchronize()
            gmsh.model.mesh.generate(3)
        finally:
            # 解锁GUI
            gmsh.fltk.unlock()

        # 唤醒GUI线程更新显示
        gmsh.fltk.awake("update")

        time.sleep(0.5)

def main():
    gmsh.initialize()
    gmsh.model.add("threading")

    gmsh.model.occ.addSphere(0, 0, 0, 0.5)
    gmsh.model.occ.synchronize()
    gmsh.model.mesh.generate(3)

    gmsh.fltk.initialize()

    # 启动后台线程
    thread = threading.Thread(target=background_task)
    thread.start()

    # 运行GUI主循环
    gmsh.fltk.run()

    thread.join()
    gmsh.finalize()

if __name__ == "__main__":
    main()
```

## 自定义GUI

### 添加菜单和按钮

```python
#!/usr/bin/env python3
"""
通过脚本控制GUI行为
"""
import gmsh
import sys

# 定义回调动作
def regenerate_mesh():
    """重新生成网格"""
    gmsh.model.mesh.clear()
    gmsh.model.mesh.generate(3)
    gmsh.fltk.update()

def refine_mesh():
    """细化网格"""
    gmsh.model.mesh.refine()
    gmsh.fltk.update()

def export_mesh():
    """导出网格"""
    gmsh.write("output.msh")
    print("网格已导出到 output.msh")

def main():
    gmsh.initialize(sys.argv)
    gmsh.model.add("custom_gui")

    # 创建几何
    gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
    gmsh.model.occ.synchronize()

    # 设置网格尺寸
    gmsh.option.setNumber("Mesh.MeshSizeMax", 0.2)
    gmsh.model.mesh.generate(3)

    # 运行GUI
    if "-nopopup" not in sys.argv:
        gmsh.fltk.run()

    gmsh.finalize()

if __name__ == "__main__":
    main()
```

### 设置相机和视角

```python
#!/usr/bin/env python3
"""
控制相机和视角
"""
import gmsh
import math

gmsh.initialize()
gmsh.model.add("camera")

gmsh.model.occ.addBox(0, 0, 0, 1, 1, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 设置相机位置
# 旋转角度（度）
gmsh.option.setNumber("General.RotationX", 45)
gmsh.option.setNumber("General.RotationY", 30)
gmsh.option.setNumber("General.RotationZ", 0)

# 平移
gmsh.option.setNumber("General.TranslationX", 0)
gmsh.option.setNumber("General.TranslationY", 0)

# 缩放
gmsh.option.setNumber("General.ScaleX", 1)
gmsh.option.setNumber("General.ScaleY", 1)
gmsh.option.setNumber("General.ScaleZ", 1)

# 设置裁剪平面
gmsh.option.setNumber("General.Clip0A", 1)  # 法向X分量
gmsh.option.setNumber("General.Clip0B", 0)  # 法向Y分量
gmsh.option.setNumber("General.Clip0C", 0)  # 法向Z分量
gmsh.option.setNumber("General.Clip0D", -0.5)  # 距离
gmsh.option.setNumber("General.ClipWholeElements", 1)

# 启用裁剪
gmsh.option.setNumber("General.Clip0", 1)

gmsh.fltk.run()
gmsh.finalize()
```

## 图像和动画导出

### 截图保存

```python
#!/usr/bin/env python3
"""
保存GUI截图
"""
import gmsh

gmsh.initialize()
gmsh.model.add("screenshot")

gmsh.model.occ.addSphere(0, 0, 0, 1)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 设置显示选项
gmsh.option.setNumber("Mesh.SurfaceEdges", 0)
gmsh.option.setNumber("Mesh.SurfaceFaces", 1)

# 初始化GUI
gmsh.fltk.initialize()

# 设置视角
gmsh.option.setNumber("General.RotationX", 30)
gmsh.option.setNumber("General.RotationY", 45)

# 更新显示
gmsh.fltk.update()

# 保存截图
# write(fileName) - 根据扩展名自动选择格式
gmsh.write("screenshot.png")
gmsh.write("screenshot.pdf")
gmsh.write("screenshot.svg")

print("截图已保存")

gmsh.fltk.run()
gmsh.finalize()
```

### 创建动画

```python
#!/usr/bin/env python3
"""
创建旋转动画
"""
import gmsh
import os

gmsh.initialize()
gmsh.model.add("animation")

gmsh.model.occ.addTorus(0, 0, 0, 1, 0.3)
gmsh.model.occ.synchronize()
gmsh.model.mesh.generate(3)

# 设置显示
gmsh.option.setNumber("Mesh.SurfaceEdges", 0)
gmsh.option.setNumber("Mesh.SurfaceFaces", 1)
gmsh.option.setColor("Mesh.SurfaceFaces", 100, 150, 200, 255)

gmsh.fltk.initialize()

# 创建帧目录
os.makedirs("frames", exist_ok=True)

# 生成旋转帧
num_frames = 36
for i in range(num_frames):
    angle = i * 360.0 / num_frames

    # 设置旋转
    gmsh.option.setNumber("General.RotationX", 20)
    gmsh.option.setNumber("General.RotationY", angle)

    # 更新显示
    gmsh.fltk.update()

    # 保存帧
    gmsh.write(f"frames/frame_{i:03d}.png")
    print(f"帧 {i+1}/{num_frames}")

print("\n动画帧已保存到 frames/ 目录")
print("使用ffmpeg合成视频:")
print("  ffmpeg -framerate 10 -i frames/frame_%03d.png -c:v libx264 animation.mp4")

gmsh.finalize()
```

### 时间步动画

```python
#!/usr/bin/env python3
"""
时间步数据动画
"""
import gmsh
import math
import os

gmsh.initialize()
gmsh.model.add("time_animation")

# 创建网格
gmsh.model.occ.addDisk(0, 0, 0, 1, 1)
gmsh.model.occ.synchronize()
gmsh.option.setNumber("Mesh.MeshSizeMax", 0.1)
gmsh.model.mesh.generate(2)

# 获取节点
node_tags, coords, _ = gmsh.model.mesh.getNodes()

# 创建视图
view = gmsh.view.add("Wave")

# 添加多个时间步
num_steps = 20
for step in range(num_steps):
    time = step * 0.1
    data = []

    for i in range(len(node_tags)):
        x = coords[i * 3]
        y = coords[i * 3 + 1]
        r = math.sqrt(x*x + y*y)
        val = math.sin(4 * math.pi * r - 2 * math.pi * time)
        data.append([val])

    gmsh.view.addModelData(view, step, "time_animation", "NodeData",
                           list(node_tags), data, time=time)

# 设置视图选项
gmsh.view.option.setNumber(view, "RangeType", 2)  # 自定义范围
gmsh.view.option.setNumber(view, "CustomMin", -1)
gmsh.view.option.setNumber(view, "CustomMax", 1)

# 初始化GUI
gmsh.fltk.initialize()

# 创建帧目录
os.makedirs("time_frames", exist_ok=True)

# 导出每个时间步
for step in range(num_steps):
    gmsh.view.option.setNumber(view, "TimeStep", step)
    gmsh.fltk.update()
    gmsh.write(f"time_frames/step_{step:03d}.png")

print(f"已导出 {num_steps} 帧到 time_frames/ 目录")

# 运行GUI播放动画
gmsh.option.setNumber("PostProcessing.AnimationCycle", 1)
gmsh.option.setNumber("PostProcessing.AnimationDelay", 100)
gmsh.fltk.run()

gmsh.finalize()
```

## GUI选项

### 窗口设置

```python
# 窗口尺寸
gmsh.option.setNumber("General.GraphicsWidth", 800)
gmsh.option.setNumber("General.GraphicsHeight", 600)

# 窗口位置
gmsh.option.setNumber("General.GraphicsPositionX", 100)
gmsh.option.setNumber("General.GraphicsPositionY", 100)

# 全屏模式
gmsh.option.setNumber("General.Fullscreen", 0)

# 高DPI缩放
gmsh.option.setNumber("General.HighResolutionGraphics", 1)
```

### 背景和颜色

```python
# 背景颜色
gmsh.option.setColor("General.Background", 255, 255, 255, 255)
gmsh.option.setColor("General.BackgroundGradient", 200, 200, 255, 255)

# 渐变背景
gmsh.option.setNumber("General.BackgroundGradient", 1)

# 前景颜色
gmsh.option.setColor("General.Foreground", 0, 0, 0, 255)

# 文本颜色
gmsh.option.setColor("General.Text", 0, 0, 0, 255)
```

### 显示元素

```python
# 坐标轴
gmsh.option.setNumber("General.Axes", 1)
gmsh.option.setNumber("General.AxesAutoPosition", 1)
gmsh.option.setNumber("General.AxesMicroMax", 10)
gmsh.option.setNumber("General.AxesTicsX", 5)

# 比例尺
gmsh.option.setNumber("General.SmallAxes", 1)
gmsh.option.setNumber("General.SmallAxesPositionX", -60)
gmsh.option.setNumber("General.SmallAxesPositionY", -40)

# 颜色条
gmsh.option.setNumber("General.ColorbarSize", 16)
```

## 完整GUI应用示例

```python
#!/usr/bin/env python3
"""
完整的GUI交互应用示例
"""
import gmsh
import sys
import math

class GmshApp:
    """Gmsh GUI应用类"""

    def __init__(self):
        gmsh.initialize(sys.argv)
        self.model_name = "gui_app"
        gmsh.model.add(self.model_name)

    def create_geometry(self):
        """创建几何"""
        # 创建带孔的板
        plate = gmsh.model.occ.addRectangle(-2, -1, 0, 4, 2)
        hole = gmsh.model.occ.addDisk(0, 0, 0, 0.3, 0.3)
        gmsh.model.occ.cut([(2, plate)], [(2, hole)])
        gmsh.model.occ.synchronize()

    def generate_mesh(self, size=0.1):
        """生成网格"""
        gmsh.option.setNumber("Mesh.MeshSizeMax", size)
        gmsh.model.mesh.generate(2)

    def add_field_data(self):
        """添加场数据"""
        node_tags, coords, _ = gmsh.model.mesh.getNodes()

        # 应力场模拟
        view = gmsh.view.add("Stress")
        data = []
        for i in range(len(node_tags)):
            x = coords[i * 3]
            y = coords[i * 3 + 1]
            r = math.sqrt(x*x + y*y)
            # 孔周围应力集中
            stress = 1.0 / max(r - 0.3, 0.01)
            stress = min(stress, 10)  # 限制最大值
            data.append([stress])

        gmsh.view.addModelData(view, 0, self.model_name, "NodeData",
                               list(node_tags), data)

        # 设置视图选项
        gmsh.view.option.setNumber(view, "IntervalsType", 2)
        gmsh.view.option.setNumber(view, "NbIso", 20)

    def setup_display(self):
        """设置显示选项"""
        # 背景
        gmsh.option.setColor("General.Background", 30, 30, 30, 255)
        gmsh.option.setColor("General.Foreground", 255, 255, 255, 255)

        # 网格显示
        gmsh.option.setNumber("Mesh.SurfaceEdges", 1)
        gmsh.option.setNumber("Mesh.SurfaceFaces", 0)

        # 几何显示
        gmsh.option.setNumber("Geometry.Points", 0)
        gmsh.option.setNumber("Geometry.Curves", 1)

    def interactive_loop(self):
        """交互循环"""
        gmsh.fltk.initialize()

        print("\n=== Gmsh GUI 应用 ===")
        print("操作说明:")
        print("  - 点击选择实体")
        print("  - 拖动旋转视图")
        print("  - 滚轮缩放")
        print("  - 按 'e' 确认选择")
        print("  - 按 'q' 或关闭窗口退出")
        print("=" * 25)

        gmsh.fltk.run()

    def export(self, filename):
        """导出结果"""
        gmsh.write(filename)
        print(f"已导出: {filename}")

    def cleanup(self):
        """清理"""
        gmsh.finalize()

    def run(self):
        """运行应用"""
        self.create_geometry()
        self.generate_mesh(size=0.05)
        self.add_field_data()
        self.setup_display()

        self.export("gui_app_result.msh")

        if "-nopopup" not in sys.argv:
            self.interactive_loop()

        self.cleanup()

def main():
    app = GmshApp()
    app.run()

if __name__ == "__main__":
    main()
```

## GUI API汇总

| 函数 | 描述 |
|------|------|
| `fltk.initialize()` | 初始化GUI |
| `fltk.run()` | 运行GUI事件循环（阻塞） |
| `fltk.update()` | 更新显示 |
| `fltk.wait(time)` | 等待事件 |
| `fltk.lock()` | 锁定GUI（多线程） |
| `fltk.unlock()` | 解锁GUI |
| `fltk.awake(action)` | 唤醒GUI线程 |
| `fltk.isAvailable()` | 检查GUI可用性 |
| `fltk.selectEntities(dim)` | 交互选择实体 |
| `fltk.selectElements()` | 交互选择元素 |
| `fltk.selectViews()` | 交互选择视图 |

## 下一步

- [05-并行与多线程](./05-并行与多线程.md) - 学习并行计算方法
