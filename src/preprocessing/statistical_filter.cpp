// =============================================================================
// src/preprocessing/statistical_filter.cpp
// =============================================================================
#include "preprocessing/statistical_filter.h"

#include "core/cloud_io.h"
#include "core/console.h"

#include <pcl/filters/statistical_outlier_removal.h>

#include <iomanip>
#include <sstream>

namespace pcp
{

AlgoResult runStatisticalOutlierRemoval(const CloudPtr &input, const StatisticalOptions &opt)
{
    AlgoResult result;

    if (!input || input->empty())
    {
        result.summary = "输入点云为空，未执行";
        printError("输入点云为空");
        return result;
    }
    if (opt.meanK < 2)
    {
        result.summary = "meanK 太小，未执行";
        printError("meanK 至少要 2（要能算出'到邻居的距离'）");
        return result;
    }
    if (opt.stdDevMulThresh <= 0.0)
    {
        result.summary = "阈值必须为正，未执行";
        printError("倍数阈值必须大于 0");
        return result;
    }

    pcl::StatisticalOutlierRemoval<PointT> sor;
    sor.setInputCloud(input);
    sor.setMeanK(opt.meanK);
    sor.setStddevMulThresh(opt.stdDevMulThresh);
    sor.setNegative(opt.negative);

    CloudPtr output(new CloudT);
    sor.filter(*output);

    const double n_in  = static_cast<double>(input->size());
    const double n_out = static_cast<double>(output->size());

    result.cloud = output;
    result.metrics["meanK"]          = static_cast<double>(opt.meanK);
    result.metrics["倍数阈值"]       = opt.stdDevMulThresh;
    result.metrics["输入点数"]       = n_in;
    result.metrics["输出点数"]       = n_out;
    result.metrics["删除点数"]       = n_in - n_out;
    result.metrics["保留比例"]       = (n_in > 0.0) ? n_out / n_in : 0.0;

    std::ostringstream tag;
    tag << "k" << opt.meanK << "_std" << std::fixed << std::setprecision(2)
        << opt.stdDevMulThresh;
    result.output_path = saveOutput(opt.negative ? "sor_negative" : "sor", tag.str(), output);

    if (result.output_path.empty())
    {
        printError("结果保存失败");
    }

    result.summary = opt.negative ? "统计滤波完成（已反选：输出的是被判为噪声的点）"
                                  : "统计滤波完成";
    return result;
}

}  // namespace pcp
