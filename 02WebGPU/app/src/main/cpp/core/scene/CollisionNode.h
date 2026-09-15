#pragma once

#include <Physics/Physics.h>
#pragma once

#include "SceneNode.h"

class CollisionNode : public SceneNode {

public:

	CollisionNode(btCollisionObject* collisionObject);
	~CollisionNode();

	const glm::mat4& getWorldTransformation() const override;

	using SceneNode::addChild;
	void addChild(CollisionNode* node);
	btCollisionObject* getCollisionObject() const;

protected:

	btCollisionObject* m_collisionObject;
};