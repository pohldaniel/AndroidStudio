#pragma once

#include <glm/gtc/quaternion.hpp>

#include "BaseNode.h"

class SceneNode : public BaseNode {

public:

	SceneNode();
	SceneNode(const SceneNode& rhs);
	SceneNode& operator=(const SceneNode& rhs);
	SceneNode(SceneNode&& rhs) noexcept;
	SceneNode& operator=(SceneNode&& rhs) noexcept;

	const glm::mat4& getWorldTransformation() const override;
	const glm::vec3& getWorldPosition(bool update = true) const override;
	const glm::vec3& getWorldScale(bool update = true) const override;
	const glm::quat& getWorldOrientation(bool update = true) const override;
	void updateWorldTransformation() const;

private:

	mutable glm::mat4 m_modelMatrix;
	static thread_local glm::vec3 WorldPosition;
	static thread_local glm::vec3 WorldScale;
	static thread_local glm::quat WorldOrientation;
};