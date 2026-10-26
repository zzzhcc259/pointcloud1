// =============================================================================
// src/data/generate_scene.cpp
//
// 合成点云的几何生成。核心是两个函数式的小块：
//   r_outer(d) : 给定"离端面的轴向距离 d"，返回该处的外轮廓半径
//   pushPoint  : 往点云里塞一个点，并叠加测量噪声
// 其余都是循环。
//
// r_outer(d) 是理解坡口的关键：
//   d = 0                       -> r = 内半径 + 钝边高度   （钝边外缘）
//   0 < d < bevelRun            -> r 沿斜面线性增长
//   d >= bevelRun               -> r = 外半径              （正常管身）
// 所以点云在端部自然形成一个向外张开的斜面，切一个截面就能量出坡口角。
// =============================================================================
#include "data/generate_scene.h"

#include "core/cloud_io.h"
#include "core/console.h"

#include <pcl/common/common.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <random>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace pcp
{

namespace
{
// 往点云里加一个点，三个方向各叠一个独立的高斯噪声
void pushPoint(CloudT &cloud,
               double x, double y, double z,
               double sigma,
               std::mt19937 &rng,
               std::normal_distribution<double> &gauss)
{
    PointT pt;
    pt.x = static_cast<float>(x + sigma * gauss(rng));
    pt.y = static_cast<float>(y + sigma * gauss(rng));
    pt.z = static_cast<float>(z + sigma * gauss(rng));
    cloud.push_back(pt);
}
}  // namespace

CloudPtr generateScene(const SceneParams &p, SceneStats &stats)
{
    stats = SceneStats{};

    auto cloud = std::make_shared<CloudT>();
    cloud->reserve(200000);

    // ---- 几何尺寸 ----
    const double R       = p.pipeRadius;
    const double Ri      = std::max(1.0, R - p.pipeThickness);   // 内半径
    const double halfGap = p.rootGap * 0.5;                      // 端面到中心平面的距离
    // 钝边不能比整个壁厚还大，否则几何自相矛盾
    const double rootFace = std::min({p.rootFace, R - Ri - 1e-3, R - Ri});
    const double angleRad = p.bevelAngleDeg * M_PI / 180.0;
    const double tanA     = std::tan(angleRad);

    // ---- 坡口斜面沿轴向的长度 ----
    // 斜面从 (钝边外缘, r = Ri + rootFace) 一直升到 (管身, r = R)，
    // 高度差是 R - Ri - rootFace，斜率是 tanA，所以轴向长度 = 高度差 / tanA。
    double bevelRun = 0.0;
    if (p.enableGroove && tanA > 1e-6)
    {
        bevelRun = (R - Ri - rootFace) / tanA;
        if (bevelRun < 0.0)
        {
            bevelRun = 0.0;
        }
        // 安全帽：坡口不能把整根管都吃掉
        const double maxRun = p.pipeLength * 0.6;
        if (bevelRun > maxRun)
        {
            bevelRun = maxRun;
        }
    }

    stats.bevelRunLength    = bevelRun;
    stats.trueRootGap       = p.rootGap;
    stats.trueBevelAngleDeg = p.enableGroove ? p.bevelAngleDeg : 0.0;
    stats.trueRootFace      = p.enableGroove ? rootFace : 0.0;
    stats.trueMisalignment  = p.misalignment;

    // ---- 随机数 ----
    // 同一个 seed 永远产生同一串随机数 -> 同一个场景可以复现
    std::mt19937 rng(p.seed);
    std::normal_distribution<double> gauss(0.0, 1.0);
    std::uniform_real_distribution<double> uni(-1.0, 1.0);

    // ---- 轴向采样位置表（两侧管共用）----
    // 坡口区用更密的步长：那里曲率变化快，点稀了就量不准角度。
    std::vector<double> ds;
    {
        const double bevelStep = std::max(0.05, p.bevelStep);
        const double axialStep = std::max(0.1, p.axialStep);
        if (bevelRun > 1e-9)
        {
            for (double d = 0.0; d < bevelRun - 1e-9; d += bevelStep)
            {
                ds.push_back(d);
            }
        }
        for (double d = bevelRun; d <= p.pipeLength + 1e-9; d += axialStep)
        {
            ds.push_back(d);
        }
    }
    if (ds.empty())
    {
        ds.push_back(0.0);
    }

    const int    nTheta      = std::max(8, p.ntheta);
    // 视角覆盖：360° 表示全周；小于 360° 时只保留 |theta| < 半角 的点，模拟单侧扫描
    const double halfViewRad = (p.viewAngleDeg >= 359.9)
                                   ? M_PI
                                   : (p.viewAngleDeg * 0.5 * M_PI / 180.0);
    // 钝边端面在径向采几个点（真实扫描打不到，教学时打开以便看到完整轮廓）
    const int rootFaceSamples = (p.enableGroove && p.sampleRootFace) ? 9 : 0;

    // ---- 两段管 ----
    for (int side = 0; side < 2; ++side)
    {
        const bool   rightSide = (side == 1);
        const double sign      = rightSide ? 1.0 : -1.0;
        // 错边：让右管整体沿 Z 平移，两管中心线就不重合了
        const double zShift    = rightSide ? p.misalignment : 0.0;

        for (int it = 0; it < nTheta; ++it)
        {
            const double theta = -M_PI + 2.0 * M_PI * static_cast<double>(it) / nTheta;
            if (std::abs(theta) > halfViewRad)
            {
                continue;   // 落在盲区里，扫不到
            }
            const double ct = std::cos(theta);
            const double st = std::sin(theta);

            // 外表面（含坡口斜面）
            for (double d : ds)
            {
                double rOuter = R;
                if (bevelRun > 1e-9 && d < bevelRun)
                {
                    rOuter = Ri + rootFace + d * tanA;
                }
                pushPoint(*cloud,
                          sign * (halfGap + d), rOuter * ct, rOuter * st + zShift,
                          p.noiseSigma, rng, gauss);

                // 内壁（可选）
                if (p.sampleInner)
                {
                    pushPoint(*cloud,
                              sign * (halfGap + d), Ri * ct, Ri * st + zShift,
                              p.noiseSigma, rng, gauss);
                }
            }

            // 钝边端面（径向的一小段竖直面）
            for (int ir = 0; ir < rootFaceSamples; ++ir)
            {
                const double t = (rootFaceSamples > 1)
                                     ? static_cast<double>(ir) / (rootFaceSamples - 1)
                                     : 0.0;
                const double r = Ri + rootFace * t;
                pushPoint(*cloud,
                          sign * halfGap, r * ct, r * st + zShift,
                          p.noiseSigma, rng, gauss);
            }
        }
    }
    stats.pipePoints = static_cast<int>(cloud->size());

    // ---- 平板 ----
    if (p.enablePlate)
    {
        const double step = std::max(0.2, p.plateStep);
        const int    nx   = static_cast<int>(p.plateSizeX / step) + 1;
        const int    nz   = static_cast<int>(p.plateSizeZ / step) + 1;
        for (int i = 0; i < nx; ++i)
        {
            const double x = -p.plateSizeX * 0.5 + i * step;
            for (int k = 0; k < nz; ++k)
            {
                const double z = -p.plateSizeZ * 0.5 + k * step;
                // 平面 y = plateCenterY，法向就是 +Y/-Y，RANSAC 平面模型很好拟合
                pushPoint(*cloud, x, p.plateCenterY, z, p.noiseSigma, rng, gauss);
            }
        }
    }
    stats.platePoints = static_cast<int>(cloud->size()) - stats.pipePoints;

    // ---- 随机飞点 ----
    for (int i = 0; i < p.outlierCount; ++i)
    {
        PointT pt;
        pt.x = static_cast<float>(uni(rng) * p.outlierBoxHalf);
        pt.y = static_cast<float>(uni(rng) * p.outlierBoxHalf);
        pt.z = static_cast<float>(uni(rng) * p.outlierBoxHalf);
        cloud->push_back(pt);
    }

    stats.totalPoints   = static_cast<int>(cloud->size());
    stats.outlierPoints = stats.totalPoints - stats.pipePoints - stats.platePoints;

    // ---- 坐标范围 ----
    if (!cloud->empty())
    {
        PointT mn;
        PointT mx;
        pcl::getMinMax3D(*cloud, mn, mx);
        stats.minXYZ[0] = mn.x;
        stats.minXYZ[1] = mn.y;
        stats.minXYZ[2] = mn.z;
        stats.maxXYZ[0] = mx.x;
        stats.maxXYZ[1] = mx.y;
        stats.maxXYZ[2] = mx.z;
    }

    // PCD 需要 width/height。生成的点是"无序点云"，所以 width = 点数，height = 1。
    cloud->width  = static_cast<uint32_t>(cloud->size());
    cloud->height = 1;
    cloud->is_dense = false;   // 含噪声，不保证每个点都有效

    return cloud;
}

bool generateAndSave(const ScenePreset &preset, SceneStats &stats, std::string &err)
{
    err.clear();

    CloudPtr cloud = generateScene(preset.params, stats);
    if (!cloud || cloud->empty())
    {
        err = "生成的点云为空，请检查场景参数（pipeLength / ntheta / 步长）";
        return false;
    }

    if (!ensureDir(dataDir()))
    {
        err = "无法创建 data 目录: " + dataDir();
        return false;
    }

    // 1) 场景自己的那份文件（互不覆盖）
    const std::string presetPath = dataDir() + "/" + presetFileName(preset);
    if (!saveCloud(presetPath, cloud, err, /*allowOriginal=*/true))
    {
        return false;
    }

    // 2) 备份旧的原始场景（只留一份 .bak，避免堆积）
    const std::string origPath = originalScenePath();
    std::error_code ec;
    if (fs::exists(origPath, ec))
    {
        const std::string bakPath = origPath + ".bak";
        fs::remove(bakPath, ec);
        fs::rename(origPath, bakPath, ec);
    }

    // 3) 写入算法默认读取的 scene_original.pcd
    if (!saveCloud(origPath, cloud, err, /*allowOriginal=*/true))
    {
        return false;
    }

    return true;
}

void printSceneStats(const SceneStats &stats)
{
    printSection("点云组成");
    printKV("管道点", stats.pipePoints);
    printKV("平板点", stats.platePoints);
    printKV("飞点（离群点）", stats.outlierPoints);
    printKV("总点数", stats.totalPoints);

    printSection("坐标范围 (mm)");
    {
        std::ostringstream oss;
        oss.setf(std::ios::fixed);
        oss.precision(2);
        oss << "X [" << stats.minXYZ[0] << ", " << stats.maxXYZ[0] << "]";
        printKV("X 范围", oss.str());
    }
    {
        std::ostringstream oss;
        oss.setf(std::ios::fixed);
        oss.precision(2);
        oss << "Y [" << stats.minXYZ[1] << ", " << stats.maxXYZ[1] << "]";
        printKV("Y 范围", oss.str());
    }
    {
        std::ostringstream oss;
        oss.setf(std::ios::fixed);
        oss.precision(2);
        oss << "Z [" << stats.minXYZ[2] << ", " << stats.maxXYZ[2] << "]";
        printKV("Z 范围", oss.str());
    }

    if (stats.trueBevelAngleDeg > 0.0)
    {
        printSection("几何真值（算法量出来的应该接近这些数）");
        printKV("单边坡口角 (度)", stats.trueBevelAngleDeg, 2);
        printKV("钝边高度 (mm)", stats.trueRootFace, 3);
        printKV("根部间隙 (mm)", stats.trueRootGap, 3);
        printKV("错边量 (mm)", stats.trueMisalignment, 3);
        printKV("坡口斜面轴向长度 (mm)", stats.bevelRunLength, 3);
    }
}

}  // namespace pcp
