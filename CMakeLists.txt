# =============================================================================
# PointCloudPractice —— 石油管道环焊缝点云处理练习台
#
# 这份文件是 CMake 的"构建说明书"：告诉 CMake 用什么编译器、要哪些依赖、
# 有哪些源文件、链接哪些库、编译成什么。
# 逐行解释见 docs/04_CMakeLists逐行解释.md
# =============================================================================

# --- 1. CMake 最低版本 -------------------------------------------------------
# 3.20 才支持 target_sources() 与 $<TARGET_RUNTIME_DLLS> 的完整行为。
cmake_minimum_required(VERSION 3.20)

# --- 2. 项目声明 -------------------------------------------------------------
# LANGUAGES CXX 表示只要 C++ 编译器，不找 C/Fortran，配置更快。
project(PointCloudPractice LANGUAGES CXX)

# --- 3. C++ 语言标准 ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)          # 用 C++17：std::filesystem、std::optional 等
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 编译器不支持 17 就直接报错，不要悄悄降级
set(CMAKE_CXX_EXTENSIONS OFF)       # 只用标准语法，不用 MSVC 的方言扩展

# --- 4. 关掉用户级 vcpkg 集成（本机必须的补丁）-------------------------------
# 本机存在 C:\Users\<你>\AppData\Local\vcpkg\vcpkg.user.targets，
# MSBuild 会无条件 Import 它，把 E:\TOOL\vcpkg\installed\x64-windows\include
# 塞进头文件搜索路径 —— 那里有另一套 PCL，与 E:\PointCloudEnv\PCL 混用会报
#     error C1189: #error: "Potential runtime error due to aligned malloc mismatch!"
# 这条 set() 会往生成的 .vcxproj 里写 Globals 属性，使那个 Import 条件不成立。
# 详细原因见 docs/03_CMake入门.md 的"为什么必须禁用 vcpkg 集成"。
set(CMAKE_VS_GLOBALS "VCPkgLocalAppDataDisabled=true;VcpkgEnabled=false")

# --- 5. 找到 PCL ------------------------------------------------------------
# 只显式指定 PCL_DIR；Boost / Eigen3 / FLANN / Qhull / VTK 由 PCLConfig.cmake
# 根据 ${PCL_ROOT}/3rdParty 自动判定（这是 PCL AllInOne 包的约定）。
set(PCL_ROOT_DIR "E:/PointCloudEnv/PCL" CACHE PATH "PCL 安装根目录")
set(PCL_DIR "${PCL_ROOT_DIR}/cmake" CACHE PATH "包含 PCLConfig.cmake 的目录")

# pcl_io.dll 在"加载期"就依赖 OpenNI2.dll（哪怕代码完全不碰 OpenNI）。
# AllInOne 的子安装器把 OpenNI2 装在 C:\Program Files\OpenNI2，不在 PCL 目录树下。
set(OPENNI2_REDIST_DIR "C:/Program Files/OpenNI2/Redist" CACHE PATH "包含 OpenNI2.dll 的目录")

# COMPONENTS 列出真正用到的模块。用不到的模块不要写，否则会带进多余的链接依赖。
find_package(PCL 1.15 REQUIRED COMPONENTS
    common       # 基础类型、几何工具、坐标变换
    io           # PCD/PLY 读写
    filters      # VoxelGrid / SOR / ROR / PassThrough / ConditionalRemoval / MLS
    features     # 法向估计、曲率
    search       # KdTree / 半径搜索
    kdtree
    segmentation # RANSAC(SACSegmentation) / 欧氏聚类
    visualization # PCLVisualizer 可视化
)

message(STATUS "找到 PCL 版本: ${PCL_VERSION}")
message(STATUS "PCL 头文件目录: ${PCL_INCLUDE_DIRS}")

# --- 6. 主程序 --------------------------------------------------------------
# 第一个参数是"目标名"(target)，同时也是生成的可执行文件名。
# 这里先不写源文件，下面用 target_sources() 分批加入，便于讲清"加文件=加一行"。
add_executable(PointCloudPractice)

target_sources(PointCloudPractice PRIVATE
    # ---- 公共层 ----
    src/core/console.cpp
    src/core/cloud_io.cpp
    # ---- 数据层 ----
    src/data/scene_presets.cpp
    src/data/generate_scene.cpp
    src/data/inspect_cloud.cpp
    # ---- 菜单层 ----
    src/menu/menu_registry.cpp
    src/menu/menu_items.cpp
    src/menu/algo_runner.cpp
    # ---- 预处理 ----
    src/preprocessing/voxel_filter.cpp
    src/preprocessing/statistical_filter.cpp
    src/preprocessing/radius_filter.cpp
    # ---- 空间搜索 ----
    src/search/kdtree_demo.cpp
    # ---- 裁剪与分割 ----
    src/segmentation/passthrough_demo.cpp
    src/segmentation/ransac_plane.cpp
    src/segmentation/euclidean_cluster.cpp
    # ---- 入口 ----
    src/main.cpp
)

# 头文件搜索路径。
#   src            -> 这样代码里可以写 #include "core/types.h" 而不必写相对路径
#   ${PCL_INCLUDE_DIRS} -> pcl/point_types.h 等
target_include_directories(PointCloudPractice PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PCL_INCLUDE_DIRS}
)

# 库文件搜索路径（.lib 在哪里）
target_link_directories(PointCloudPractice PRIVATE ${PCL_LIBRARY_DIRS})

# 要链接的库（pcl_common.lib、pcl_io.lib、vtk*.lib ...）
target_link_libraries(PointCloudPractice PRIVATE ${PCL_LIBRARIES})

# PCL 需要的一些编译宏（如 BOOST_* 相关）。这是"目录级"命令，
# 写在 add_subdirectory() 之前，所以下面的 practice 子目录也会继承。
add_definitions(${PCL_DEFINITIONS})

# --- 7. 把项目路径编译进程序 ------------------------------------------------
# 好处：程序运行时不需要猜"当前工作目录"，即使在 VS 里按 F5 也能找对 data/。
# 代价：移动项目目录后必须重新运行 cmake 配置（见 docs/03 的说明）。
add_compile_definitions("PCP_PROJECT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}\"")

# --- 8. MSVC 专用编译选项 ---------------------------------------------------
if(MSVC)
    target_compile_options(PointCloudPractice PRIVATE
        /utf-8    # 源码里的中文按 UTF-8 解释，否则字符串会乱码
        /bigobj   # 模板实例化过多时单个 .obj 会超出对象文件限制
        /wd4996   # 关掉"已弃用函数"(PCL 内部大量使用) 的警告
        /wd4819   # 关掉"文件包含非 ANSI 字符"的警告
    )
    target_compile_definitions(PointCloudPractice PRIVATE
        NOMINMAX                 # 阻止 <windows.h> 定义 min/max 宏，否则 std::min 会崩
        _USE_MATH_DEFINES        # 让 <cmath> 提供 M_PI
        _CRT_SECURE_NO_WARNINGS  # fopen/sprintf 等的安全警告静音
        BOOST_ALL_NO_LIB         # 不让 Boost 自动用 #pragma 指定库名
    )
endif()

# --- 9. 运行期 DLL 收集（Windows）------------------------------------------
# 编译链接成功不等于能运行：Windows 需要把 .dll 放到 .exe 旁边或放进 PATH。
# 这里在构建完成后自动把需要的 DLL 拷到输出目录，省掉每次手动设置 PATH。
#
# 两个坑：
#   坑 1  $<TARGET_RUNTIME_DLLS> 只对 imported target 给出正确的 .dll（VTK::* 属于这类）。
#         对以裸路径链接的 PCL 模块，它会把 .lib 路径原样吐出来，
#         所以 PCL 的 .dll 必须自己从 PCL_LIBRARIES 映射到 bin 目录。
#   坑 2  pcl_io.dll 加载期依赖 OpenNI2.dll，缺了进程直接以 0xC0000135 退出，毫无提示。
if(WIN32)
    set(_pcl_stems "")
    foreach(_entry IN LISTS PCL_LIBRARIES)
        get_filename_component(_name "${_entry}" NAME_WE)
        if(_name MATCHES "^pcl_")
            # debug 变体带尾缀 d（pcl_iod / pcl_commond），剥掉后才是基名
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

    set(_pcl_dlls "")
    set(_pcl_lib_junk "")
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
        list(APPEND _pcl_lib_junk
             "$<IF:$<CONFIG:Debug>,$<TARGET_FILE_DIR:PointCloudPractice>/${_stem}d.lib,$<TARGET_FILE_DIR:PointCloudPractice>/${_stem}.lib>")
    endforeach()

    if(NOT _pcl_dlls)
        message(WARNING "无法从 PCL_LIBRARIES 解析出 PCL 模块名；运行时请手动把 ${PCL_ROOT_DIR}/bin 加入 PATH")
    endif()

    add_custom_command(TARGET PointCloudPractice POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:PointCloudPractice> "$<TARGET_FILE_DIR:PointCloudPractice>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_pcl_dlls} "$<TARGET_FILE_DIR:PointCloudPractice>"
        COMMAND_EXPAND_LISTS
        VERBATIM)

    if(EXISTS "${OPENNI2_REDIST_DIR}/OpenNI2.dll")
        add_custom_command(TARGET PointCloudPractice POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${OPENNI2_REDIST_DIR}/OpenNI2.dll" "$<TARGET_FILE_DIR:PointCloudPractice>"
            VERBATIM)
    else()
        message(WARNING "未找到 OpenNI2.dll（${OPENNI2_REDIST_DIR}）；请把该目录加入 PATH，否则 pcl_io 加载失败")
    endif()

    # 清掉 TARGET_RUNTIME_DLLS 顺带拷进来的 PCL .lib（那是垃圾文件）
    add_custom_command(TARGET PointCloudPractice POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -f ${_pcl_lib_junk}
        COMMAND_EXPAND_LISTS
        VERBATIM)
endif()

# --- 10. 练习区（独立编译）-------------------------------------------------
# 练习代码写崩了，只会让 exercises.exe 编译失败，不会影响主程序。
option(BUILD_PRACTICE_EXERCISES "同时编译练习区 exercises.exe" ON)
if(BUILD_PRACTICE_EXERCISES)
    add_subdirectory(practice)
endif()
