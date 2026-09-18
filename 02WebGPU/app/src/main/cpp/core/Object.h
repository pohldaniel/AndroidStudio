#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

class Object2D {

public:

	Object2D();
	Object2D(Object2D const& rhs);
	Object2D& operator=(const Object2D& rhs);
	Object2D(Object2D&& rhs) noexcept;
	Object2D& operator=(Object2D&& rhs) noexcept;
	virtual ~Object2D();

	virtual void setScale(float sx, float sy);
	virtual void setScale(const glm::vec2& scale);
	virtual void setScale(float s);

	virtual void setPosition(float x, float y);
	virtual void setPosition(const glm::vec2& position);

	virtual void setOrientation(float degrees);

	virtual void translate(const glm::vec2& trans);
	virtual void translate(float dx, float dy);

	virtual void translateRelative(const glm::vec2& trans);
	virtual void translateRelative(float dx, float dy);

	virtual void scale(const glm::vec2& scale);
	virtual void scale(float sx, float sy);
	virtual void scale(float s);

	virtual void rotate(float degrees);

	const glm::vec2& getPosition() const;
	const glm::vec2& getScale() const;
	float getOrientation() const;

	const glm::mat4& getTransformationSOP() const;
	const glm::mat4& getTransformationSO() const;
	const glm::mat4& getTransformationSP() const;
	const glm::mat4& getTransformationOP() const;
	const glm::mat4& getTransformationO() const;
	const glm::mat4& getTransformationP() const;
	const glm::mat4& getTransformationS() const;

	static const glm::mat4 &GetTransformation();

protected:

	glm::vec2 m_position;
	glm::vec2 m_scale;
	float m_orientation;

	static thread_local glm::mat4 Transformation;
};

class Object {

public:

	Object();
	Object(Object const& rhs);
	Object& operator=(const Object& rhs);
	Object(Object&& rhs) noexcept;
	Object& operator=(Object&& rhs) noexcept;

	virtual void setScale(float sx, float sy, float sz) const;
	virtual void setScale(const glm::vec3& scale) const;
	virtual void setScale(float s) const;

	virtual void setPosition(float x, float y, float z) const;
	virtual void setPosition(const glm::vec3& position) const;

	virtual void setOrientation(const glm::vec3& axis, float degrees) const;
	virtual void setOrientation(float degreesX, float degreesY, float degreesZ) const;
	virtual void setOrientation(const glm::vec3& eulerAngle) const;
	virtual void setOrientation(const glm::quat& orientation) const;
	virtual void setOrientation(float x, float y, float z, float w) const;

	virtual void translate(const glm::vec3& trans);
	virtual void translate(float dx, float dy, float dz);

	virtual void translateRelative(const glm::vec3& trans);
	virtual void translateRelative(float dx, float dy, float dz);
	
	virtual void scale(const glm::vec3& scale);
	virtual void scale(float sx, float sy, float sz);
	virtual void scale(float s);

	virtual void rotate(float pitch, float yaw, float roll);
	virtual void rotate(const glm::vec3& eulerAngle);
	virtual void rotate(const glm::vec3& axis, float degrees);
	virtual void rotate(const glm::quat& orientation);
	virtual void rotate(float x, float y, float z, float w);

	virtual const glm::vec3& getPosition() const;
	virtual const glm::vec3& getScale() const;
	virtual const glm::quat& getOrientation() const;
	glm::vec3& getPosition();
	glm::vec3& getScale();
	glm::quat& getOrientation();

	const glm::mat4& getTransformationSOP() const;
	const glm::mat4& getTransformationSO() const;
	const glm::mat4& getTransformationSP() const;
	const glm::mat4& getTransformationOP() const;
	const glm::mat4& getTransformationO() const;
	const glm::mat4& getTransformationP() const;
	const glm::mat4& getTransformationS() const;

	static const glm::mat4 &GetTransformation();

protected:

	mutable glm::vec3 m_position;
	mutable glm::vec3 m_scale;
	mutable glm::quat m_orientation;

	static thread_local glm::mat4 Transformation;
};