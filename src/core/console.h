// =============================================================================
// src/core/console.h
//
// 控制台输出的小工具集：标题、分隔线、键值对、警告、错误。
//
// 为什么值得单独做一个模块：
//   10 个算法如果各自手写 std::cout << "=====\n"，格式会逐渐不一致，
//   改一次排版要改 10 个文件。抽成函数后只改这里一处。
//   这也是"不要重复实现同一件事"的具体例子。
// =============================================================================
#pragma once

#include <string>

namespace pcp
{

// 把 Windows 控制台切到 UTF-8，否则中文会显示成乱码。
// 必须在任何输出之前调用（main 的第一行）。
void setupConsole();

// 打印一个带上下分隔线的大标题
void printHeader(const std::string &title);

// 打印一条分隔线
void printRule();

// 打印二级小标题（前面空一行）
void printSection(const std::string &title);

// ---- 键值对打印 ------------------------------------------------------------
void printKV(const std::string &key, const std::string &value);
void printKV(const std::string &key, int value);
void printKV(const std::string &key, double value, int precision = 3);

// ---- 提示类 ----------------------------------------------------------------
void printInfo(const std::string &msg);
void printWarn(const std::string &msg);
void printError(const std::string &msg);

// ---- 中文对齐 --------------------------------------------------------------
// 终端里一个汉字占 2 列，但 std::string::size() 数的是字节。
// 想让"表头"和"数据"对齐，就得按"显示宽度"算，这两个函数就是干这个的。
size_t textWidth(const std::string &s);
std::string padRight(const std::string &s, size_t width);

// ---- 交互输入 --------------------------------------------------------------
// 统一用 std::getline 读整行，避免 "cin >> 数字" 和 "getline" 混用时留下换行符的经典坑。
// 直接回车 = 采用默认值。
std::string readLine(const std::string &prompt, const std::string &defaultValue = "");
int         readInt(const std::string &prompt, int defaultValue);
double      readDouble(const std::string &prompt, double defaultValue);
bool        readYesNo(const std::string &prompt, bool defaultValue);

}  // namespace pcp
