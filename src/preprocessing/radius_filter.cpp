// =============================================================================
// src/preprocessing/radius_filter.cpp
// =============================================================================
#include "preprocessing/radius_filter.h"

#include "core/cloud_io.h"
#include "core/console.h"

#include <pcl/filters/radius_outlier_removal.h>

#include <iomanip>
#include <sstream>

namespace pcp
{

AlgoResult runRadiusOutlierRemoval(const CloudPtr &input, const RadiusOptions &opt)
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
    if (opt.minNeighbors < 1)
    {
        result.summary = "minNeighbors 太小，未执行";
        printError("最少邻居数至少为 1");
        return result;
    }

    pcl::RadiusOutlierRemoval<PointT> ror;
    ror.setInputCloud(input);
    ror.setRadiusSearch(opt.radius);
    ror.setMinNeighborsInRadius(opt.minNeighbors);
    ror.setNegative(opt.negative);

    CloudPtr output(new CloudT);
    ror.filter(*output);

    const double n_in  = static_cast<double>(input->size());
    const double n_out = static_cast<double>(output->size());

    result.cloud = output;
    result.metrics["搜索半径(mm)"]   = opt.radius;
    result.metrics["最少邻居数"]     = static_cast<double>(opt.minNeighbors);
    result.metrics["输入点数"]       = n_in;
    result.metrics["输出点数"]       = n_out;
    result.metrics["删除点数"]       = n_in - n_out;
    result.metrics["保留比例"]       = (n_in > 0.0) ? n_out / n_in : 0.0;

    std::ostringstream tag;
    tag << "r" << std::fixed << std::setprecision(2) << opt.radius
        << "_n" << opt.minNeighbors;
    result.output_path = saveOutput(opt.negative ? "ror_negative" : "ror", tag.str(), output);

    if (result.output_path.empty())
    {
        printError("结果保存失败");
    }

    result.summary = opt.negative ? "半径滤波完成（已反选：输出的是被判为离群的点）"
                                  : "半径滤波完成";
    return result;
}

}  // namespace pcp
