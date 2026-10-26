// =============================================================================
// src/data/inspect_cloud.cpp
// =============================================================================
#include "data/inspect_cloud.h"

#include "core/console.h"

#include <pcl/common/centroid.h>
#include <pcl/common/common.h>
#include <pcl/kdtree/kdtree_flann.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

namespace pcp
{

namespace
{
std::string fmt3(double v)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << v;
    return oss.str();
}
}  // namespace

void describeCloud(CloudConstPtr cloud, const std::string &title)
{
    printSection(title);

    if (!cloud)
    {
        printWarn("点云指针为空");
        return;
    }
    if (cloud->empty())
    {
        printWarn("点云里一个点也没有");
        return;
    }

    // ---- 基本信息 ----
    printKV("点数", static_cast<int>(cloud->size()));
    printKV("width x height",
            std::to_string(cloud->width) + " x " + std::to_string(cloud->height));
    printKV("is_dense（是否含无效点）", cloud->is_dense ? "true" : "false");
    printKV("每个点的内存字节数", static_cast<int>(sizeof(PointT)));

    // ---- 坐标范围 ----
    PointT mn;
    PointT mx;
    pcl::getMinMax3D(*cloud, mn, mx);
    printKV("X 范围 (mm)",
            "[" + fmt3(mn.x) + ", " + fmt3(mx.x) + "]  跨度 " + fmt3(mx.x - mn.x));
    printKV("Y 范围 (mm)",
            "[" + fmt3(mn.y) + ", " + fmt3(mx.y) + "]  跨度 " + fmt3(mx.y - mn.y));
    printKV("Z 范围 (mm)",
            "[" + fmt3(mn.z) + ", " + fmt3(mx.z) + "]  跨度 " + fmt3(mx.z - mn.z));

    // ---- 重心 ----
    Eigen::Vector4f centroid;
    pcl::compute3DCentroid(*cloud, centroid);
    printKV("重心 (mm)",
            "(" + fmt3(centroid[0]) + ", " + fmt3(centroid[1]) + ", " + fmt3(centroid[2]) + ")");

    // ---- 包围盒对角线 ----
    const double dx = static_cast<double>(mx.x - mn.x);
    const double dy = static_cast<double>(mx.y - mn.y);
    const double dz = static_cast<double>(mx.z - mn.z);
    printKV("包围盒对角线 (mm)", fmt3(std::sqrt(dx * dx + dy * dy + dz * dz)));

    // ---- 密度 ----
    const double spacing = estimateMeanSpacing(cloud);
    printKV("平均点间距 (mm)", fmt3(spacing));
    if (spacing > 1e-9)
    {
        printKV("1mm 内大约有几个点", fmt3(1.0 / spacing));
    }
    printInfo("点间距是调参数的标尺：体素取 1~3 倍，聚类容差取 2~5 倍");
}

double estimateMeanSpacing(CloudConstPtr cloud, int sampleCount)
{
    if (!cloud || cloud->size() < 2)
    {
        return 0.0;
    }

    // 用 KD 树查询最近邻。点云很大时逐个查太慢，随机抽样。
    pcl::KdTreeFLANN<PointT> kdtree;
    kdtree.setInputCloud(cloud);

    const int total = static_cast<int>(cloud->size());
    const int n = std::min(std::max(1, sampleCount), total);

    // 随机抽样（固定种子，保证同一个点云每次算出同样的值）
    std::mt19937 rng(12345u);
    std::uniform_int_distribution<int> pick(0, total - 1);

    std::vector<int> idx(2);
    std::vector<float> dist(2);

    double sum = 0.0;
    int used = 0;
    for (int i = 0; i < n; ++i)
    {
        const int query = pick(rng);
        // 查 2 个最近邻：第 0 个是它自己（距离 0），第 1 个才是真正的最近邻
        if (kdtree.nearestKSearch(static_cast<unsigned int>(query), 2, idx, dist) == 2)
        {
            sum += std::sqrt(static_cast<double>(dist[1]));
            ++used;
        }
    }

    return (used > 0) ? (sum / used) : 0.0;
}

void printFirstPoints(CloudConstPtr cloud, int n)
{
    if (!cloud || cloud->empty())
    {
        printWarn("点云为空，无法打印");
        return;
    }

    const int count = std::min(n, static_cast<int>(cloud->size()));
    printSection("前 " + std::to_string(count) + " 个点（索引 : x, y, z）");
    for (int i = 0; i < count; ++i)
    {
        const PointT &pt = (*cloud)[static_cast<size_t>(i)];
        std::ostringstream oss;
        oss << "[" << std::setw(6) << i << "]  "
            << std::fixed << std::setprecision(3)
            << std::setw(10) << pt.x << std::setw(10) << pt.y << std::setw(10) << pt.z;
        std::cout << oss.str() << "\n";
    }
}

}  // namespace pcp
