// =============================================================================
// src/segmentation/euclidean_cluster.h
//
// 欧氏聚类（Euclidean Cluster Extraction）。
//
// 一句话：把"彼此距离小于 tolerance 的点"连成一片，连不上的就是另一片。
// 它不关心物体是什么形状，只关心"它们挨不挨着"。
//
// 这在焊接场景里非常实用：一个场景里有两段管子 + 一块平板 + 一堆飞点，
// 跑一次聚类就能把"每个独立物体"分出来（飞点因为孤立，会被 minSize 过滤掉）。
//
// 文档：docs/13_欧氏聚类.md
// =============================================================================
#pragma once

#include "core/algo_result.h"

namespace pcp
{

struct ClusterOptions
{
    // 聚类容差(mm)。两个点距离小于它，就算"连通"。
    //
    // 这个参数必须结合"点间距"来定，不能凭感觉：
    //   太小（小于点间距）-> 连原本同属一个表面的点都连不起来，一个大管子被切成几百块
    //   太大（大于物体间距）-> 两个物体被连成一片，聚类失效
    // 经验：取 2~5 倍的平均点间距。本场景点间距约 1.5mm，所以 3~7mm 比较合理。
    double tolerance = 3.0;

    // 小于这个点数的簇直接丢掉（通常是小飞点团、噪声碎片）。
    int minSize = 50;

    // 大于这个点数的簇丢掉。设成很大就等于不限。
    // 真实用途：过滤掉"背景"（比如整面墙），只要工件。
    int maxSize = 1000000;
};

AlgoResult runEuclideanCluster(const CloudPtr &input, const ClusterOptions &opt);

}  // namespace pcp
