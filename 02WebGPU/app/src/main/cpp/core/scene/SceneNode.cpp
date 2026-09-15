#include <iostream>
#include "SceneNode.h"

thread_local glm::vec3 SceneNode::WorldPosition;
thread_local glm::vec3 SceneNode::WorldScale;
thread_local glm::quat SceneNode::WorldOrientation;

SceneNode::SceneNode() : BaseNode() {
	m_modelMatrix = glm::mat4(1.0f);
}

SceneNode::SceneNode(const SceneNode& rhs) : BaseNode(rhs) {

}

SceneNode::SceneNode(SceneNode&& rhs) : BaseNode(rhs) {

}

SceneNode& SceneNode::operator=(const SceneNode& rhs) {
	BaseNode::operator=(rhs);
	return *this;
}

SceneNode& SceneNode::operator=(SceneNode&& rhs) {
	BaseNode::operator=(rhs);
	return *this;
}

const glm::mat4& SceneNode::getWorldTransformation() const {
	if (m_isDirty) {
		m_modelMatrix = getTransformationSOP();
		if (m_parent)
			m_modelMatrix = static_cast<BaseNode*>(m_parent)->getWorldTransformation() * m_modelMatrix;

		m_isDirty = false;
	}

	return m_modelMatrix;
}

void SceneNode::updateWorldTransformation() const {
	if (m_isDirty) {
		m_modelMatrix = getTransformationSOP();
		if (m_parent)
			m_modelMatrix = static_cast<BaseNode*>(m_parent)->getWorldTransformation() * m_modelMatrix;

		m_isDirty = false;
	}
}

const glm::vec3& SceneNode::getWorldPosition(bool update) const {
	if(update)
		WorldPosition = glm::vec3(getWorldTransformation()[3]);
	return WorldPosition;
}

const glm::vec3& SceneNode::getWorldScale(bool update) const {
	if (update){
		glm::mat4 worldMatrix = getWorldTransformation();
        WorldScale.x = glm::length(glm::vec3(worldMatrix[0]));
        WorldScale.y = glm::length(glm::vec3(worldMatrix[1]));
        WorldScale.z = glm::length(glm::vec3(worldMatrix[2]));
	}
	return WorldScale;
}

const glm::quat& SceneNode::getWorldOrientation(bool update) const {
	if (update)
		WorldOrientation = glm::quat_cast(getWorldTransformation());
	return WorldOrientation;
}