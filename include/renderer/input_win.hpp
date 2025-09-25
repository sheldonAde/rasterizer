#pragma once
// Windows 键盘输入（GetAsyncKeyState），仅在 _WIN32 下有效
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <array>
#include <cstdint>


struct KeyInput {
	std::array<uint8_t, 256> prev{};
	std::array<uint8_t, 256> curr{};
	void update() {
		prev = curr;
		for (int vk = 0; vk < 256; ++vk) curr[vk] = (GetAsyncKeyState(vk) & 0x8000) ? 1 : 0;
	}
	bool down(int vk) const { return curr[vk] != 0; }
	bool pressed(int vk)const { return curr[vk] && !prev[vk]; }
};
#endif