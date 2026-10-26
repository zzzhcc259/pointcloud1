// =============================================================================
// src/data/generate_scene.h
//
// 合成场景生成器。
//
// 坐标系约定（全项目统一，务必记住）：
//   管道轴线 = X 轴
//   径向      = Y / Z 平面内的极坐标 (r, theta)
//   theta = 0 指向 +Y 方向，逆时针增大
// 所以：
//   - "环向切片"（沿轴向切）是 x = 常数 的平面
//   - "径向切片"（取坡口截面）是 theta = 常数 的平面，截面里横轴是 x、纵轴是 r
// =============================================================================
#pragma once

#include "core/types.h"
#include "data/scene_presets.h"

#include <string>

namespace pcp
{

// 生成结果的统计信息。一部分是"数出来的"，一部分是"世界里本来就知道的真值"。
// 有了真值，才能判断算法量出来的对不对 —— 这是合成数据的最大好处。
struct SceneStats
{
    int pipePoints    = 0;   // 两段管共多少个点
    int platePoints   = 0;   // 平板多少个点
    int outlierPoints = 0;   // 飞点多少个
    int totalPoints   = 0;

    double minXYZ[3] = {0.0, 0.0, 0.0};
    double maxXYZ[3] = {0.0, 0.0, 0.0};

    // ---- 几何真值 ----
    double trueRootGap       = 0.0;  // 根部间隙 (mm)
    double trueBevelAngleDeg = 0.0;  // 单边坡口角 (度)
    double trueRootFace      = 0.0;  // 钝边高度 (mm)
    double trueMisalignment  = 0.0;  // 错边量 (mm)
    double bevelRunLength    = 0.0;  // 坡口斜面沿轴向的长度 (mm)
};

// 只生成点云，不碰磁盘。
CloudPtr generateScene(const SceneParams &params, SceneStats &stats);

// 生成 + 落盘：
//   1. 存一份 data/scene_<预设名>.pcd（每个场景各留一份，互不覆盖）
//   2. 把旧的 scene_original.pcd 改名为 scene_original.pcd.bak（只保留一份备份）
//   3. 写入新的 data/scene_original.pcd（算法默认读这个文件）
bool generateAndSave(const ScenePreset &preset, SceneStats &stats, std::string &err);

// 把统计信息打印到控制台
void printSceneStats(const SceneStats &stats);

}  // namespace pcp
