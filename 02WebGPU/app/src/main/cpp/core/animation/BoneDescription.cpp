#include "BoneDescription.h"

BoneDescription::BoneDescription() :
	initialPosition(glm::vec3(0.0f, 0.0f, 0.0f)),
	initialRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
	initialScale(glm::vec3(1.0f, 1.0f, 1.0f)),
	offsetMatrix(glm::mat4(1.0f)),
	parentIndex(0u),
	radius(0.0f){
}

BoneDescription::~BoneDescription(){

}