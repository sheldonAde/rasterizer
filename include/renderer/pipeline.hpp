#pragma once
// 顶点/裁剪/插值相关
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "mesh.hpp"
#include "buffers.hpp"
#include "common.hpp"

enum class ShadingMode { Shaded, UV, Depth };

struct VertexOut {
    glm::vec4 clip;
    glm::vec3 ndc;
    glm::ivec2 screen;
    float depth01;
    float invW;
    glm::vec3 color;
    glm::vec2 uv;
    glm::vec3 normal; // 世界空间
    glm::vec3 world;  // 世界空间
    glm::vec4 lightClip; // 光空间裁剪坐标
    bool inFront = true;
};

struct ShadowVOut {
    glm::vec4 clip;
    glm::vec3 ndc;
    glm::ivec2 screen;
    float depth01;
    float invW;
    bool inFront = true;
};

// 顶点阶段（相机 + 光）
static inline VertexOut vertexStage(
    const VertexIn& vin,
    const glm::mat4& M, const glm::mat4& MVP,
    const glm::mat4& LVP, const glm::mat3& normalMat,
    int screenW, int screenH)
{
    VertexOut vo{};
    glm::vec4 worldPos = M * glm::vec4(vin.pos, 1.0f);
    vo.world = glm::vec3(worldPos);
    vo.lightClip = LVP * worldPos;
    vo.clip = MVP * glm::vec4(vin.pos, 1.0f);
    vo.inFront = vo.clip.w > 0.0f;
    vo.invW = 1.0f / vo.clip.w;
    vo.ndc = glm::vec3(vo.clip) * vo.invW;
    float sx = (vo.ndc.x * 0.5f + 0.5f) * float(screenW - 1);
    float sy = (1.0f - (vo.ndc.y * 0.5f + 0.5f)) * float(screenH - 1);
    vo.screen = glm::ivec2(int(std::round(sx)), int(std::round(sy)));
    vo.depth01 = vo.ndc.z;
    vo.color = vin.color;
    vo.uv = vin.uv;
    vo.normal = glm::normalize(normalMat * vin.normal);
    return vo;
}

static inline ShadowVOut vertexStageLight(const glm::vec3& posModel, const glm::mat4& M, const glm::mat4& LVP, int W, int H) {
    ShadowVOut o{};
    glm::vec4 worldPos = M * glm::vec4(posModel, 1.0f);
    o.clip = LVP * worldPos;
    o.inFront = (o.clip.w > 0.0f);
    o.invW = 1.0f / o.clip.w;
    o.ndc = glm::vec3(o.clip) * o.invW;
    float sx = (o.ndc.x * 0.5f + 0.5f) * float(W - 1);
    float sy = (1.0f - (o.ndc.y * 0.5f + 0.5f)) * float(H - 1);
    o.screen = glm::ivec2(int(std::round(sx)), int(std::round(sy)));
    o.depth01 = o.ndc.z;
    return o;
}

static inline bool isBackFaceNDC(const glm::vec2& a_ndc, const glm::vec2& b_ndc, const glm::vec2& c_ndc, bool ccwIsFront = true) {
    float areaN = edgeFunction(a_ndc, b_ndc, c_ndc);
    return ccwIsFront ? (areaN >= 0.0f) : (areaN <= 0.0f);
}
static inline bool isBackFaceNDC(const VertexOut& v0, const VertexOut& v1, const VertexOut& v2, bool ccwIsFront = true) {
    return isBackFaceNDC(glm::vec2(v0.ndc), glm::vec2(v1.ndc), glm::vec2(v2.ndc), ccwIsFront);
}
static inline bool isBackFaceNDC(const ShadowVOut& v0, const ShadowVOut& v1, const ShadowVOut& v2, bool ccwIsFront = true) {
    return isBackFaceNDC(glm::vec2(v0.ndc), glm::vec2(v1.ndc), glm::vec2(v2.ndc), ccwIsFront);
}

static inline void perspectiveWeights(float w0, float w1, float w2,
    float invW0, float invW1, float invW2,
    float& l0, float& l1, float& l2) {
    float a0 = w0 * invW0, a1 = w1 * invW1, a2 = w2 * invW2;
    float sum = a0 + a1 + a2;
    if (sum != 0.0f) { l0 = a0 / sum; l1 = a1 / sum; l2 = a2 / sum; }
    else { l0 = l1 = 0.0f; l2 = 1.0f; }
}

// 近平面（ZO：z∈[0,1]）包含测试
template<typename V>
static inline bool insideNearZO(const V& v) {
    return (v.clip.w > 0.0f) && (v.clip.z >= 0.0f);
}

// 边上插值
static inline VertexOut lerpVertexOut(const VertexOut& a, const VertexOut& b, float t, int W, int H) {
    VertexOut o{};
    o.clip = a.clip + t * (b.clip - a.clip);
    o.invW = 1.0f / o.clip.w;
    o.ndc = glm::vec3(o.clip) * o.invW;
    float sx = (o.ndc.x * 0.5f + 0.5f) * float(W - 1);
    float sy = (1.0f - (o.ndc.y * 0.5f + 0.5f)) * float(H - 1);
    o.screen = glm::ivec2(int(std::round(sx)), int(std::round(sy)));
    o.depth01 = o.ndc.z;
    o.color = a.color + t * (b.color - a.color);
    o.uv = a.uv + t * (b.uv - a.uv);
    o.normal = glm::normalize(a.normal + t * (b.normal - a.normal));
    o.world = a.world + t * (b.world - a.world);
    o.lightClip = a.lightClip + t * (b.lightClip - a.lightClip);
    o.inFront = (o.clip.w > 0.0f);
    return o;
}

static inline ShadowVOut lerpShadowVOut(const ShadowVOut& a, const ShadowVOut& b, float t, int W, int H) {
    ShadowVOut o{};
    o.clip = a.clip + t * (b.clip - a.clip);
    o.invW = 1.0f / o.clip.w;
    o.ndc = glm::vec3(o.clip) * o.invW;
    float sx = (o.ndc.x * 0.5f + 0.5f) * float(W - 1);
    float sy = (1.0f - (o.ndc.y * 0.5f + 0.5f)) * float(H - 1);
    o.screen = glm::ivec2(int(std::round(sx)), int(std::round(sy)));
    o.depth01 = o.ndc.z;
    o.inFront = (o.clip.w > 0.0f);
    return o;
}

// Sutherland–Hodgman：仅裁剪近平面（z>=0）
template<typename V>
static std::vector<V> clipPolygonNearZO(const std::vector<V>& input, int W, int H) {
    if (input.empty()) return {};
    std::vector<V> out; out.clear();
    V S = input.back(); bool S_in = insideNearZO(S);
    for (const V& E : input) {
        bool E_in = insideNearZO(E);
        if (E_in) {
            if (S_in) {
                out.push_back(E);
            }
            else {
                float denom = (E.clip.z - S.clip.z);
                float t = (std::abs(denom) < 1e-8f) ? 0.0f : (0.0f - S.clip.z) / denom;
                if constexpr (std::is_same<V, VertexOut>::value) out.push_back(lerpVertexOut(S, E, t, W, H));
                else out.push_back(lerpShadowVOut(S, E, t, W, H));
                out.push_back(E);
            }
        }
        else if (S_in) {
            float denom = (E.clip.z - S.clip.z);
            float t = (std::abs(denom) < 1e-8f) ? 0.0f : (0.0f - S.clip.z) / denom;
            if constexpr (std::is_same<V, VertexOut>::value) out.push_back(lerpVertexOut(S, E, t, W, H));
            else out.push_back(lerpShadowVOut(S, E, t, W, H));
        }
        S = E; S_in = E_in;
    }
    return out;
}

static inline int clipTriangleNearZO(const VertexOut& a, const VertexOut& b, const VertexOut& c,
    VertexOut out[4], int W, int H) {
    std::vector<VertexOut> poly = { a, b, c };
    auto clipped = clipPolygonNearZO(poly, W, H);
    int n = (int)clipped.size(); if (n > 4) n = 4; for (int i = 0; i < n; ++i) out[i] = clipped[i]; return n;
}

static inline int clipTriangleNearZO(const ShadowVOut& a, const ShadowVOut& b, const ShadowVOut& c,
    ShadowVOut out[4], int W, int H) {
    std::vector<ShadowVOut> poly = { a, b, c };
    auto clipped = clipPolygonNearZO(poly, W, H);
    int n = (int)clipped.size(); if (n > 4) n = 4; for (int i = 0; i < n; ++i) out[i] = clipped[i]; return n;
}
