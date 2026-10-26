// =============================================================================
// src/data/inspect_cloud.h
//
// "看一眼这份点云到底是什么"：点数、坐标范围、重心、密度、前几个点。
//
// 这个功能看起来简单，但它是后面所有调试的基础：
//   每次算法输出异常时，第一件事都是把输入和输出都 describe 一遍，
//   看是"点数不对""坐标范围不对"还是"密度不对"。
// =============================================================================
#pragma once

#include "core/types.h"

#include <string>

namespace pcp
{

// 打印点云的完整基本情况：点数、宽高、坐标范围、重心、包围盒、点间距
void describeCloud(CloudConstPtr cloud, const std::string &title);

// 估算点间距（点云的平均最近邻距离，单位 mm）。
// 它是很多算法参数的"标尺"：
//   体素尺寸一般取 1~3 倍点间距；聚类容差一般取 2~5 倍点间距。
// 为避免大点云太慢，只随机抽 sampleCount 个点来算。
double estimateMeanSpacing(CloudConstPtr cloud, int sampleCount = 2000);

// 打印前 n 个点的坐标（用于理解"索引 i 对应哪个点"）
void printFirstPoints(CloudConstPtr cloud, int n);

}  // namespace pcp
