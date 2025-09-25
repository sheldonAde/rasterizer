#pragma once
// 定向光相机（正交投影）
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


static inline void buildLightMatrices(const glm::vec3& lightDirWS,
	glm::mat4& Lview, glm::mat4& Lproj, glm::mat4& LVP) {
	glm::vec3 dir = glm::normalize(lightDirWS);
	glm::vec3 center(0.0f, -0.25f, 0.0f);
	float dist = 4.0f;
	glm::vec3 eye = center + dir * dist; // 观察方向 = center - eye = -dir
	glm::vec3 up = (std::abs(dir.y) > 0.99f) ? glm::vec3(0, 0, 1) : glm::vec3(0, 1, 0);
	Lview = glm::lookAtRH(eye, center, up);
	float half = 3.0f;
	float nearPlane = 0.1f, farPlane = 10.0f;
	Lproj = glm::orthoRH_ZO(-half, half, -half, half, nearPlane, farPlane);
	LVP = Lproj * Lview;
}