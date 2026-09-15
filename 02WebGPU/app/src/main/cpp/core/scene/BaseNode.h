#pragma once

#include <list>
#include <functional>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../Object.h"
#include "Node.h"

class BaseNode : public Node, public Object {

	friend class SceneNode;

public:

	BaseNode();
	BaseNode(const BaseNode& rhs);
	BaseNode& operator=(const BaseNode& rhs);
	BaseNode(BaseNode&& rhs);
	BaseNode& operator=(BaseNode&& rhs);

	virtual const glm::mat4& getWorldTransformation() const = 0;
	virtual const glm::vec3& getWorldPosition(bool update = true) const = 0;
	virtual const glm::vec3& getWorldScale(bool update = true) const = 0;
	virtual const glm::quat& getWorldOrientation(bool update = true) const = 0;

	void setScale(const float sx, const float sy, const float sz) const override;
	void setScale(const glm::vec3& scale) const override;
	void setScale(const float s) const override;

	void setPosition(const float x, const float y, const float z) const override;
	void setPosition(const glm::vec3& position) const override;

	void setOrientation(const glm::vec3& axis, float degrees) const override;
	void setOrientation(const float degreesX, const float degreesY, const float degreesZ) const override;
	void setOrientation(const glm::vec3& euler) const override;
	void setOrientation(const glm::quat& orientation) const override;
	void setOrientation(const float x, const float y, const float z, const float w) const override;

	void translate(const glm::vec3& trans) override;
	void translate(const float dx, const float dy, const float dz) override;

	void translateRelative(const glm::vec3& trans) override;
	void translateRelative(const float dx, const float dy, const float dz) override;

	void scale(const glm::vec3& scale) override;
	void scale(const float sx, const float sy, const float sz) override;
	void scale(const float s) override;

	void rotate(const float pitch, const float yaw, const float roll) override;
	void rotate(const glm::vec3& eulerAngle) override;
	void rotate(const glm::vec3& axis, float degrees) override;
	void rotate(const glm::quat& orientation) override;
	void rotate(const float x, const float y, const float z, const float w) override;

protected:

	virtual void OnTransformChanged() const;
	mutable bool m_isDirty;
};