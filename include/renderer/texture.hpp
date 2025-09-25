#pragma once
// 2D 纹理（8bit RGBA），支持最近点/双线性采样
#include <stb_image.h>
#include <vector>
#include <glm/glm.hpp>

struct Texture2D {
    int w = 0, h = 0, c = 4; // RGBA
    std::vector<unsigned char> data; // 8bit RGBA

    bool load(const char* path); // 在 src/texture.cpp 中实现

    void makeChecker(int W = 512, int H = 512, int grid = 16) {
        w = W; h = H; c = 4; data.resize((size_t)w * h * 4);
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int gx = (x / (w / grid)) % 2;
                int gy = (y / (h / grid)) % 2;
                bool on = (gx ^ gy) != 0;
                unsigned char v = on ? 230 : 40;
                unsigned char* p = &data[(y * w + x) * 4];
                p[0] = v; p[1] = v; p[2] = v; p[3] = 255;
            }
        }
        // UV 参考线
        for (int x = 0; x < w; ++x) {
            int y = h / 2; unsigned char* p = &data[(y * w + x) * 4];
            p[0] = 255; p[1] = 64; p[2] = 64; p[3] = 255;
        }
        for (int y = 0; y < h; ++y) {
            int x = w / 2; unsigned char* p = &data[(y * w + x) * 4];
            p[0] = 64; p[1] = 255; p[2] = 64; p[3] = 255;
        }
    }

    void makeSolid(unsigned char r = 255, unsigned char g = 255, unsigned char b = 255, unsigned char a = 255) {
        w = 1; h = 1; c = 4; data.assign(4, 0);
        data[0] = r; data[1] = g; data[2] = b; data[3] = a;
    }

    static inline float wrap01(float u) {
        u = std::fmod(u, 1.0f); if (u < 0.0f) u += 1.0f; return u;
    }

    glm::vec3 sampleNearest(float u, float v) const {
        if (w <= 0 || h <= 0) return glm::vec3(1, 0, 1);
        u = wrap01(u); v = wrap01(v);
        int x = (int)std::floor(u * (w));
        int y = (int)std::floor(v * (h));
        if (x == w) x = w - 1; if (y == h) y = h - 1;
        const unsigned char* p = &data[(y * w + x) * 4];
        return glm::vec3(p[0], p[1], p[2]) * (1.0f / 255.0f);
    }

    glm::vec3 sampleBilinear(float u, float v) const {
        if (w <= 0 || h <= 0) return glm::vec3(1, 0, 1);
        u = wrap01(u) * (w - 1); v = wrap01(v) * (h - 1);
        int x0 = (int)std::floor(u), y0 = (int)std::floor(v);
        int x1 = (x0 + 1 < w) ? (x0 + 1) : x0; int y1 = (y0 + 1 < h) ? (y0 + 1) : y0;
        float tx = u - x0, ty = v - y0;
        auto texel = [&](int x, int y)->glm::vec3 {
            const unsigned char* p = &data[(y * w + x) * 4];
            return glm::vec3(p[0], p[1], p[2]) * (1.0f / 255.0f);
            };
        glm::vec3 c00 = texel(x0, y0), c10 = texel(x1, y0);
        glm::vec3 c01 = texel(x0, y1), c11 = texel(x1, y1);
        glm::vec3 cx0 = c00 * (1.0f - tx) + c10 * tx;
        glm::vec3 cx1 = c01 * (1.0f - tx) + c11 * tx;
        return cx0 * (1.0f - ty) + cx1 * ty;
    }

    glm::vec3 sample(float u, float v, bool bilinear) const {
        return bilinear ? sampleBilinear(u, v) : sampleNearest(u, v);
    }
};
