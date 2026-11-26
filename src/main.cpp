#include "problem.h"

#include <exception>
#include <iostream>
#include <windows.h>

static fs::path exampleFolder(int argc, wchar_t* argv[], int index) {
    if (argc > index)
        return argv[index];
    fs::path example = projectDirectory() / L"problems" / L"example";
    if (fs::is_directory(example))
        return example;
    return {};
}

int wmain(int argc, wchar_t* argv[]) {
    SetConsoleOutputCP(CP_UTF8);

    try {
        if (argc == 2 && std::wstring(argv[1]) == L"--work") {
            workProblem();
            std::cout << "已生成 out.json" << std::endl;
        } else if (argc == 3 && std::wstring(argv[1]) == L"--write") {
            fs::path output = writeProblem(argv[2]);
            std::cout << "已生成 " << output.u8string() << std::endl;
        } else if ((argc == 3 || argc == 4) && std::wstring(argv[1]) == L"--read") {
            readProblem(argv[2], exampleFolder(argc, argv, 3));
            std::cout << "已生成 in.json" << std::endl;
        } else if ((argc == 2 || argc == 3) && std::wstring(argv[1]).rfind(L"--", 0) != 0) {
            readProblem(argv[1], exampleFolder(argc, argv, 2));
            std::cout << "已生成 in.json，正在请求 AI" << std::endl;
            workProblem();
            std::cout << "已生成 out.json" << std::endl;
            fs::path output = writeProblem(argv[1]);
            std::cout << "已生成 " << output.u8string() << std::endl;
        } else {
            std::cerr << "用法: maker.exe <题目目录> [example目录]" << std::endl;
            std::cerr << "分步运行: --read <题目目录> [example目录] | --work | --write <题目目录>" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
