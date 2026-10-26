// =============================================================================
// src/core/cloud_io.cpp
//
// 路径拼接、目录创建、PCD 读写、输出命名。
// 这个文件是"算法与磁盘之间唯一的通道"，所以两件事在这里集中保证：
//   1. 原始数据 scene_original.pcd 永远只读，任何保存请求都写不到它上面；
//   2. 输出文件的命名规则统一，参数写进文件名，不同实验不会互相覆盖。
// =============================================================================
#include "core/cloud_io.h"

#include <pcl/io/pcd_io.h>

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace pcp
{

namespace
{
// 缓存一次项目根目录，避免每次都查文件系统
std::string g_projectDir;

// 从某个起点向上找"含 CMakeLists.txt 的目录"，找不到返回空串
std::string findProjectRootUpwards(const fs::path &start)
{
    fs::path cur = start;
    for (int i = 0; i < 8; ++i)   // 最多向上走 8 层，防止跑到盘符根
    {
        std::error_code ec;
        if (fs::exists(cur / "CMakeLists.txt", ec))
        {
            return cur.generic_string();
        }
        if (!cur.has_parent_path() || cur.parent_path() == cur)
        {
            break;
        }
        cur = cur.parent_path();
    }
    return "";
}
}  // namespace

std::string projectDir()
{
    if (!g_projectDir.empty())
    {
        return g_projectDir;
    }

    // 第一选择：CMake 注入的绝对路径（最可靠）
#ifdef PCP_PROJECT_DIR
    {
        fs::path fromMacro(PCP_PROJECT_DIR);
        std::error_code ec;
        if (fs::exists(fromMacro / "CMakeLists.txt", ec))
        {
            g_projectDir = fromMacro.generic_string();
            return g_projectDir;
        }
    }
#endif

    // 第二选择：从当前工作目录往上找（用户手工把 exe 拷走运行时走这条路）
    {
        std::error_code ec;
        fs::path cwd = fs::current_path(ec);
        if (!ec)
        {
            std::string found = findProjectRootUpwards(cwd);
            if (!found.empty())
            {
                g_projectDir = found;
                return g_projectDir;
            }
        }
    }

    // 实在找不到就用当前目录，后续读文件会报"文件不存在"，比崩溃好
    std::error_code ec;
    g_projectDir = fs::current_path(ec).generic_string();
    return g_projectDir;
}

std::string dataDir()
{
    return projectDir() + "/data";
}

std::string outputDir()
{
    return projectDir() + "/output";
}

std::string originalScenePath()
{
    return dataDir() + "/scene_original.pcd";
}

bool ensureDir(const std::string &dir)
{
    std::error_code ec;
    if (fs::exists(dir, ec))
    {
        return true;
    }
    return fs::create_directories(dir, ec) && !ec;
}

std::string toProjectRelative(const std::string &absPath)
{
    const std::string root = projectDir();
    if (absPath.size() > root.size() && absPath.compare(0, root.size(), root) == 0)
    {
        std::string rel = absPath.substr(root.size());
        if (!rel.empty() && (rel[0] == '/' || rel[0] == '\\'))
        {
            rel.erase(0, 1);
        }
        return rel;
    }
    return absPath;
}

// ---------------------------------------------------------------------------
// 读
// ---------------------------------------------------------------------------
CloudPtr loadCloud(const std::string &absPath, std::string &err)
{
    err.clear();

    std::error_code ec;
    if (!fs::exists(absPath, ec))
    {
        err = "文件不存在: " + absPath;
        return nullptr;
    }

    auto cloud = std::make_shared<CloudT>();
    // loadPCDFile 返回负数表示失败
    const int ret = pcl::io::loadPCDFile<PointT>(absPath, *cloud);
    if (ret < 0)
    {
        err = "PCL 读取 PCD 失败（返回码 " + std::to_string(ret) + "）: " + absPath;
        return nullptr;
    }

    if (cloud->empty())
    {
        err = "文件读到了但点数为 0: " + absPath;
        return nullptr;
    }

    return cloud;
}

// ---------------------------------------------------------------------------
// 写
// ---------------------------------------------------------------------------
bool saveCloud(const std::string &absPath, CloudConstPtr cloud, std::string &err,
               bool allowOriginal)
{
    err.clear();

    if (!cloud || cloud->empty())
    {
        err = "要保存的点云是空的，已跳过";
        return false;
    }

    // ---- 硬保护：默认不允许覆盖原始场景 ----
    // 除了判断完整路径，还判断文件名，防止有人把别处的同名文件写过来。
    const std::string fileName = fs::path(absPath).filename().generic_string();
    if (!allowOriginal && fileName == "scene_original.pcd")
    {
        err = "拒绝写入 scene_original.pcd：原始数据只读，算法结果请写到 output/ 目录";
        return false;
    }

    // 自动创建父目录（output/ 可能还不存在）
    const fs::path parent = fs::path(absPath).parent_path();
    if (!parent.empty())
    {
        ensureDir(parent.generic_string());
    }

    // savePCDFileBinary：二进制格式，比 ascii 小 2~3 倍，读写也更快
    const int ret = pcl::io::savePCDFileBinary(absPath, *cloud);
    if (ret < 0)
    {
        err = "PCL 保存 PCD 失败（返回码 " + std::to_string(ret) + "）: " + absPath;
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 命名
// ---------------------------------------------------------------------------
std::string makeOutputName(const std::string &algorithm, const std::string &paramTag)
{
    std::string name = algorithm;
    if (!paramTag.empty())
    {
        name += "_" + paramTag;
    }
    // 文件名里不能有 : * ? " < > | 等字符，参数标签里的这些一律换成下划线
    for (char &c : name)
    {
        if (c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|' || c == ' ')
        {
            c = '_';
        }
    }
    return name + ".pcd";
}

std::string saveOutput(const std::string &algorithm,
                       const std::string &paramTag,
                       CloudConstPtr cloud)
{
    if (!cloud || cloud->empty())
    {
        return "";
    }

    const std::string full = outputDir() + "/" + makeOutputName(algorithm, paramTag);

    std::string err;
    if (!saveCloud(full, cloud, err))
    {
        return "";
    }
    return toProjectRelative(full);
}

std::vector<std::string> listOutputFiles()
{
    std::vector<std::string> names;
    std::error_code ec;
    if (!fs::exists(outputDir(), ec))
    {
        return names;
    }

    for (const auto &entry : fs::directory_iterator(outputDir(), ec))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        if (entry.path().extension() == ".pcd")
        {
            names.push_back(entry.path().filename().generic_string());
        }
    }
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace pcp
