// =============================================================================
// src/data/scene_presets.cpp
//
// 五个预设场景的表格。
//
// 为什么用"表格 + 编号"而不是命令行参数：
//   1. 你可以一眼看到每个场景长什么样、用来练什么；
//   2. 想试新场景时，复制一段改数字即可，不用记一堆命令行开关；
//   3. 顺序稳定，菜单编号不会因为新增算法而变化。
// =============================================================================
#include "data/scene_presets.h"

namespace pcp
{

const std::vector<ScenePreset> &scenePresets()
{
    // static 局部变量：第一次调用时构造，之后一直复用（C++11 起线程安全）
    static const std::vector<ScenePreset> presets = [] {
        std::vector<ScenePreset> v;

        // ---- 0. 默认场景：管道 + 平板 + 飞点 ----
        // 这是项目最初要求的那个"统一测试场景"，也是 scene_original.pcd 的来源。
        // 三个几何体空间分离，欧氏聚类在合适容差下能分开；平板足够大，RANSAC 能拟合。
        {
            ScenePreset s;
            s.name  = "pipe_plate";
            s.title = "管道 + 平板 + 飞点（默认，原始场景）";
            s.description = "直管（无坡口）与平板在空间上分离，另有 200 个随机飞点。"
                            "用来练：体素降采样、统计/半径滤波、RANSAC 平面、欧氏聚类。";
            s.params.enableGroove = false;
            s.params.enablePlate  = true;
            s.params.noiseSigma   = 0.05;
            s.params.axialStep    = 1.5;
            s.params.ntheta       = 240;
            s.params.outlierCount = 200;
            v.push_back(s);
        }

        // ---- 1. 理想管道：几乎无噪声、密度高 ----
        // 用来先看清"算法本身在理想情况下应该给出什么结果"，排除噪声干扰。
        {
            ScenePreset s;
            s.name  = "ideal_pipe";
            s.title = "理想管道（低噪声、高密度）";
            s.description = "纯管道，噪声 0.01mm，无平板、无飞点。"
                            "用来验证：体素降采样点数比例、RANSAC 圆柱拟合精度。";
            s.params.enableGroove = false;
            s.params.enablePlate  = false;
            s.params.noiseSigma   = 0.01;
            s.params.axialStep    = 1.0;
            s.params.ntheta       = 360;
            s.params.outlierCount = 0;
            v.push_back(s);
        }

        // ---- 2. 脏扫描：高噪声 + 飞点 + 单视角盲区 ----
        // 模拟真实线激光扫描：噪声大、有飞点、背面扫不到。
        // 用来学：统计滤波和半径滤波到底能去掉什么、去不掉什么。
        {
            ScenePreset s;
            s.name  = "noisy_scan";
            s.title = "脏扫描（高噪声 + 飞点 + 单视角盲区）";
            s.description = "噪声 0.25mm，800 个飞点，只有 200° 视角可见（模拟单侧扫描）。"
                            "用来练：SOR / ROR 参数怎么定，滤波过头会损失坡口细节。";
            s.params.enableGroove = false;
            s.params.enablePlate  = true;
            s.params.noiseSigma   = 0.25;
            s.params.axialStep    = 1.0;
            s.params.ntheta       = 300;
            s.params.viewAngleDeg = 200.0;
            s.params.outlierCount = 800;
            v.push_back(s);
        }

        // ---- 3. 坡口管道：V 形坡口对接环焊缝 ----
        // 本项目的主角。用来练：环向切片、坡口轮廓提取、坡口角/钝边/间隙的测量。
        {
            ScenePreset s;
            s.name  = "groove_pipe";
            s.title = "坡口管道（V 形坡口对接环焊缝）";
            s.description = "两段管对接，单边坡口角 30°、钝边 2mm、根部间隙 3mm、错边 1mm。"
                            "用来练：环向切片取截面、坡口几何参数测量、等面积法道次划分。";
            s.params.enableGroove   = true;
            s.params.sampleRootFace = true;
            s.params.bevelAngleDeg  = 30.0;
            s.params.rootFace       = 2.0;
            s.params.rootGap        = 3.0;
            s.params.misalignment   = 1.0;
            s.params.enablePlate    = false;
            s.params.noiseSigma     = 0.05;
            s.params.axialStep      = 1.2;
            s.params.bevelStep      = 0.35;
            s.params.ntheta         = 300;
            s.params.outlierCount   = 30;
            v.push_back(s);
        }

        // ---- 4. 薄壁小管：管径小、壁厚薄 ----
        // 用来对比"参数不能照抄"：管径变了，采样密度、搜索半径、体素尺寸都要跟着变。
        {
            ScenePreset s;
            s.name  = "thin_pipe";
            s.title = "薄壁小管（管径 60mm、壁厚 6mm）";
            s.description = "外半径 30mm、壁厚 6mm，带坡口。"
                            "用来练：同一套参数换管径后为什么会失效、怎么按几何比例缩放参数。";
            s.params.pipeRadius    = 30.0;
            s.params.pipeThickness = 6.0;
            s.params.pipeLength    = 50.0;
            s.params.enableGroove  = true;
            s.params.sampleRootFace = true;
            s.params.rootFace      = 1.5;
            s.params.rootGap       = 2.0;
            s.params.bevelAngleDeg = 37.5;
            s.params.ntheta        = 240;
            s.params.axialStep     = 0.8;
            s.params.bevelStep     = 0.25;
            s.params.enablePlate   = false;
            s.params.noiseSigma    = 0.04;
            s.params.outlierCount  = 20;
            v.push_back(s);
        }

        return v;
    }();

    return presets;
}

int findPresetIndex(const std::string &name)
{
    const auto &all = scenePresets();
    for (int i = 0; i < static_cast<int>(all.size()); ++i)
    {
        if (all[static_cast<size_t>(i)].name == name)
        {
            return i;
        }
    }
    return -1;
}

std::string presetFileName(const ScenePreset &preset)
{
    return "scene_" + preset.name + ".pcd";
}

}  // namespace pcp
