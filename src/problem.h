#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <windows.h>

namespace fs = std::filesystem;

inline fs::path projectDirectory() {
    std::wstring executable(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()));
    if (length == 0 || length >= executable.size())
        throw std::runtime_error("无法取得程序路径");
    executable.resize(length);

    fs::path directory = fs::path(executable).parent_path();
    if (directory.filename() == L"bin")
        return directory.parent_path();
    return fs::current_path();
}

struct ProblemFile {
    const char* key;
    const wchar_t* filename;
};

inline const ProblemFile problemFiles[] = {
    { "题面.md", L"tm.md" },
    { "题解.md", L"tj.md" },
    { "std.cpp", L"std.cpp" },
    { "gen.cpp", L"gen.cpp" },
    { "valid.cpp", L"valid.cpp" },
    { "题目评估.md", L"题目评估.md" }
};

std::string readText(const fs::path& file);
void writeText(const fs::path& file, const std::string& text);
fs::path findNotes(const fs::path& dir);
void readProblem(const fs::path& source, const fs::path& example);
void workProblem();
fs::path writeProblem(const fs::path& source);
