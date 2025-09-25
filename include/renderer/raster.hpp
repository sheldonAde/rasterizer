#pragma once
// 阴影贴图与主通道栅格化
#include <glm/glm.hpp>
#include "pipeline.hpp"
#include "texture.hpp"
#include "buffers.hpp"
#include "common.hpp"

static inline float shadowPCF(const DepthBuffer& sm, float u, float v, float depth, float bias, int kernel = 1) {
    if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) return 1.0f;
    int w = sm.w, h = sm.h;
    float ux = u * (float)w, vy = v * (float)h;
    int cx = (int)std::floor(ux), cy = (int)std::floor(vy);
    float lit = 0.0f; int count = 0;
    for (int dy = -kernel; dy <= kernel; ++dy) {
        for (int dx = -kernel; dx <= kernel; ++dx) {
            int x = clampT(cx + dx, 0, w - 1);
            int y = clampT(cy + dy, 0, h - 1);
            float zref = sm.z[y * w + x];
            float vis = ((depth - bias) <= zref + 1e-6f) ? 1.0f : 0.0f;
            lit += vis; ++count;
        }
    }
    return (count > 0) ? (lit / (float)count) : 1.0f;
}

static inline void rasterTriangleDepth(const ShadowVOut& V0, const ShadowVOut& V1, const ShadowVOut& V2,
    DepthBuffer& db, bool cullFrontFaces) {
    auto inNDC = [](const ShadowVOut& v) {
        return v.inFront && v.ndc.x >= -1 && v.ndc.x <= 1 && v.ndc.y >= -1 && v.ndc.y <= 1 && v.ndc.z >= 0 && v.ndc.z <= 1;
        };
    if (!inNDC(V0) && !inNDC(V1) && !inNDC(V2)) return;

    bool back = isBackFaceNDC(V0, V1, V2, true);
    if (cullFrontFaces) { if (!back) return; }
    else { if (back) return; }

    ShadowVOut v0 = V0, v1 = V1, v2 = V2;
    glm::vec2 p0(v0.screen), p1(v1.screen), p2(v2.screen);
    float area = edgeFunction(p0, p1, p2); if (area == 0.0f) return; if (area < 0.0f) { std::swap(v1, v2); std::swap(p1, p2); area = -area; }

    int minX = std::max(0, std::min(v0.screen.x, std::min(v1.screen.x, v2.screen.x)));
    int maxX = std::min(db.w - 1, std::max(v0.screen.x, std::max(v1.screen.x, v2.screen.x)));
    int minY = std::max(0, std::min(v0.screen.y, std::min(v1.screen.y, v2.screen.y)));
    int maxY = std::min(db.h - 1, std::max(v0.screen.y, std::max(v1.screen.y, v2.screen.y)));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            glm::vec2 p(float(x) + 0.5f, float(y) + 0.5f);
            float w0 = edgeFunction(p1, p2, p), w1 = edgeFunction(p2, p0, p), w2 = edgeFunction(p0, p1, p);
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                w0 /= area; w1 /= area; w2 /= area;
                float l0, l1, l2; perspectiveWeights(w0, w1, w2, v0.invW, v1.invW, v2.invW, l0, l1, l2);
                float z = l0 * v0.depth01 + l1 * v1.depth01 + l2 * v2.depth01;
                float& zref = db.at(x, y); if (z < zref) zref = z;
            }
        }
    }
}

static inline void rasterTriangleTexShadow(
    const VertexOut& V0, const VertexOut& V1, const VertexOut& V2,
    const Texture2D& tex, Framebuffer& fb, DepthBuffer& db,
    const DepthBuffer& shadowMap,
    ShadingMode mode,
    bool enableCull, bool bilinear,
    bool enableShadows, bool enableLighting,
    const glm::vec3& lightDirWS, const glm::vec3& ambient, const glm::vec3& lightColor)
{
    auto inNDC = [](const VertexOut& v) {
        return v.inFront && v.ndc.x >= -1 && v.ndc.x <= 1 && v.ndc.y >= -1 && v.ndc.y <= 1 && v.ndc.z >= 0 && v.ndc.z <= 1;
        };
    if (!inNDC(V0) && !inNDC(V1) && !inNDC(V2)) return;

    if (enableCull && isBackFaceNDC(V0, V1, V2, true)) return;

    VertexOut v0 = V0, v1 = V1, v2 = V2;
    glm::vec2 p0(v0.screen), p1(v1.screen), p2(v2.screen);
    float area = edgeFunction(p0, p1, p2); if (area == 0.0f) return; if (area < 0.0f) { std::swap(v1, v2); std::swap(p1, p2); area = -area; }

    int minX = std::max(0, std::min(v0.screen.x, std::min(v1.screen.x, v2.screen.x)));
    int maxX = std::min(fb.w - 1, std::max(v0.screen.x, std::max(v1.screen.x, v2.screen.x)));
    int minY = std::max(0, std::min(v0.screen.y, std::min(v1.screen.y, v2.screen.y)));
    int maxY = std::min(fb.h - 1, std::max(v0.screen.y, std::max(v1.screen.y, v2.screen.y)));

    glm::vec3 Ldir = glm::normalize(lightDirWS);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            glm::vec2 p(float(x) + 0.5f, float(y) + 0.5f);
            float w0 = edgeFunction(p1, p2, p), w1 = edgeFunction(p2, p0, p), w2 = edgeFunction(p0, p1, p);
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                w0 /= area; w1 /= area; w2 /= area;
                float l0, l1, l2; perspectiveWeights(w0, w1, w2, v0.invW, v1.invW, v2.invW, l0, l1, l2);

                float z = l0 * v0.depth01 + l1 * v1.depth01 + l2 * v2.depth01;
                float& zref = db.at(x, y);
                if (z < zref) {
                    zref = z;

                    glm::vec2 uv = l0 * v0.uv + l1 * v1.uv + l2 * v2.uv;
                    glm::vec3 colVtx = l0 * v0.color + l1 * v1.color + l2 * v2.color;
                    glm::vec3 normalW = glm::normalize(l0 * v0.normal + l1 * v1.normal + l2 * v2.normal);

                    // 光空间
                    glm::vec4 lclip = l0 * v0.lightClip + l1 * v1.lightClip + l2 * v2.lightClip;
                    glm::vec3 lndc = glm::vec3(lclip) / lclip.w;
                    float u = lndc.x * 0.5f + 0.5f;
                    float v = 1.0f - (lndc.y * 0.5f + 0.5f);
                    float depthL = lndc.z;

                    // 根据模式输出颜色
                    glm::vec3 outColor(0.0f);
                    if (mode == ShadingMode::UV) {
                        outColor = glm::vec3(Texture2D::wrap01(uv.x), Texture2D::wrap01(uv.y), 0.0f);
                    }
                    else if (mode == ShadingMode::Depth) {
                        float d = 1.0f - glm::clamp(z, 0.0f, 1.0f); // 近白远黑（可反转）
                        outColor = glm::vec3(d);
                    }
                    else { // Shaded
                        float NdL = std::max(0.0f, glm::dot(normalW, Ldir));
                        float bias = std::max(0.001f, 0.0025f * (1.0f - NdL));
                        float s = 1.0f; if (enableShadows) s = shadowPCF(shadowMap, u, v, depthL, bias, 1);
                        glm::vec3 texel = tex.sample(uv.x, uv.y, bilinear);
                        float NdotL = std::max(0.0f, glm::dot(normalW, Ldir));
                        glm::vec3 lit = enableLighting ? (ambient + lightColor * (NdotL * s)) : glm::vec3(1.0f);
                        outColor = texel * colVtx * lit;
                    }
                    fb.putPixel(x, y, packARGB8(outColor));
                }
            }
        }
    }
}
