// =============================================================================
// src/search/kdtree_demo.cpp
// =============================================================================
#include "search/kdtree_demo.h"

#include "core/cloud_io.h"
#include "core/console.h"

#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/search/kdtree.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace pcp
{

AlgoResult runKdtreeKnn(const CloudPtr &input, const KdtreeOptions &opt)
{
    AlgoResult result;

    if (!input || input->empty())
    {
        result.summary = "输入点云为空，未执行";
        printError("输入点云为空");
        return result;
    }
    if (opt.k < 1)
    {
        result.summary = "k 太小，未执行";
        printError("k 至少为 1");
        return result;
    }
    if (opt.k > static_cast<int>(input->size()))
    {
        printWarn("k 比总点数还大，会自动缩小到总点数");
    }

    const PointT query(opt.qx, opt.qy, opt.qz);

    // ---- 建树 --------------------------------------------------------------
    // 建树这一步本身是有代价的（要把所有点按空间分块排好）。
    // 所以：只查一次的时候，KD-Tree 未必比暴力快；
    //       要查成千上万次的时候，一次建树 + 多次查询就非常划算。
    pcl::KdTreeFLANN<PointT> kdtree;
    kdtree.setInputCloud(input);

    // ---- 查询 --------------------------------------------------------------
    // pointIdxNKNSearch : 邻居的下标（用来去 input 里取回这个点的 xyz）
    // pointNKNSquaredDistance : 到查询点的"距离的平方"（不是距离！）
    //   为什么不直接返回距离：平方不用开根号，快；比较大小的时候平方和距离等价。
    //   要得到真实距离，得自己 std::sqrt()。
    std::vector<int>   indices;
    std::vector<float> sq_distances;

    const int k = std::min<int>(opt.k, static_cast<int>(input->size()));
    const int found = kdtree.nearestKSearch(query, k, indices, sq_distances);

    if (found <= 0)
    {
        result.summary = "没有找到任何邻居";
        printWarn("没有找到邻居，可能是点云里全是 NaN");
        return result;
    }

    // ---- 把邻居点收集成一个点云，方便存下来看 ------------------------------
    CloudPtr neighbors(new CloudT);
    neighbors->reserve(static_cast<size_t>(found));

    std::cout << "\n  查询点: (" << query.x << ", " << query.y << ", " << query.z << ")\n";
    std::cout << "  找到 " << found << " 个最近邻：\n\n";
    std::cout << "    " << padRight("序号", 8)
              << padRight("点坐标 (x, y, z)", 30)
              << padRight("平方距离", 14)
              << "真实距离\n";

    for (int i = 0; i < found; ++i)
    {
        const PointT &p = input->points[static_cast<size_t>(indices[static_cast<size_t>(i)])];
        neighbors->push_back(p);

        std::ostringstream coord;
        coord << "(" << std::fixed << std::setprecision(2)
              << p.x << ", " << p.y << ", " << p.z << ")";

        // 注意 sqrt：PCL 给的是平方距离
        const double sq = static_cast<double>(sq_distances[static_cast<size_t>(i)]);
        const double dist = std::sqrt(sq);

        std::ostringstream sqStr, distStr;
        sqStr << std::fixed << std::setprecision(4) << sq;
        distStr << std::fixed << std::setprecision(4) << dist;

        std::cout << "    " << padRight(std::to_string(i + 1), 8)
                  << padRight(coord.str(), 30)
                  << padRight(sqStr.str(), 14)
                  << distStr.str() << "\n";
    }

    neighbors->width = static_cast<uint32_t>(neighbors->size());
    neighbors->height = 1;
    neighbors->is_dense = true;

    result.cloud = neighbors;
    result.metrics["查询点X"] = opt.qx;
    result.metrics["查询点Y"] = opt.qy;
    result.metrics["查询点Z"] = opt.qz;
    result.metrics["k"] = static_cast<double>(k);
    result.metrics["找到个数"] = static_cast<double>(found);
    result.metrics["最近点距离(mm)"] = std::sqrt(static_cast<double>(sq_distances[0]));
    result.metrics["最远邻居距离(mm)"] =
        std::sqrt(static_cast<double>(sq_distances[static_cast<size_t>(found - 1)]));

    std::ostringstream tag;
    tag << "k" << k;
    result.output_path = saveOutput("kdtree_knn", tag.str(), neighbors);

    result.summary = "KD-Tree K 近邻搜索完成";
    return result;
}

AlgoResult runKdtreeRadiusSearch(const CloudPtr &input, const KdtreeRadiusOptions &opt)
{
    AlgoResult result;

    if (!input || input->empty())
    {
        result.summary = "输入点云为空，未执行";
        printError("输入点云为空");
        return result;
    }
    if (opt.radius <= 0.0)
    {
        result.summary = "半径必须为正，未执行";
        printError("搜索半径必须大于 0");
        return result;
    }

    const PointT query(opt.qx, opt.qy, opt.qz);

    pcl::KdTreeFLANN<PointT> kdtree;
    kdtree.setInputCloud(input);

    // 半径搜索用 radiusSearch，参数含义和 KNN 一样，只是"给范围不给个数"
    std::vector<int>   indices;
    std::vector<float> sq_distances;

    const double r = opt.radius;
    const int found = kdtree.radiusSearch(query, r, indices, sq_distances);

    CloudPtr neighbors(new CloudT);
    const int shown = std::min<int>(found, opt.maxResults);
    neighbors->reserve(static_cast<size_t>(shown));

    std::cout << "\n  查询点: (" << query.x << ", " << query.y << ", " << query.z << ")\n";
    std::cout << "  半径 " << r << "mm 内共 " << found << " 个点";

    if (found > opt.maxResults)
    {
        std::cout << "（只显示前 " << opt.maxResults << " 个）";
    }
    std::cout << "：\n\n";

    for (int i = 0; i < shown; ++i)
    {
        const PointT &p = input->points[static_cast<size_t>(indices[static_cast<size_t>(i)])];
        neighbors->push_back(p);

        const double dist = std::sqrt(static_cast<double>(sq_distances[static_cast<size_t>(i)]));
        std::cout << "    " << std::left << std::setw(8) << (i + 1)
                  << "(" << std::fixed << std::setprecision(2)
                  << p.x << ", " << p.y << ", " << p.z << ")"
                  << "  距离 " << std::fixed << std::setprecision(4) << dist << "mm\n";
    }

    neighbors->width = static_cast<uint32_t>(neighbors->size());
    neighbors->height = 1;
    neighbors->is_dense = true;

    result.cloud = neighbors;
    result.metrics["查询点X"] = opt.qx;
    result.metrics["查询点Y"] = opt.qy;
    result.metrics["查询点Z"] = opt.qz;
    result.metrics["搜索半径(mm)"] = opt.radius;
    result.metrics["命中点数"] = static_cast<double>(found);

    std::ostringstream tag;
    tag << "r" << std::fixed << std::setprecision(2) << opt.radius;
    result.output_path = saveOutput("kdtree_radius", tag.str(), neighbors);

    result.summary = "KD-Tree 半径搜索完成";
    return result;
}

}  // namespace pcp
