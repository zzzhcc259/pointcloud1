// =============================================================================
// src/menu/menu_registry.cpp
// =============================================================================
#include "menu/menu_registry.h"

#include "core/console.h"

#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>

namespace pcp
{

namespace
{
// 去掉字符串两端的空白（含 \r，Windows 换行会留下它）
std::string trim(const std::string &s)
{
    const char *ws = " \t\r\n";
    const size_t begin = s.find_first_not_of(ws);
    if (begin == std::string::npos)
    {
        return "";
    }
    const size_t end = s.find_last_not_of(ws);
    return s.substr(begin, end - begin + 1);
}
}  // namespace

void printMenu(const std::vector<MenuItem> &items)
{
    std::cout << "\n";
    std::cout << "================ 石油管道环焊缝点云练习台 ================\n";

    std::string currentCategory;
    for (size_t i = 0; i < items.size(); ++i)
    {
        // 分组名变化时打印一个小标题
        if (items[i].category != currentCategory)
        {
            std::cout << "\n  [" << items[i].category << "]\n";
            currentCategory = items[i].category;
        }
        std::cout << "    " << std::right << std::setw(2) << (i + 1)
                  << "  " << items[i].title << "\n";
    }

    std::cout << "\n     0  退出\n";
    std::cout << "==========================================================\n";
}

int runMenuLoop(const std::vector<MenuItem> &items)
{
    while (true)
    {
        printMenu(items);

        std::cout << "\n请输入编号（0 退出）: " << std::flush;

        std::string line;
        if (!std::getline(std::cin, line))
        {
            // 输入流结束（Ctrl+Z / 管道结束），安静退出
            std::cout << "\n";
            break;
        }

        line = trim(line);
        if (line.empty())
        {
            continue;
        }

        if (line == "0")
        {
            std::cout << "\n已退出。\n";
            break;
        }

        // 只接受纯数字，避免 "abc" 被 atoi 悄悄当成 0
        bool allDigits = true;
        for (char c : line)
        {
            if (c < '0' || c > '9')
            {
                allDigits = false;
                break;
            }
        }
        if (!allDigits)
        {
            printWarn("请输入数字编号，例如 3");
            continue;
        }

        const int choice = std::atoi(line.c_str());
        if (choice < 1 || choice > static_cast<int>(items.size()))
        {
            printWarn("编号超出范围（1 ~ " + std::to_string(items.size()) + "）");
            continue;
        }

        const MenuItem &item = items[static_cast<size_t>(choice - 1)];
        if (!item.action)
        {
            printWarn("这一项没有绑定功能（只是标题行）");
            continue;
        }

        // 用 try/catch 把单个功能的异常挡在菜单层，避免一个算法出错就整个程序退出
        try
        {
            item.action();
        }
        catch (const std::exception &e)
        {
            printError(std::string("执行该功能时发生异常: ") + e.what());
        }
        catch (...)
        {
            printError("执行该功能时发生未知异常");
        }

        std::cout << "\n（按回车继续）" << std::flush;
        std::string dummy;
        std::getline(std::cin, dummy);
    }

    return 0;
}

}  // namespace pcp
