# 04 CMakeLists 逐行解释

> 这份文档只讲一份文件：`E:\dianyunxulian\CMakeLists.txt`（202 行）。
>
> 每一条命令都按**五个问题**讲：
> ① 语法是什么意思 ② 参数是什么意思 ③ 本项目为什么需要 ④ 删掉/改掉会怎样 ⑤ 怎么验证。
>
> 全程对着真实文件。如果文档和文件不一致，**以文件为准** —— 文件才是真的。

---

## 零、先补三个语法常识（不然后面看不懂）

### 1. CMake 的"命令"和"括号"

```cmake
命令名(参数1 参数2 参数3)
```

- 空格分隔参数，**不是逗号**
- **缩进不影响**任何行为（只给人看）
- 注释用 `#`
- 命令名**大小写不敏感**（`set` 和 `SET` 一样），但**变量名大小写敏感**

### 2. CMake 里没有"类型"，一切皆字符串

```cmake
set(PCL_ROOT_DIR "E:/PointCloudEnv/PCL")
```

`PCL_ROOT_DIR` 的值就是一个字符串 `E:/PointCloudEnv/PCL`。

**列表**也只是"用分号分隔的长字符串"：

```cmake
set(_pcl_stems "pcl_common;pcl_io;pcl_filters")
```

引用列表时用 `${_pcl_stems}`（展开成一个参数，分号还在）或者
`"${_pcl_stems}"`（展开成一个整体）。`foreach(... IN LISTS _pcl_stems)` 会自动按分号拆开。

### 3. 生成器表达式 `$<...>`：**构建时才求值**的东西

```cmake
$<CONFIG:Debug>
$<IF:$<CONFIG:Debug>,这个,那个>
$<TARGET_FILE_DIR:PointCloudPractice>
$<TARGET_RUNTIME_DLLS:PointCloudPractice>
```

它们和普通变量的区别：

| | 普通变量 `${...}` | 生成器表达式 `$<...>` |
|---|---|---|
| 什么时候定值 | **configure 阶段**（一次性） | **build 阶段**（按 Debug/Release 分别定） |
| 能不能 `if()` 判断 | 能 | 不能 |
| 为什么需要 | 大部分情况 | **因为 VS 一套工程同时包含 Debug 和 Release** |

本项目的 DLL 拷贝就靠它：Debug 配置拷 `pcl_iod.dll`，Release 配置拷 `pcl_io.dll`。
如果用普通变量，configure 时就只能二选一，另一套配置就错了。

---

## 第 1 段：CMake 最低版本（第 9~11 行）

```cmake
# --- 1. CMake 最低版本 -------------------------------------------------------
# 3.20 才支持 target_sources() 与 $<TARGET_RUNTIME_DLLS> 的完整行为。
cmake_minimum_required(VERSION 3.20)
```

**① 语法**：声明"这份 CMakeLists 至少需要 CMake 3.20"。

**② 参数**：`VERSION 3.20` 是版本号。

**③ 为什么需要**：它**必须**是文件里第一条真正的命令（前面可以有注释）。
没有它，CMake 会用一套很老的兼容策略，`project()` 等命令的行为都会变。

本项目用到 `$<TARGET_RUNTIME_DLLS>`，那个特性是 CMake 3.21 才完整，
这里写 3.20 稍宽松了些（本机 CMake 是 4.4.3，远高于要求，所以不影响）。

**④ 删掉会怎样**：

```
CMake Error: No project() command is present.
CMake Error: The source directory ... does not appear to contain CMakeLists.txt.
```
（具体报错随版本变化，核心是"没有声明最小版本"）

**⑤ 验证**：

```powershell
cmake --version
```

预期 `cmake version 4.4.3`。如果想故意看报错，把 `VERSION 3.20` 改成 `VERSION 99.0`，
重新 configure 会看到：

```
CMake Error at CMakeLists.txt:11 (cmake_minimum_required):
  CMake 99.0 or higher is required.  You are running version 4.4.3
```

---

## 第 2 段：项目声明（第 13~15 行）

```cmake
# --- 2. 项目声明 -------------------------------------------------------------
# LANGUAGES CXX 表示只要 C++ 编译器，不找 C/Fortran，配置更快。
project(PointCloudPractice LANGUAGES CXX)
```

**① 语法**：`project(<名字> [LANGUAGES <语言列表>])`

**② 参数**：
- `PointCloudPractice` 是项目名。它不是 exe 名（exe 名由 `add_executable` 的第一个参数定）。
- `LANGUAGES CXX` 只启用 C++。默认是 `C CXX`，如果加 `Fortran` 还会去找 Fortran 编译器。

**③ 为什么需要**：`project()` 是所有 CMake 项目**必须**有的第二行。它做三件事：
1. 初始化编译器检查（"这台机器上的 C++ 编译器能用吗、支持 C++17 吗"）
2. 定义 `PROJECT_NAME`、`PROJECT_SOURCE_DIR` 等变量
3. 决定 `CMAKE_CXX_COMPILER` 用哪个

**④ 改掉会怎样**：
- 只改名字 `project(MyTest ...)`：`build\` 里的 `.sln` 会变成 `MyTest.sln`（下次 configure 时）。
  **`.vcxproj` 还是 `PointCloudPractice.vcxproj`** —— 因为那是 `add_executable` 决定的。
- 删掉 `LANGUAGES CXX`：CMake 会启用 C 和 C++，稍微慢一点，不影响结果。

**⑤ 验证**（这是本文档练习 1）：

```powershell
# 改名为 MyTest，然后
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
Get-ChildItem build -Filter *.sln | Select-Object Name
```

预期看到 `MyTest.sln`。

**别忘了改回来。**

---

## 第 3 段：C++ 标准（第 17~20 行）

```cmake
# --- 3. C++ 语言标准 ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)          # 用 C++17：std::filesystem、std::optional 等
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 编译器不支持 17 就直接报错，不要悄悄降级
set(CMAKE_CXX_EXTENSIONS OFF)       # 只用标准语法，不用 MSVC 的方言扩展
```

**① 语法**：`set(<变量名> <值>)`

**② 三个变量的含义**：

| 变量 | 作用 | 本项目取值 |
|---|---|---|
| `CMAKE_CXX_STANDARD` | 用哪个标准版本 | `17` |
| `CMAKE_CXX_STANDARD_REQUIRED` | 编译器不支持时：`ON`=报错，`OFF`=悄悄降级 | `ON` |
| `CMAKE_CXX_EXTENSIONS` | 允许编译器方言（如 `-std=gnu++17`）吗 | `OFF`（纯标准 `-std=c++17`） |

**③ 为什么需要 C++17**：本项目用到：

- `std::filesystem`（`src\core\cloud_io.cpp` 里的 `fs::exists`、`fs::directory_iterator`）
- `std::optional`（部分参数处理）
- 结构化绑定（`for (const auto& [k, v] : metrics)`）

C++11 编不过，直接报 `std::filesystem 不是 std 的成员`。

**④ 改掉会怎样**：
- 改成 `14`：`src\core\cloud_io.cpp` 报 `error C2653: "filesystem": 不是类或命名空间名称`
- `_REQUIRED` 改成 `OFF` 且编译器又只支持 C++11：CMake 会悄悄用 C++11，
  然后你在几百行外看到莫名其妙的错误，极难排查 —— **所以必须 ON**。

**⑤ 验证**（本文档练习 2）：改成 `14` 重新 configure + build，抄下报错，再改回来。

**注意一个坑**：这三个 `set()` 必须在 `project()` **之后**。写在前面无效。

---

## 第 4 段：禁用 vcpkg 集成（第 22~29 行）

```cmake
set(CMAKE_VS_GLOBALS "VCPkgLocalAppDataDisabled=true;VcpkgEnabled=false")
```

**① 语法**：`CMAKE_VS_GLOBALS` 是一个特殊变量，它的值会被写进生成的 `.vcxproj` 的
`<PropertyGroup Label="Globals">` 节，每项是 `名字=值`，用分号分隔。

**② 参数含义**：
- `VCPkgLocalAppDataDisabled=true` — 让 vcpkg 的用户级集成失效
- `VcpkgEnabled=false` — 显式关掉 vcpkg 集成

**③ 为什么本项目**必须有**这一行**：

这台机器上存在这个文件：

```powershell
Test-Path C:\Users\m1445\AppData\Local\vcpkg\vcpkg.user.targets
# 预期 True
```

只要它存在，MSBuild 在构建任何 `.vcxproj` 时都会自动 `Import` 它。
那个文件会把 `E:\TOOL\vcpkg\installed\x64-windows\include` 加进 include 路径。

而那个目录里有**另一套 PCL**（vcpkg 装的），和 `E:\PointCloudEnv\PCL` 不是同一个版本、
不是同一套编译选项。两套混着用时，PCL 的 `memory.h` 会检测到 malloc 对齐方式不一致：

```
E:\TOOL\vcpkg\installed\x64-windows\include\pcl\memory.h(69,1):
error C1189: #error: "Potential runtime error due to aligned malloc mismatch!
PCL was likely compiled without AVX support but you enabled AVX for your code
(to silence this message at your own risk, define PCL_SILENCE_MALLOC_WARNING=1)"
```

这个错误信息会让人以为是 PCL 装坏了，其实是"两套 PCL 打架"。**本项目实测过这个报错。**

**④ 删掉会怎样**：上面那个 `C1189`，编译阶段直接失败，一个 `.obj` 都出不来。

**⑤ 验证**：

```powershell
# 1. 看 .vcxproj 里有没有写进去
Select-String -Path E:\dianyunxulian\build\PointCloudPractice.vcxproj -Pattern "VcpkgEnabled"
# 预期能看到 <VcpkgEnabled>false</VcpkgEnabled>

# 2. 看 vcpkg 是不是真的存在
Test-Path E:\TOOL\vcpkg\installed\x64-windows\include\pcl\memory.h
# 预期 True —— 这就是那套"不该被引入"的 PCL
```

> **补充**：这个 patch 是**本机特定的**。换一台没有装过 vcpkg 的机器，
> 这行留着也无害（只是两个没人读的 Globals 属性）。

---

## 第 5 段：找到 PCL（第 31~54 行）

这一段是整个文件里最关键的。分四块看。

### 5.1 指定 PCL 在哪（第 34~35 行）

```cmake
set(PCL_ROOT_DIR "E:/PointCloudEnv/PCL" CACHE PATH "PCL 安装根目录")
set(PCL_DIR "${PCL_ROOT_DIR}/cmake" CACHE PATH "包含 PCLConfig.cmake 的目录")
```

**① 语法**：`set(<变量> <值> CACHE <类型> "<帮助文字>")`

`CACHE` 是重点：这类变量会被**存进 `build\CMakeCache.txt`**，
下次 configure 时**如果缓存里已经有值，就用缓存里的**，不重新赋这个默认值。

**② 参数**：
- `PATH` 是缓存类型（另有 `BOOL`、`STRING`、`FILEPATH`）
- 第三段是 `cmake-gui` 里显示的提示文字

**③ 为什么需要**：CMake 得知道 `PCLConfig.cmake` 在哪。
`PCL_DIR` 就是"这个文件所在目录"。PCL AllInOne 包的约定是 `<根>/cmake/PCLConfig.cmake`。

**④ 改掉会怎样**：
- 路径写错 → `Could not find a package configuration file provided by "PCL"`
- **只改文件里的默认值，不删缓存 → 没用**！因为 `CACHE` 变量优先用旧缓存。

**这是初学者最常踩的坑。** 正确做法二选一：

```powershell
# 办法 A：命令行覆盖（会更新缓存）
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DPCL_ROOT_DIR="D:/Other/PCL"

# 办法 B：直接删缓存
Remove-Item -Recurse -Force build
```

**⑤ 验证**：

```powershell
Select-String -Path E:\dianyunxulian\build\CMakeCache.txt -Pattern "PCL_(ROOT_)?DIR"
```

预期：
```
PCL_DIR:PATH=E:/PointCloudEnv/PCL/cmake
PCL_ROOT_DIR:PATH=E:/PointCloudEnv/PCL
```

### 5.2 OpenNI2 的路径（第 37~39 行）

```cmake
set(OPENNI2_REDIST_DIR "C:/Program Files/OpenNI2/Redist" CACHE PATH "包含 OpenNI2.dll 的目录")
```

**为什么需要**：这是 PCL AllInOne 包一个很坑的地方。

`pcl_io.dll` 在**加载时**（LoadLibrary 那一刻）就依赖 `OpenNI2.dll`，
哪怕你的代码完全不碰深度相机。而 OpenNI2 是 PCL 的子安装器单独装的，
默认装到 `C:\Program Files\OpenNI2`，**不在 PCL 目录树下**。

结果就是：DLL 拷贝脚本从 `PCL_ROOT\bin` 里找不到 OpenNI2.dll，
程序一启动就以 `0xC0000135`（STATUS_DLL_NOT_FOUND）静默退出，**没有任何提示**。

**④ 删掉会怎样**：第 9 段少了那个 `copy_if_different`，
程序运行时先看 `PATH` 里有没有 OpenNI2 —— 没有就崩。

**⑤ 验证**：

```powershell
Test-Path "C:\Program Files\OpenNI2\Redist\OpenNI2.dll"
# 预期 True
```

### 5.3 `find_package`（第 41~51 行）

```cmake
find_package(PCL 1.15 REQUIRED COMPONENTS
    common
    io
    filters
    features
    search
    kdtree
    segmentation
    visualization
)
```

**① 语法**：`find_package(<包名> [版本] [REQUIRED] [COMPONENTS <组件...>])`

**② 参数**：
- `PCL` — 要找的包名。CMake 会去找 `PCLConfig.cmake` 或 `FindPCL.cmake`
- `1.15` — 最低版本要求
- `REQUIRED` — 找不到就 `FATAL_ERROR`，停止 configure
- `COMPONENTS` — 只要这几个模块

**③ 为什么要限定组件**：PCL 有 20 个模块。全要不写 `COMPONENTS` 的话，
`PCL_LIBRARIES` 里会包含 `pcl_apps`、`pcl_people`、`pcl_stereo` 等一堆用不到的，
链接变慢、依赖变多、还可能撞上没装的模块。

**本项目实际用到的模块对照表**：

| 组件 | 本项目里的实际用途 | 代码位置 |
|---|---|---|
| `common` | `pcl::PointXYZ`、`compute3DCentroid`、`getMinMax3D` | 几乎所有文件 |
| `io` | `loadPCDFile` / `savePCDFileBinary` | `src\core\cloud_io.cpp` |
| `filters` | `VoxelGrid`、`StatisticalOutlierRemoval`、`RadiusOutlierRemoval`、`PassThrough` | `src\preprocessing\*.cpp`、`src\segmentation\passthrough_demo.cpp` |
| `features` | 预留（法向估计还没实现） | — |
| `search` | `pcl::search::KdTree` | `src\segmentation\euclidean_cluster.cpp` |
| `kdtree` | `pcl::KdTreeFLANN` | `src\search\kdtree_demo.cpp` |
| `segmentation` | `SACSegmentation`、`EuclideanClusterExtraction` | `src\segmentation\*.cpp` |
| `visualization` | 预留（`PCLVisualizer` 还没接进菜单） | — |

**④ 删掉一个组件会怎样**（本文档练习 6）：

删掉 `segmentation` → 编译通过、链接失败：

```
LNK2019: 无法解析的外部符号 "public: virtual void __cdecl pcl::SACSegmentation<...>::segment(...)"
```

因为 `pcl_segmentation.lib` 没被链接。

**⑤ 验证**：

```powershell
Select-String -Path E:\dianyunxulian\build\CMakeCache.txt -Pattern "^PCL_" | Select-Object -First 20
```

### 5.4 打印找到的版本（第 53~54 行）

```cmake
message(STATUS "找到 PCL 版本: ${PCL_VERSION}")
message(STATUS "PCL 头文件目录: ${PCL_INCLUDE_DIRS}")
```

**① 语法**：`message(<级别> "<文本>")`

**级别**：`STATUS`（普通信息，带 `--` 前缀）、`WARNING`、`FATAL_ERROR`（直接停）、`DEBUG`。

**③ 为什么需要**：这是**排障的第一手信息**。
configure 时如果看到 `-- 找到 PCL 版本: `（空的），立刻就知道 `find_package` 没成功。

**⑤ 验证**：configure 时终端会打印：

```
-- 找到 PCL 版本: 1.15.1
-- PCL 头文件目录: E:/PointCloudEnv/PCL/include/pcl-1.15;...
```

**如果这两行没出现**，说明 configure 在更早的地方就失败了。

---

## 第 6 段：主程序与源文件（第 56~85 行）

### 6.1 `add_executable`（第 59 行）

```cmake
add_executable(PointCloudPractice)
```

**① 语法**：`add_executable(<目标名> [源文件...])`

**② 参数**：`PointCloudPractice` 既是"目标名"（CMake 内部引用它的名字），
也是"生成的 exe 文件名"（`PointCloudPractice.exe`）。

**③ 为什么这里**没有**列源文件**：因为紧接着用 `target_sources()` 加。
分成两条命令是为了演示"加一个文件 = 加一行"，这在几十个文件时更清楚。

**④ 改掉会怎样**：改名字 → exe 和 `.vcxproj` 都改名。
删掉 → 后面的 `target_sources(...)` 报"目标不存在"。

### 6.2 `target_sources`（第 61~85 行）

```cmake
target_sources(PointCloudPractice PRIVATE
    # ---- 公共层 ----
    src/core/console.cpp
    src/core/cloud_io.cpp
    ...
    src/main.cpp
)
```

**① 语法**：`target_sources(<目标> <可见性> <文件...>)`

**② 参数**：
- `PRIVATE` — 这些源文件只属于这个目标，不传递给别的目标（本项目只有一个主目标，写 `PRIVATE` 是正确习惯）
- 路径是**相对于 `CMakeLists.txt` 所在目录**的，用 `/` 而不是 `\`

**③ 本项目为什么需要**：这 20 个 `.cpp` 就是全部要编译的文件。
**漏一个 → `LNK2019`**，因为那个 `.cpp` 里的函数没被编译，谁调用它谁就链接失败。

**④ 删掉一行会怎样**（本文档练习 4）：

删掉 `src/preprocessing/voxel_filter.cpp` → 重新 configure + build：

```
voxel_filter.obj : error LNK2019:
无法解析的外部符号 "struct pcp::AlgoResult __cdecl pcp::runVoxelGrid(...)"
```

**⑤ 验证**：

```powershell
Select-String -Path E:\dianyunxulian\build\PointCloudPractice.vcxproj -Pattern "ClCompile Include" | Measure-Object
```

预期 Count = 20（对应 20 个 `.cpp`）。

**注意**：`.h` 文件**不需要**写进 `target_sources`。
CMake 只关心"要编译哪些 `.cpp`"。但如果你把 `.h` 加进去（放在旁边当参考），
CMake 会忽略它，不会报错 —— 只是有些 IDE 里看着舒服点。

---

## 第 7 段：头文件搜索路径（第 87~93 行）

```cmake
target_include_directories(PointCloudPractice PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PCL_INCLUDE_DIRS}
)
```

**① 语法**：`target_include_directories(<目标> <可见性> <目录...>)`

**② 参数**：
- `${CMAKE_CURRENT_SOURCE_DIR}` — `CMakeLists.txt` 所在目录（= `E:\dianyunxulian`）
- `${PCL_INCLUDE_DIRS}` — `find_package(PCL)` 得到的 PCL 头文件目录列表（可能有多个）

**③ 为什么需要**：

第一个目录让代码能写 `#include "core/cloud_io.h"` 而不是 `#include "../../core/cloud_io.h"`。
第二个目录让代码能写 `#include <pcl/io/pcd_io.h>`。

**④ 删掉第一个会怎样**（本文档练习 5）：

```
error C1083: 无法打开包括文件: "core/algo_result.h": No such file or directory
```

`C1083` 是**预处理阶段**错误，最早的阶段。所有 `#include "xxx/yyy.h"` 全部失效。

**⑤ 验证**：

```powershell
Select-String -Path E:\dianyunxulian\build\PointCloudPractice.vcxproj -Pattern "AdditionalIncludeDirectories" | Select-Object -First 1
```

预期能看到 `E:\dianyunxulian\src;E:\PointCloudEnv\PCL\include\pcl-1.15;...`。

---

## 第 8 段：链接（第 95~99 行）

```cmake
target_link_directories(PointCloudPractice PRIVATE ${PCL_LIBRARY_DIRS})
target_link_libraries(PointCloudPractice PRIVATE ${PCL_LIBRARIES})
```

### 8.1 `target_link_directories`

**作用**：告诉链接器"去哪些目录找 `.lib`"。对应 `link.exe` 的 `/LIBPATH:` 参数。

**本项目**：`${PCL_LIBRARY_DIRS}` 通常就是 `E:\PointCloudEnv\PCL\lib`。

### 8.2 `target_link_libraries`

**作用**：真正列出要链接的库。

**`${PCL_LIBRARIES}` 里是什么**：一长串 `.lib` 的**完整路径**，例如：

```
E:/PointCloudEnv/PCL/lib/pcl_common.lib
E:/PointCloudEnv/PCL/lib/pcl_io.lib
E:/PointCloudEnv/PCL/lib/pcl_filters.lib
...
E:/PointCloudEnv/PCL/3rdParty/VTK/lib/vtkCommonCore-9.4.lib
E:/PointCloudEnv/PCL/3rdParty/Boost/lib/libboost_system-vc143-mt-x64-1_86.lib
...
```

**注意**：因为给的是**完整路径**，`target_link_directories` 其实**可以省略**。
保留它是为了兼容 `PCLConfig.cmake` 某些情况下给出裸文件名的场景。

**③ 为什么需要**：没有这一步，链接器只知道"你要调 `pcl::io::loadPCDFile`"，
但不知道去哪个 `.lib` 里找它的实现 → `LNK2019`。

**④ 删掉会怎样**：几百个 `LNK2019`，全是 `pcl::*`、`vtk*` 的符号。

**⑤ 验证**（本文档练习 6）：

```powershell
Select-String -Path E:\dianyunxulian\build\PointCloudPractice.vcxproj -Pattern "AdditionalDependencies" | Select-Object -First 1
```

输出的那一长行就是链接器实际收到的库列表。

**Debug 和 Release 的区别在这里体现**：同一个 `.vcxproj` 里有两套配置，
Debug 那套的 `AdditionalDependencies` 是 `pcl_common-gd.lib`，
Release 那套是 `pcl_common.lib`。**这是 VS 多配置生成器自动处理的，CMakeLists 里不用写 if。**

---

## 第 9 段：`add_definitions` 与编译宏（第 101~124 行）

### 9.1 `add_definitions`（第 101~103 行）

```cmake
add_definitions(${PCL_DEFINITIONS})
```

**① 语法**：`add_definitions(<编译选项...>)`（老命令，现在推荐 `target_compile_definitions`）

**② `${PCL_DEFINITIONS}` 里是什么**：`PCLConfig.cmake` 给出的编译宏，主要是
`-DBOOST_ALL_NO_LIB` 之类。

**③ 注意一个副作用**：`add_definitions` 是**目录级**命令 ——
它对**当前目录及其所有子目录**生效。所以写在 `add_subdirectory(practice)` 之前，
`practice\exercises.cpp` 也会继承这些宏。这是故意的。

### 9.2 把项目路径编译进程序（第 105~108 行）

```cmake
add_compile_definitions("PCP_PROJECT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")
```

**① 语法**：`add_compile_definitions(<宏定义...>)`，等价于给编译器加 `/DPCP_PROJECT_DIR="E:/dianyunxulian"`。

**② 为什么要转义 `\"`**：宏的值是一个**字符串字面量**，
所以最终的 C++ 代码里要变成 `const char* = "E:/dianyunxulian"`。
CMake 里写 `\"` 是为了让引号传达到编译器。

**③ 为什么需要**：`src\core\cloud_io.cpp` 的 `projectDir()` 优先用这个宏：

```cpp
#ifdef PCP_PROJECT_DIR
    dir = PCP_PROJECT_DIR;
#endif
```

这样程序**无论从哪个工作目录启动**都能找到 `data\scene_original.pcd`。
在 VS 里按 F5 时，工作目录是 `build\Release\`，靠这个宏才找得到项目根。

**④ 删掉会怎样**：`projectDir()` 退化成"从当前工作目录向上找 `CMakeLists.txt`"。
在项目根目录下运行没问题，但：
- 在 VS 里 F5 可能找错
- 把 exe 拷到别处就彻底找不到

**⑤ 验证**：

```powershell
Select-String -Path E:\dianyunxulian\build\PointCloudPractice.vcxproj -Pattern "PCP_PROJECT_DIR" | Select-Object -First 1
```

运行时程序第一行就打印「项目根目录: E:/dianyunxulian」，也能验证。

### 9.3 MSVC 专用选项（第 110~124 行）

```cmake
if(MSVC)
    target_compile_options(PointCloudPractice PRIVATE
        /utf-8
        /bigobj
        /wd4996
        /wd4819
    )
    target_compile_definitions(PointCloudPractice PRIVATE
        NOMINMAX
        _USE_MATH_DEFINES
        _CRT_SECURE_NO_WARNINGS
        BOOST_ALL_NO_LIB
    )
endif()
```

**`if(MSVC)` 的意义**：这些是 **MSVC 编译器特有的选项**。
如果用 GCC/Clang 编译（比如将来在 Linux 上），`/utf-8` 会报"未知选项"。
用 `if(MSVC)` 包起来，换编译器时自动跳过。

**每个选项的作用与"删掉会怎样"**：

| 选项 | 作用 | 删掉会怎样 |
|---|---|---|
| `/utf-8` | 源码按 UTF-8 解释 | **中文全部乱码**。因为 MSVC 默认按系统代码页（GBK）读源文件，而本项目源码是 UTF-8 |
| `/bigobj` | 单个 `.obj` 允许超过 65535 个节 | 模板多的文件报 `fatal error C1128: 节数超过对象文件格式限制` |
| `/wd4996` | 关掉"已弃用函数"警告 | 编译时刷屏 `warning C4996: 'fopen': This function or variable may be unsafe` |
| `/wd4819` | 关掉"文件包含非 ANSI 字符"警告 | 每个含中文的文件报一条 `warning C4819` |
| `NOMINMAX` | 阻止 `<windows.h>` 定义 `min`/`max` 宏 | `std::min(a, b)` 报 `error C2589: "(": 右边的非法标记` —— 因为 `min` 被宏替换了 |
| `_USE_MATH_DEFINES` | 让 `<cmath>` 提供 `M_PI` | 用到 `M_PI` 的地方报"未定义标识符" |
| `_CRT_SECURE_NO_WARNINGS` | 关掉 `fopen`/`sprintf` 安全警告 | 大量 `warning C4996` |
| `BOOST_ALL_NO_LIB` | 阻止 Boost 用 `#pragma comment(lib,...)` 自动链接 | 可能链接到 Boost 的**错误版本**，报 `LNK2005` 或 `LNK2019` |

**注意 `target_compile_options` 是"目标级"的**，只管 `PointCloudPractice`，
不管 `exercises`。所以 `practice\CMakeLists.txt` 里又写了一遍同样的选项 ——
**这是有意的重复**，为了让练习区能独立控制。

**④ 整体删掉 `if(MSVC)` 块会怎样**：先是中文乱码（最明显），再是一堆警告淹没有用信息。

---

## 第 10 段：运行期 DLL 收集（第 126~195 行）

**这是整个文件最复杂的一段**，也是最容易让人觉得"这是黑魔法"的一段。慢慢拆。

### 背景：为什么需要它

C++ 在 Windows 上，**编译链接成功不等于能运行**：

```
编译 / 链接成功 → exe 生成
    ↓
运行时 → 加载器去找 pcl_io.dll、vtkCommonCore-9.4.dll ...
    ↓
找不到 → 进程立刻退出，退出码 -1073741515（0xC0000135），没有任何错误信息
```

正常情况下，你要么把 `PCL\bin` 加进 `PATH`，要么把 DLL 拷到 exe 旁边。
**本项目的做法是：构建完成后自动拷**，这样在 VS 里按 F5 也能直接跑。

### 10.1 从 `.lib` 反推 DLL 名字（第 136~151 行）

```cmake
set(_pcl_stems "")
foreach(_entry IN LISTS PCL_LIBRARIES)
    get_filename_component(_name "${_entry}" NAME_WE)
    if(_name MATCHES "^pcl_")
        set(_base "${_name}")
        if(_name MATCHES "d$")
            string(REGEX REPLACE "d$" "" _try "${_name}")
            if(EXISTS "${PCL_ROOT_DIR}/bin/${_try}.dll" OR EXISTS "${PCL_ROOT_DIR}/bin/${_try}d.dll")
                set(_base "${_try}")
            endif()
        endif()
        list(APPEND _pcl_stems "${_base}")
    endif()
endforeach()
list(REMOVE_DUPLICATES _pcl_stems)
```

**逐行理解**：

| 行 | 在干什么 |
|---|---|
| `set(_pcl_stems "")` | 准备一个空列表，存"模块基名" |
| `foreach(_entry IN LISTS PCL_LIBRARIES)` | 遍历 `${PCL_LIBRARIES}` 里每个路径 |
| `get_filename_component(_name "${_entry}" NAME_WE)` | 取文件名**去掉扩展名**：`.../pcl_common.lib` → `pcl_common` |
| `if(_name MATCHES "^pcl_")` | 只处理 PCL 自己的库，跳过 vtk/boost/eigen |
| `if(_name MATCHES "d$")` | PCL 的 debug 库名以 `d` 结尾（`pcl_iod`、`pcl_commond`） |
| `string(REGEX REPLACE "d$" "" _try ...)` | 试着去掉结尾的 `d`，得到 `pcl_io` |
| `if(EXISTS ...)` | **只有当 `bin\pcl_io.dll` 或 `bin\pcl_iod.dll` 真的存在**，才确认这个 `d` 是 debug 后缀 |
| `list(REMOVE_DUPLICATES ...)` | `pcl_common` 和 `pcl_commond` 去重后都变成 `pcl_common`，去重避免重复拷贝 |

**为什么需要这段"去掉 d"的绕弯逻辑**：

PCL 的模块基名本身有的以 `d` 结尾吗？检查一下：

```powershell
Get-ChildItem E:\PointCloudEnv\PCL\lib -Filter "pcl_*.lib" | Select-Object -ExpandProperty Name | Select-Object -First 10
```

你会看到 `pcl_common.lib`、`pcl_commond.lib`、`pcl_io.lib`、`pcl_iod.lib`……
没有天然以 `d` 结尾的基名（`pcl_gpu` 之类的不在这些里）。
但还是加了 `EXISTS` 检查，因为万一某个模块叫 `pcl_xxxd` 本身，直接剥 `d` 会剥错。

### 10.2 配对 release / debug 的 DLL（第 153~167 行）

```cmake
foreach(_stem IN LISTS _pcl_stems)
    set(_rel "${PCL_ROOT_DIR}/bin/${_stem}.dll")
    set(_dbg "${PCL_ROOT_DIR}/bin/${_stem}d.dll")
    if(EXISTS "${_rel}" AND EXISTS "${_dbg}")
        list(APPEND _pcl_dlls "$<IF:$<CONFIG:Debug>,${_dbg},${_rel}>")
    elseif(EXISTS "${_rel}")
        list(APPEND _pcl_dlls "${_rel}")
    elseif(EXISTS "${_dbg}")
        list(APPEND _pcl_dlls "${_dbg}")
    endif()
    ...
endforeach()
```

**核心是第 159 行那个生成器表达式**：

```cmake
$<IF:$<CONFIG:Debug>, ${_dbg}, ${_rel}>
```

意思是：**构建 Debug 配置时用 debug 的 DLL，构建 Release 时用 release 的**。

为什么不能让 configure 时决定？因为 VS 的**一个工程同时包含 Debug 和 Release 两套配置**。
configure 只发生一次，而 Debug/Release 是构建时才选的。
所以只能在 POST_BUILD 里用生成器表达式，让构建时才知道拷哪个。

**如果不这样写**（比如只拷 release 版）：Debug 版 exe 会加载 release 版 DLL，
而两者的 CRT 不同（`MSVCRTD.dll` vs `MSVCRT.dll`），运行时崩溃或行为诡异。

### 10.3 清掉误拷进来的 `.lib`（第 165~167 行 + 第 190~194 行）

```cmake
list(APPEND _pcl_lib_junk
     "$<IF:$<CONFIG:Debug>,$<TARGET_FILE_DIR:PointCloudPractice>/${_stem}d.lib,$<TARGET_FILE_DIR:PointCloudPractice>/${_stem}.lib>")
...
add_custom_command(TARGET PointCloudPractice POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -f ${_pcl_lib_junk}
    COMMAND_EXPAND_LISTS
    VERBATIM)
```

**为什么需要**：因为 `$<TARGET_RUNTIME_DLLS>` 对"以裸路径链接、不是 imported target"的库
行为不严谨 —— 它有时会把 `.lib` 也当成"运行期依赖"拷到输出目录。
输出目录里出现一堆 `.lib` 不影响运行，但很脏，而且会让人误以为要手动处理。

所以这段把它们删掉。

**`COMMAND_EXPAND_LISTS` 的作用**：让列表在传给命令时**自动展开成多个参数**。
没有它，`${_pcl_lib_junk}`（一个含分号的长字符串）会被当成**一个**参数传给 `rm`，
导致找不到文件。

### 10.4 主拷贝命令（第 173~179 行）

```cmake
add_custom_command(TARGET PointCloudPractice POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:PointCloudPractice> "$<TARGET_FILE_DIR:PointCloudPractice>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${_pcl_dlls} "$<TARGET_FILE_DIR:PointCloudPractice>"
    COMMAND_EXPAND_LISTS
    VERBATIM)
```

**① 语法**：`add_custom_command(TARGET <目标> POST_BUILD COMMAND <命令> ...)`

**② 参数**：
- `POST_BUILD` — 在"链接完成之后"执行
- `${CMAKE_COMMAND} -E copy_if_different` — CMake 自带的跨平台拷贝命令（文件相同就跳过）
- `$<TARGET_RUNTIME_DLLS:PointCloudPractice>` — **CMake 3.21+ 的功能**：自动算出这个 exe 需要的所有 DLL
- `$<TARGET_FILE_DIR:...>` — exe 所在目录

**③ 为什么分两次拷**：
- 第一次靠 `TARGET_RUNTIME_DLLS` 拷 VTK 的 DLL（VTK 是 imported target，这个机制对它有效）
- 第二次靠我们算出来的 `_pcl_dlls` 拷 PCL 的 DLL（对 PCL 无效，必须自己算）

**这是本项目踩过的坑的具体表现**：如果只写第一次，PCL 的 DLL 一个都不会被拷过来。

### 10.5 OpenNI2（第 181~188 行）

```cmake
if(EXISTS "${OPENNI2_REDIST_DIR}/OpenNI2.dll")
    add_custom_command(TARGET PointCloudPractice POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OPENNI2_REDIST_DIR}/OpenNI2.dll" "$<TARGET_FILE_DIR:PointCloudPractice>"
        VERBATIM)
else()
    message(WARNING "未找到 OpenNI2.dll(...)")
endif()
```

**为什么**：见 5.2 节。`pcl_io.dll` 加载期依赖它，缺了就是 `0xC0000135`。

**注意 `else()` 里的 `message(WARNING ...)`**：这是"软失败"处理 ——
找不到不阻止构建，只是提醒。这样即使换台机器没装 OpenNI2，也能编出来。

### 10.6 验证这段到底做了什么

```powershell
# 看输出目录里有哪些 DLL
Get-ChildItem E:\dianyunxulian\build\Release -Filter *.dll | Measure-Object
Get-ChildItem E:\dianyunxulian\build\Release -Filter *.dll | Select-Object -First 10 Name

# 看有没有误留的 .lib（应该没有）
Get-ChildItem E:\dianyunxulian\build\Release -Filter *.lib
```

**预期**：几十个 DLL（PCL + VTK + OpenNI2），**零个 `.lib`**。

**把整个 `if(WIN32)` 段删掉会怎样**：exe 照样能编出来，但运行立刻退出（`-1073741515`）。
这时候只能靠 `. E:\PointCloudEnv\env.ps1` 设 PATH 才能跑。

---

## 第 11 段：练习区子目录（第 197~202 行）

```cmake
option(BUILD_PRACTICE_EXERCISES "同时编译练习区 exercises.exe" ON)
if(BUILD_PRACTICE_EXERCISES)
    add_subdirectory(practice)
endif()
```

**① `option` 语法**：`option(<变量名> "<说明>" <默认值>)`，创建一个**布尔缓存变量**。

**② 为什么需要**：练习区是"额外"的东西，有时候你只想编主程序（快一点）。
可以这样关掉：

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DBUILD_PRACTICE_EXERCISES=OFF
```

**③ `add_subdirectory(practice)` 干什么**：
去执行 `practice\CMakeLists.txt`，在**同一个构建树**里再加一个目标。
所以 `build\practice\Release\exercises.exe` 会出现在 `build\` 下面（不是 `practice\` 下面）。

**④ 删掉会怎样**：`exercises.exe` 不再生成，主程序不受影响。

**⑤ 验证**：

```powershell
Get-ChildItem E:\dianyunxulian\build\practice\Release\*.exe
```

预期看到 `exercises.exe`。

---

## 第十二部分：Visual Studio 多配置 vs `CMAKE_BUILD_TYPE`

这个主题在 [03_CMake入门.md](03_CMake入门.md) 第七节讲过，这里补一个**具体的、能验证的**对比。

| | 单配置生成器（Makefiles / Ninja） | 多配置生成器（Visual Studio / Xcode） |
|---|---|---|
| 配置怎么选 | configure 时定死：`-DCMAKE_BUILD_TYPE=Release` | build 时选：`--config Release` |
| 生成的工程 | 一个 `Makefile` | `.vcxproj` 里**同时**含 Debug 和 Release 两套 |
| `CMAKE_BUILD_TYPE` 有效吗 | 有效 | **无效**，会被忽略 |
| 切换 Debug/Release 要不要重新 configure | 要 | **不要** |
| 能不能用 `$<CONFIG:Debug>` | 能（值固定） | 能（值随 `--config` 变） |

**验证 `CMAKE_BUILD_TYPE` 无效**：

```powershell
cd E:\dianyunxulian
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Debug
```

configure 结尾会出警告：

```
CMake Warning:
  Manually-specified variables were not used by the project:
    CMAKE_BUILD_TYPE
```

**这就是证据** —— 你给了它，但这个项目的配置流程里根本没用这个变量。

**本项目为什么不需要 `if(CMAKE_BUILD_TYPE STREQUAL "Debug")`**：因为所有 Debug/Release
的差异都由 VS 自动处理（不同的 `/O2`、不同的 `-gd.lib`、不同的 CRT），
CMakeLists 里只需在需要区分的地方用 `$<CONFIG:Debug>`。

---

## 第十三部分：8 个实战练习

每个练习都按"改什么 → 预期 → 报错 → 为什么 → 怎么恢复"写。
**请真的动手做，特别是练习 4、5** —— 那两个报错是你以后最常遇到的。

### 练习 1：改项目名称（5 分钟）

**改什么**：第 15 行 `project(PointCloudPractice LANGUAGES CXX)` → `project(MyTest LANGUAGES CXX)`

**命令**：
```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
Get-ChildItem build -Filter *.sln
```

**预期**：`MyTest.sln`（原来叫 `PointCloudPractice.sln`）。
**但 exe 名字不变**，还是 `PointCloudPractice.exe`。

**为什么**：`.sln` 名来自 `project()`，`.vcxproj` 和 `.exe` 名来自 `add_executable()`。

**怎么恢复**：改回 `PointCloudPractice`，重新 configure。

---

### 练习 2：改 C++ 标准（10 分钟）

**改什么**：第 18 行 `set(CMAKE_CXX_STANDARD 17)` → `set(CMAKE_CXX_STANDARD 14)`

**命令**：
```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

**预期报错**（大意）：
```
error C2653: "filesystem": 不是类或命名空间名称
```
出现在 `src\core\cloud_io.cpp`。

**为什么**：`std::filesystem` 是 C++17 才进标准库的。

**注意**：如果只改 `CMAKE_CXX_STANDARD` 为 14，而 `_STANDARD_REQUIRED` 还是 `ON`，
CMake 会用 C++14 编译，然后报上面那个错。
如果把 `CMAKE_CXX_STANDARD_REQUIRED` 改成 `OFF`，MSVC 可能"自动升到能编过的版本"，
反而不报错 —— **这就是为什么必须保持 `ON`**。

**怎么恢复**：改回 `17`，重新 configure + build。

---

### 练习 3：新增一个 `.cpp` 并加入构建（20 分钟）

**改什么**：
1. 新建 `src\core\hello.cpp`：
```cpp
#include <iostream>
void helloFromNewFile() { std::cout << "hello from new file\n"; }
```
2. 在 `src\main.cpp` 的 `main()` 开头加一行：`helloFromNewFile();`（记得在前面声明或 include）
3. 在 `CMakeLists.txt` 的 `target_sources` 里加 `src/core/hello.cpp`

**命令**：
```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
.\build\Release\PointCloudPractice.exe
```

**预期**：启动时先打印 `hello from new file`，然后菜单。

**中途报错**（一定会遇到）：如果你**加了源文件但忘了改 CMakeLists**，只跑 `cmake --build`：

```
error LNK2019: 无法解析的外部符号 "void __cdecl helloFromNewFile(void)"
```

**为什么**：`.cpp` 没参与编译，函数就没有实现。

**怎么恢复**：删掉 `hello.cpp` 和 `main.cpp` 里那行，从 `target_sources` 删掉那一行，重新 configure。

---

### 练习 4：故意遗漏源文件，看链接错误（10 分钟）

**改什么**：在 `target_sources` 里注释掉 `src/preprocessing/voxel_filter.cpp`

**命令**：
```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

**预期报错**：
```
error LNK2019: 无法解析的外部符号 "struct pcp::AlgoResult __cdecl pcp::runVoxelGrid(
    class std::shared_ptr<class pcl::PointCloud<struct pcp::PointT> const> const &,
    struct pcp::VoxelOptions const &)"
```

**逐字读懂它**：

| 部分 | 含义 |
|---|---|
| `LNK2019` | **链接**阶段错误（不是编译） |
| `无法解析的外部符号` | 有人要用，但找不到实现 |
| `struct pcp::AlgoResult` | 返回类型 |
| `__cdecl pcp::runVoxelGrid` | 函数全名（含命名空间） |
| 最后两行 | 参数类型。`shared_ptr<const PointCloud<PointT>> const&` 说明声明是 `const CloudPtr &` |

**为什么编译不报错**：`menu_items.cpp` 里 `#include "preprocessing/voxel_filter.h"`，
编译器看到声明就够了。**声明是编译期的事，定义是链接期的事。**

**怎么恢复**：取消注释，重新 configure + build。

**这个练习的价值**：以后你看到 `LNK2019`，第一反应就应该是"哪个 `.cpp` 没加进构建"，
或者"`.h` 里声明了但 `.cpp` 里没实现"。

---

### 练习 5：故意遗漏头文件搜索路径，看包含错误（10 分钟）

**改什么**：第 91 行，把 `${CMAKE_CURRENT_SOURCE_DIR}/src` 删掉

**命令**：
```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

**预期报错**：
```
error C1083: 无法打开包括文件: "core/algo_result.h": No such file or directory
```

会报几十条，因为几乎每个文件都 include 了 `core/` 下的东西。

**为什么**：`#include "core/algo_result.h"` 的双引号先找"当前文件所在目录"
（`src\segmentation\`，没有 `core\` 子目录），再找 `-I` 列表（现在没有 `src`）。

**注意 `C1083` 和 `LNK2019` 的本质区别**：
- `C1083` 在**预处理**阶段 —— 连代码都没开始编
- `LNK2019` 在**链接**阶段 —— 所有代码都编完了

**怎么恢复**：加回 `${CMAKE_CURRENT_SOURCE_DIR}/src`，重新 configure。

---

### 练习 6：查看 PCL 的头文件和链接配置（10 分钟，不用改任何东西）

**做什么**：

```powershell
# 1. 看 configure 时打印的 PCL 版本和头文件目录
#    重新 configure 一次就能看到
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 2>&1 | Select-String "PCL"

# 2. 看生成的工程里，PCL 的 include 路径
Select-String -Path build\PointCloudPractice.vcxproj -Pattern "AdditionalIncludeDirectories" | Select-Object -First 1

# 3. 看链接器实际收到的库（这一行会非常长）
$line = Select-String -Path build\PointCloudPractice.vcxproj -Pattern "AdditionalDependencies" | Select-Object -First 1
($line.Line -split ';').Count

# 4. 数一下实际链接了多少个库
```

**预期**：
- 第 1 步打印 `-- 找到 PCL 版本: 1.15.1`
- 第 2 步能看到 `E:\PointCloudEnv\PCL\include\pcl-1.15`
- 第 4 步的库数量在 40~120 之间（取决于 PCL 版本和 VTK 模块多少）

**这个练习的意义**：让你亲眼看到"CMakeLists 里的 8 个 COMPONENTS 展开成了几十个 `.lib`"。
`find_package` 是个"放大器"。

---

### 练习 7：新增一个点云算法模块（30 分钟，综合）

这是"加模块"的完整流程，[02_项目目录与多文件C++.md](02_项目目录与多文件C++.md) 第十一节有详细七步。

这里只强调 **CMake 这一侧只需要改两处**：

```cmake
# ① target_sources 里加一行
    src/preprocessing/median_filter.cpp

# ② 如果新模块需要新的 PCL 组件（比如需要 pcl::surface），
#    在 find_package 的 COMPONENTS 里加一行
    surface
```

**其余都不用改** —— 不需要写新的 `add_executable`，不需要新的 `target_link_libraries`。

**这体现了 `add_executable` + `target_*` 这套"目标式"写法的好处**：
所有设置都是"针对这个目标"的，加文件只是往列表里加一项。

**验证**：

```powershell
Select-String -Path build\PointCloudPractice.vcxproj -Pattern "median_filter" 
```

应该能看到 `<ClCompile Include="E:\dianyunxulian\src\preprocessing\median_filter.cpp" />`。

---

### 练习 8：分别构建 Debug 和 Release（15 分钟）

**做什么**：

```powershell
cmake --build build --config Debug
cmake --build build --config Release
```

**然后比较**：

```powershell
Get-ChildItem build\Debug\PointCloudPractice.exe | Select-Object Name, Length
Get-ChildItem build\Release\PointCloudPractice.exe | Select-Object Name, Length
```

**预期**：Debug 版明显更大（本项目 Debug 约 3 MB，Release 约 200 KB）。

**再比较依赖的 PCL DLL**：

```powershell
Get-ChildItem build\Debug\pcl_common*.dll | Select-Object Name
Get-ChildItem build\Release\pcl_common*.dll | Select-Object Name
```

**预期**：Debug 目录里是 `pcl_common-gd.dll` 或 `pcl_commond.dll`，
Release 目录里是 `pcl_common.dll`。

**如果 Debug 构建失败**：检查 `E:\PointCloudEnv\PCL\lib` 里有没有 `-gd` 后缀的 `.lib`。
没有的话说明你的 PCL 只装了 Release 一套，Debug 配置用不了。

**验证 `$<CONFIG:Debug>` 真的生效了**：

```powershell
Select-String -Path build\CMakeFiles -Pattern "CONFIG" -ErrorAction SilentlyContinue | Select-Object -First 3
```

更直接的办法：删掉 `build\Debug` 和 `build\Release` 两个目录，然后只 build Debug，
看 `build\Debug\` 里拷进来的 DLL 是不是 debug 版。

---

## 第十四部分：一页速查表

| 你改了 | 命令 | 为什么 |
|---|---|---|
| 某个 `.cpp` 的一行 | `cmake --build build --config Release` | 只重编那一个 `.obj` |
| 某个 `.h` 的一行 | 同上 | 所有 include 它的 `.cpp` 重编（可能很慢） |
| `CMakeLists.txt` 任意一行 | 先 `cmake -S . -B build ...` 再 build | 工程文件要重新生成 |
| 加/删源文件 | 同上 | `target_sources` 变了 |
| 改 `PCL_ROOT_DIR` | 同上（或用 `-D` 覆盖） | 缓存变量要更新 |
| 换生成器 | 删 `build\` 再 configure | `CMakeCache.txt` 记着旧生成器 |
| 换项目目录位置 | 删 `build\` 再 configure | `PCP_PROJECT_DIR` 是写死的绝对路径 |
| 想彻底重来 | `Remove-Item -Recurse -Force build` + configure | 最有效的排障第一招 |

**记住这个顺序，报错时先判断在哪一段**：

```
configure  → 编译 compile  → 链接 link  → 运行 run
```

- `CMake Error` → 第一段
- `error C****` → 第二段
- `error LNK****` → 第三段
- 无报错但退出码非零 → 第四段

---

## 下一步

去 [05_编译链接与调试.md](05_编译链接与调试.md)。

本文讲完了"CMake 怎么产出 `.exe`"，下一份讲"生成了 `.exe` 之后，怎么在 VS 里找到问题、
怎么读懂那些报错、怎么用调试器看清变量的值"。
