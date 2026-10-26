// =============================================================================
// src/menu/algo_runner.h
//
// 菜单层和算法层之间的"中间人"。
//
// 为什么需要它：
//   如果每个菜单项都自己写一遍"读原始点云 -> 判空 -> 跑算法 -> 打印 -> 提示保存"，
//   7 个算法就是 7 份几乎一样的代码，改一处要改七处。
//   把它们收在一个函数里，菜单项就只剩"收集参数"这一件事。
//
// ★ 这行代码是"每个算法都独立读原始数据"这条需求的唯一实现位置 ★
//   （对应 docs/05 里讲的"不要串联算法"）
// =============================================================================
#pragma once

#include "core/algo_result.h"

#include <functional>
#include <string>

namespace pcp
{

// 算法函数的样子：给它输入点云，它返回 AlgoResult。
// 用 std::function 是为了让菜单层能用 lambda 打包"参数"。
using AlgoFunc = std::function<AlgoResult(const CloudPtr &)>;

// 读取 data/scene_original.pcd，执行 algo，打印结果摘要。
// 读取失败会打印明确提示并返回 false。
bool runOnOriginalCloud(const std::string &title, const AlgoFunc &algo);

// 打印 AlgoResult 的摘要、指标和输出文件路径。
void printAlgoResult(const AlgoResult &result);

}  // namespace pcp
