// =============================================================================
// src/segmentation/passthrough_demo.cpp
// =============================================================================
#include "segmentation/passthrough_demo.h"

#include "core/cloud_io.h"
#include "core/console.h"

#include <pcl/filters/passthrough.h>

#include <iomanip>
#include <sstream>

namespace pcp
{

AlgoResult runPassThrough(const CloudPtr &input, const PassThroughOptions &opt)
{
    AlgoResult result;

    if (!input || input->empty())
    {
        result.summary = "输入点云为空，未执行";
        printError("输入点云为空");
        return result;
    }
    if (opt.minLimit > opt.maxLimit)
    {
        result.summary = "范围上下限写反了，未执行";
        printError("下限大于上限，请检查");
        return result;
    }

    // 字段名必须是 PCL 认识的字符串："x" / "y" / "z"
    std::string fieldName;
    switch (opt.field)
    {
    case 'x': case 'X': fieldName = "x"; break;
    case 'y': case 'Y': fieldName = "y"; break;
    case 'z': case 'Z': fieldName = "z"; break;
    default:
        result.summary = "字段名不合法，未执行";
        printError("字段只能是 x / y / z");
        return result;
    }

    pcl::PassThrough<PointT> pass;
    pass.setInputCloud(input);
    pass.setFilterFieldName(fieldName);
    pass.setFilterLimits(static_cast<float>(opt.minLimit), static_cast<float>(opt.maxLimit));
    pass.setNegative(opt.negative);

    CloudPtr output(new CloudT);
    pass.filter(*output);

    const double n_in  = static_cast<double>(input->size());
    const double n_out = static_cast<double>(output->size());

    result.cloud = output;
    result.metrics["下限(mm)"]  = opt.minLimit;
    result.metrics["上限(mm)"]  = opt.maxLimit;
    result.metrics["输入点数"]  = n_in;
    result.metrics["输出点数"]  = n_out;
    result.metrics["保留比例"]  = (n_in > 0.0) ? n_out / n_in : 0.0;

    std::ostringstream tag;
    tag << fieldName << "_" << std::fixed << std::setprecision(1)
        << opt.minLimit << "_to_" << opt.maxLimit;
    result.output_path = saveOutput(opt.negative ? "passthrough_negative" : "passthrough",
                                    tag.str(), output);

    if (result.output_path.empty())
    {
        printError("结果保存失败");
    }

    result.summary = "坐标裁剪完成（按 " + fieldName + " 轴，保留 " +
                     std::to_string(static_cast<long long>(opt.minLimit)) + " ~ " +
                     std::to_string(static_cast<long long>(opt.maxLimit)) + " mm）";
    return result;
}

}  // namespace pcp
