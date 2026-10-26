# 07 PCL 点云数据结构

> 对应代码：`src/core/types.h`、`src/core/cloud_io.h/.cpp`、`src/data/inspect_cloud.cpp`
> 这篇文档不教算法，只回答一个问题：**"点云"在内存里和文件里到底长什么样？**

---

## 1. 一句话：点云是什么

**点云 = 一堆三维坐标点。**

就这么简单。没有拓扑（点之间没有"边"），没有面，没有颜色（除非你额外存）。

```cpp
// 概念上等价于：
struct Point { float x, y, z; };
std::vector<Point> cloud;     // 30251 个
```

**PCL 做的事，就是在这个朴素结构上加了一层封装**（元数据、内存管理、算法接口）。

**理解这句话很重要**：任何时候你搞不清 PCL 的某个行为，都可以回到
"它本质上就是个数组"这个模型上来推理。

---

## 2. `PointXYZ`：一个点占多少字节

### 内存布局

```cpp
struct PointXYZ
{
    float x, y, z;
    float data[4];      // 第四个是"对齐填充"，也叫 padding
};
```

**`sizeof(pcl::PointXYZ) == 16`**，不是 12。

**为什么？**

因为 x/y/z 三个 float 是 12 字节，但 PCL 强制**16 字节对齐**（为了用 SIMD 指令
一次处理 4 个 float，也就是 `__m128`）。

**为什么要对齐？** 因为现代 CPU 的 SSE 指令要求操作数在 16 字节边界上。
不对齐的话，要么慢，要么崩。

**怎么验证？**

在项目里加一行打印：

```cpp
std::cout << "sizeof(PointT) = " << sizeof(pcp::PointT) << "\n";
```

**输出 `16`。**

**菜单 2（检查原始点云）里已经打印了这个值**：

```
  每点字节数     : 16
```

### 那第四个数是什么？

**没有意义的数据。** 它是为了让结构体凑够 16 字节而存在的。

**但注意**：它**不会被写入 PCD 文件**（见第 6 节）——
文件里只存 12 字节/点。**这是理解"内存 16 字节 vs 文件 12 字节"的关键。**

### 其他常见点类型

| 类型 | 字段 | 大小 |
|---|---|---|
| `pcl::PointXYZ` | x, y, z | 16 |
| `pcl::PointXYZI` | x, y, z, intensity | 16 |
| `pcl::PointXYZRGB` | x, y, z, rgb（打包成 float） | 32 |
| `pcl::Normal` | normal_x, normal_y, normal_z, curvature | 16 |
| `pcl::PointXYZINormal` | xyz + intensity + normal + curvature | 48 |

**本项目只用 `PointXYZ`**，因为合成场景没有强度也没有颜色。

**但真实线激光传感器通常输出 `PointXYZI`**（强度能反映反射率，对区分
"焊缝金属"和"氧化皮"有用）。这就是为什么 `src/core/types.h` 里
把点类型抽成了 `PointT` 别名：

```cpp
// src/core/types.h:27
using PointT = pcl::PointXYZ;
```

**要换成带强度，只改这一行：**

```cpp
using PointT = pcl::PointXYZI;
```

**全项目 20 个 .cpp 都不用动。** （但要注意：`PointXYZI` 需要
`Ctrl` 之外的代码配合——比如读取的 PCD 必须有 `intensity` 字段。）

---

## 3. `pcl::PointCloud<PointT>`：一个点云的结构

```cpp
template <typename PointT>
class PointCloud
{
public:
    std::vector<PointT> points;      // ← 真正的数据，点的数组
    uint32_t width  = 0;             // ← 宽度
    uint32_t height = 1;             // ← 高度
    bool     is_dense = true;        // ← 是否所有点都是有限值
    Eigen::Vector4f sensor_origin_;  // ← 传感器位置（获取视角）
    Eigen::Quaternionf sensor_orientation_;
};
```

**五个成员，但只要真正搞懂前三个。**

### 3.1 `points`：点数组

```cpp
cloud->points[i]        // 第 i 个点
cloud->size()           // 点数（= points.size()）
cloud->at(i)            // 同上，带越界检查（慢）
cloud->empty()          // 是否为空
cloud->push_back(p)     // 加点
cloud->reserve(n)       // 预分配
```

**`cloud->size()` 和 `cloud->points.size()` 完全一样**（前者是后者的缩写）。

**为什么有两种写法？** 历史原因。PCL 早期提供 `size()` 是为了兼容，
后来发现没意义，但为了兼容性保留了下来。

### 3.2 `width` / `height`：点的"形状"

这是**PCL 里最容易出错的地方**。

**规则**：

| 情况 | width | height | 含义 |
|---|---|---|---|
| **无序点云**（本项目） | = 点数 | **1** | 就是一维数组 |
| **有序点云**（深度相机） | 图像宽度 | 图像高度 | 像图像一样按格子排列 |

**为什么会有"有序"点云？**

因为深度相机（Kinect、RealSense）输出的是**图像**：每个像素一个深度值。
所以它的点云天然有"行、列"结构。

```
有序点云（4×3 的深度图）：
  width = 4, height = 3
  ┌────┬────┬────┬────┐
  │ p0 │ p1 │ p2 │ p3 │
  ├────┼────┼────┼────┤
  │ p4 │ p5 │ p6 │ p7 │
  ├────┼────┼────┼────┤
  │ p8 │ p9 │p10 │p11 │
  └────┴────┴────┴────┘

  点数 = width × height = 12
```

**激光扫描仪（线激光）输出的是无序点云**：它每次扫一条线，
点的顺序是"沿线的顺序"，没有二维网格结构。**本项目也是这样。**

### 3.3 关键约束：`width × height` 必须等于点数

**如果不等，PCL 会做出奇怪的事。**

最典型的坑是 **`push_back` 不会更新 `width`**：

```cpp
CloudPtr cloud(new CloudT);
cloud->push_back(p1);
cloud->push_back(p2);
// 现在 cloud->points.size() == 2
// 但 cloud->width == 0 ！！！
```

**后果**：`savePCDFileBinary` 写出 `WIDTH 0`，读回来点数 = 0。

**本项目的代码里都显式设置了**（例 `src/search/kdtree_demo.cpp:102-104`）：

```cpp
neighbors->width  = static_cast<uint32_t>(neighbors->size());
neighbors->height = 1;
neighbors->is_dense = true;
```

**而 PCL 的滤波器（`VoxelGrid::filter()` 等）会自动设置 `width`** ——
这就是为什么本项目大部分算法不需要手工设，只有"手工攒点云"的地方要设。

**记住这条**：

> **只要你自己 `push_back` 凑点云，最后一定要手工设 `width`/`height`。**

### 3.4 `is_dense`：有没有无效点

`true` = 所有点的 x/y/z 都是有限值（不是 NaN/Inf）
`false` = 可能有无效点

**本项目遇到的坑**：读回 PCD 后 `is_dense` 恒为 `true`。

**菜单 2 的输出**：

```
  是否稠密       : 是
```

**但原始数据其实有 200 个飞点……飞点不是无效点！** 飞点的坐标是完全正常的数字，
只是位置离群。所以 `is_dense = true` **是正确的**。

**真正会让 `is_dense = false` 的是 NaN** —— 比如深度相机的"测不出距离"的像素。

**为什么读回 PCD 后是 true？** 因为 **PCD 格式不存 `is_dense` 字段**。
读的时候默认 `true`。如果原始数据有 NaN，这个信息就丢了 —— **这是 PCD 格式的一个缺陷**。

### 3.5 `sensor_origin_` / `sensor_orientation_`

记录"这个点云是从哪个位置、什么姿态扫出来的"。

**本项目的 PCD 头部有**：

```
VIEWPOINT 0 0 0 1 0 0 0
```

意思是：传感器在原点 `(0,0,0)`，朝向 `(0,0,0,1)`（四元数 = 无旋转）。

**这对应 `sensor_origin_ = (0,0,0,1)` 和 `sensor_orientation_ = (1,0,0,0)`。**

**本项目不用这两个字段**（合成数据没有真实传感器），但**真实扫描数据必须用** ——
多视角拼接时，每个点云都要带自己的传感器位姿。

---

## 4. 智能指针：`Ptr` / `ConstPtr`

**点云很大**（3 万点 = 480 KB），**传参数时不能拷贝**。

PCL 的惯例是用 `shared_ptr`：

```cpp
// src/core/types.h:38-39
using CloudPtr      = CloudT::Ptr;        // = std::shared_ptr<PointCloud<PointT>>
using CloudConstPtr = CloudT::ConstPtr;   // = std::shared_ptr<const PointCloud<PointT>>
```

### 什么时候用哪个

| | 用途 | 能改内容吗 |
|---|---|---|
| `CloudPtr` | 输出、中间结果 | **能** |
| `CloudConstPtr` | **算法输入** | **不能**（编译器强制） |

**本项目的算法签名**：

```cpp
AlgoResult runVoxelGrid(const CloudPtr &input, const VoxelOptions &opt);
              ↑ 这个其实是 CloudPtr（可写），但用了 const&
```

**为什么不用 `CloudConstPtr`？**

因为 PCL 的 `setInputCloud()` 签名是：

```cpp
void setInputCloud(const PointCloudConstPtr &cloud);
```

它要的是 `ConstPtr`。如果传 `CloudPtr`，会自动转成 `ConstPtr`（shared_ptr 支持
从 `shared_ptr<T>` 到 `shared_ptr<const T>` 的隐式转换）。

**所以两者都行，本项目用 `CloudPtr` 是为了让"输入也能被改"的可能性留着** ——
但实际上没有任何算法改输入（改了会违反"原始数据只读"的原则）。

### `shared_ptr` 的基本行为

```cpp
CloudPtr a(new CloudT);   // 引用计数 = 1
CloudPtr b = a;           // 引用计数 = 2（共享同一份数据）
// a 和 b 指向同一个点云，改 b 就是改 a
```

**什么时候释放？** 最后一个指向它的 `shared_ptr` 被销毁时。

**这意味着**：

```cpp
CloudPtr makeCloud()
{
    CloudPtr c(new CloudT);
    c->push_back(PointT(1,2,3));
    return c;              // 返回时引用计数 +1，然后局部变量销毁 -1 → 还是 1
}                          // 安全，数据不会被提前释放
```

**这是 C++ 里最推荐的"管内存"方式**（不会再出现 `new` 了忘 `delete`）。

---

## 5. 点索引：三种写法

### 写法 1：下标（最常用）

```cpp
const PointT &p = cloud->points[i];
std::cout << p.x << p.y << p.z;
```

**注意 `const PointT &` 是引用**（不拷贝点）。

**如果写成 `PointT p = cloud->points[i];`** —— 会拷贝一个点（16 字节）。
3 万次循环里拷贝 3 万个 16 字节 = 480 KB 的拷贝，**慢但不算致命**。
**但养成用引用的习惯。**

### 写法 2：范围 for（最安全）

```cpp
for (const auto &p : cloud->points)
{
    std::cout << p.x << "\n";
}
```

**优点**：不用管下标、不会越界。
**缺点**：**拿不到下标 i**（如果你需要"这是第几个点"，就得用写法 1）。

### 写法 3：迭代器（老式）

```cpp
for (auto it = cloud->points.begin(); it != cloud->points.end(); ++it)
{
    std::cout << it->x << "\n";
}
```

**现代 C++ 里基本不用了**，除非你需要 `std::distance` 之类。

### 本项目的例子

`src/data/inspect_cloud.cpp` 的 `printFirstPoints()`：

```cpp
for (std::size_t i = 0; i < n && i < cloud->size(); ++i)
{
    const PointT &p = cloud->points[i];
    ...
}
```

**注意 `i < n && i < cloud->size()`** —— 两个条件，防止 n 比点数还大。

---

## 6. PCD 文件格式：逐行拆解

**PCD（Point Cloud Data）= PCL 自己的点云文件格式。**

**它分两部分：文本头 + 数据区。**

### 本项目的真实文件头

`data/scene_original.pcd` 的前 11 行：

```
# .PCD v0.7 - Point Cloud Data file format
VERSION 0.7
FIELDS x y z
SIZE 4 4 4
TYPE F F F
COUNT 1 1 1
WIDTH 30251
HEIGHT 1
VIEWPOINT 0 0 0 1 0 0 0
POINTS 30251
DATA binary
```

**逐行解释**：

| 行 | 含义 |
|---|---|
| `# ...` | 注释行，`#` 开头的都是注释 |
| `VERSION 0.7` | PCD 格式版本（现在是 0.7） |
| `FIELDS x y z` | **有哪些字段**。这里是 x、y、z 三个 |
| `SIZE 4 4 4` | **每个字段几字节**。float 是 4 字节 |
| `TYPE F F F` | **字段的数据类型**。`F`=float，`U`=unsigned int，`I`=int |
| `COUNT 1 1 1` | **每个字段有几个元素**。1 表示标量（不是数组） |
| `WIDTH 30251` | 宽度（= 点数，因为无序） |
| `HEIGHT 1` | 高度（无序点云固定为 1） |
| `VIEWPOINT 0 0 0 1 0 0 0` | 传感器位置 + 朝向（前 3 个是位置，后 4 个是四元数） |
| `POINTS 30251` | **总点数**（应该 = WIDTH × HEIGHT） |
| `DATA binary` | **数据区格式**：`ascii` / `binary` / `binary_compressed` |

### 数据区：`binary` 意味着什么

**头部之后立刻是原始字节，没有换行、没有分隔符。**

每 12 字节 = 一个点（`4+4+4`：x、y、z 各 4 字节 float）。

**用文件大小验证**：

```
总文件大小          = 363184 字节
头部（文本，11 行）  = 172 字节（估算）
数据区              = 363184 - 172 = 363012 字节
363012 / 12         = 30251 个点   ✓
```

**精确对上了。** 这个验算是理解 PCD 格式最好的练习。

**关键推论**：

> **文件里每个点是 12 字节，内存里是 16 字节。**
>
> 差的 4 字节是 padding（第 2 节讲的 `data[3]`），**不写入文件**。

**所以你算文件大小时要用 12，算内存占用时要用 16。**

### 三种 `DATA` 格式对比

| 格式 | 大小 | 可读性 | 读写速度 |
|---|---|---|---|
| `ascii` | 最大（约 40 字节/点） | **人眼可读**（能 `Get-Content` 看到坐标） | 慢 |
| `binary` | 12 字节/点 | 二进制乱码 | **快** |
| `binary_compressed` | 最小（LZF 压缩，约 8 字节/点） | 完全不可读 | 解压要额外时间 |

**本项目用 `binary`**（`src/core/cloud_io.cpp:192`）：

```cpp
const int ret = pcl::io::savePCDFileBinary(absPath, *cloud);
```

**为什么不用 ascii？** 3 万点的 ascii 文件约 1.2 MB（vs 363 KB），
读写慢 3~5 倍。

**为什么不用 `binary_compressed`？** 它需要 PCL 编译时启用 LZF 支持
（本机的 PCL 1.15 预编译包里**没有**）。用了会报错：

```
[pcl::PCDWriter::writeBinaryCompressed] Compressed PCD not supported!
```

**这个坑很常见** —— 网上下载的 PCD 有时候是 `binary_compressed`，
本机读到一半失败。**这就是为什么 `loadCloud()` 要把返回码写进错误信息。**

### 想看 `ascii` 格式怎么办

**最简单的办法**：用 CloudCompare 打开 PCD，另存为 ascii。

**或者**，直接看 `output/` 里的文件头（反正也是 binary）——
**想看到坐标数字，就得自己转换**：

```powershell
# 用 Python（如果你装了）：
python -c "import open3d as o3d; p=o3d.io.read_point_cloud('data/scene_original.pcd'); o3d.io.write_point_cloud('data/scene_ascii.pcd', p, write_ascii=True)"
```

**（本项目不依赖 Python，这条只是备选。）**

---

## 7. 代码里的读写：`src/core/cloud_io.cpp` 逐段

### 7.1 `projectDir()`：项目根目录从哪来

```cpp
std::string projectDir()
{
    // 静态变量：只在第一次调用时计算，之后直接返回
    static std::string cached = [...]() -> std::string {
        ...
    }();
    return cached;
}
```

**为什么要"找项目根目录"？**

因为程序运行时的工作目录**不确定**：

- 在 `build\Release\` 里双击 exe → 工作目录是 `build\Release\`
- 在 PowerShell 里 `cd E:\dianyunxulian` 后运行 `.\build\Release\...` → 工作目录是项目根
- 在 Visual Studio 里按 F5 → 工作目录是**项目目录**（VS 的默认设置）

**如果代码里写 `"data/scene_original.pcd"`（相对路径）**，
在 VS 里按 F5 就找不到文件。

**本项目的方案**：CMake 在编译时把项目根目录**写进 exe**：

```cmake
# CMakeLists.txt:108
add_compile_definitions("PCP_PROJECT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
```

然后在 C++ 里读这个宏：

```cpp
#ifdef PCP_PROJECT_DIR
    fs::path dir(PCP_PROJECT_DIR);
    if (fs::exists(dir / "CMakeLists.txt")) return dir.generic_string();
#endif
```

**`fs` 是 `std::filesystem` 的别名**（C++17 起标准库自带）。
`dir / "CMakeLists.txt"` 是路径拼接，会自动处理斜杠。

**兜底逻辑**：如果宏没定义（比如有人手动编译），就从当前工作目录
**向上找最多 8 层**，看哪个目录里有 `CMakeLists.txt`。

**这个"找根目录"的模式在工业项目里很常见** —— 因为可执行文件的位置
和项目文件的位置通常不同。

### 7.2 `loadCloud()`：读的一步步

```cpp
CloudPtr loadCloud(const std::string &absPath, std::string &err)
{
    err.clear();

    std::error_code ec;
    if (!fs::exists(absPath, ec))
    {
        err = "文件不存在: " + absPath;
        return nullptr;
    }

    auto cloud = std::make_shared<CloudT>();
    const int ret = pcl::io::loadPCDFile<PointT>(absPath, *cloud);
    if (ret < 0)
    {
        err = "PCL 读取 PCD 失败（返回码 " + std::to_string(ret) + "）: " + absPath;
        return nullptr;
    }

    if (cloud->empty())
    {
        err = "文件读到了但点数为 0: " + absPath;
        return nullptr;
    }

    return cloud;
}
```

**四个设计点**：

**① 先查文件存在，再调 PCL**

PCL 的 `loadPCDFile` 在文件不存在时返回 -1，但**不告诉你"文件不存在"**
（可能是格式错误、可能是权限）。**提前查一次能给更准确的错误信息。**

**② `std::error_code ec` 的用法**

`fs::exists(path)` 有一个会抛异常的重载。**传 `ec` 进去，就变成"返回 false + 填 ec"**，
不抛异常。

**`std::filesystem` 的这个设计有点怪**（大多数函数有两个重载：抛异常版和不抛版），
但好处是——**你不会因为一个路径检查而让程序崩掉**。

**③ `ret < 0` 检查**

PCL 的所有 IO 函数都返回 `int`：

- `0` 或正数 = 成功
- **负数 = 失败**

**具体是 -1 还是 -2 还是别的，取决于失败原因。** 本项目把它打进错误信息里，
方便你搜索（比如 "返回码 -1 PCD"）。

**④ 检查空点云**

**为什么要单独查 `cloud->empty()`？**

因为 `loadPCDFile` 可能在**文件头合法、但数据区损坏**时返回成功，
此时 `cloud->size() == 0`。**如果不查，后面的算法会拿到空点云。**
（虽然各个算法里也检查了，但多一层保险。）

**⑤ `std::make_shared<CloudT>()` vs `new CloudT`**

**`make_shared` 更好**：它把"控制块"和"对象"分配在同一块内存里，
比 `new` + `shared_ptr` 少一次内存分配，也更快。

**所以**：`loadCloud` 里用 `make_shared`（性能敏感，可能读几十万点），
而算法里写 `CloudPtr output(new CloudT)` 只是习惯问题（两者都能用）。

### 7.3 `saveCloud()`：原始数据保护

```cpp
const std::string fileName = fs::path(absPath).filename().generic_string();
if (!allowOriginal && fileName == "scene_original.pcd")
{
    err = "拒绝写入 scene_original.pcd：原始数据只读，算法结果请写到 output/ 目录";
    return false;
}
```

**这段代码是整个项目最重要的安全设计。**

**为什么判断"文件名"而不是"完整路径"？**

因为如果只判断路径，有人可能传 `"E:/dianyunxulian/./data/scene_original.pcd"`
（带 `./`）绕过检查。**判断文件名更鲁棒。**

**`fs::path(absPath).filename()`** 取路径的最后一段（文件名部分）；
**`.generic_string()`** 转成用 `/` 的字符串（避免 `\` 和 `/` 混用）。

**`allowOriginal` 的默认值是 `false`**（在头文件里）：

```cpp
// src/core/cloud_io.h:52-53
bool saveCloud(const std::string &absPath, CloudConstPtr cloud, std::string &err,
               bool allowOriginal = false);
```

**只有场景生成器会传 `true`**（它必须能重写原始场景）：

```cpp
// src/core/cloud_io.cpp 里 generateAndSave 的调用处
saveCloud(originalScenePath(), newScene, err, /*allowOriginal=*/true);
```

**这个设计叫"默认安全"**（secure by default）：
**忘记传参数 = 安全的那一侧。** 这比"默认允许写，需要时再保护"安全得多。

### 7.4 输出文件名：为什么带参数

```cpp
std::string makeOutputName(const std::string &algorithm, const std::string &paramTag)
{
    std::string name = algorithm;
    if (!paramTag.empty())
    {
        name += "_" + paramTag;
    }
    return name + ".pcd";
}
```

**`makeOutputName("voxel", "leaf2.000")` → `"voxel_leaf2.000.pcd"`**

**这个命名规则让 `output/` 目录成为一个"实验记录"**：

```
voxel_leaf2.000.pcd
kdtree_knn_k8.pcd
sor_k20_std1.00.pcd
ransac_plane_plane_th1.00_iter1000.pcd
```

**看文件名就知道用了什么参数。** 这是做对比实验的基础。

---

## 8. 常见的数据读取错误

### 错误 1：`error C1083: 无法打开包括文件: "pcl/point_types.h"`

**不是读取错误，是编译错误** —— 头文件搜索路径没配好。

检查 `CMakeLists.txt` 的 `target_include_directories` 里有 `${PCL_INCLUDE_DIRS}`。

### 错误 2：`loadPCDFile` 返回 -1，文件明明存在

**四个可能**：

1. 文件是 `binary_compressed` 但 PCL 没编译 LZF 支持
2. 文件的 `POINTS` 和实际的点数不一致（文件损坏）
3. 文件被其他程序占用
4. **路径里有中文**（PCL 的 IO 在某些版本里对非 ASCII 路径有问题）

**本项目的数据路径是纯 ASCII**（`E:\dianyunxulian\data\`），不会遇到第 4 种。
但如果你把项目放在 `E:\我的项目\`，可能就要小心。

### 错误 3：读回来 `size() == 0`

**最常见的原因**：文件头里 `WIDTH` 或 `POINTS` 写的是 0。

**你自己的代码 `push_back` 完忘了设 `width` → 存的时候写出 `WIDTH 0` → 读回来 0 个点。**

**怎么查**：

```powershell
Get-Content .\output\xxx.pcd -TotalCount 11
```

看 `WIDTH` 和 `POINTS` 是不是 0。

### 错误 4：点的坐标差了 1000 倍

**单位问题。** 见下一节。

### 错误 5：`is_dense` 是 true 但点云里明显有 NaN

**PCD 格式不存 `is_dense`**，读回来默认 true。见第 3.4 节。

**要检测 NaN**：

```cpp
for (const auto &p : cloud->points)
{
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
    {
        // 这是一个无效点
    }
}
```

---

## 9. 单位：本项目的 mm 约定

**PCL 没有单位。** 一个点的坐标是 `(0, 50, 0)`，
PCL 不知道这是 50 毫米、50 米还是 50 英寸。

**本项目的约定**：**全部用毫米（mm）**。

**为什么？**

- 工业管道的直径是 100~1000mm 量级 → 用 mm 数字好看（100 而不是 0.1）
- 焊缝特征（张口宽度、坡口深度）都是 mm 级
- 激光传感器的原始输出通常是 mm

**这个约定体现在代码里**：

```cpp
result.metrics["体素边长(mm)"] = opt.leaf;      // 名字里带 (mm)
result.metrics["搜索半径(mm)"] = opt.radius;
```

**所有参数名都带 `(mm)`**，就是为了让人不会搞混。

**如果你拿到一份用"米"的 PCD**（很多公开数据集是米）：

```cpp
// 全部坐标 × 1000
for (auto &p : cloud->points)
{
    p.x *= 1000.0f;
    p.y *= 1000.0f;
    p.z *= 1000.0f;
}
```

**这个"单位陷阱"在跨数据集对比时非常常见** ——
一个 50 米长的管道，另一个 50000 毫米的管道，看起来完全不同。

---

## 10. 三个练习

### 练习 1（10 分钟）：验证"文件 12 字节、内存 16 字节"

**做什么**：

1. 用 `Get-Item` 看 `data/scene_original.pcd` 的大小 → 363184 字节
2. 看头部 11 行，数一下字符数（用 `Get-Content` 复制到记事本里算，或者用 PowerShell）
3. 用 `(文件大小 - 头部大小) / 30251` 算"每个点占几字节"
4. 和 `sizeof(PointT) == 16` 对比

**要回答**：差的 4 字节是什么？为什么它不写入文件？

<details>
<summary>答案</summary>

`(363184 - 172) / 30251 = 12`。

差的 4 字节是 `PointXYZ::data[3]`（对齐填充）。
**它不携带任何信息**（PCL 在读取时会把它设成 1.0），所以不写入文件能省 25% 空间。

</details>

### 练习 2（15 分钟）：手工造一个 3 点云

**任务**：在 `practice/exercises.cpp` 里写一个函数，手工造 3 个点的点云并保存。

```cpp
pcp::CloudPtr c(new pcp::CloudT);
c->push_back(pcp::PointT(0.0f, 0.0f, 0.0f));
c->push_back(pcp::PointT(10.0f, 0.0f, 0.0f));
c->push_back(pcp::PointT(0.0f, 10.0f, 0.0f));
// TODO: 设置 width/height/is_dense
// TODO: 保存到 output/exercise_three_points.pcd
```

**先故意不设 `width`，保存，然后用 `Get-Content` 看文件头。**

**要回答**：

1. 不设 `width` 时，`WIDTH` 和 `POINTS` 是多少？
2. 读回来点数是多少？
3. 设了 `width` 之后呢？

<details>
<summary>答案</summary>

1. `WIDTH 0`、`POINTS 3`（PCL 会写 POINTS，但 WIDTH 是 0）
2. **读回来 0 个点**（PCL 按 WIDTH 分配空间）
3. 正常 3 个点

**这个练习值得亲手做一遍**，因为它是"最隐蔽的坑"——
你的代码看起来完全正确，但存的文件坏了。

</details>

### 练习 3（10 分钟）：用坐标范围反推场景

**任务**：不看代码，只跑菜单 2，用输出的坐标范围反推场景：

```
  X 范围 : [-179.476, 169.643]
  Y 范围 : [-178.475, 200.183]
  Z 范围 : [-179.192, 178.132]
```

**要回答**：

1. 为什么 X 范围是 -179 到 170，但管道长度只有 80？
2. Y 范围上界 200.183 说明什么？
3. 如果去掉飞点，X 范围应该变成多少？

<details>
<summary>答案</summary>

1. **飞点撑大的**。飞点均匀分布在 ±180 的立方体里
2. **平板在 y = 200**（`plateCenterY`），加上噪声 0.05 和舍入
3. 约 **[-40, 40]**（管道长度 80，轴在 X 上）

**这个练习让你建立"坐标范围 = 几何尺寸 + 噪声 + 离群点"的直觉。**

</details>

---

## 11. 与管道焊缝点云处理的联系

### 联系 1：真实焊缝点云必然用 `PointXYZI`

**线激光传感器（Keyence LJ-X、LMI Gocator、国产的镭烁/中科视语）输出的是**
**"位置 + 反射强度"**。

**强度有什么用？**

- 焊缝表面（熔融金属冷却后）和母材（轧制面）的反射率不同
- 可以辅助"区分焊缝和母材"
- 可以识别"氧化皮""飞溅"

**本项目没有强度**，因为合成数据造不出真实反射率。

**要扩展到 `PointXYZI`**，见 `src/core/types.h` 的注释 ——
但注意：**合成器要同时造出 intensity 字段**，否则读 PCD 会失败。

### 联系 2：`width`/`height` 在"沿焊缝切片"时的意义

**做坡口特征提取时，通常把点云切成一片片"横截面"**：

```
管道点云（3D）
    ↓ 按轴向位置切片
切片 k（一个薄环）
    ↓ 压到 2D（x-r 平面）
2D 截面点云
```

**如果你想让"切片"变成一个有序点云**（比如按角度排序），
就可以用 `width = 环向点数, height = 1` 表示有序结构，
这样"相邻点"就是"环向上的相邻点"，可以直接算间距。

**本项目没做切片**，但 `docs/15` 里提到过这个方向。

### 联系 3：原始数据的不可变性 = 版本管理的需要

**本项目用 `saveCloud` 的 `allowOriginal` 参数确保"代码永远不覆盖原始数据"。**

**但在真实项目里，还有另一类"覆盖"**：`git checkout` 误操作、
或者手动改文件。

**所以文档 06 里建议把 `data/scene_original.pcd` 纳入 git** ——
它是全项目的参照物，被改坏了就没法对比了。

---

## 12. 数据结构层面的局限

| 局限 | 说明 | 本项目是否遇到 |
|---|---|---|
| **PCD 不存 `is_dense`** | 读回来默认 true，NaN 信息丢失 | 不遇到（无 NaN） |
| **PCD 不存单位** | 换数据集必须手工确定 | 不遇到（固定 mm） |
| **二进制格式不可读** | 必须用工具才能看坐标 | 遇到（靠头部能看点数） |
| **无拓扑结构** | 点之间没有"邻居"关系，要用 KD-Tree 现算 | 遇到（这就是 KD-Tree 的用武之地） |
| **`PointXYZ` 的 padding** | 内存 16 字节 vs 文件 12 字节，容易算错 | 遇到（文档里专门讲了） |
| **大点云读入内存** | 1000 万点 ≈ 160 MB | 不遇到（3 万点） |
| **`shared_ptr` 的循环引用** | 两个点云互相指向会泄漏 | 不遇到（单向数据流） |

---

## 下一步

- 想搞懂"为什么 3 万点要 160 字节/点" → 已经讲了，回到 [01_原始点云数据.md](01_原始点云数据.md) 看数据是怎么生成的
- 想看点的"邻居关系"怎么算 → [10_KDTree邻域搜索.md](10_KDTree邻域搜索.md)
- 想动手改点类型 → [14_项目练习与故障排查.md](14_项目练习与故障排查.md) 的综合练习
- 卡住了 → [14_项目练习与故障排查.md](14_项目练习与故障排查.md)
