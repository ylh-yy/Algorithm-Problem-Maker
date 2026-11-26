#pragma once

#include "json.hpp"

#include <string>
#include <string_view>

nlohmann::json generateProblem(const std::string& input, std::string_view provider,
    const std::string& apiKey, std::string_view model = {});
