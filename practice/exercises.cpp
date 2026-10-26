// =============================================================================
// practice/exercises.cpp
//
// ★ 练习区。这里可以随便改坏，主程序 PointCloudPractice 不受影响。★
//
// 编译位置：build\practice\<Debug|Release>\exercises.exe
// 运行方式：先 . E:\PointCloudEnv\env.ps1，再运行 exercises.exe
//
// 练习清单见 docs/14_项目练习与故障排查.md。
// 每个练习下面都有一个"待你实现"的函数；实现完把 main 里对应的开关改成 true
// 就能看到效果。参考答案在主项目对应模块里（注释里写了去哪找）。
//
// 提醒：练习区故意不做任何保护（不检查文件是否存在、不捕获异常），
//      因为踩到崩溃、自己看懂栈、再修好，才是这个练习区存在的意义。
// =============================================================================

#include "core/cloud_io.h"
#include "core/console.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

#include <iostream>

// -----------------------------------------------------------------------------
// 练习 1：手写包围盒
// -----------------------------------------------------------------------------
// 目标：不调用 pcl::getMinMax3D，自己遍历所有点求 x/y/z 的最小值、最大值。
//
// 参考答案：src/data/inspect_cloud.cpp 里 describeCloud() 用 pcl::getMinMax3D 的那一段
//
// 做法提示：
//   1. 把 min 初始化为一个"最大的数"（例如 1e30），max 初始化成 -1e30
//   2. for (const auto& p : cloud->points) { min[0] = std::min(min[0], p.x); ... }
//   3. 注意 NaN 点：pcl::isFinite(p) 为 false 时要跳过
//
// 自检判据：拿 data/scene_original.pcd 跑，结果必须和菜单 2「检查原始点云」
//          打印的 X/Y/Z 范围**完全一致**（差 0.000000）。
void exercise1_boundingBox(const pcp::CloudPtr& cloud)
{
    float minX = 0, minY = 0, minZ = 0;
    float maxX = 0, maxY = 0, maxZ = 0;

    // TODO(练习1): 在这里写你的循环

    std::cout << "  包围盒 X: [" << minX << ", " << maxX << "]\n";
    std::cout << "  包围盒 Y: [" << minY << ", " << maxY << "]\n";
    std::cout << "  包围盒 Z: [" << minZ << ", " << maxZ << "]\n";
}

// -----------------------------------------------------------------------------
// 练习 2：手写按 z 过滤（等价于 PassThrough 的 filter_field_name="z"）
// -----------------------------------------------------------------------------
// 目标：把 900 < z < 1100 之外的点全部丢掉，返回新点云。
//
// 参考答案：主项目里的 src/.../passthrough_demo.cpp（等它写完再看）
// 自检判据：输出点数必须和 PCL 的 PassThrough + setFilterLimits(900,1100) 一致。
void exercise2_filterByZ(const pcp::CloudPtr& cloud)
{
    (void)cloud;
    // TODO(练习2): 新建一个 CloudPtr 结果，遍历 cloud，把 z 在范围内的点 push_back 进去，
    //              最后打印 "保留 N 点 / 共 M 点"
    std::cout << "  练习 2 还没实现\n";
}

// -----------------------------------------------------------------------------
// 练习 3：手写体素内点数统计
// -----------------------------------------------------------------------------
// 目标：给定边长 leaf（默认 1.0），统计"点最多的体素里有多少个点"和"一共有多少个非空体素"。
//       这是理解 VoxelGrid 的第一步——VoxelGrid 干的事就是把这个统计结果取平均。
//
// 提示：体素编号 = (floor(x/leaf), floor(y/leaf), floor(z/leaf))
//       用 std::map<std::tuple<int,int,int>, int> 计数最简单
void exercise3_voxelCount(const pcp::CloudPtr& cloud)
{
    (void)cloud;
    // TODO(练习3)
    std::cout << "  练习 3 还没实现\n";
}

// -----------------------------------------------------------------------------
// 练习 4：暴力最近邻（把每个点到查询点的距离全算一遍）
// -----------------------------------------------------------------------------
// 目标：自己写 O(n) 的暴力搜索，找出离查询点最近的那个点的下标。
//       然后和 KdTree 的结果对照 —— 这就是"为什么需要 KD 树"的直观体验：
//       结果一样，但你的程序是 O(n)，KD 树平均 O(log n)。
//
// 参考答案：src/search/kdtree_demo.cpp（等它写完）
void exercise4_bruteForceKnn(const pcp::CloudPtr& cloud)
{
    (void)cloud;
    // TODO(练习4)
    std::cout << "  练习 4 还没实现\n";
}

// -----------------------------------------------------------------------------
// 练习 5：导出 CSV
// -----------------------------------------------------------------------------
// 目标：把点云写成 "x,y,z" 每行一个点的 CSV，用 Excel 能打开。
//       这一步的意义：你以后写 Python 脚本画图、算误差，都靠它把 C++ 的结果搬出去。
void exercise5_exportCsv(const pcp::CloudPtr& cloud)
{
    (void)cloud;
    // TODO(练习5)
    std::cout << "  练习 5 还没实现\n";
}

// =============================================================================
int main()
{
    pcp::setupConsole();

    std::cout << "==============================================\n";
    std::cout << " 练习区（practice/exercises.cpp）\n";
    std::cout << "==============================================\n\n";

    const std::string path = pcp::originalScenePath();
    std::cout << "读取: " << pcp::toProjectRelative(path) << "\n";

    std::string err;
    pcp::CloudPtr cloud = pcp::loadCloud(path, err);
    if (!cloud)
    {
        std::cout << "\n  读取失败: " << err << "\n";
        std::cout << "  请先运行主程序 PointCloudPractice，用菜单 1 生成场景\n";
        return 1;
    }

    std::cout << "  点数: " << cloud->size() << "\n\n";

    // ---- 想跑哪个练习就把哪一行取消注释，一次只开一个，出错好定位 ----
    exercise1_boundingBox(cloud);
    // exercise2_filterByZ(cloud);
    // exercise3_voxelCount(cloud);
    // exercise4_bruteForceKnn(cloud);
    // exercise5_exportCsv(cloud);

    std::cout << "\n完成。\n";
    return 0;
}
