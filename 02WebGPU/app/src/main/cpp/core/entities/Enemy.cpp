#include <glm/gtx/norm.hpp>
#include "Enemy.h"

Enemy::Enemy(btCollisionObject* collisionObject, const glm::vec3& target) : CollisionNode(collisionObject), target(target){
    m_collisionObject->setUserPointer(this);
}

Enemy::~Enemy() {

}

void Enemy::update(const float dt) {
    const float monsterSpeed = 0.6f;
    float distanceSq = glm::length2(target - getPosition());

    if (distanceSq < 0.35f)
        return;

    glm::quat rot = glm::quat(glm::vec3(0.0f, getLookAtYRotation(getPosition(), target), 0.0f));
    setOrientation(rot);
    translateRelative(glm::vec3(0.0f, 0.0f, 1.0f) * dt * monsterSpeed);
}

void Enemy::fixedUpdate(float fdt) {
    if (!m_isActive) return;

    const glm::vec3& pos = getPosition();
    const glm::quat& rot = getOrientation();

    m_collisionObject->setWorldTransform(Physics::BtTransform(pos, rot));
}

float Enemy::getLookAtYRotation(const glm::vec3& objectPos, const glm::vec3& targetPos) {
    float dx = targetPos[0] - objectPos[0];
	float dz = targetPos[2] - objectPos[2];

	if (abs(dx) < 0.01f && abs(dz) < 0.01f)
		return 0.0f;

	return std::atan2(dx, dz);
}