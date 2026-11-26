#include "problem.h"
#include "json.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

using json = nlohmann::json;

std::string readText(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in)
        throw std::runtime_error("无法读取文件: " + file.u8string());
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

fs::path findNotes(const fs::path& dir) {
    for (const auto* name : { L"extra.txt", L"交代", L"交代.txt" }) {
        fs::path file = dir / name;
        if (fs::is_regular_file(file) && fs::file_size(file) != 0)
            return file;
    }
    return {};
}

static json readFolder(const fs::path& dir) {
    if (!fs::is_directory(dir))
        throw std::runtime_error("题目目录不存在: " + dir.u8string());

    json files = json::object();
    for (const auto& item : problemFiles) {
        fs::path file = dir / item.filename;
        if (!fs::is_regular_file(file) || fs::file_size(file) == 0)
            file = dir / fs::u8path(item.key);
        if (fs::is_regular_file(file) && fs::file_size(file) != 0)
            files[item.key] = readText(file);
    }

    fs::path notes = findNotes(dir);
    if (!notes.empty())
        files["交代"] = readText(notes);

    fs::path data = dir / L"data";
    if (fs::is_directory(data)) {
        for (const auto& entry : fs::directory_iterator(data)) {
            const fs::path& file = entry.path();
            if (entry.is_regular_file() && (file.extension() == L".in" || file.extension() == L".out"))
                files["data/" + file.filename().u8string()] = readText(file);
        }
    }
    return files;
}

void readProblem(const fs::path& source, const fs::path& example) {
    const fs::path project = projectDirectory();
    json input;
    input["task"] = readText(project / L"task.txt");
    if (input["task"].get_ref<const std::string&>().find_first_not_of(" \t\r\n") == std::string::npos)
        throw std::runtime_error("task.txt 为空");
    input["files"] = readFolder(source);
    input["example"] = example.empty() ? json::object() : readFolder(example);
    writeText(project / L"in.json", input.dump(2) + '\n');
}
