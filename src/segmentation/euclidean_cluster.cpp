// =============================================================================
// src/segmentation/euclidean_cluster.cpp
// =============================================================================
#include "segmentation/euclidean_cluster.h"

#include "core/cloud_io.h"
#include "core/console.h"

#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

namespace pcp
{

AlgoResult runEuclideanCluster(const CloudPtr &input, const ClusterOptions &opt)
{
    AlgoResult result;

    if (!input || input->empty())
    {
        result.summary = "输入点云为空，未执行";
        printError("输入点云为空");
        return result;
    }
    if (opt.tolerance <= 0.0)
    {
        result.summary = "容差必须为正，未执行";
        printError("聚类容差必须大于 0");
        return result;
    }
    if (opt.minSize < 1 || opt.minSize > opt.maxSize)
    {
        result.summary = "簇大小范围不合法，未执行";
        printError("要求 1 <= minSize <= maxSize");
        return result;
    }

    // ---- 1. 聚类本身需要一个"邻域搜索结构" ---------------------------------
    // 聚类要反复问"这个点周围 tolerance 以内有谁"，所以内部依赖 KD-Tree。
    // 这里显式地建一棵，交给 extractEuclideanClusters 用。
    pcl::search::KdTree<PointT>::Ptr tree(new pcl::search::KdTree<PointT>);
    tree->setInputCloud(input);

    // ---- 2. 执行聚类 --------------------------------------------------------
    // 输出是 vector<PointIndices>，每个元素是一簇点的下标。
    std::vector<pcl::PointIndices> clusterIndices;
    pcl::EuclideanClusterExtraction<PointT> ec;
    ec.setClusterTolerance(opt.tolerance);
    ec.setMinClusterSize(opt.minSize);
    ec.setMaxClusterSize(opt.maxSize);
    ec.setSearchMethod(tree);
    ec.setInputCloud(input);
    ec.extract(clusterIndices);

    if (clusterIndices.empty())
    {
        result.summary = "没有找到任何簇";
        printError("没有找到任何簇。常见原因：容差太小（每点都孤立）、minSize 太大");
        printInfo("试试先把容差调到 ~3 倍点间距，minSize 调到 10");
        return result;
    }

    // ---- 3. 按点数从大到小排序 ----------------------------------------------
    // 默认顺序是按"第一个被扫到的点"排的，没什么意义。
    // 按大小排序后，最大的几簇一眼就能看到（通常就是管道和平板）。
    std::sort(clusterIndices.begin(), clusterIndices.end(),
              [](const pcl::PointIndices &lhs, const pcl::PointIndices &rhs)
              { return lhs.indices.size() > rhs.indices.size(); });

    // ---- 4. 打印每一簇 ------------------------------------------------------
    printSection("聚类结果");
    std::cout << "  共 " << clusterIndices.size() << " 簇（按点数从大到小）\n\n";
    std::cout << "    " << padRight("序号", 8)
              << padRight("点数", 12)
              << "重心 (x, y, z)\n";

    const size_t showCount = std::min<size_t>(clusterIndices.size(), 20);
    for (size_t i = 0; i < showCount; ++i)
    {
        const auto &indices = clusterIndices[i].indices;

        // 手算重心：把这一簇所有点的 xyz 分别加起来除以个数。
        // （PCL 的 compute3DCentroid 也是这么算的，这里写出来是为了让你看见它。）
        double sx = 0.0, sy = 0.0, sz = 0.0;
        for (const int idx : indices)
        {
            const auto &p = input->points[static_cast<size_t>(idx)];
            sx += p.x;
            sy += p.y;
            sz += p.z;
        }
        const double n = static_cast<double>(indices.size());
        const double cx = sx / n, cy = sy / n, cz = sz / n;

        std::ostringstream centroid;
        centroid << "(" << std::fixed << std::setprecision(2)
                 << cx << ", " << cy << ", " << cz << ")";

        std::cout << "    " << padRight(std::to_string(i + 1), 8)
                  << padRight(std::to_string(indices.size()), 12)
                  << centroid.str() << "\n";
    }
    if (clusterIndices.size() > showCount)
    {
        std::cout << "    ... 还有 " << (clusterIndices.size() - showCount) << " 簇未显示\n";
    }

    // ---- 5. 拼出"最大簇"和"其余点" -----------------------------------------
    const auto &largest = clusterIndices.front().indices;

    CloudPtr largestCloud(new CloudT);
    largestCloud->reserve(largest.size());
    for (const int idx : largest)
    {
        largestCloud->push_back(input->points[static_cast<size_t>(idx)]);
    }
    largestCloud->width  = static_cast<uint32_t>(largestCloud->size());
    largestCloud->height = 1;

    // 其余点 = 所有点 - 最大簇
    std::vector<bool> inLargest(input->size(), false);
    for (const int idx : largest)
    {
        inLargest[static_cast<size_t>(idx)] = true;
    }
    CloudPtr rest(new CloudT);
    rest->reserve(input->size() - largest.size());
    for (size_t i = 0; i < input->size(); ++i)
    {
        if (!inLargest[i])
        {
            rest->push_back(input->points[i]);
        }
    }
    rest->width  = static_cast<uint32_t>(rest->size());
    rest->height = 1;

    // ---- 6. 把簇重心单独存一份 ----------------------------------------------
    // 这一招很实用：一万个点变成十来个重心点，你一眼就能看出"物体在哪、有几个"。
    CloudPtr centroids(new CloudT);
    centroids->reserve(clusterIndices.size());
    for (const auto &cluster : clusterIndices)
    {
        const auto &indices = cluster.indices;
        double sx = 0.0, sy = 0.0, sz = 0.0;
        for (const int idx : indices)
        {
            const auto &p = input->points[static_cast<size_t>(idx)];
            sx += p.x;
            sy += p.y;
            sz += p.z;
        }
        const double n = static_cast<double>(indices.size());
        centroids->push_back(PointT(static_cast<float>(sx / n),
                                    static_cast<float>(sy / n),
                                    static_cast<float>(sz / n)));
    }
    centroids->width  = static_cast<uint32_t>(centroids->size());
    centroids->height = 1;

    // ---- 7. 指标与保存 ------------------------------------------------------
    result.cloud = largestCloud;
    result.rest  = rest;
    result.metrics["聚类容差(mm)"] = opt.tolerance;
    result.metrics["minSize"]      = static_cast<double>(opt.minSize);
    result.metrics["簇数"]         = static_cast<double>(clusterIndices.size());
    result.metrics["最大簇点数"]   = static_cast<double>(largest.size());
    result.metrics["最大簇占比"]   =
        (input->size() > 0) ? static_cast<double>(largest.size()) / static_cast<double>(input->size()) : 0.0;

    std::ostringstream tag;
    tag << "tol" << std::fixed << std::setprecision(2) << opt.tolerance
        << "_min" << opt.minSize;

    result.output_path = saveOutput("cluster_largest", tag.str(), largestCloud);
    const std::string centroidPath = saveOutput("cluster_centroids", tag.str(), centroids);
    if (!centroidPath.empty())
    {
        printKV("簇重心已保存", centroidPath);
    }

    result.summary = "欧氏聚类完成";
    return result;
}

}  // namespace pcp
