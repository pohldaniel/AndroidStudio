#pragma once

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct BoneDescription {
	BoneDescription();
	~BoneDescription();
	std::string name;
	glm::vec3 initialPosition;
	glm::quat initialRotation;
	glm::vec3 initialScale;
	glm::mat4 offsetMatrix;
	size_t parentIndex;
	float radius;
};