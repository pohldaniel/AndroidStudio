#pragma once

#include <Physics/Physics.h>
#include <scene/CollisionNode.h>
#include "Entity.h"

class Enemy : public CollisionNode, public Entity {

public:

	Enemy(btCollisionObject* collisionObject, const glm::vec3& target);
	~Enemy();

	void update(float dt) override;
	void fixedUpdate(float fdt) override;

	const glm::vec3 getDirection() const;
	void setIsDeath(bool isDeath);
	bool isDeath();

private:

	float getLookAtYRotation(const glm::vec3& objectPos, const glm::vec3& targetPos);

	const glm::vec3& target;
	bool m_isDeath;
};