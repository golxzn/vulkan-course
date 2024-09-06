#pragma once

#include <string_view>
#include "core/types.hpp"

namespace vc::core::info {

namespace application {

namespace version {

constexpr u32 major{ 0 };
constexpr u32 minor{ 1 };
constexpr u32 patch{ 0 };
constexpr u32 bits { (major << 22) | (minor << 12) | (patch << 0) };

constexpr std::string_view name{ "0.1.0" };

} // namespace version

constexpr std::string_view name        { "Vulkan Corpse" };
constexpr std::string_view name_version{ "Vulkan Corpse | v0.1.0" };

} // namespace application

} // namespace vc::core::info
