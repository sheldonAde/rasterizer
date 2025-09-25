#include "renderer/obj_loader.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <tuple>
#include <limits>
#include <cctype>
#include <glm/gtx/norm.hpp>

static bool endsWith(const std::string& s, const std::string& suf) {
    if (s.size() < suf.size()) return false; auto n = s.size(), m = suf.size();
    for (size_t i = 0; i < m; ++i) {
        char a = s[n - m + i], b = suf[i];
#ifdef _WIN32
        a = (char)tolower((unsigned char)a); b = (char)tolower((unsigned char)b);
#endif
        if (a != b) return false;
    }
    return true;
}

struct IdxKey { int v, t, n; bool operator==(const IdxKey& o) const { return v == o.v && t == o.t && n == o.n; } };
struct IdxKeyHash {
    size_t operator()(const IdxKey& k) const noexcept {
        size_t h = (size_t)k.v * 73856093u ^ (size_t)(k.t + 1) * 19349663u ^ (size_t)(k.n + 2) * 83492791u;
        return h;
    }
};

static int toIndex(int idx, int count) { if (idx > 0) return idx - 1; if (idx < 0) return count + idx; return -1; }

static IdxKey parseVTN(const std::string& tok, int nv, int nt, int nn) {
    int v = -1, t = -1, n = -1; std::string num; std::vector<int> parts;
    for (size_t i = 0; i <= tok.size(); ++i) {
        if (i == tok.size() || tok[i] == '/') { if (!num.empty()) { parts.push_back(std::stoi(num)); num.clear(); } else parts.push_back(0); }
        else num.push_back(tok[i]);
    }
    if (parts.size() == 1) { v = toIndex(parts[0], nv); }
    else if (parts.size() == 2) { v = toIndex(parts[0], nv); t = toIndex(parts[1], nt); }
    else if (parts.size() >= 3) { v = toIndex(parts[0], nv); t = toIndex(parts[1], nt); n = toIndex(parts[2], nn); }
    return { v, t, n };
}

bool loadOBJ(const char* path, OBJMesh& out, bool normalizeToUnit, bool flipV) {
    std::ifstream fin(path); if (!fin) return false;
    std::vector<glm::vec3> pos; std::vector<glm::vec2> tex; std::vector<glm::vec3> nor;

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line); std::string tag; iss >> tag;
        if (tag == "v") { float x, y, z; iss >> x >> y >> z; pos.emplace_back(x, y, z); }
        else if (tag == "vt") { float u, v; iss >> u >> v; if (flipV) v = 1.0f - v; tex.emplace_back(u, v); }
        else if (tag == "vn") { float x, y, z; iss >> x >> y >> z; nor.emplace_back(glm::normalize(glm::vec3(x, y, z))); }
        else if (tag == "f") {
            std::vector<std::string> tks; std::string tk; while (iss >> tk) tks.push_back(tk); if (tks.size() < 3) continue;
            static std::unordered_map<IdxKey, int, IdxKeyHash> table; table.clear();
            auto getOrCreate = [&](const IdxKey& k)->int {
                auto it = table.find(k); if (it != table.end()) return it->second;
                VertexIn vin{}; vin.color = glm::vec3(1.0f);
                vin.pos = (k.v >= 0 && k.v < (int)pos.size()) ? pos[k.v] : glm::vec3(0);
                vin.uv = (k.t >= 0 && k.t < (int)tex.size()) ? tex[k.t] : glm::vec2(0);
                vin.normal = (k.n >= 0 && k.n < (int)nor.size()) ? nor[k.n] : glm::vec3(0);
                int idx = (int)out.verts.size(); out.verts.push_back(vin); table.emplace(k, idx); return idx;
                };
            std::vector<int> vidx; vidx.reserve(tks.size());
            for (auto& s : tks) { IdxKey k = parseVTN(s, (int)pos.size(), (int)tex.size(), (int)nor.size()); if (k.v < 0 || k.v >= (int)pos.size()) continue; vidx.push_back(getOrCreate(k)); }
            for (size_t i = 2; i < vidx.size(); ++i) out.idx.push_back(glm::ivec3(vidx[0], vidx[i - 1], vidx[i]));
        }
    }

    if (out.verts.empty() || out.idx.empty()) return false;

    bool needNormals = false; for (auto& v : out.verts) { if (glm::length2(v.normal) < 1e-12f) { needNormals = true; break; } }
    if (needNormals) {
        std::vector<glm::vec3> acc(out.verts.size(), glm::vec3(0));
        for (auto& t : out.idx) {
            const glm::vec3& a = out.verts[t.x].pos; const glm::vec3& b = out.verts[t.y].pos; const glm::vec3& c = out.verts[t.z].pos;
            glm::vec3 n = glm::cross(b - a, c - a); float len = glm::length(n); if (len > 0) n /= len;
            acc[t.x] += n; acc[t.y] += n; acc[t.z] += n;
        }
        for (size_t i = 0; i < out.verts.size(); ++i) { glm::vec3 n = acc[i]; if (glm::length2(n) < 1e-12f) n = glm::vec3(0, 0, 1); out.verts[i].normal = glm::normalize(n); }
    }

    if (normalizeToUnit) {
        glm::vec3 mn(std::numeric_limits<float>::max()); glm::vec3 mx(-std::numeric_limits<float>::max());
        for (auto& v : out.verts) { mn = glm::min(mn, v.pos); mx = glm::max(mx, v.pos); }
        glm::vec3 center = (mn + mx) * 0.5f; glm::vec3 size = mx - mn;
        float maxDim = std::max(size.x, std::max(size.y, size.z)); float scale = (maxDim > 0) ? (1.0f / maxDim) : 1.0f;
        for (auto& v : out.verts) v.pos = (v.pos - center) * scale;
    }

    return true;
}

bool loadOBJ(const char* path,
    std::vector<VertexIn>& outVerts,
    std::vector<glm::ivec3>& outIdx,
    bool normalizeToUnit,
    bool flipV) {
    OBJMesh m; if (!loadOBJ(path, m, normalizeToUnit, flipV)) return false;
    outVerts = std::move(m.verts); outIdx = std::move(m.idx); return true;
}
