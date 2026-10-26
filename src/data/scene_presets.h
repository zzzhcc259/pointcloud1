// =============================================================================
// src/data/scene_presets.h
//
// 场景参数 + 预设表。
//
// "切换场景"这件事在这里实现：场景不是一个开关，而是一整组几何/噪声/采样参数。
// 你不需要改源码来换场景，只需要在菜单里选一个编号。
// 同一个预设 + 同一个随机种子 = 完全相同的点云（可复现）。
// =============================================================================
#pragma once

#include <string>
#include <vector>

namespace pcp
{

// 一个合成场景的全部可调参数。单位统一为毫米（mm）。
// 真实激光传感器输出常是 mm，PCL 本身不关心单位，但"全项目一致"很重要：
// 阈值 3.0 到底是 3mm 还是 3m，直接决定算法是有效还是全错。
struct SceneParams
{
    // ---- 管道几何 ----
    double pipeRadius    = 50.0;   // 外半径（管径 100mm）
    double pipeThickness = 12.0;   // 壁厚（内半径 = 外半径 - 壁厚）
    double pipeLength    = 80.0;   // 每段管的轴向长度

    // ---- 坡口几何（石油管道环焊缝）----
    bool   enableGroove   = false; // 是否加工出 V 形坡口
    double bevelAngleDeg  = 30.0;  // 单边坡口面角度（度）。V 形总夹角 = 2 × 这个值
    double rootFace       = 2.0;   // 钝边高度（根部竖直面，沿径向）
    double rootGap        = 3.0;   // 根部间隙（两管端面之间的距离）
    double misalignment   = 0.0;   // 错边量（右管整体沿 Z 偏移，模拟对口错边）
    bool   sampleRootFace = false; // 是否采样钝边端面（真实扫描常打不到，教学时打开更完整）

    // ---- 采样 ----
    int    ntheta        = 240;    // 环向采样点数（绕管道一周）
    double axialStep     = 1.5;    // 轴向采样步长（mm），管身区
    double bevelStep     = 0.4;    // 轴向采样步长（mm），坡口区（更密）
    double noiseSigma    = 0.05;   // 测量噪声标准差（mm），三个方向独立高斯
    bool   sampleInner   = false;  // 是否采内壁（真实外扫打不到内壁，默认关闭）
    double viewAngleDeg  = 360.0;  // 单视角覆盖角度（度）。360 = 全周可见

    // ---- 平板（用于练 RANSAC 平面 / 欧氏聚类）----
    bool   enablePlate   = true;
    double plateCenterY  = 200.0;  // 平板所在平面 y = 这个值
    double plateSizeX    = 160.0;  // 平板在 X 方向的边长
    double plateSizeZ    = 100.0;  // 平板在 Z 方向的边长
    double plateStep     = 2.0;    // 平板上采样点间距

    // ---- 离群点 ----
    int    outlierCount  = 200;    // 随机飞点个数
    double outlierBoxHalf = 180.0; // 飞点撒在 [-这个值, +这个值] 的立方体内

    // ---- 随机种子 ----
    unsigned seed = 20250101u;     // 固定种子 -> 每次生成结果相同
};

// 一个命名场景预设
struct ScenePreset
{
    std::string name;        // 英文短名，用于文件名：scene_<name>.pcd
    std::string title;       // 菜单里显示的中文名
    std::string description; // 一句话说明这个场景用来练什么
    SceneParams params;
};

// 全部预设（顺序即菜单编号顺序，第 0 个是默认场景）
const std::vector<ScenePreset> &scenePresets();

// 按名字找预设下标，找不到返回 -1
int findPresetIndex(const std::string &name);

// 点云文件名：scene_<name>.pcd
std::string presetFileName(const ScenePreset &preset);

}  // namespace pcp
