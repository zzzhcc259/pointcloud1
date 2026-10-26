// =============================================================================
// src/preprocessing/statistical_filter.h
//
// 统计离群点滤波（StatisticalOutlierRemoval，简称 SOR）的接口。
// 文档：docs/09_统计滤波与半径滤波.md
// =============================================================================
#pragma once

#include "core/algo_result.h"

namespace pcp
{

struct StatisticalOptions
{
    // 每个点要看多少个邻居，用它来计算"这个点到邻居的平均距离"。
    // 经验和点密度有关：一般是几十。太小（比如 3）噪声估计不稳，太大（比如 200）把细节也抹平。
    int meanK = 20;

    // 判定阈值，单位是"标准差倍数"。
    //
    // 算法做的事：
    //   1. 对每个点，求它到最近 meanK 个点的平均距离 d_i
    //   2. 对全体 d_i 求均值 μ 和标准差 σ
    //   3. d_i > μ + k·σ 的点判定为离群点，删掉
    //
    // k 越小删得越狠。1.0 会把约 16% 的点删掉（正态分布尾部），2.0 约 2.3%，3.0 约 0.1%。
    double stdDevMulThresh = 1.0;

    // 删掉离群点(false) 还是反过来只保留离群点(true)。
    // "只保留离群点"这个功能在调试时很有用：你能直接看到"被判为噪声的到底是哪些点"。
    bool negative = false;
};

AlgoResult runStatisticalOutlierRemoval(const CloudPtr &input, const StatisticalOptions &opt);

}  // namespace pcp
