# PointCloudPractice —— 石油管道环焊缝点云练习台

一个**用来学 C++ 项目开发和传统几何点云处理**的练手项目。

它不是毕业论文的交付物，也不是一个能用的焊接系统。它的唯一目的是：让你在一个真实的、
能编译、能运行、能看到数字变化的项目里，把下面这些东西真正学会一遍：

- 多个 `.h` / `.cpp` 文件怎么组织、怎么互相调用
- CMake 到底在干什么，`configure` 和 `build` 差在哪
- 编译、链接、运行时 DLL 是三件不同的事
- Git 本地版本管理怎么用（不涉及 GitHub）
- 体素降采样、统计滤波、半径滤波、KD-Tree、PassThrough、RANSAC、欧氏聚类
  这七个传统算法各自解决什么问题、参数怎么影响结果

---

## 一、这个项目里有什么

一份**固定不变**的合成原始点云 `data/scene_original.pcd`，里面有三部分东西：

| 部分 | 点数（默认场景） | 用来练什么 |
|---|---|---|
| 圆柱形管道（带环焊缝坡口） | 25920 | 圆柱面分割、点云切片、焊缝特征 |
| 与管道空间分离的平板 | 4131 | RANSAC 平面拟合 |
| 随机飞点 | 200 | 统计滤波、半径滤波 |

七个算法**各自独立**读这份文件、各自独立跑、各自把结果写到 `output/`。
任何算法都不会改写 `data/scene_original.pcd`（代码里有硬保护，详见
[docs/05_编译链接与调试.md](docs/05_编译链接与调试.md) 的实测部分）。

---

## 二、环境要求

本项目在下面这套环境里**实际配置并编译运行通过**：

| 组件 | 实际使用的版本 / 位置 |
|---|---|
| 操作系统 | Windows |
| CMake | 4.4.3，`C:\Program Files\CMake\bin\cmake.exe` |
| 生成器 | `Visual Studio 18 2026`（VS2026，MSVC 14.51） |
| 备选生成器 | `Visual Studio 17 2022`（VS2022 Build Tools，MSVC 14.44）—— 也实测通过 |
| PCL | 1.15.1，`E:\PointCloudEnv\PCL` |
| C++ 标准 | C++17 |

### 运行前必须先激活 PCL 环境

程序的 exe 依赖 PCL 和 VTK 的一堆 `.dll`。如果不设 `PATH`，双击或直接运行会立刻退出，
退出码 `-1073741515`（0xC0000135，意思是"缺少 DLL"）。

在 PowerShell 里跑之前先执行一次：

```powershell
. E:\PointCloudEnv\env.ps1
```

这个脚本会把 `cmake`、`PCL\bin`、`VTK\bin`、`FLANN\bin`、`Qhull\bin`、`OpenNI2` 都加进 `PATH`。
**它只对当前这个 PowerShell 窗口有效**，新开窗口要重新执行。

> 项目里已经配了 `POST_BUILD` 步骤自动把大部分 DLL 拷到 `build\Release\` 旁边，
> 但 OpenNI2 和部分 VTK 插件仍可能有遗漏，所以"先 `env.ps1` 再运行"是最省事的做法。

---

## 三、目录结构

```
E:\dianyunxulian\
├── CMakeLists.txt              ← 唯一的构建脚本（每行都有中文注释）
├── README.md                   ← 你正在看的这个
├── .gitignore
├── data\
│   └── scene_original.pcd      ← 原始点云，只读，算法绝不覆盖
├── output\                     ← 所有算法结果，文件名自带参数
├── build\                      ← CMake 生成的 VS 工程和中间文件（不进 Git）
├── practice\
│   ├── CMakeLists.txt          ← 练习区，单独编译成 exercises.exe
│   └── exercises.cpp           ← 5 个待你手写的练习
├── src\
│   ├── main.cpp                ← 程序入口，只做三件事
│   ├── core\                   ← 基础设施（和点云算法无关）
│   │   ├── types.h                 点类型别名
│   │   ├── algo_result.h           所有算法统一的返回结构
│   │   ├── console.h / .cpp        中文对齐打印、输入读取
│   │   └── cloud_io.h / .cpp       PCD 读写、路径管理、写保护
│   ├── data\                   ← 造数据和看数据
│   │   ├── scene_presets.h / .cpp      5 个场景预设的参数表
│   │   ├── generate_scene.h / .cpp     合成点云生成器
│   │   └── inspect_cloud.h / .cpp      点云体检（范围、间距、重心）
│   ├── preprocessing\          ← 预处理
│   │   ├── voxel_filter.h / .cpp
│   │   ├── statistical_filter.h / .cpp
│   │   └── radius_filter.h / .cpp
│   ├── search\                 ← 空间搜索（不删点）
│   │   └── kdtree_demo.h / .cpp
│   ├── segmentation\           ← 裁剪与分割
│   │   ├── passthrough_demo.h / .cpp
│   │   ├── ransac_plane.h / .cpp
│   │   └── euclidean_cluster.h / .cpp
│   └── menu\                   ← 菜单
│       ├── menu_registry.h / .cpp   菜单框架（编号自动生成）
│       ├── menu_items.cpp           11 个菜单项的具体动作
│       └── algo_runner.h / .cpp     "独立跑一个算法"的统一流程
└── docs\                       ← 全部教学文档
```

---

## 四、三步跑起来

### 1. 打开 PowerShell，进项目目录，激活环境

```powershell
cd E:\dianyunxulian
. E:\PointCloudEnv\env.ps1
```

### 2. 配置 + 编译

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

第一条是**配置**：读 `CMakeLists.txt`，在 `build\` 里生成 Visual Studio 工程。
第二条是**构建**：调用 MSBuild，编译 + 链接出 `build\Release\PointCloudPractice.exe`。

如果 `Visual Studio 18 2026` 报找不到，换成 `"Visual Studio 17 2022"`，其余不变。

### 3. 运行

```powershell
.\build\Release\PointCloudPractice.exe
```

---

## 五、菜单说明

程序启动后是纯文字菜单，输入编号回车即可：

```
  [数据]
     1  生成合成场景（选择预设）
     2  检查原始点云 scene_original.pcd
  [下采样]
     3  体素降采样 VoxelGrid
  [滤波]
     4  统计离群点滤波 SOR
     5  半径离群点滤波 ROR
  [空间搜索（不删点）]
     6  K 近邻搜索 KNN
     7  半径搜索
  [裁剪]
     8  PassThrough 按坐标轴裁掉范围外的点
  [分割]
     9  RANSAC 平面分割
    10  欧氏聚类（按距离分成几块）
  [工具]
    11  查看 output 目录里已有的结果
     0  退出
```

**每个参数都有默认值**，一路回车就能跑出结果。想改就输入数字。

### 一个重要的设计约定

菜单 3~10 里每一个算法，**都在开始前重新读一次 `data/scene_original.pcd`**，
不读上一个算法的输出。所以你调整参数反复跑，永远不会"越跑越少"。

这个约定只在一个地方实现：`src/menu/algo_runner.cpp` 里的 `runOnOriginalCloud()`。
想看代码怎么保证这件事，去读那个文件。

---

## 六、输出文件命名规则

`output\` 里的文件名**自带参数**，跑十次不同参数不会互相覆盖，也不用自己记：

| 实际文件 | 来自 | 参数含义 |
|---|---|---|
| `voxel_leaf2.000.pcd` | 体素降采样 | 体素边长 2.000 mm |
| `sor_k20_std1.00.pcd` | 统计滤波 | 邻居数 20，标准差倍数 1.00 |
| `ror_r3.00_n6.pcd` | 半径滤波 | 半径 3.00 mm，最少 6 个邻居 |
| `kdtree_knn_k8.pcd` | K 近邻 | 查询 8 个邻居（只含邻居点） |
| `kdtree_radius_r5.00.pcd` | 半径搜索 | 半径 5.00 mm 内的所有点 |
| `passthrough_z_-20.0_to_20.0.pcd` | 裁剪 | 保留 z 在 -20 ~ 20 mm |
| `ransac_plane_plane_th1.00_iter1000.pcd` | 平面分割 | 内点（平面上的点） |
| `ransac_plane_rest_th1.00_iter1000.pcd` | 平面分割 | 剩余点 |
| `cluster_largest_tol3.00_min50.pcd` | 欧氏聚类 | 最大的那一簇 |
| `cluster_centroids_tol3.00_min50.pcd` | 欧氏聚类 | 每簇重心的虚拟点云 |

命名由 `makeOutputName(算法名, 参数标签)` 负责，在 `src/core/cloud_io.cpp` 里。

---

## 七、文档阅读顺序

**不要一次全读完。** 按你需要的东西挑：

### 今天就跑起来（约 30 分钟）
1. [docs/00_学习路线.md](docs/00_学习路线.md) —— 先看这个，它告诉你每条路怎么走
2. 本文第四节的三条命令

### 想搞懂"这项目是怎么搭起来的"（建议按序，约 3~4 天）
3. [docs/02_项目目录与多文件C++.md](docs/02_项目目录与多文件C++.md)
4. [docs/03_CMake入门.md](docs/03_CMake入门.md)
5. [docs/04_CMakeLists逐行解释.md](docs/04_CMakeLists逐行解释.md)
6. [docs/05_编译链接与调试.md](docs/05_编译链接与调试.md)
7. [docs/06_Git本地版本管理.md](docs/06_Git本地版本管理.md)

### 想搞懂点云和算法（按需跳读）
8. [docs/01_原始点云数据.md](docs/01_原始点云数据.md)
9. [docs/07_PCL点云数据结构.md](docs/07_PCL点云数据结构.md)
10. [docs/08_VoxelGrid体素降采样.md](docs/08_VoxelGrid体素降采样.md)
11. [docs/09_统计滤波与半径滤波.md](docs/09_统计滤波与半径滤波.md)
12. [docs/10_KDTree邻域搜索.md](docs/10_KDTree邻域搜索.md)
13. [docs/11_PassThrough坐标裁剪.md](docs/11_PassThrough坐标裁剪.md)
14. [docs/12_RANSAC平面分割.md](docs/12_RANSAC平面分割.md)
15. [docs/13_欧氏聚类.md](docs/13_欧氏聚类.md)

### 卡住了
16. [docs/14_项目练习与故障排查.md](docs/14_项目练习与故障排查.md)
17. [docs/15_可视化用法.md](docs/15_可视化用法.md)

---

## 八、已验证 / 未验证（诚实清单）

### 已经实际做过、有输出截图的

- ✅ CMake 4.4.3 用生成器 `Visual Studio 18 2026` 配置成功
- ✅ 同样的代码用生成器 `Visual Studio 17 2022` 配置成功
- ✅ Release 配置编译通过，0 error
- ✅ Debug 配置编译通过（0 error），并且实际跑过菜单 3：输出 17847 点、
      与 Release 版**完全一致**（说明算法不依赖优化级别，也没被未初始化变量影响）
- ✅ `data\` 与 `build\Release\` 的 DLL 拷贝逻辑生效（Debug 用的是 `-gd` 后缀的 VTK 库）
- ✅ 程序能启动、能打印菜单、能读输入
- ✅ 菜单 1 生成 5 个预设场景，`data/scene_original.pcd` 写出成功（30251 点）
- ✅ 菜单 2 点云体检输出正确（平均点间距 1.470 mm）
- ✅ 菜单 3~10 七个算法全部跑通，输出文件全部写出
- ✅ 算法结果**不会**写坏原始数据（实测：故意让算法写回 `scene_original.pcd`，
      被拒绝，且文件 MD5 前后一致）
- ✅ `practice/exercises.exe` 编译成功
- ✅ 不激活 `env.ps1` 直接运行会因缺 DLL 退出（退出码 -1073741515），激活后正常

### 没有验证或做不到的

- ❌ **没在别的电脑上试过**。路径写死成 `E:\PointCloudEnv\PCL` 的默认值，
      换机器要在 CMake 配置时改 `PCL_ROOT_DIR`
- ❌ **没有 PCLVisualizer 的可视化菜单项**。当前版本纯命令行 + 写文件，
      看效果需要另开 PCL 的 `pcl_viewer`（用法见 [docs/15_可视化用法.md](docs/15_可视化用法.md)）
- ❌ **没有真实扫描数据**。全部是合成点云，几何理想、密度均匀，
      与线激光实测数据差距很大（差异见 [docs/01_原始点云数据.md](docs/01_原始点云数据.md)）
- ❌ **没有焊接工艺相关功能**：不分道次、不生成轨迹、不做可达性、不做碰撞。
      这些在真实课题里存在，但本项目只做"几何特征提取"这一层
- ❌ **没有单元测试**。验证方式是"跑一遍看数字合不合理"，不是自动测试

---

## 九、最后一句

这个项目里所有代码都是完整的、能编译能跑的，**没有留给你填空的空函数**。
练习在 `practice\exercises.cpp` 里，是另外单独编译的小程序，写崩了不影响主程序。

从 [docs/00_学习路线.md](docs/00_学习路线.md) 开始。
