#pragma once
// 通用小工具
#include <cstdint>
#include <algorithm>
#include <glm/glm.hpp>


static inline std::uint32_t packARGB8(const glm::vec3& c, float a = 1.0f) {
	auto clamp01 = [](float v) { return std::max(0.0f, std::min(1.0f, v)); };
	std::uint8_t R = (std::uint8_t)(clamp01(c.r) * 255.0f);
	std::uint8_t G = (std::uint8_t)(clamp01(c.g) * 255.0f);
	std::uint8_t B = (std::uint8_t)(clamp01(c.b) * 255.0f);
	std::uint8_t A = (std::uint8_t)(clamp01(a) * 255.0f);
	return (std::uint32_t(A) << 24) | (std::uint32_t(R) << 16) | (std::uint32_t(G) << 8) | std::uint32_t(B);
}


static inline float edgeFunction(const glm::vec2& a, const glm::vec2& b, const glm::vec2& p) {
	return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}


template <typename T>
static inline const T& clampT(const T& v, const T& lo, const T& hi) {
	return (v < lo) ? lo : (hi < v) ? hi : v;
}