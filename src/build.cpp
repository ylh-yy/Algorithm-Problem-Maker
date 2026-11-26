#include "problem.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

int main() {
    std::cout << "输入题目名称" << std::endl;
    std::string input;
    if (!(std::cin >> input))
        return 0;
    int len = MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, nullptr, 0);
    if (len == 0) {
        std::cerr << "题目名称转换失败" << std::endl;
        return 1;
    }
    std::wstring wname(len, L'\0');
    if (MultiByteToWideChar(CP_ACP, 0, input.c_str(), -1, &wname[0], len) == 0) {
        std::cerr << "题目名称转换失败" << std::endl;
        return 1;
    }
    wname.pop_back();

    try {
        fs::path name(wname);
        if (name.has_root_path() || name.filename() != name || name == L"." || name == L"..") {
            std::cerr << "请输入题目名称" << std::endl;
            return 1;
        }
        fs::path problems = projectDirectory() / L"problems";
        fs::create_directory(problems);
        fs::path dir = problems / name;
        if (fs::is_directory(dir)) {
            std::cout << "题目名称已经存在，是否覆盖？（输出y或者n）" << std::endl;
            std::string res;
            std::cin >> res;
            if (res[0] == 'y') {

            } else {
                return 0;
            }
        }
        fs::create_directory(dir);

        std::vector<std::wstring> suffixs = { L"tm.md", L"tj.md", L"std.cpp", L"gen.cpp", L"extra.txt" };
        for (const auto& s : suffixs) {
            fs::path full = dir / s;
            std::ofstream file(full);
            if (!file) {
                std::cerr << "文件创建失败" << std::endl;
                return 1;
            }
        }

        std::cout << "创建成功！" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "目录操作失败: " << e.what() << std::endl;
        return 1;
    }
}
