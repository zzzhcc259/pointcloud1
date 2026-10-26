// =============================================================================
// src/menu/menu_items.cpp
//
// 菜单里每一项到底做什么。
//
// ★ 这个文件是"加算法"唯一需要动的地方 ★
//   加一个算法模块后，在这里：
//     1. 写一个 actionXxx() 函数（收集参数 -> 交给 runOnOriginalCloud）
//     2. 在 buildMenu() 里 push_back 一行
//   编号会自动排上，不需要手工维护。
//
// 每个 action 都是这个套路：
//   printHeader(标题)
//   用 readDouble/readInt/readLine 问参数        <- 只有这一步是各算法独有的
//   runOnOriginalCloud(标题, lambda)            <- 读原始数据 + 跑 + 打印 + 保存
// =============================================================================
#include "menu/menu_registry.h"

#include "core/cloud_io.h"
#include "core/console.h"
#include "data/generate_scene.h"
#include "data/inspect_cloud.h"
#include "data/scene_presets.h"
#include "menu/algo_runner.h"
#include "preprocessing/radius_filter.h"
#include "preprocessing/statistical_filter.h"
#include "preprocessing/voxel_filter.h"
#include "search/kdtree_demo.h"
#include "segmentation/euclidean_cluster.h"
#include "segmentation/passthrough_demo.h"
#include "segmentation/ransac_plane.h"

#include <iostream>
#include <string>

namespace pcp
{
namespace
{

// ---------------------------------------------------------------------------
// 数据：生成场景
// ---------------------------------------------------------------------------
void actionGenerateScene()
{
    printHeader("生成合成场景");

    const auto &presets = scenePresets();
    std::cout << "\n  可选场景：\n";
    for (size_t i = 0; i < presets.size(); ++i)
    {
        std::cout << "\n    " << (i + 1) << ". " << presets[i].title << "\n";
        std::cout << "       " << presets[i].description << "\n";
    }
    std::cout << "\n";

    const int choice = readInt("选择场景编号", 1);
    if (choice < 1 || choice > static_cast<int>(presets.size()))
    {
        printError("编号超出范围");
        return;
    }

    const ScenePreset &preset = presets[static_cast<size_t>(choice - 1)];

    SceneStats stats;
    std::string err;
    if (!generateAndSave(preset, stats, err))
    {
        printError(err);
        return;
    }

    std::cout << "\n";
    printKV("已生成场景", preset.title);
    printSceneStats(stats);

    printSection("写出的文件");
    printKV("场景文件", toProjectRelative(dataDir() + "/" + presetFileName(preset)));
    printKV("算法默认读取", toProjectRelative(originalScenePath()));
    printInfo("若 scene_original.pcd 原本已存在，旧文件已改名为 scene_original.pcd.bak");
}

// ---------------------------------------------------------------------------
// 数据：检查原始点云
// ---------------------------------------------------------------------------
void actionInspectOriginal()
{
    printHeader("检查原始点云");

    std::string err;
    CloudPtr cloud = loadCloud(originalScenePath(), err);
    if (!cloud)
    {
        printError(err);
        printInfo("请先执行菜单 1 生成场景");
        return;
    }

    printKV("文件", toProjectRelative(originalScenePath()));
    describeCloud(cloud, "原始点云内容");
    printFirstPoints(cloud, 5);
}

// ---------------------------------------------------------------------------
// 下采样
// ---------------------------------------------------------------------------
void actionVoxelGrid()
{
    VoxelOptions opt;
    opt.leaf = readDouble("体素边长 mm（本场景平均点间距约 1.5mm，建议 1~4）", 2.0);

    runOnOriginalCloud("体素降采样 VoxelGrid", [opt](const CloudPtr &c)
                       { return runVoxelGrid(c, opt); });
}

// ---------------------------------------------------------------------------
// 滤波
// ---------------------------------------------------------------------------
void actionStatisticalFilter()
{
    StatisticalOptions opt;
    opt.meanK = readInt("邻居数 meanK（建议 10~50）", 20);
    opt.stdDevMulThresh = readDouble("标准差倍数阈值（越小删得越狠，建议 1.0~3.0）", 1.0);
    opt.negative = readYesNo("是否反选（只输出被判为噪声的点）", false);

    runOnOriginalCloud("统计离群点滤波 SOR", [opt](const CloudPtr &c)
                       { return runStatisticalOutlierRemoval(c, opt); });
}

void actionRadiusFilter()
{
    RadiusOptions opt;
    opt.radius = readDouble("搜索半径 mm（建议 2~4）", 3.0);
    opt.minNeighbors = readInt("半径内最少邻居数（建议 3~10）", 6);
    opt.negative = readYesNo("是否反选（只输出被判为噪声的点）", false);

    runOnOriginalCloud("半径离群点滤波 ROR", [opt](const CloudPtr &c)
                       { return runRadiusOutlierRemoval(c, opt); });
}

// ---------------------------------------------------------------------------
// 空间搜索
// ---------------------------------------------------------------------------
void actionKnnSearch()
{
    std::cout << "\n  提示：可以先跑菜单 2 看原始点云的坐标范围，再挑一个查询点\n";
    KdtreeOptions opt;
    opt.qx = static_cast<float>(readDouble("查询点 X (mm)", 0.0));
    opt.qy = static_cast<float>(readDouble("查询点 Y (mm)", 50.0));
    opt.qz = static_cast<float>(readDouble("查询点 Z (mm)", 0.0));
    opt.k = readInt("要查几个最近邻 k", 8);

    runOnOriginalCloud("KD-Tree K 近邻搜索", [opt](const CloudPtr &c)
                       { return runKdtreeKnn(c, opt); });
}

void actionRadiusSearch()
{
    std::cout << "\n  提示：可以先跑菜单 2 看原始点云的坐标范围，再挑一个查询点\n";
    KdtreeRadiusOptions opt;
    opt.qx = static_cast<float>(readDouble("查询点 X (mm)", 0.0));
    opt.qy = static_cast<float>(readDouble("查询点 Y (mm)", 50.0));
    opt.qz = static_cast<float>(readDouble("查询点 Z (mm)", 0.0));
    opt.radius = readDouble("搜索半径 mm", 5.0);
    opt.maxResults = readInt("最多显示多少个", 50);

    runOnOriginalCloud("KD-Tree 半径搜索", [opt](const CloudPtr &c)
                       { return runKdtreeRadiusSearch(c, opt); });
}

// ---------------------------------------------------------------------------
// 裁剪
// ---------------------------------------------------------------------------
void actionPassThrough()
{
    PassThroughOptions opt;

    const std::string field = readLine("按哪个坐标裁？(x / y / z)", "z");
    opt.field = field.empty() ? 'z' : field[0];

    opt.minLimit = readDouble("下限（保留 >= 这个值）", -20.0);
    opt.maxLimit = readDouble("上限（保留 <= 这个值）", 20.0);
    opt.negative = readYesNo("是否反选（只保留范围外的点）", false);

    runOnOriginalCloud("PassThrough 坐标裁剪", [opt](const CloudPtr &c)
                       { return runPassThrough(c, opt); });
}

// ---------------------------------------------------------------------------
// 分割
// ---------------------------------------------------------------------------
void actionRansacPlane()
{
    printInfo("本场景里平板只占 14% 的点，迭代次数太少可能抽不到它（会先找到管壁上的一条近似平面）");

    RansacPlaneOptions opt;
    opt.distanceThreshold = readDouble("距离阈值 mm（建议 1~3 倍点间距，本场景约 1.5mm）", 1.0);
    opt.maxIterations = readInt("最大迭代次数", 1000);
    opt.probability = readDouble("期望成功率 probability (0~1)", 0.99);
    opt.optimizeCoefficients = readYesNo("找到平面后再最小二乘精修一次", true);

    runOnOriginalCloud("RANSAC 平面分割", [opt](const CloudPtr &c)
                       { return runRansacPlane(c, opt); });
}

void actionEuclideanCluster()
{
    ClusterOptions opt;
    opt.tolerance = readDouble("聚类容差 mm（建议 2~5 倍点间距，本场景约 1.5mm）", 3.0);
    opt.minSize = readInt("最小簇点数（小于它的丢掉）", 50);
    opt.maxSize = readInt("最大簇点数（大于它的丢掉）", 1000000);

    runOnOriginalCloud("欧氏聚类", [opt](const CloudPtr &c)
                       { return runEuclideanCluster(c, opt); });
}

// ---------------------------------------------------------------------------
// 工具：看看 output 里有什么
// ---------------------------------------------------------------------------
void actionListOutputs()
{
    printHeader("output 目录里的结果文件");

    const auto files = listOutputFiles();
    if (files.empty())
    {
        printInfo("output/ 里还没有 .pcd 文件");
        printInfo("先跑一个算法（例如菜单 3 体素降采样）就会生成结果");
        return;
    }

    printKV("文件数", static_cast<int>(files.size()));
    std::cout << "\n";
    for (size_t i = 0; i < files.size(); ++i)
    {
        std::cout << "    " << (i + 1) << ". " << files[i] << "\n";
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// 菜单表：加功能就在这里加一行
// ---------------------------------------------------------------------------
std::vector<MenuItem> buildMenu()
{
    std::vector<MenuItem> menu;

    // ---- 数据 ----
    menu.push_back({"数据", "生成合成场景（选择预设）", actionGenerateScene});
    menu.push_back({"数据", "检查原始点云 scene_original.pcd", actionInspectOriginal});

    // ---- 下采样 ----
    menu.push_back({"下采样", "体素降采样 VoxelGrid", actionVoxelGrid});

    // ---- 滤波 ----
    menu.push_back({"滤波", "统计离群点滤波 SOR", actionStatisticalFilter});
    menu.push_back({"滤波", "半径离群点滤波 ROR", actionRadiusFilter});

    // ---- 空间搜索 ----
    menu.push_back({"空间搜索（不删点）", "K 近邻搜索 KNN", actionKnnSearch});
    menu.push_back({"空间搜索（不删点）", "半径搜索", actionRadiusSearch});

    // ---- 裁剪 ----
    menu.push_back({"裁剪", "PassThrough 按坐标轴裁掉范围外的点", actionPassThrough});

    // ---- 分割 ----
    menu.push_back({"分割", "RANSAC 平面分割", actionRansacPlane});
    menu.push_back({"分割", "欧氏聚类（按距离分成几块）", actionEuclideanCluster});

    // ---- 工具 ----
    menu.push_back({"工具", "查看 output 目录里已有的结果", actionListOutputs});

    return menu;
}

}  // namespace pcp
