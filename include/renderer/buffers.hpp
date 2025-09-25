#pragma once
// ÷°ª∫≥Â”Î…Ó∂»ª∫≥Â
#include <vector>
#include <cstdint>


struct Framebuffer {
	int w, h;
	std::vector<std::uint32_t> pixels;
	Framebuffer(int W, int H) : w(W), h(H), pixels(W* H, 0xff000000u) {}
	void clear(std::uint32_t argb) { std::fill(pixels.begin(), pixels.end(), argb); }
	inline void putPixel(int x, int y, std::uint32_t argb) {
		if ((unsigned)x >= (unsigned)w || (unsigned)y >= (unsigned)h) return;
		pixels[y * w + x] = argb;
	}
};


struct DepthBuffer {
	int w, h;
	std::vector<float> z;
	DepthBuffer(int W, int H) : w(W), h(H), z(W* H, 1.0f) {}
	void clear(float v = 1.0f) { std::fill(z.begin(), z.end(), v); }
	inline float& at(int x, int y) { return z[y * w + x]; }
};