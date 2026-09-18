#pragma once

#include <Physics/Physics.h>
#include <scene/CollisionNode.h>
#include "Entity.h"

class Enemy : public CollisionNode, public Entity {

public:

	Enemy(btCollisionObject* collisionObject, const glm::vec3& target);
	~Enemy();

	void update(const float dt) override;
	void fixedUpdate(float fdt) override;
private:

	float getLookAtYRotation(const glm::vec3& objectPos, const glm::vec3& targetPos);

	const glm::vec3& target;
};