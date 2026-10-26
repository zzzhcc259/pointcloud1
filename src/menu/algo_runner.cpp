// =============================================================================
// src/menu/algo_runner.cpp
// =============================================================================
#include "menu/algo_runner.h"

#include "core/cloud_io.h"
#include "core/console.h"

#include <cmath>
#include <iomanip>
#include <iostream>

namespace pcp
{

bool runOnOriginalCloud(const std::string &title, const AlgoFunc &algo)
{
    printHeader(title);

    // ---- 1. 每次都重新读原始文件 --------------------------------------------
    // 注意：这里没有"接上一个算法的输出"这种东西。
    // 每次都是从磁盘上重新读 data/scene_original.pcd，
    // 所以你在菜单里连跑 5 个算法，它们互相之间没有任何影响。
    //
    // 为什么非要这样：如果算法 A 的结果被 B 悄悄吃掉了，
    // 你看 B 的结果时就分不清"这是 B 的功劳还是 A 的功劳"，
    // 调参也就无从下手。串联是"任务演示"才做的事，而且必须显式写出来。
    const std::string path = originalScenePath();
    printKV("输入文件", toProjectRelative(path));

    std::string err;
    CloudPtr cloud = loadCloud(path, err);
    if (!cloud)
    {
        printError(err);
        printInfo("请先执行菜单 1「生成合成场景」");
        return false;
    }

    printKV("输入点数", static_cast<int>(cloud->size()));

    // ---- 2. 执行算法 --------------------------------------------------------
    AlgoResult result = algo(cloud);

    // ---- 3. 打印 ------------------------------------------------------------
    printAlgoResult(result);
    return true;
}

void printAlgoResult(const AlgoResult &result)
{
    if (!result.summary.empty())
    {
        std::cout << "\n  " << result.summary << "\n";
    }

    // metrics 是一个 std::map<string,double>，所以这里按"键名字典序"打印。
    // 顺序不是我们指定的，这是用 map 换"新增指标不用改结构体"所付出的代价。
    if (!result.metrics.empty())
    {
        printSection("关键指标");
        for (const auto &kv : result.metrics)
        {
            // 点云点数在下面单独打印（输入点数在调用处已打印），这里跳过免得重复两遍
            if (kv.first == "输入点数" || kv.first == "输出点数" || kv.first == "剩余点数")
            {
                continue;
            }
            const double v = kv.second;
            // 整数就按整数打印（1000 而不是 1000.000），小数保留 4 位
            if (std::abs(v - std::round(v)) < 1e-9 && std::abs(v) < 1e15)
            {
                printKV(kv.first, std::to_string(std::llround(v)));
            }
            else
            {
                printKV(kv.first, v, 4);
            }
        }
    }

    if (result.cloud)
    {
        printKV("输出点数", static_cast<int>(result.cloud->size()));
    }

    if (!result.output_path.empty())
    {
        printKV("已保存", result.output_path);
    }
    else if (result.cloud)
    {
        printWarn("结果没有保存到 output/");
    }

    if (result.rest)
    {
        printKV("剩余点数", static_cast<int>(result.rest->size()));
    }
}

}  // namespace pcp
