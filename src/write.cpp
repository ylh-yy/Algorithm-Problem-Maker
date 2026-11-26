#include "problem.h"
#include "json.hpp"

#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

void writeText(const fs::path& file, const std::string& text) {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("无法写入文件: " + file.u8string());
    out << text;
    out.close();
    if (!out)
        throw std::runtime_error("文件写入失败: " + file.u8string());
}

fs::path writeProblem(const fs::path& source) {
    fs::path dir = fs::absolute(source).lexically_normal();
    if (dir.filename().empty())
        dir = dir.parent_path();
    if (!fs::is_directory(dir) || dir.filename().empty())
        throw std::runtime_error("无效的题目目录: " + source.u8string());

    fs::path output = dir.parent_path() / (dir.filename().wstring() + L"_已完成");
    if (fs::exists(output))
        throw std::runtime_error("成品目录已存在: " + output.u8string());

    const json files = json::parse(readText(projectDirectory() / L"out.json"));
    for (const auto& item : problemFiles) {
        if (!files.contains(item.key) || !files.at(item.key).is_string())
            throw std::runtime_error(std::string("out.json 缺少文本字段: ") + item.key);
    }

    fs::path notes = findNotes(dir);
    fs::path staging = output;
    staging += L".tmp";
    if (!fs::create_directory(staging))
        throw std::runtime_error("临时目录已存在: " + staging.u8string());

    try {
        fs::create_directory(staging / L"data");
        for (const auto& item : problemFiles)
            writeText(staging / item.filename, files.at(item.key).get_ref<const std::string&>());
        writeText(staging / L"extra.txt", notes.empty() ? std::string() : readText(notes));
        fs::rename(staging, output);
    } catch (...) {
        std::error_code error;
        for (const auto& item : problemFiles)
            fs::remove(staging / item.filename, error);
        fs::remove(staging / L"extra.txt", error);
        fs::remove(staging / L"data", error);
        fs::remove(staging, error);
        throw;
    }
    return output;
}
