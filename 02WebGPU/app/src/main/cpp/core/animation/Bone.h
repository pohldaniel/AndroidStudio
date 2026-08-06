#pragma once
#include <list>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

class Bone  {

	friend class AnimatedMesh;
	friend class AnimatedModel;
	friend class AnimationState;

public:

	Bone();
	~Bone();
	
	void countChildBones();

	bool animationEnabled() const;
	size_t getNumChildBones() const;

	void setPosition(const glm::vec3& position);
	void setOrientation(const glm::quat& orientation);
	void setOrientation(float pitch, float yaw, float roll);
	void setScale(const glm::vec3& scale);
	void setName(const std::string& name);
	void setParent(Bone* node);
	void setIsRootBone(bool rootBone);
	void setTransformSilent(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
	void setTransform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

	const glm::vec3& getPosition() const;
	const glm::quat& getOrientation() const;
	const glm::vec3& getScale() const;

	bool isRootBone() const;
	void setHasParent(bool hasParent);
	void rotate(float pitch, float yaw, float roll);

	const glm::mat4& getWorldTransformation() const;
	const glm::mat4& getTransformationSOP() const;

protected:

	void OnTransformChanged();
	const std::string& getName() const;

private:
	
	Bone* addChild(Bone* node);
	void eraseChild(Bone* child);
	void eraseAllChildren(size_t offset = 0u);
	Bone* findChild(const std::string& name, bool recursive) const;

	Bone* m_parent;
	std::list<std::unique_ptr<Bone>> m_children;

	size_t m_numChildBones;
	bool m_isRootBone;
	bool m_hasParent;

	mutable bool m_isDirty;
	bool m_animationEnabled;
	
	std::string m_name;
	glm::vec3 m_position;
	glm::vec3 m_scale;
	glm::quat m_orientation;
	glm::mat4 m_offsetMatrix;
	
	mutable glm::mat4 m_modelMatrix;
	static thread_local glm::mat4 Transformation;
};