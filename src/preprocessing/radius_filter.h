// =============================================================================
// src/preprocessing/radius_filter.h
//
// 半径离群点滤波（RadiusOutlierRemoval，简称 ROR）的接口。
// 文档：docs/09_统计滤波与半径滤波.md
// =============================================================================
#pragma once

#include "core/algo_result.h"

namespace pcp
{

struct RadiusOptions
{
    // 搜索半径，单位 mm。
    // 这个半径内的点数少于 minNeighbors 的点会被删掉。
    double radius = 3.0;

    // 半径内至少要有几个邻居才算"正常点"。
    // 1 是"至少要有一个邻居"（孤立点才被删），
    // 6 就是"要成小片"才留下。
    int minNeighbors = 6;

    // 是否反选（只输出被判为离群的点），调试用。
    bool negative = false;
};

// 注意 SOR 和 ROR 的差别：
//   SOR 用"平均距离"这种统计量，阈值是全自动算出来的（μ+kσ）；
//   ROR 用固定的"半径 + 最少邻居数"，阈值要你手工给。
//   所以 SOR 好上手，ROR 更可控 —— 密度不均匀时 ROR 反而更好使。
AlgoResult runRadiusOutlierRemoval(const CloudPtr &input, const RadiusOptions &opt);

}  // namespace pcp
