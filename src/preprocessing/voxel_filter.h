// =============================================================================
// src/preprocessing/voxel_filter.h
//
// 体素降采样（VoxelGrid）的接口。
//
// 一个算法模块固定放在一对 .h/.cpp 里：
//   .h 告诉别人"这个模块提供什么"（这里只有 runVoxelGrid 和它的参数结构体）
//   .cpp 里才写"怎么做的"
// 别的文件只需要 #include 这个 .h，永远不需要看 .cpp。
// =============================================================================
#pragma once

#include "core/algo_result.h"

namespace pcp
{

// 体素降采样的参数。
// 单独放一个结构体而不是给函数加一堆参数，是为了：
//   - 以后加参数不用改函数签名（不会连累所有调用处）
//   - 菜单里可以先把参数收集完，再一次性传进去
struct VoxelOptions
{
    // 体素边长，单位与点云相同（本项目是 mm）。
    // 这是这个算法唯一真正重要的参数：整个点云的空间被切成 leaf×leaf×leaf 的小立方体。
    double leaf = 2.0;
};

// 运行体素降采样。
// 参数：input 是输入点云（只读），opt 是参数
// 返回：AlgoResult，其中 cloud 是降采样后的点云，output_path 是保存的文件
AlgoResult runVoxelGrid(const CloudPtr &input, const VoxelOptions &opt);

}  // namespace pcp
