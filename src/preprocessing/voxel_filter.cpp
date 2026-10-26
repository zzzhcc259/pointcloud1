// =============================================================================
// src/preprocessing/voxel_filter.cpp
//
// 体素降采样的实现。对应文档 docs/08_VoxelGrid体素降采样.md
// =============================================================================
#include "preprocessing/voxel_filter.h"

#include "core/cloud_io.h"
#include "core/console.h"

#include <pcl/filters/voxel_grid.h>

#include <iomanip>
#include <sstream>

namespace pcp
{

AlgoResult runVoxelGrid(const CloudPtr &input, const VoxelOptions &opt)
{
    AlgoResult result;

    // ---- 1. 检查输入 --------------------------------------------------------
    // 初学者最容易漏的一步。指针为空、点云为空、参数为 0，
    // 这三种情况在 PCL 里不一定报错，而是给你一个莫名其妙的结果（或者干脆崩）。
    if (!input || input->empty())
    {
        result.summary = "输入点云为空，未执行";
        printError("输入点云为空");
        return result;
    }
    if (opt.leaf <= 0.0)
    {
        result.summary = "体素边长为 0 或负数，未执行";
        printError("体素边长必须大于 0");
        return result;
    }
    // 体素边长比点间距还小时，降采样几乎是"没有效果"（每个体素最多一个点）。
    // 这里不禁止，只提醒 —— 因为"看它为什么没效果"本身也是一次学习。
    if (opt.leaf < 0.3)
    {
        printWarn("体素边长小于 0.3mm，可能几乎不减少点数（每个体素本来就没几个点）");
    }

    // ---- 2. 组装 PCL 算法对象并设参数 ---------------------------------------
    // VoxelGrid<PointT> 是一个"模板类"，尖括号里写你要处理的点类型。
    // 本项目统一用 PointT = pcl::PointXYZ（只有 xyz 坐标，没有颜色和法向）。
    pcl::VoxelGrid<PointT> voxel;
    voxel.setInputCloud(input);

    const float leaf = static_cast<float>(opt.leaf);
    voxel.setLeafSize(leaf, leaf, leaf);   // 三个方向同一个边长 = 正方体体素

    // ---- 3. 执行 ------------------------------------------------------------
    // 输出必须自己 new 一份。PCL 的 filter(*output) 不会帮你分配内存。
    CloudPtr output(new CloudT);
    voxel.filter(*output);

    // PCL 的 filter() 不会修改 width/height 以外的东西，
    // 但降采样后 is_dense 的含义变了，习惯上重置一下，避免误导后续算法。
    output->is_dense = input->is_dense;

    // ---- 4. 记录指标 --------------------------------------------------------
    const double n_in  = static_cast<double>(input->size());
    const double n_out = static_cast<double>(output->size());

    result.cloud = output;
    result.metrics["体素边长(mm)"]   = opt.leaf;
    result.metrics["输入点数"]       = n_in;
    result.metrics["输出点数"]       = n_out;
    result.metrics["保留比例"]       = (n_in > 0.0) ? n_out / n_in : 0.0;
    result.metrics["平均每体素点数"] = (n_out > 0.0) ? n_in / n_out : 0.0;

    // ---- 5. 保存 ------------------------------------------------------------
    // 文件名带参数，避免"改了个参数再跑一次，上一次的结果被覆盖"。
    std::ostringstream tag;
    tag << "leaf" << std::fixed << std::setprecision(3) << opt.leaf;
    result.output_path = saveOutput("voxel", tag.str(), output);

    if (result.output_path.empty())
    {
        printError("结果保存失败（详见上面的提示）");
    }

    result.summary = "体素降采样完成";
    return result;
}

}  // namespace pcp
