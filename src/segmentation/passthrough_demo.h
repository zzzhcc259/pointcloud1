// =============================================================================
// src/segmentation/passthrough_demo.h
//
// PassThrough：按某个坐标轴的范围裁剪点云。
//
// 也许你会觉得这算法"太简单了，算什么算法"——
// 但在真实管道扫描里，它是最常用的第一步：
//   "我只关心焊缝附近 ±20mm 的那一圈点，其他全扔掉"，
//   这一句需求就是用 PassThrough 实现的，而且它比任何高级算法都可靠（因为它没有假设）。
//
// 文档：docs/11_PassThrough坐标裁剪.md
// =============================================================================
#pragma once

#include "core/algo_result.h"

namespace pcp
{

struct PassThroughOptions
{
    // 按哪个字段裁。'x' / 'y' / 'z'。
    // 注意这不是"绕哪个轴"，而是"看哪个坐标值"。
    char field = 'z';

    // 保留 [minLimit, maxLimit] 区间内的点（两端都算，闭区间）。
    double minLimit = -1000.0;
    double maxLimit = 1000.0;

    // 反选：只保留区间外的点。
    // 真实用途：先把"确定的焊缝区"抠出来，再用反选拿到"其余部分"做对比。
    bool negative = false;
};

AlgoResult runPassThrough(const CloudPtr &input, const PassThroughOptions &opt);

}  // namespace pcp
