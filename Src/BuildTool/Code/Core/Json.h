#pragma once

#include <rapidjson/document.h>

#include <utility>

namespace SE::BuildTool::Json
{
    using Document = rapidjson::Document;
    using Value = rapidjson::Value;
    using Array = decltype(std::declval<rapidjson::Document&>().GetArray());
}
