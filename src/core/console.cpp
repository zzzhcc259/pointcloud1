// =============================================================================
// src/core/console.cpp
//
// console.h 里那些函数的实现（"声明在 .h，定义在 .cpp"的具体例子）。
// 文档 docs/02_项目目录与多文件C++.md 就用这个文件讲声明与定义的关系。
// =============================================================================
#include "core/console.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#  include <windows.h>   // SetConsoleOutputCP
#endif

namespace pcp
{

void setupConsole()
{
#ifdef _WIN32
    // 65001 = UTF-8 代码页。Visual Studio 的输出窗口和 PowerShell 都认这个。
    SetConsoleOutputCP(CP_UTF8);
#endif
}

void printRule()
{
    std::cout << "----------------------------------------------------------------------\n";
}

void printHeader(const std::string &title)
{
    std::cout << "\n";
    printRule();
    std::cout << "  " << title << "\n";
    printRule();
}

void printSection(const std::string &title)
{
    std::cout << "\n-- " << title << " --\n";
}

// ---------------------------------------------------------------------------
// 中文对齐的小工具
//
// 为什么 std::setw 对不齐中文：
//   std::setw(n) 数的是"字节数/字符数"，而终端里一个汉字占 2 列。
//   "管道点" = 3 个汉字 = 9 字节，setw(26) 只补 17 个空格，
//   屏幕上却占了 6 列 + 17 = 23 列，于是冒号对不齐。
// 办法：自己算"显示宽度"（汉字算 2 列，ASCII 算 1 列），手工补空格。
// ---------------------------------------------------------------------------
namespace
{
size_t displayWidthImpl(const std::string &s)
{
    size_t width = 0;
    for (size_t i = 0; i < s.size();)
    {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        unsigned int cp = 0;
        size_t len = 1;

        if (c < 0x80)              { cp = c;         len = 1; }
        else if ((c >> 5) == 0x6)  { cp = c & 0x1F;  len = 2; }
        else if ((c >> 4) == 0xE)  { cp = c & 0x0F;  len = 3; }
        else if ((c >> 3) == 0x1E) { cp = c & 0x07;  len = 4; }

        for (size_t k = 1; k < len && i + k < s.size(); ++k)
        {
            cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
        }

        const bool wide = (cp >= 0x1100 && cp <= 0x115F) ||
                          (cp >= 0x2E80 && cp <= 0xA4CF) ||
                          (cp >= 0xAC00 && cp <= 0xD7A3) ||
                          (cp >= 0xF900 && cp <= 0xFAFF) ||
                          (cp >= 0xFE30 && cp <= 0xFE6F) ||
                          (cp >= 0xFF00 && cp <= 0xFF60) ||
                          (cp >= 0xFFE0 && cp <= 0xFFE6) ||
                          (cp >= 0x20000);
        width += wide ? 2 : 1;
        i += len;
    }
    return width;
}
}  // namespace

size_t textWidth(const std::string &s)
{
    return displayWidthImpl(s);
}

std::string padRight(const std::string &s, size_t width)
{
    const size_t w = displayWidthImpl(s);
    if (w >= width)
    {
        return s;
    }
    return s + std::string(width - w, ' ');
}

void printKV(const std::string &key, const std::string &value)
{
    // 键占 26 个"显示列"，不够补空格，保证冒号对齐
    constexpr size_t kKeyWidth = 26;
    const size_t w = displayWidthImpl(key);
    const size_t pad = (w >= kKeyWidth) ? 1 : (kKeyWidth - w);
    std::cout << "  " << key << std::string(pad, ' ') << ": " << value << "\n";
}

void printKV(const std::string &key, int value)
{
    printKV(key, std::to_string(value));
}

void printKV(const std::string &key, double value, int precision)
{
    // 先按指定小数位格式化成字符串，再交给上面那个重载，保证对齐一致
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value;
    printKV(key, oss.str());
}

void printInfo(const std::string &msg)
{
    std::cout << "  [信息] " << msg << "\n";
}

void printWarn(const std::string &msg)
{
    std::cout << "  [警告] " << msg << "\n";
}

void printError(const std::string &msg)
{
    // 错误额外走 stderr 一份，方便脚本里单独捕获
    std::cout << "  [错误] " << msg << "\n";
    std::cerr << "  [错误] " << msg << "\n";
}

// ---------------------------------------------------------------------------
// 交互输入
// ---------------------------------------------------------------------------
namespace
{
std::string trimSpaces(const std::string &s)
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

std::string readLine(const std::string &prompt, const std::string &defaultValue)
{
    std::cout << "  " << prompt;
    if (!defaultValue.empty())
    {
        std::cout << " [直接回车 = " << defaultValue << "]";
    }
    std::cout << ": " << std::flush;

    std::string line;
    if (!std::getline(std::cin, line))
    {
        return defaultValue;   // 输入流已结束
    }
    line = trimSpaces(line);
    return line.empty() ? defaultValue : line;
}

int readInt(const std::string &prompt, int defaultValue)
{
    const std::string s = readLine(prompt, std::to_string(defaultValue));
    try
    {
        size_t used = 0;
        const int v = std::stoi(s, &used);
        if (used != s.size())
        {
            printWarn("输入里有多余字符，按 " + std::to_string(v) + " 处理");
        }
        return v;
    }
    catch (const std::exception &)
    {
        printWarn("不是合法整数，使用默认值 " + std::to_string(defaultValue));
        return defaultValue;
    }
}

double readDouble(const std::string &prompt, double defaultValue)
{
    // 默认值用 6 位小数，避免 50 显示成 50.000000 太长
    std::ostringstream def;
    def << std::setprecision(6) << defaultValue;

    const std::string s = readLine(prompt, def.str());
    try
    {
        size_t used = 0;
        const double v = std::stod(s, &used);
        if (used != s.size())
        {
            printWarn("输入里有多余字符，已忽略");
        }
        return v;
    }
    catch (const std::exception &)
    {
        printWarn("不是合法数字，使用默认值 " + def.str());
        return defaultValue;
    }
}

bool readYesNo(const std::string &prompt, bool defaultValue)
{
    const std::string s = readLine(prompt, defaultValue ? "y" : "n");
    const char c = s.empty() ? (defaultValue ? 'y' : 'n') : s[0];
    return (c == 'y' || c == 'Y' || c == '1');
}

}  // namespace pcp
