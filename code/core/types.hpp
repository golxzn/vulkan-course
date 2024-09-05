#pragma once

#include <cstdint>

namespace vc {

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using b8  = bool;
using b32 = u32;

using f32 = float;
using f64 = double;

[[nodiscard]] constexpr u8 operator""_u8(const u64 value) noexcept {
	return static_cast<u8>(value);
}

[[nodiscard]] constexpr u16 operator""_u16(const u64 value) noexcept {
	return static_cast<u16>(value);
}

[[nodiscard]] constexpr u32 operator""_u32(const u64 value) noexcept {
	return static_cast<u32>(value);
}

[[nodiscard]] constexpr u64 operator""_u64(const u64 value) noexcept {
	return value;
}

[[nodiscard]] constexpr i8 operator""_i8(const u64 value) noexcept {
	return static_cast<i8>(0xFF & value);
}

[[nodiscard]] constexpr i16 operator""_i16(const u64 value) noexcept {
	return static_cast<i16>(0xFFFF & value);
}

[[nodiscard]] constexpr i32 operator""_i32(const u64 value) noexcept {
	return static_cast<i32>(0xFFFFFFFF & value);
}

[[nodiscard]] constexpr i64 operator""_i64(const u64 value) noexcept {
	return value;
}



} // namespace vc
