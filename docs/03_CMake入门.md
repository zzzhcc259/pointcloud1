# 03 CMake 入门

> 这份文档只讲一件事：**你在 PowerShell 里敲的那两条 `cmake` 命令，到底干了什么。**
>
> 不讲语法（那是 [04_CMakeLists逐行解释.md](04_CMakeLists逐行解释.md) 的事），只讲原理和关系。

---

## 一、CMake 是什么：一个"翻译官"

先说结论：

> **CMake 本身不编译任何代码。它只是把你的 `CMakeLists.txt` 翻译成"Visual Studio 认识的工程文件"。**

打个比方：

- 你写了一份**点菜清单**（`CMakeLists.txt`）：“我要一个叫 PointCloudPractice 的可执行文件，用这 20 个 `.cpp` 做，要链接 PCL”
- CMake 是**翻译官**，把这份清单翻译成**后厨能看懂的操作单**（`.sln` + `.vcxproj`）
- 真正炒菜的是**后厨**：MSBuild 指挥 MSVC 编译器去编译、链接

### 那为什么不直接写 `.vcxproj`？

因为 `.vcxproj` 是 XML，几百行，全是 MSVC 特有的东西。
一旦你换了编译器（比如换成 Linux 上的 GCC），整个文件作废。
而 `CMakeLists.txt` 换个生成器就能变成 Linux 的 Makefile —— **一份清单，到处能用**。

PCL 官方示例、你以后要看的任何开源点云项目，99% 都提供 `CMakeLists.txt` 而不是 `.vcxproj`。这就是必须学 CMake 的原因。

---

## 二、五个角色，各管什么（一定要分清）

```
  你手写的                    CMake 翻译的             微软的工具链
┌──────────────┐          ┌──────────────┐        ┌──────────────────┐
│ CMakeLists   │  ──────→ │  .sln        │ ─────→ │ MSBuild          │
│   .txt       │  CMake   │  .vcxproj    │        │   ↓              │
└──────────────┘          └──────────────┘        │ MSVC 编译器 cl.exe│
                                                   │   ↓              │
                                                   │ 链接器 link.exe   │
                                                   └──────────────────┘
```

| 角色 | 是什么 | 在本机的实际位置 | 它负责什么 |
|---|---|---|---|
| **CMake** | 独立软件，跨平台 | `C:\Program Files\CMake\bin\cmake.exe`（4.4.3） | 读 `CMakeLists.txt`，写 `.sln`/`.vcxproj` |
| **生成器（Generator）** | CMake 的一个参数，指定"翻译成哪种方言" | `-G "Visual Studio 18 2026"` | 决定输出 VS 工程还是 Makefile |
| **MSBuild** | 微软的"构建引擎" | 随 VS 安装 | 读 `.vcxproj`，决定"编哪些文件、按什么顺序" |
| **MSVC 编译器** | 真正的编译器 | `E:\visual studio\VC\Tools\MSVC\14.51...\bin\Hostx64\x64\cl.exe` | 把每个 `.cpp` 编成 `.obj` |
| **链接器** | 合并 `.obj` | 同目录下 `link.exe` | 把 `.obj` + `.lib` 拼成 `.exe` |
| **PCL** | 第三方库（一堆 `.h` + `.lib` + `.dll`） | `E:\PointCloudEnv\PCL` | 提供点云算法，你的代码调用它 |

### 最容易混淆的一条

> **CMake 不调用 `cl.exe`。是 CMake 生成工程，然后 MSBuild 调用 `cl.exe`。**

你在 PowerShell 里敲的 `cmake --build build`，其实是"CMake 帮你转手去调用 MSBuild"。
等价于你用 VS 打开 `.sln` 按 F7。

---

## 三、`configure` 和 `build` 是两件完全不同的事

这是初学者最大的坎。**两条命令，两个阶段，两种失败原因。**

### configure（配置）

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
```

| 参数 | 含义 |
|---|---|
| `-S .` | Source 目录 = 当前目录（就是 `E:\dianyunxulian`，放着 `CMakeLists.txt`） |
| `-B build` | Build 目录 = `build\`（CMake 在这里放它生成的东西） |
| `-G "..."` | 用哪个生成器（哪个方言） |
| `-A x64` | 目标平台架构 = 64 位 |

**它干的事**：

1. 读 `CMakeLists.txt`，逐行执行
2. 遇到 `find_package(PCL ...)` → **去硬盘上找 PCL**，把找到的路径记下来
3. 检查编译器能不能用（会偷偷编译一个几行的小程序试试）
4. 在 `build\` 里生成 `.sln`、`.vcxproj`、`CMakeCache.txt`

**它不编译你的代码。** 一个 `.obj` 都不会产生。

**成功的标志**：

```
-- Configuring done
-- Generating done
-- Build files have been written to: E:/dianyunxulian/build
```

**失败会发生在哪**：
- 找不到 PCL → `Could not find a package configuration file provided by "PCL"`
- 编译器版本不支持 C++17 → `target_compile_features`
- `CMakeLists.txt` 语法错 → `CMake Error at CMakeLists.txt:12 (find_package)`

### build（构建）

```powershell
cmake --build build --config Release
```

| 参数 | 含义 |
|---|---|
| `--build build` | 去 `build\` 里找已经生成好的工程，开始构建 |
| `--config Release` | 选 Release 配置（VS 是多配置生成器，见第六节） |

**它干的事**：让 MSBuild 去读 `.vcxproj`，然后调 `cl.exe` 编译、调 `link.exe` 链接。

**成功的标志**：

```
  PointCloudPractice.vcxproj -> E:\dianyunxulian\build\Release\PointCloudPractice.exe
  exercises.vcxproj -> E:\dianyunxulian\build\practice\Release\exercises.exe
```

（本项目有两个可执行目标，所以有两条 `->` 输出。）

### 一条必须记住的规则

| 你改了什么 | 需要做什么 |
|---|---|
| 改 `.cpp` 里的代码 | **只需 build** |
| 改 `.h` 里的内容 | **只需 build**（但会触发大量重编） |
| **改 `CMakeLists.txt`** | **必须先 configure，再 build** |
| 新建或删除 `.cpp` 文件 | **必须先 configure，再 build** |
| 改了 `PCL_ROOT_DIR` 之类的路径 | 必须先 configure |
| 换了生成器（VS2026 → VS2022） | 必须删掉 `build\` 重新 configure |

**为什么改 `CMakeLists.txt` 必须重新 configure**：
因为 `CMakeLists.txt` 是"翻译官的输入"。你改了清单，翻译官必须重新翻译一遍，
新的 `.vcxproj` 才会包含新文件、新路径。不重新 configure 的话，
MSBuild 拿到的是**旧的** `.vcxproj`，它压根不知道你加了文件。

**症状**：你新加了个 `median_filter.cpp`，加进了 `target_sources`，
但只跑了 `cmake --build`，结果报 `LNK2019`。因为旧的 `.vcxproj` 里没有这个文件。

---

## 四、为什么必须有独立的 `build\` 目录

本项目用的是**源外构建（out-of-source build）**：所有生成的东西都在 `build\` 里。

### 对比一下

**不用 `build\` 目录（"源内构建"，不推荐）**：

```
E:\dianyunxulian\
├── CMakeLists.txt
├── CMakeCache.txt        ← 生成物污染源码目录
├── CMakeFiles\           ← 一堆文件夹
├── PointCloudPractice.sln
├── PointCloudPractice.vcxproj
├── src\...
└── Release\              ← exe 混在源码旁边
```

**用了 `build\`（本项目的做法）**：

```
E:\dianyunxulian\
├── CMakeLists.txt        ← 只有源文件
├── src\
├── docs\
└── build\                ← 所有生成物都在这里面
    ├── CMakeCache.txt
    ├── PointCloudPractice.sln
    ├── PointCloudPractice.vcxproj
    ├── PointCloudPractice.dir\Release\*.obj
    └── Release\PointCloudPractice.exe
```

### 三个好处

1. **删起来干净**：`Remove-Item -Recurse -Force build` 之后，源码目录一点没动。
   遇到"莫名其妙编译不过"时，删 `build` 重来是最有效的第一招。
2. **可以并存多套配置**：`build-vs2026\`、`build-vs2022\`、`build-debug\` 互不干扰。
3. **Git 里一个 `build/` 就全忽略了**，不会有人误提交几十 MB 中间文件。

### 什么时候"源内构建"还行

只有一个情况：项目小到只有 3 个文件、而且你确定不换编译器。
本项目 32 个源文件 + 2 个可执行目标，绝对应该用 `build\`。

---

## 五、CMake 生成的工程文件长什么样

configure 成功之后，`build\` 里最重要的是这几个：

```powershell
Get-ChildItem E:\dianyunxulian\build -File | Select-Object Name, Length
```

| 文件 | 是什么 | 你需要直接改它吗 |
|---|---|---|
| `PointCloudPractice.sln` | **解决方案**：一个"项目集合"，VS 打开它 | ❌ 不要。下次 configure 会被覆盖 |
| `PointCloudPractice.vcxproj` | **项目**：一个可执行目标的完整构建配置（哪些 `.cpp`、哪些 include 路径、链接哪些 `.lib`） | ❌ 不要，改 `CMakeLists.txt` |
| `practice\exercises.vcxproj` | 第二个可执行目标的项目文件 | ❌ 同上 |
| `CMakeCache.txt` | **缓存**：记录"上次 configure 找到了什么"（PCL 在哪、生成器是哪个、缓存变量当前值） | ⚠️ 只有排障时才删 |
| `CMakeFiles\` | CMake 内部用的临时文件 | ❌ 不要碰 |

### `.vcxproj` 里能看到 CMakeLists 的"翻译结果"

```powershell
Select-String -Path E:\dianyunxulian\build\PointCloudPractice.vcxproj -Pattern "ClCompile Include" | Select-Object -First 5
```

你会看到 `<ClCompile Include="E:\dianyunxulian\src\core\cloud_io.cpp" />` 这样的行 ——
**这就是 `target_sources` 那一堆 `.cpp` 翻译出来的样子。**

同理：

```powershell
Select-String -Path E:\dianyunxulian\build\PointCloudPractice.vcxproj -Pattern "AdditionalIncludeDirectories" | Select-Object -First 2
```

能看到 `target_include_directories` 翻译出来的 `src;E:\PointCloudEnv\PCL\include\pcl-1.15;...`。

**这个观察很有价值**：以后你在 VS 里看到"这个文件怎么没参与编译"，
可以直接去 `.vcxproj` 里搜文件名，比猜快得多。

---

## 六、`.sln` / `.vcxproj` / `.obj` / `.lib` / `.dll` / `.exe` 都是什么

这是 Windows C++ 开发最基础的一张表。**每个都对应构建流水线上的一个产物**：

```
源码 .h/.cpp
    │  cl.exe 编译
    ↓
目标文件 .obj          ← 每个 .cpp 对应一个，机器码 + 待填的"外部符号"
    │  link.exe 链接（把 .obj 和 .lib 缝合起来）
    ↓
可执行文件 .exe        ← 你的程序
    │  运行时加载
    ↓
动态库 .dll            ← 运行时才找，找不到就 0xC0000135
```

| 后缀 | 全称 | 是什么 | 本项目的例子 |
|---|---|---|---|
| `.h` | header | 头文件，声明，`#include` 进去 | `src\core\algo_result.h` |
| `.cpp` | C++ source | 源文件，实现，**每个都单独编译** | `src\segmentation\ransac_plane.cpp` |
| `.obj` | object file | 编译产物。机器码 + 一堆"这里要调别人，地址待定" | `build\PointCloudPractice.dir\Release\ransac_plane.obj` |
| `.lib` | static library | **两种**：①静态库（代码直接进 exe）②导入库（只含 DLL 的"目录"） | `E:\PointCloudEnv\PCL\lib\pcl_io.lib`（属于②） |
| `.dll` | dynamic link library | 动态库。**运行时**才加载 | `E:\PointCloudEnv\PCL\bin\pcl_io.dll` |
| `.exe` | executable | 可执行文件 | `build\Release\PointCloudPractice.exe` |
| `.sln` | solution | VS 的"项目集合"，可以包含多个项目 | `build\PointCloudPractice.sln` |
| `.vcxproj` | VC++ project | 一个项目的构建配置（XML） | `build\PointCloudPractice.vcxproj` |
| `.pdb` | program database | 调试符号（变量名、行号）。**Debug 必备** | `build\Release\PointCloudPractice.pdb` |

### `.lib` 的两种含义 —— 这是初学者最容易懵的地方

**静态库**：`.lib` 里就是编译好的机器码。链接时**整个复制进 exe**。exe 变大，但不依赖外部文件。

**导入库（import library）**：`.lib` 里**没有真正的代码**，只有一张"函数名 → 在哪个 DLL 的哪个偏移"的表。
真正跑的时候要用 DLL。

**PCL 用的是第二种。** 证据：

```powershell
Get-ChildItem E:\PointCloudEnv\PCL\lib\pcl_io.lib | Select-Object Name, Length
Get-ChildItem E:\PointCloudEnv\PCL\bin\pcl_io.dll | Select-Object Name, Length
```

`pcl_io.lib` 只有几百 KB，`pcl_io.dll` 有几 MB —— 代码在 DLL 里，`.lib` 只是张目录。

**这解释了两件事**：

1. 为什么链接阶段能找到 `pcl_io.lib` 就算成功（链接器只要那张表）
2. 为什么程序运行时还要 DLL（真正执行要代码）

也就解释了为什么"编译链接都过了，一运行就退出（退出码 -1073741515）"——
缺的是 `.dll`，不是 `.lib`。

---

## 七、Debug 和 Release 到底差在哪

VS 是**多配置生成器**：同一个 `.vcxproj` 里同时存了 Debug 和 Release 两套设置。

### 差异表

| | Debug | Release |
|---|---|---|
| 编译器优化 | `/Od`（不优化） | `/O2`（速度优先） |
| 调试信息 | `/Zi`，完整 | 少量（为了 pdb） |
| 运行库 | `MSVCRTD.dll`（调试版 CRT） | `MSVCRT.dll`（正式版） |
| 额外的安全检查 | `/RTC1`（运行时检查）、`_DEBUG` 宏 | 无 |
| 断言 `assert` | 有效 | 被编译掉 |
| 速度 | 慢 | 快 5~20 倍 |
| exe 体积 | 大（本项目约 3 MB） | 小（本项目约 200 KB） |
| **依赖的 PCL 库** | `pcl_common-gd.lib` / `pcl_common-gd.dll` | `pcl_common.lib` / `pcl_common.dll` |

### 最后一行是重点

PCL 的 Debug 库文件名带 `-gd` 后缀（**g**eneral **d**ebug），Release 库不带。
两套库**不能混用**，混用会在链接时找不到符号，或者运行时行为诡异。

本项目能用 Debug 配置，是因为 `E:\PointCloudEnv\PCL\lib` 里**两套都有**：

```powershell
Get-ChildItem E:\PointCloudEnv\PCL\lib\pcl_common* | Select-Object Name
```

预期看到 `pcl_common.lib` 和 `pcl_common-gd.lib` 两个。

VTK 同理：

```powershell
Get-ChildItem E:\PointCloudEnv\PCL\3rdParty\VTK\bin\vtkCommonCore* | Select-Object Name
```

预期看到 `vtkCommonCore-9.4.dll` 和 `vtkCommonCore-9.4-gd.dll`。

**如果你的 PCL 只有 Release 一套**，那 Debug 配置会在链接时报一堆 `LNK2019`，
这时候只能：① 用 Release 配置开发（不能下断点），② 或者找到 Debug 版的 PCL。

### 怎么切换

**命令行**：`--config Debug` 或 `--config Release`

```powershell
cmake --build build --config Debug
cmake --build build --config Release
```

产物分别在：
- `build\Debug\PointCloudPractice.exe`
- `build\Release\PointCloudPractice.exe`

**VS 里**：工具栏上那个下拉框（默认显示 "Debug" 或 "Release"），改完按 F5。

### `CMAKE_BUILD_TYPE` 是什么，为什么本项目没用它

如果你看过 Linux 的 CMake 教程，会看到：

```cmake
set(CMAKE_BUILD_TYPE Release)
```

**在多配置生成器（Visual Studio、Xcode）里，这个变量是无效的**，因为它被设计给单配置生成器（Makefile、Ninja）。
VS 的 Debug/Release 由 `.vcxproj` 里两套 `<ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">` 表达。

所以本项目**没有** `set(CMAKE_BUILD_TYPE ...)` —— 写了也不生效，还会误导人。
你在 configure 时可能会看到这条警告：

```
CMake Warning:
  Manually-specified variables were not used by the project:
    CMAKE_BUILD_TYPE
```

看到了不用管，那是没写这句话的正常表现。

---

## 八、`find_package(PCL ...)` 到底做了什么

这是本项目 CMakeLists 里最"神秘"的一行：

```cmake
find_package(PCL 1.15 REQUIRED COMPONENTS common io filters features search kdtree segmentation visualization)
```

### 它做的事，按顺序

1. **去找 `PCLConfig.cmake` 这个文件**。CMake 按约定俗成的顺序搜索：
   - `CMAKE_PREFIX_PATH` 里的每个目录下的 `cmake\`
   - 环境变量 `PCL_ROOT` 下的 `cmake\`
   - 系统路径
2. **本项目是怎么让 CMake 找到的**：`CMakeLists.txt` 里先设置了两行：

```cmake
set(PCL_ROOT_DIR "E:/PointCloudEnv/PCL" CACHE PATH "...")
set(PCL_DIR "${PCL_ROOT_DIR}/cmake" CACHE PATH "...")
```

   `PCL_DIR` 就是"`PCLConfig.cmake` 所在的目录"。CMake 一旦看到 `PCL_DIR` 有值，直接去那里读。

3. **`PCLConfig.cmake` 被执行**。它内部会：
   - 逐层 `find_package(Boost)`、`find_package(Eigen3)`、`find_package(FLANN)`、`find_package(VTK)`、`find_package(Qhull)`
   - 把 PCL 的头文件目录、库文件路径全部收集起来
   - 设置一堆变量（`PCL_INCLUDE_DIRS`、`PCL_LIBRARIES`、`PCL_DEFINITIONS`）
   - 检查版本是否满足 `1.15`
4. **`REQUIRED` 的含义**：找不到就直接 `FATAL_ERROR`，不继续。
   去掉 `REQUIRED` 的话，找不到只会警告，然后你的 `target_include_directories(${PCL_INCLUDE_DIRS})` 就是空的 —— 后面报一堆 `C1083`，比直接报"找不到 PCL"难查十倍。
5. **`COMPONENTS` 的含义**：只要这些模块，不必拉全部。

### 本项目需要的组件

| 组件 | 为什么需要 | 用在哪个文件 |
|---|---|---|
| `common` | `pcl::PointXYZ`、`PointCloud` 本身 | 所有文件 |
| `io` | `loadPCDFile` / `savePCDFileBinary` | `src\core\cloud_io.cpp` |
| `filters` | VoxelGrid、SOR、ROR、PassThrough | `src\preprocessing\*.cpp` |
| `features` | （预留，`pcl::compute3DCentroid` 在 common 里） | — |
| `search` | KD-Tree、欧氏聚类用的搜索结构 | `src\search\kdtree_demo.cpp`、`euclidean_cluster.cpp` |
| `kdtree` | `pcl::KdTreeFLANN` | 同上 |
| `segmentation` | RANSAC（`SACSegmentation`）、`EuclideanClusterExtraction` | `src\segmentation\*.cpp` |
| `visualization` | `PCLVisualizer`（练习/将来用） | 目前没有菜单项用它，但留着方便扩展 |

**少写一个组件会怎样**：对应的 `.h` 虽然能被 include（头文件在 `include\` 里，全都有），
但链接时会缺对应的 `.lib`，报 `LNK2019`。比如去掉 `segmentation`，
`ransac_plane.cpp` 里的 `pcl::SACSegmentation` 就链接不上。

### `PCL_DEFINITIONS` 是干什么的

```cmake
add_definitions(${PCL_DEFINITIONS})
```

它传递的是编译宏，比如：
- `-DBOOST_ALL_NO_LIB`（告诉 Boost 不要自动去链接，避免重复链接）
- PCL 内部的一些开关

**不写会怎样**：通常能编过，但可能在某些机器上撞 Boost 的自动链接机制，报奇怪的 `LNK2005`。

---

## 九、本项目 CMakeLists 里那些"必须写"的特殊行

完整的逐行解释在 [04_CMakeLists逐行解释.md](04_CMakeLists逐行解释.md)，这里只挑三条**跟这台机器特定环境有关**的：

### 1. `set(CMAKE_VS_GLOBALS "VCPkgLocalAppDataDisabled=true;VcpkgEnabled=false")`

**为什么要它**：这台机器的 `C:\Users\m1445\AppData\Local\vcpkg\vcpkg.user.targets` 存在，
MSBuild 会自动 Import 它，把 `E:\TOOL\vcpkg\installed\x64-windows\include` 塞进所有 VS 工程的 include 路径。
那里有**另一套 PCL**，和 `E:\PointCloudEnv\PCL` 的版本、编译选项都不同。
两套混着用，就会出现：

```
E:\TOOL\vcpkg\installed\x64-windows\include\pcl\memory.h(69,1):
error C1189: #error: "Potential runtime error due to aligned malloc mismatch! ..."
```

这两行 `CMAKE_VS_GLOBALS` 会写进 `.vcxproj` 的 `<Globals>` 节，
让 `vcpkg.user.targets` 里的 `Condition` 判断失败，从而不导入。

**删掉会怎样**：编译阶段报 `C1189`（本项目实测过）。

### 2. `add_compile_definitions("PCP_PROJECT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")`

**为什么**：程序需要知道"项目根目录在哪"，才能找到 `data\scene_original.pcd`。

如果不写，`src\core\cloud_io.cpp` 里的 `projectDir()` 会退化成"从当前工作目录往上找 `CMakeLists.txt`"。
那样**只有从项目根目录启动 exe 才对**；如果你在 VS 里按 F5，工作目录是 `build\Release\`，
往上找也能找到（因为 `build` 在项目根下），但一旦你把 exe 拷到别处就废了。

编译时写死路径，是**开发阶段的方便做法**。发布时必须改（否则换台机器就找不到路径）。
这一点在 `src\core\cloud_io.cpp` 的 `projectDir()` 注释里有说明。

### 3. `POST_BUILD` 拷 DLL

```cmake
add_custom_command(TARGET PointCloudPractice POST_BUILD ...)
```

**为什么**：`$<TARGET_RUNTIME_DLLS:PointCloudPractice>` 是 CMake 3.21+ 的功能，
它会自动把"这个 exe 链接的所有 DLL"（在已知的导入库旁边）拷到 exe 旁边。

再加上一段手动推导 `pcl_*.dll` 名称的循环（因为 PCL 的 `PCL_LIBRARIES` 是 `.lib` 全路径，
DLL 名要靠规则推：把 `.lib` → `.dll`，Debug 配置再补 `-gd`）。

**不写会怎样**：exe 能编出来，但一运行就退出，退出码 `-1073741515`。

**注意**：即使写了这个，`OpenNI2.dll` 和部分 VTK 插件仍可能漏，
所以"跑之前先 `. E:\PointCloudEnv\env.ps1`"仍是最稳妥的做法。**这不算失败，这是 Windows 上搞 C++ 的常态。**

---

## 十、PowerShell 实操：从零到跑起来

以下命令在本机**实际执行过**。

### 全新构建

```powershell
cd E:\dianyunxulian
. E:\PointCloudEnv\env.ps1
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
.\build\Release\PointCloudPractice.exe
```

### 改了 `.cpp` 之后

```powershell
cmake --build build --config Release
```

### 改了 `CMakeLists.txt` 之后

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

### 彻底重来（最有效的排障第一招）

```powershell
Remove-Item -Recurse -Force E:\dianyunxulian\build
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

### 看看 configure 找到了什么

```powershell
Select-String -Path E:\dianyunxulian\build\CMakeCache.txt -Pattern "^PCL_"
```

预期能看到 `PCL_DIR:PATH=E:/PointCloudEnv/PCL/cmake`、`PCL_ROOT_DIR:PATH=E:/PointCloudEnv/PCL` 等。

### 换生成器（用 VS2022）

```powershell
Remove-Item -Recurse -Force build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

**必须删 build**，因为 `CMakeCache.txt` 里记着旧的生成器，不删会报：

```
CMake Error: Error: generator : Visual Studio 17 2022
Does not match the generator used previously: Visual Studio 18 2026
```

---

## 十一、四个阶段的失败，症状完全不同

| 阶段 | 命令 | 典型报错 | 说明什么 |
|---|---|---|---|
| **configure** | `cmake -S . -B build ...` | `CMake Error at CMakeLists.txt:22` | `CMakeLists.txt` 语法错、路径错、找不到包 |
| 同上 | 同上 | `Could not find a package configuration file provided by "PCL"` | `PCL_DIR` 不对，或 `PCLConfig.cmake` 不在那 |
| **compile** | `cmake --build ...` | `error C1083`（找不到头文件） | include 路径缺 |
| 同上 | 同上 | `error C2065` / `C3861`（未声明的标识符） | 忘了 `#include`，或拼错名字 |
| 同上 | 同上 | `error C2664`（参数类型不匹配） | 函数签名对不上 |
| **link** | 同上 | `error LNK2019`（无法解析的外部符号） | 有声明没实现 / `.cpp` 没加进构建 / 命名空间不一致 |
| 同上 | 同上 | `error LNK2005`（重复定义） | 同一个函数定义了两遍 |
| **run** | `.\build\Release\...exe` | 无报错，直接退出，`-1073741515` | 缺 DLL。先 `. env.ps1` |
| 同上 | 同上 | 输出中文乱码 | 控制台没设 UTF-8（本项目 `setupConsole()` 已处理） |

**记住这个顺序：configure → compile → link → run**。
报错在哪一段，就只在该段找原因，别乱试。

---

## 十二、这一份学完，去做这个

现在去做 [02_项目目录与多文件C++.md](02_项目目录与多文件C++.md) 第九节的**练习 3**（测头文件改动引发的重编时间）。

那个实验会让你亲眼看到"改一个 `.h` 要重编 8 个文件"，
比读十遍"头文件改动代价大"都有用。

然后进 [04_CMakeLists逐行解释.md](04_CMakeLists逐行解释.md)，
把本项目那份 `CMakeLists.txt` 的每一行真正读懂。
