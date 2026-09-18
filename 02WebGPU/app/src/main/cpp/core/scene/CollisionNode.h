#pragma once

#include <Physics/Physics.h>
#include "SceneNode.h"

class CollisionNode : public SceneNode {

public:

	CollisionNode(btCollisionObject* collisionObject);
	~CollisionNode();

	const glm::mat4& getWorldTransformation() const override;

	using SceneNode::addChild;
	void addChild(CollisionNode* node);
	btCollisionObject* getCollisionObject() const;
	void setActive(bool active);
	bool isActive();

protected:

	btCollisionObject* m_collisionObject;
	bool m_isActive;
};