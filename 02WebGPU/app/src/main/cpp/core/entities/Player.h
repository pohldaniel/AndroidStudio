#pragma once

#include <Physics/Physics.h>
#include <scene/CollisionNode.h>
#include <animation/AnimatedModel.h>
#include "Entity.h"

class Player : public CollisionNode, public Entity {

public:

	Player(btCollisionObject* collisionObject, AnimatedModel& model);
	~Player();

	void update(float dt) override;
	void fixedUpdate(float fdt) override;

	void translate(float dx, float dy, float dz)  override;
	const glm::vec3& getPosition() const override;

private:

	AnimatedModel& model;
};