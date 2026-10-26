// =============================================================================
// src/core/cloud_io.h
//
// 与"文件"有关的一切：路径、目录、PCD 读写、输出命名。
//
// 每个算法都从这里取数据、往这里存结果，所以"读哪份原始数据""存到哪"
// 这件事只在一个文件里定义，不会出现"算法 A 存到 output、算法 B 存到 bin"。
// =============================================================================
#pragma once

#include "core/types.h"

#include <string>
#include <vector>

namespace pcp
{

// ---- 路径 ------------------------------------------------------------------

// 项目根目录（绝对路径，带正斜杠）。
// 来源：CMake 在编译时注入的宏 PCP_PROJECT_DIR。
// 好处是运行时不用关心"当前工作目录"是什么——在 Visual Studio 里按 F5 也能找对。
std::string projectDir();

// 项目根/data 与 项目根/output
std::string dataDir();
std::string outputDir();

// 原始场景文件的绝对路径：项目根/data/scene_original.pcd
std::string originalScenePath();

// 若目录不存在则创建（等价于 mkdir -p）。成功返回 true。
bool ensureDir(const std::string &dir);

// 把绝对路径变成相对项目根的路径，只用于打印。
std::string toProjectRelative(const std::string &absPath);

// ---- 读写 ------------------------------------------------------------------

// 读取 PCD 文件。失败时返回 nullptr 并往 err 里写中文原因。
// 常见的失败原因：文件不存在、路径写错、PCD 是 binary_compressed 而当前 PCL 不支持等。
CloudPtr loadCloud(const std::string &absPath, std::string &err);

// 保存为 binary 格式 PCD（体积比 ascii 小、读得快）。
// 会自动创建父目录。成功返回 true。
//
// allowOriginal 是"原始数据保护"的开关：
//   false（默认）= 拒绝写入任何名为 scene_original.pcd 的文件 —— 算法只能往 output/ 写；
//   true         = 允许，只给场景生成器用（它必须能重写原始场景）。
// 这个参数故意做成"默认安全"，忘了传就是安全的。
bool saveCloud(const std::string &absPath, CloudConstPtr cloud, std::string &err,
               bool allowOriginal = false);

// ---- 输出命名 --------------------------------------------------------------

// 生成输出文件名：<算法名>_<参数标签>.pcd
// 例：makeOutputName("voxel", "leaf0.020") -> "voxel_leaf0.020.pcd"
std::string makeOutputName(const std::string &algorithm, const std::string &paramTag);

// 把点云存进 output/ 目录，返回"相对项目根"的路径（用于打印）。
// 若保存失败，返回空字符串。
std::string saveOutput(const std::string &algorithm,
                       const std::string &paramTag,
                       CloudConstPtr cloud);

// ---- 列表 ------------------------------------------------------------------

// 列出 output/ 下所有 .pcd 文件名（按名字排序），用于菜单里回看结果。
std::vector<std::string> listOutputFiles();

}  // namespace pcp
