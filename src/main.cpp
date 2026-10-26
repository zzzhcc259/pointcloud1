// =============================================================================
// src/main.cpp
//
// 程序的入口。这里故意写得很短 —— 只有三件事：
//   1. 把控制台切到 UTF-8（否则中文乱码）
//   2. 问菜单注册表要"菜单表"
//   3. 把表交给菜单循环去跑
//
// 为什么不在这里写算法逻辑：
//   算法改一行就要重编整个 main.cpp，而且越写越乱。
//   每个算法在自己的 .h/.cpp 里，main 只负责"把它们串起来"。
// =============================================================================
#include "core/cloud_io.h"
#include "core/console.h"
#include "menu/menu_registry.h"

#include <iostream>

int main()
{
    pcp::setupConsole();

    std::cout << "项目根目录: " << pcp::projectDir() << "\n";
    std::cout << "原始数据  : " << pcp::toProjectRelative(pcp::originalScenePath()) << "\n";

    // 菜单表在 menu_items.cpp 的 buildMenu() 里定义
    const std::vector<pcp::MenuItem> menu = pcp::buildMenu();

    return pcp::runMenuLoop(menu);
}
