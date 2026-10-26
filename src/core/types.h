// =============================================================================
// src/core/types.h
//
// 全项目统一的"点类型"和"点云类型"别名。
//
// 为什么要单独抽这个文件：
//   PCL 的每个算法都是模板，写成 VoxelGrid<pcl::PointXYZ> 就是把点类型写死了。
//   如果全项目到处都写 pcl::PointXYZ，将来想换成带强度的 PointXYZI（真实
//   线激光传感器通常还输出反射强度），就要在整个项目里搜索替换几十处。
//   集中成一个别名 PointT，将来只要改下面这一行。
//
// 这是本项目的"留后路"设计之一，见 docs/02_项目目录与多文件C++.md。
// =============================================================================
#pragma once

#include <pcl/point_types.h>
#include <pcl/point_cloud.h>

#include <memory>

namespace pcp   // pcp = PointCloudPractice，项目的命名空间
{

// ---- 点类型 ----------------------------------------------------------------
// pcl::PointXYZ 只有 x / y / z 三个 float 字段。
// 将来若要强度：改成 pcl::PointXYZI（0~255 的 intensity）。
using PointT = pcl::PointXYZ;

// ---- 点云类型 --------------------------------------------------------------
// pcl::PointCloud<PointT> 可以理解成"一个装点的数组 + 宽高等元数据"。
// 它的详细结构见 docs/07_PCL点云数据结构.md。
using CloudT = pcl::PointCloud<PointT>;

// ---- 智能指针别名 ----------------------------------------------------------
// PCL 大量使用"共享所有权的指针"来传点云，避免每次拷贝几十万个点。
// Ptr        = std::shared_ptr<CloudT>       可写
// ConstPtr   = std::shared_ptr<const CloudT> 只读（算法输入一般用这个）
using CloudPtr      = CloudT::Ptr;
using CloudConstPtr = CloudT::ConstPtr;

}  // namespace pcp
