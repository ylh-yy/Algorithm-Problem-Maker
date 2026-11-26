#include "problem.h"
#include "ai.h"

#include <windows.h>
#include <winhttp.h>

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string_view>

using json = nlohmann::json;
using InternetHandle = std::unique_ptr<void, decltype(&WinHttpCloseHandle)>;

static void check(BOOL success, const char* step) {
    if (!success) {
        const DWORD code = GetLastError();
        throw std::runtime_error(std::string(step) + " 失败，错误码 " + std::to_string(code));
    }
}

static std::string askAI(const std::string& prompt, std::string_view provider,
    const std::string& apiKey, std::string_view model) {
    const bool useDeepSeek = provider == "deepseek";
    if (!useDeepSeek && provider != "openai")
        throw std::runtime_error("AI_PROVIDER 请选择 openai 或 deepseek");
    if (model.empty())
        model = useDeepSeek ? "deepseek-flash" : "gpt-5-mini";

    const wchar_t* host = useDeepSeek ? L"api.deepseek.com" : L"api.openai.com";
    const wchar_t* endpoint = useDeepSeek ? L"/responses" : L"/v1/responses";

    const InternetHandle session(WinHttpOpen(L"Algorithm-Problem-Maker/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, nullptr, nullptr, 0), WinHttpCloseHandle);
    check(session != nullptr, "WinHttpOpen");
    check(WinHttpSetTimeouts(session.get(), 10000, 15000, 30000, 300000), "WinHttpSetTimeouts");

    const InternetHandle connection(WinHttpConnect(session.get(), host,
        INTERNET_DEFAULT_HTTPS_PORT, 0), WinHttpCloseHandle);
    check(connection != nullptr, "WinHttpConnect");

    const InternetHandle request(WinHttpOpenRequest(connection.get(), L"POST",
        endpoint, nullptr, nullptr, nullptr, WINHTTP_FLAG_SECURE), WinHttpCloseHandle);
    check(request != nullptr, "WinHttpOpenRequest");
    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    check(WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY,
        &redirectPolicy, sizeof(redirectPolicy)), "WinHttpSetOption");

    json payload = {{"model", model}, {"input", prompt}};
    if (useDeepSeek)
        payload["text"]["format"] = {{"type", "json_object"}};
    std::string body = payload.dump();
    const std::wstring headers = L"Content-Type: application/json\r\nAuthorization: Bearer "
        + std::wstring(apiKey.begin(), apiKey.end());
    const DWORD bodySize = static_cast<DWORD>(body.size());
    check(WinHttpSendRequest(request.get(), headers.c_str(), static_cast<DWORD>(headers.size()),
        body.data(), bodySize, bodySize, 0), "WinHttpSendRequest");
    check(WinHttpReceiveResponse(request.get(), nullptr), "WinHttpReceiveResponse");

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    check(WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        nullptr, &status, &statusSize, nullptr), "WinHttpQueryHeaders");

    std::string response;
    char buffer[4096];
    DWORD bytesRead = 0;
    do {
        check(WinHttpReadData(request.get(), buffer, sizeof(buffer), &bytesRead), "WinHttpReadData");
        response.append(buffer, bytesRead);
    } while (bytesRead != 0);

    if (status != 200)
        throw std::runtime_error("API 请求失败，HTTP 状态码 " + std::to_string(status));

    const json result = json::parse(response);
    if (result.value("status", "") != "completed")
        throw std::runtime_error("AI 响应未完成");

    std::string text;
    for (const auto& item : result.at("output")) {
        if (item.value("type", "") != "message")
            continue;
        for (const auto& part : item.at("content")) {
            if (part.value("type", "") == "output_text")
                text += part.at("text").get<std::string>();
        }
    }
    if (text.find_first_not_of(" \t\r\n") == std::string::npos)
        throw std::runtime_error("AI 响应中没有文本");
    return text;
}

nlohmann::json generateProblem(const std::string& input, std::string_view provider,
    const std::string& apiKey, std::string_view model) {
    const json prompt = json::parse(input);
    if (!prompt.contains("task") || !prompt.at("task").is_string() ||
        !prompt.contains("files") || !prompt.at("files").is_object())
        throw std::runtime_error("in.json 需要 task 字符串和 files 对象");
    json output = json::parse(askAI(input, provider, apiKey, model));
    for (const auto& item : problemFiles) {
        if (!output.contains(item.key) || !output.at(item.key).is_string())
            throw std::runtime_error(std::string("AI 回复缺少文本字段: ") + item.key);
    }
    return output;
}

void workProblem() {
    const fs::path project = projectDirectory();
    const std::string input = readText(project / L"in.json");
    const char* configuredProvider = std::getenv("AI_PROVIDER");
    const std::string_view provider = configuredProvider && *configuredProvider ? configuredProvider : "openai";
    if (provider != "openai" && provider != "deepseek")
        throw std::runtime_error("AI_PROVIDER 请选择 openai 或 deepseek");

    const char* keyVariable = provider == "deepseek" ? "DEEPSEEK_API_KEY" : "OPENAI_API_KEY";
    const char* apiKey = std::getenv(keyVariable);
    if (!apiKey || !*apiKey)
        throw std::runtime_error(std::string("请先设置环境变量 ") + keyVariable);
    const char* model = std::getenv(provider == "deepseek" ? "DEEPSEEK_MODEL" : "OPENAI_MODEL");

    json output = generateProblem(input, provider, apiKey, model ? model : "");
    writeText(project / L"out.json", output.dump(2) + '\n');
}
