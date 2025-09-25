#pragma once
// ���� OBJ ��ȡ��֧�� v/vt/vn�����ǻ�������ȱʧ���Զ����ɣ���λ������Χ������Ϊ 1��
#include <vector>
#include <glm/glm.hpp>

#include "mesh.hpp"


struct OBJMesh {
	std::vector<VertexIn> verts;
	std::vector<glm::ivec3> idx; // ����������
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