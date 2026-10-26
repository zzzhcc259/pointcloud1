// =============================================================================
// src/menu/menu_registry.h
//
// 菜单的"注册表"结构。
//
// 这是本项目"加算法很便宜"的关键设计：
//   菜单不是一个巨大的 if/else 链，而是一张表。表里每一行是一个功能。
//   加一个算法 = 往表里 push_back 一行，编号自动排上，不需要改任何已有代码。
//
// 每一项的动作是一个 std::function<void()>，意思是"一个不带参数、不返回值的函数"。
// 这样每个算法内部可以自由决定怎么读参数、怎么输出，公共层完全不限制它。
// =============================================================================
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace pcp
{

struct MenuItem
{
    // 分组名。连续几项用同一个分组名，打印时会合并到一个小标题下面。
    std::string category;

    // 菜单上显示的文字
    std::string title;

    // 选中后执行什么。空的话这一项不可点（用来做纯标题行）。
    std::function<void()> action;
};

// 构建全部菜单项 —— 实现在 menu_items.cpp，那里是唯一需要动的地方
std::vector<MenuItem> buildMenu();

// 打印菜单（分组标题 + 编号 + 项名）
void printMenu(const std::vector<MenuItem> &items);

// 菜单主循环。返回进程退出码。
int runMenuLoop(const std::vector<MenuItem> &items);

}  // namespace pcp
