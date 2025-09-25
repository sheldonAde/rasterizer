#pragma once
// 简单相机（右手 + ZO 深度）
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


struct Camera {
	glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f);
	float yawDeg = -90.0f;
	float pitchDeg = 0.0f;
	float fovDeg = 60.0f;
	float zNear = 0.1f;
	float zFar = 100.0f;


	glm::vec3 forward() const {
		float yawRad = glm::radians(yawDeg);
		float pitchRad = glm::radians(pitchDeg);
		float cy = std::cos(yawRad), sy = std::sin(yawRad);
		float cp = std::cos(pitchRad), sp = std::sin(pitchRad);
		return glm::normalize(glm::vec3(cy * cp, sp, sy * cp));
	}
	glm::mat4 view() const {
		glm::vec3 f = forward();
		return glm::lookAtRH(pos, pos + f, glm::vec3(0, 1, 0));
	}
	glm::mat4 proj(float aspect) const {
		return glm::perspectiveRH_ZO(glm::radians(fovDeg), aspect, zNear, zFar);
	}
};