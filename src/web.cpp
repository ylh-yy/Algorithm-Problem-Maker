#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include "httplib.h"
#include "problem.h"
#include "ai.h"

#include <iostream>

using json = nlohmann::json;

static void sendJson(httplib::Response& response, const json& body, int status = 200) {
    response.status = status;
    response.set_content(body.dump(-1, ' ', false, json::error_handler_t::replace), "application/json; charset=utf-8");
}

static bool validFiles(const json& files) {
    if (!files.is_object())
        return false;
    for (const auto& file : files) {
        if (!file.is_string())
            return false;
    }
    return true;
}

static void generate(const httplib::Request& request, httplib::Response& response, const fs::path& taskFile) {
    if (request.get_header_value("Content-Type").rfind("application/json", 0) != 0) {
        sendJson(response, {{"error", "请提交 JSON 格式的材料。"}}, 415);
        return;
    }

    const std::string authorization = request.get_header_value("Authorization");
    if (authorization.rfind("Bearer ", 0) != 0 || authorization.size() <= 7 || authorization.size() > 512 ||
        authorization.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~+/=",
            7) != std::string::npos) {
        sendJson(response, {{"error", "请填写有效的 API Key。"}}, 400);
        return;
    }

    const json input = json::parse(request.body, nullptr, false);
    if (!input.contains("provider") || !input.at("provider").is_string() ||
        !input.contains("files") || !validFiles(input.at("files")) || input.at("files").empty() ||
        (input.contains("example") && !validFiles(input.at("example")))) {
        sendJson(response, {{"error", "请提供 API 服务和至少一份题目材料。"}}, 400);
        return;
    }
    const std::string provider = input.at("provider").get<std::string>();
    if (provider != "openai" && provider != "deepseek") {
        sendJson(response, {{"error", "请选择 OpenAI 或 DeepSeek。"}}, 400);
        return;
    }

    try {
        const json prompt = {
            {"task", readText(taskFile)},
            {"files", input.at("files")},
            {"example", input.value("example", json::object())}
        };
        const json files = generateProblem(prompt.dump(2), provider, authorization.substr(7));
        sendJson(response, {{"files", files}});
    } catch (const json::exception&) {
        sendJson(response, {{"error", "AI 回复无法解析为题目 JSON，请重试。"}}, 502);
    } catch (const std::exception& error) {
        sendJson(response, {{"error", error.what()}}, 502);
    }
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);

    try {
        int port = 8787;
        if (argc > 2)
            throw std::runtime_error("用法: web.exe [端口]");
        if (argc == 2) {
            std::size_t end = 0;
            port = std::stoi(argv[1], &end);
            if (argv[1][end] != '\0' || port < 1024 || port > 65535)
                throw std::runtime_error("端口应在 1024 到 65535 之间");
        }

        const fs::path project = projectDirectory();
        const fs::path taskFile = project / L"task.txt";
        const std::string address = "127.0.0.1:" + std::to_string(port);
        const std::string localName = "localhost:" + std::to_string(port);
        httplib::Server server;
        if (!server.set_mount_point("/", (project / L"web").u8string()))
            throw std::runtime_error("找不到网页目录 web");
        server.set_payload_max_length(8 * 1024 * 1024);
        server.set_default_headers({
            {"Cache-Control", "no-store"},
            {"Content-Security-Policy", "default-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'; img-src 'self' data:"},
            {"X-Content-Type-Options", "nosniff"}
        });
        server.set_pre_routing_handler([&](const httplib::Request& request, httplib::Response& response) {
            const std::string host = request.get_header_value("Host");
            const std::string origin = request.get_header_value("Origin");
            if ((host != address && host != localName) || (!origin.empty() && origin != "http://" + host)) {
                sendJson(response, {{"error", "请通过本机网页地址访问。"}}, 403);
                return httplib::Server::HandlerResponse::Handled;
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });
        server.Post("/api/generate", [&](const httplib::Request& request, httplib::Response& response) {
            generate(request, response, taskFile);
        });
        server.set_error_handler([](const httplib::Request&, httplib::Response& response) {
            if (!response.body.empty())
                return;
            if (response.status == 413)
                sendJson(response, {{"error", "上传材料超过 8 MiB。"}}, 413);
            else
                sendJson(response, {{"error", "网页请求失败。"}}, response.status);
        });

        if (!server.bind_to_port("127.0.0.1", port))
            throw std::runtime_error("无法监听端口 " + std::to_string(port));
        std::cout << "出题机网页 http://" << address << "/" << std::endl;
        if (!server.listen_after_bind())
            throw std::runtime_error("网页服务已停止");
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
    return 0;
}
