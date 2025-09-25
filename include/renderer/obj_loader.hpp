#pragma once
// 极简 OBJ 读取（支持 v/vt/vn，三角化，法线缺失则自动生成，单位化到包围盒最大边为 1）
#include <vector>
#include <glm/glm.hpp>

#include "mesh.hpp"


struct OBJMesh {
	std::vector<VertexIn> verts;
	std::vector<glm::ivec3> idx; // 三角形索引
};


bool loadOBJ(const char* path,
    OBJMesh& out,
    bool normalizeToUnit = true,
    bool flipV = true);

bool loadOBJ(const char* path,
    std::vector<VertexIn>& outVerts,
    std::vector<glm::ivec3>& outIdx,
    bool normalizeToUnit = true,
    bool flipV = true);