#include <SDL.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

#include "renderer/common.hpp"
#include "renderer/buffers.hpp"
#include "renderer/camera.hpp"
#include "renderer/texture.hpp"
#include "renderer/obj_loader.hpp"
#include "renderer/pipeline.hpp"
#include "renderer/raster.hpp"
#include "renderer/light.hpp"
#include "renderer/input_win.hpp"

int main(int argc, char** argv) {
    const int width = 1280, height = 720;
    const int SHADOW_W = 1024, SHADOW_H = 1024;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) { std::printf("SDL_Init Error: %s\n", SDL_GetError()); return 1; }
    SDL_Window* window = SDL_CreateWindow("Software Renderer: OBJ + Texture + Lambert + ShadowMap + Ground", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if (!window) { std::printf("SDL_CreateWindow Error: %s\n", SDL_GetError()); SDL_Quit(); return 1; }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { std::printf("SDL_CreateRenderer Error: %s\n", SDL_GetError()); SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    SDL_Texture* texSDL = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texSDL) { std::printf("SDL_CreateTexture Error: %s\n", SDL_GetError()); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    SDL_SetRelativeMouseMode(SDL_TRUE);

    Framebuffer fb(width, height);
    DepthBuffer  zbuf(width, height);
    DepthBuffer  shadowMap(SHADOW_W, SHADOW_H);
    Camera cam;

    // 命令行： [model.obj] [texture.xxx]
    const char* objPath = nullptr; const char* texPath = nullptr;
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s.size() >= 4 && (s.substr(s.size() - 4) == ".obj" || s.substr(s.size() - 4) == ".OBJ")) objPath = argv[i];
        else texPath = argv[i];
    }

    // 模型纹理：提供则加载，否则棋盘
    Texture2D texModel;
    if (texPath && texModel.load(texPath)) { std::printf("Loaded texture: %s (%dx%d)\n", texPath, texModel.w, texModel.h); }
    else { texModel.makeChecker(512, 512, 16); if (texPath) std::printf("Failed to load texture %s, fallback to checkerboard.\n", texPath); else std::printf("Using generated checkerboard texture.\n"); }

    // 地面（白色）
    Texture2D texWhite; texWhite.makeSolid(255, 255, 255, 255);

    // 网格：优先 OBJ，否则立方体示例
    std::vector<VertexIn> meshVerts; std::vector<glm::ivec3> meshIdx;
    if (objPath) {
        if (loadOBJ(objPath, meshVerts, meshIdx, true, true)) {
            std::printf("Loaded OBJ: %s  verts=%zu  tris=%zu", objPath, meshVerts.size(), meshIdx.size()); }
        else { std::printf("Failed to load OBJ %s, fallback to cube.\n", objPath); }
        }
        if (meshVerts.empty() || meshIdx.empty()) {
            // 24 顶点立方体（每面独立 UV/法线）
            std::vector<VertexIn> cube(24);
            auto V = [&](int i, glm::vec3 p, glm::vec2 uv, glm::vec3 n, glm::vec3 color = glm::vec3(1)) { cube[i].pos = p; cube[i].uv = uv; cube[i].color = color; cube[i].normal = n; };
            V(0, { -0.5f,-0.5f, 0.5f }, { 0,1 }, { 0,0, 1 }); V(1, { 0.5f,-0.5f, 0.5f }, { 1,1 }, { 0,0, 1 }); V(2, { 0.5f, 0.5f, 0.5f }, { 1,0 }, { 0,0, 1 }); V(3, { -0.5f, 0.5f, 0.5f }, { 0,0 }, { 0,0, 1 });
            V(4, { 0.5f,-0.5f,-0.5f }, { 0,1 }, { 0,0,-1 }); V(5, { -0.5f,-0.5f,-0.5f }, { 1,1 }, { 0,0,-1 }); V(6, { -0.5f, 0.5f,-0.5f }, { 1,0 }, { 0,0,-1 }); V(7, { 0.5f, 0.5f,-0.5f }, { 0,0 }, { 0,0,-1 });
            V(8, { 0.5f,-0.5f, 0.5f }, { 0,1 }, { 1,0,0 });  V(9, { 0.5f,-0.5f,-0.5f }, { 1,1 }, { 1,0,0 });  V(10, { 0.5f, 0.5f,-0.5f }, { 1,0 }, { 1,0,0 });  V(11, { 0.5f, 0.5f, 0.5f }, { 0,0 }, { 1,0,0 });
            V(12, { -0.5f,-0.5f,-0.5f }, { 0,1 }, { -1,0,0 }); V(13, { -0.5f,-0.5f, 0.5f }, { 1,1 }, { -1,0,0 }); V(14, { -0.5f, 0.5f, 0.5f }, { 1,0 }, { -1,0,0 }); V(15, { -0.5f, 0.5f,-0.5f }, { 0,0 }, { -1,0,0 });
            V(16, { -0.5f, 0.5f, 0.5f }, { 0,1 }, { 0,1,0 });  V(17, { 0.5f, 0.5f, 0.5f }, { 1,1 }, { 0,1,0 });  V(18, { 0.5f, 0.5f,-0.5f }, { 1,0 }, { 0,1,0 });  V(19, { -0.5f, 0.5f,-0.5f }, { 0,0 }, { 0,1,0 });
            V(20, { -0.5f,-0.5f,-0.5f }, { 0,1 }, { 0,-1,0 }); V(21, { 0.5f,-0.5f,-0.5f }, { 1,1 }, { 0,-1,0 }); V(22, { 0.5f,-0.5f, 0.5f }, { 1,0 }, { 0,-1,0 }); V(23, { -0.5f,-0.5f, 0.5f }, { 0,0 }, { 0,-1,0 });
            std::vector<glm::ivec3> idx = {
                {0,1,2},{0,2,3}, {4,5,6},{4,6,7}, {8,9,10},{8,10,11}, {12,13,14},{12,14,15}, {16,17,18},{16,18,19}, {20,21,22},{20,22,23}
            };
            meshVerts = std::move(cube); meshIdx = std::move(idx);
        }

        // 地面白色方块（位于 y = -2）
        std::vector<VertexIn> groundVerts(4);
        auto GV = [&](int i, glm::vec3 p, glm::vec2 uv, glm::vec3 n) { groundVerts[i].pos = p; groundVerts[i].uv = uv; groundVerts[i].color = glm::vec3(1.0f); groundVerts[i].normal = n; };
        float gHalf = 2.0f, gy = -2.0f;
        GV(0, { -gHalf, gy, -gHalf }, { 0,1 }, { 0,1,0 });
        GV(1, { gHalf, gy, -gHalf }, { 1,1 }, { 0,1,0 });
        GV(2, { gHalf, gy,  gHalf }, { 1,0 }, { 0,1,0 });
        GV(3, { -gHalf, gy,  gHalf }, { 0,0 }, { 0,1,0 });
        std::vector<glm::ivec3> groundIdx = { {0,2,1}, {0,3,2} };

        bool running = true; double freq = (double)SDL_GetPerformanceFrequency(); Uint64 t0 = SDL_GetPerformanceCounter();
        const float mouseSensitivity = 0.12f; bool mouseCaptured = true; bool enableCull = true; bool bilinear = true;

        // 光照与阴影参数
        bool enableLighting = true; bool enableShadows = true;
        ShadingMode mode = ShadingMode::Shaded; // 初始为正常着色
        glm::vec3 lightDirWS = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
        glm::vec3 ambient(0.15f), lightColor(1.0f);

#ifdef _WIN32
        KeyInput keys;
#endif

        while (running) {
            Uint64 t1 = SDL_GetPerformanceCounter(); float dt = float((t1 - t0) / freq); t0 = t1;
            SDL_Event e; int mouseDX = 0, mouseDY = 0;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_MOUSEMOTION && mouseCaptured) { mouseDX += e.motion.xrel; mouseDY += e.motion.yrel; }
            }
            if (mouseCaptured) { cam.yawDeg += mouseDX * mouseSensitivity; cam.pitchDeg -= mouseDY * mouseSensitivity; cam.pitchDeg = std::max(-89.0f, std::min(89.0f, cam.pitchDeg)); }

#ifdef _WIN32
            keys.update();
            if (keys.pressed(VK_ESCAPE)) running = false;
            if (keys.pressed(VK_TAB)) { mouseCaptured = !mouseCaptured; SDL_SetRelativeMouseMode(mouseCaptured ? SDL_TRUE : SDL_FALSE); }
            if (keys.pressed('C')) enableCull = !enableCull;
            if (keys.pressed('B')) bilinear = !bilinear;
            if (keys.pressed('L')) { enableLighting = !enableLighting; std::printf("Lighting: %s\n", enableLighting ? "ON" : "OFF"); }
            if (keys.pressed('H')) {
                enableShadows = !enableShadows; std::printf("Shadows: %s", enableShadows ? "ON" : "OFF"); }
                    if (keys.pressed('M')) { // 着色模式循环：Shaded -> UV -> Depth -> Shaded
                        if (mode == ShadingMode::Shaded) mode = ShadingMode::UV;
                        else if (mode == ShadingMode::UV) mode = ShadingMode::Depth;
                        else mode = ShadingMode::Shaded;
                        const char* name = (mode == ShadingMode::Shaded ? "SHADED" : (mode == ShadingMode::UV ? "UV" : "DEPTH"));
                        std::printf("Mode: %s", name);
                    }

                glm::vec3 fwd = cam.forward(); glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0))); glm::vec3 up = glm::normalize(glm::cross(right, fwd));
                float v = ((keys.down(VK_LSHIFT) || keys.down(VK_RSHIFT)) ? 9.0f : 3.0f) * dt;
                if (keys.down('W')) cam.pos += fwd * v; if (keys.down('S')) cam.pos -= fwd * v;
                if (keys.down('A')) cam.pos -= right * v; if (keys.down('D')) cam.pos += right * v;
                if (keys.down('Q')) cam.pos -= up * v; if (keys.down('E')) cam.pos += up * v;
#endif

                float aspect = float(width) / float(height);

                // 模型自转
                glm::mat4 M_model = glm::rotate(glm::mat4(1.0f), float(SDL_GetTicks64() * 0.001) * 0.5f, glm::vec3(0, 1, 0));
                glm::mat4 M_ground = glm::mat4(1.0f);

                glm::mat4 V = cam.view(); glm::mat4 P = cam.proj(aspect);
                glm::mat4 MVP_model = P * V * M_model; glm::mat4 MVP_ground = P * V * M_ground;
                glm::mat3 normalMat_model = glm::transpose(glm::inverse(glm::mat3(M_model)));
                glm::mat3 normalMat_ground = glm::transpose(glm::inverse(glm::mat3(M_ground)));

                // 光源矩阵（每帧更新）
                glm::mat4 Lview, Lproj, LVP; buildLightMatrices(lightDirWS, Lview, Lproj, LVP);

                // ---------- Shadow Pass ----------
                shadowMap.clear(1.0f);
                std::vector<ShadowVOut> lightModel(meshVerts.size());
                for (size_t i = 0; i < meshVerts.size(); ++i) lightModel[i] = vertexStageLight(meshVerts[i].pos, M_model, LVP, SHADOW_W, SHADOW_H);
                std::vector<ShadowVOut> lightGround(groundVerts.size());
                for (size_t i = 0; i < groundVerts.size(); ++i) lightGround[i] = vertexStageLight(groundVerts[i].pos, M_ground, LVP, SHADOW_W, SHADOW_H);

                bool cullFrontInShadow = true; // 常用正面剔除以减弱 acne
                for (auto t : meshIdx) {
                    const ShadowVOut& A = lightModel[t.x], & B = lightModel[t.y], & C = lightModel[t.z]; ShadowVOut poly[4];
                    int nv = clipTriangleNearZO(A, B, C, poly, SHADOW_W, SHADOW_H);
                    if (nv == 3) rasterTriangleDepth(poly[0], poly[1], poly[2], shadowMap, cullFrontInShadow);
                    else if (nv == 4) { rasterTriangleDepth(poly[0], poly[1], poly[2], shadowMap, cullFrontInShadow); rasterTriangleDepth(poly[0], poly[2], poly[3], shadowMap, cullFrontInShadow); }
                }
                for (auto t : groundIdx) {
                    const ShadowVOut& A = lightGround[t.x], & B = lightGround[t.y], & C = lightGround[t.z]; ShadowVOut poly[4];
                    int nv = clipTriangleNearZO(A, B, C, poly, SHADOW_W, SHADOW_H);
                    if (nv == 3) rasterTriangleDepth(poly[0], poly[1], poly[2], shadowMap, cullFrontInShadow);
                    else if (nv == 4) { rasterTriangleDepth(poly[0], poly[1], poly[2], shadowMap, cullFrontInShadow); rasterTriangleDepth(poly[0], poly[2], poly[3], shadowMap, cullFrontInShadow); }
                }

                // ---------- Camera Pass ----------
                fb.clear(packARGB8(glm::vec3(0.07f, 0.07f, 0.1f))); zbuf.clear(1.0f);

                std::vector<VertexOut> voModel(meshVerts.size());
                for (size_t i = 0; i < meshVerts.size(); ++i) voModel[i] = vertexStage(meshVerts[i], M_model, MVP_model, LVP, normalMat_model, width, height);
                std::vector<VertexOut> voGround(groundVerts.size());
                for (size_t i = 0; i < groundVerts.size(); ++i) voGround[i] = vertexStage(groundVerts[i], M_ground, MVP_ground, LVP, normalMat_ground, width, height);

                for (auto t : meshIdx) {
                    const VertexOut& A = voModel[t.x], & B = voModel[t.y], & C = voModel[t.z]; VertexOut poly[4];
                    int nv = clipTriangleNearZO(A, B, C, poly, width, height);
                    if (nv == 3) rasterTriangleTexShadow(poly[0], poly[1], poly[2], texModel, fb, zbuf, shadowMap, mode, enableCull, bilinear, enableShadows, enableLighting, lightDirWS, ambient, lightColor);
                    else if (nv == 4) {
                        rasterTriangleTexShadow(poly[0], poly[1], poly[2], texModel, fb, zbuf, shadowMap, mode, enableCull, bilinear, enableShadows, enableLighting, lightDirWS, ambient, lightColor);
                        rasterTriangleTexShadow(poly[0], poly[2], poly[3], texModel, fb, zbuf, shadowMap, mode, enableCull, bilinear, enableShadows, enableLighting, lightDirWS, ambient, lightColor);
                    }
                }
                for (auto t : groundIdx) {
                    const VertexOut& A = voGround[t.x], & B = voGround[t.y], & C = voGround[t.z]; VertexOut poly[4];
                    int nv = clipTriangleNearZO(A, B, C, poly, width, height);
                    if (nv == 3) rasterTriangleTexShadow(poly[0], poly[1], poly[2], texWhite, fb, zbuf, shadowMap, mode, enableCull, bilinear, enableShadows, enableLighting, lightDirWS, ambient, lightColor);
                    else if (nv == 4) {
                        rasterTriangleTexShadow(poly[0], poly[1], poly[2], texWhite, fb, zbuf, shadowMap, mode, enableCull, bilinear, enableShadows, enableLighting, lightDirWS, ambient, lightColor);
                        rasterTriangleTexShadow(poly[0], poly[2], poly[3], texWhite, fb, zbuf, shadowMap, mode, enableCull, bilinear, enableShadows, enableLighting, lightDirWS, ambient, lightColor);
                    }
                }

                SDL_UpdateTexture(texSDL, nullptr, fb.pixels.data(), width * sizeof(std::uint32_t));
                SDL_RenderClear(renderer); SDL_RenderCopy(renderer, texSDL, nullptr, nullptr); SDL_RenderPresent(renderer);
            }

            SDL_DestroyTexture(texSDL); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
        }
