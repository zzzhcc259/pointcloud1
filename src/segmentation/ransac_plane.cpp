// =============================================================================
// src/segmentation/ransac_plane.cpp
// =============================================================================
#include "segmentation/ransac_plane.h"

#include "core/cloud_io.h"
#include "core/console.h"

#include <pcl/ModelCoefficients.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>

#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

namespace pcp
{

AlgoResult runRansacPlane(const CloudPtr &input, const RansacPlaneOptions &opt)
{
    AlgoResult result;

    if (!input || input->empty())
    {
        result.summary = "输入点云为空，未执行";
        printError("输入点云为空");
        return result;
    }
    if (opt.distanceThreshold <= 0.0)
    {
        result.summary = "距离阈值必须为正，未执行";
        printError("距离阈值必须大于 0");
        return result;
    }
    if (opt.maxIterations < 10)
    {
        printWarn("迭代次数太少（<10），找到正确平面的概率很低");
    }

    // ---- 1. 准备 ------------------------------------------------------------
    // ModelCoefficients 用来装"模型参数"。对平面来说就是 4 个数：a b c d
    //   平面方程：a·x + b·y + c·z + d = 0
    //   (a,b,c) 是法向量（已经归一化，长度=1），d 是原点到平面的有符号距离的相反数。
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);

    // indices 装"属于这个平面的点"在输入点云里的下标
    pcl::PointIndices::Ptr inliers(new pcl::PointIndices);

    pcl::SACSegmentation<PointT> seg;
    seg.setOptimizeCoefficients(opt.optimizeCoefficients);

    // 模型类型：平面
    seg.setModelType(pcl::SACMODEL_PLANE);
    // 采样方法：RANSAC
    // 同一个模型可以配不同的采样方法，例如 SACMODEL_PLANE + SAC_RANSAC 是最常用的组合。
    // 也可以用 SAC_RRANSAC（随机化 RANSAC）、SAC_LMEDS（最小中值，抗噪更强但更慢）。
    seg.setMethodType(pcl::SAC_RANSAC);

    seg.setDistanceThreshold(static_cast<float>(opt.distanceThreshold));
    seg.setMaxIterations(opt.maxIterations);
    seg.setProbability(opt.probability);

    // ---- 2. 执行分割 --------------------------------------------------------
    seg.setInputCloud(input);
    seg.segment(*inliers, *coefficients);

    if (inliers->indices.empty())
    {
        // 这是真实会遇到的情况：点太少、点全共线、阈值太小。
        result.summary = "没有找到任何平面";
        printError("RANSAC 没有找到平面内点。常见原因：点太少、点几乎共线、距离阈值过小");
        printInfo("试试把距离阈值调大，或换一个场景（例如 1 管道+平板）");
        return result;
    }

    // ---- 3. 拆出内点和剩余点 ------------------------------------------------
    // 内点：平面上的点。剩余点 = 总数 - 内点。
    // PCL 提供了 ExtractIndices 来按下标抠点，这里为了让你看清"这就是按下标挑点"，
    // 用最直白的方式手写一遍（ExtractIndices 的效果完全一样）。
    CloudPtr plane(new CloudT);
    CloudPtr rest(new CloudT);
    plane->reserve(inliers->indices.size());
    rest->reserve(input->size() - inliers->indices.size());

    std::vector<bool> isInlier(input->size(), false);
    for (const int idx : inliers->indices)
    {
        isInlier[static_cast<size_t>(idx)] = true;
    }
    for (size_t i = 0; i < input->size(); ++i)
    {
        if (isInlier[i])
        {
            plane->push_back(input->points[i]);
        }
        else
        {
            rest->push_back(input->points[i]);
        }
    }
    plane->width  = static_cast<uint32_t>(plane->size());
    plane->height = 1;
    rest->width   = static_cast<uint32_t>(rest->size());
    rest->height  = 1;

    // ---- 4. 打印平面方程 ----------------------------------------------------
    // 把系数翻译成"人话"，这是判断结果是否合理最有效的办法。
    const auto &c = coefficients->values;
    const double a = c[0], b = c[1], cc = c[2], d = c[3];

    printSection("平面模型");
    std::cout << "  方程 : " << std::fixed << std::setprecision(6)
              << a << "·x + " << b << "·y + " << cc << "·z + " << d << " = 0\n";
    std::cout << "  法向量 : (" << std::fixed << std::setprecision(4)
              << a << ", " << b << ", " << cc << ")"
              << "   长度 = " << std::sqrt(a * a + b * b + cc * cc) << "\n";
    std::cout << "  原点到平面的距离 : " << std::fixed << std::setprecision(3)
              << std::abs(d) / std::sqrt(a * a + b * b + cc * cc) << " mm\n";

    // 如果这个场景是"管道 + 平板"，那么平板是水平的(y=常数)，
    // 法向量应该接近 (0, ±1, 0)。法向量指向哪个方向取决于点的排列，正负都可能。
    if (std::abs(std::abs(b) - 1.0) < 0.1)
    {
        printInfo("法向量接近 ±Y 轴 —— 说明找到的是水平平板，符合预期");
    }
    else
    {
        printInfo("法向量不是 Y 轴方向 —— 可能找到的并不是平板（例如被管壁上的近似平面吸走了）");
    }

    // ---- 5. 指标与保存 ------------------------------------------------------
    const double n_in  = static_cast<double>(input->size());
    const double n_inlier = static_cast<double>(plane->size());

    result.cloud = plane;
    result.rest  = rest;
    result.metrics["距离阈值(mm)"] = opt.distanceThreshold;
    result.metrics["迭代上限"]     = static_cast<double>(opt.maxIterations);
    result.metrics["输入点数"]     = n_in;
    result.metrics["平面内点数"]   = n_inlier;
    result.metrics["剩余点数"]     = static_cast<double>(rest->size());
    result.metrics["内点比例"]     = (n_in > 0.0) ? n_inlier / n_in : 0.0;
    result.metrics["法向X"]        = a;
    result.metrics["法向Y"]        = b;
    result.metrics["法向Z"]        = cc;

    std::ostringstream tag;
    tag << "th" << std::fixed << std::setprecision(2) << opt.distanceThreshold
        << "_iter" << opt.maxIterations;

    result.output_path = saveOutput("ransac_plane_plane", tag.str(), plane);
    const std::string restPath = saveOutput("ransac_plane_rest", tag.str(), rest);

    if (!restPath.empty())
    {
        printKV("剩余点已保存", restPath);
    }

    result.summary = "RANSAC 平面分割完成";
    return result;
}

}  // namespace pcp
