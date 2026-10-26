// =============================================================================
// src/segmentation/ransac_plane.h
//
// RANSAC 平面分割。
//
// RANSAC 干的事，一句话：
//   "反复随机抽 3 个点，拿它们定一个平面，看看全云有多少点正好贴在这个平面上；
//    抽很多次，留下'贴上去的点最多'的那个平面。"
// 它不要求数据干净，反而**天生能容忍大量离群点**——因为离群点凑不出一个平面。
// 这正是它在焊接点云里用得最多的原因：一个场景里又有点云又有飞点又有别的工件。
//
// 文档：docs/12_RANSAC平面分割.md
// =============================================================================
#pragma once

#include "core/algo_result.h"

namespace pcp
{

struct RansacPlaneOptions
{
    // 距离阈值(mm)。点到平面的垂直距离小于它，就算"内点"。
    // 这是最关键的参数：它大致应该等于"你能接受的表面厚度"，通常取 1~3 倍点间距。
    //   太小 -> 只有极少数点算内点，平面断成碎片
    //   太大 -> 把旁边的曲面、管壁也吸进来，平面"变胖"
    double distanceThreshold = 1.0;

    // 最多随机尝试多少次。
    // 内点比例越低，需要的次数越多（指数级）。1000 次对一般场景够用。
    // 设很大也不会慢多少，因为 PCL 内部会提前终止（见 probability）。
    int maxIterations = 1000;

    // 期望"至少成功找到一次正确模型"的概率，0~1。
    // 当迭代次数已经足够时，PCL 会提前 break —— 这个参数就是提前终止的判据。
    double probability = 0.99;

    // 找到平面后，用平面内所有点做最小二乘重新拟合一次，让平面更准。
    // 建议开着（这是 PCL 推荐的用法），代价很小。
    bool optimizeCoefficients = true;
};

AlgoResult runRansacPlane(const CloudPtr &input, const RansacPlaneOptions &opt);

}  // namespace pcp
