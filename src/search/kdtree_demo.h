// =============================================================================
// src/search/kdtree_demo.h
//
// KD-Tree 邻域搜索的演示。
//
// ★ 这里有个特别容易被误解的地方：KD-Tree 不是滤波算法。★
//   滤波 = 删点/改点（输出的点云点数会变）
//   搜索 = 只是查"谁离我近"，一个点都不删
//   KD-Tree 是一种"空间索引数据结构"，作用是把"找最近邻"从 O(n) 降到约 O(log n)。
//   所以它属于 pcl/search 模块，不属于 pcl/filters。
//
// 文档：docs/10_KDTree邻域搜索.md
// =============================================================================
#pragma once

#include "core/algo_result.h"

namespace pcp
{

struct KdtreeOptions
{
    // 查询点。默认用原点 (0,0,0)，菜单里会让你改。
    float qx = 0.0f;
    float qy = 0.0f;
    float qz = 0.0f;

    // 要查询几个邻居
    int k = 8;
};

struct KdtreeRadiusOptions
{
    float qx = 0.0f;
    float qy = 0.0f;
    float qz = 0.0f;

    // 半径搜索：查"半径 r 以内"的所有点（个数不定）
    // KNN 和半径搜索的区别就在这：KNN 你能确定个数但不知道范围，半径搜索你能确定范围但不知道个数。
    double radius = 5.0;

    // 最多返回多少个（防止半径太大时返回几十万个点把控制台刷爆）
    int maxResults = 50;
};

// K 近邻搜索。返回的 result.cloud 里装的是"被找到的邻居点"，方便你存下来用工具看。
AlgoResult runKdtreeKnn(const CloudPtr &input, const KdtreeOptions &opt);

// 半径搜索。
AlgoResult runKdtreeRadiusSearch(const CloudPtr &input, const KdtreeRadiusOptions &opt);

}  // namespace pcp
