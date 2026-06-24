#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

class Transform {

public:

	Transform();
	Transform(const glm::mat4& m);
	Transform(Transform const& rhs);
	Transform(Transform&& rhs) noexcept;
	Transform &operator=(const Transform& rhs);
    Transform &operator=(Transform&& rhs) noexcept;
	~Transform();

	const glm::mat4& getTransformationMatrix() const;
	const glm::mat4& getInvTransformationMatrix() const;

	void setPosition(float x, float y, float z);
	void setPosition(const glm::vec3& position);

	void translate(float dx, float dy, float dz);
	void translate(const glm::vec3& trans);

	void rotate(const glm::vec3& axis, float degrees, bool inPlace = true);
	void rotate(const glm::quat& quat, bool inPlace = true);
	void rotate(float pitch, float yaw, float roll, bool inPlace = true);

	void rotate(const glm::vec3& axis, float degrees, const glm::vec3& centerOfRotation);
	void rotate(const glm::quat& quat, const glm::vec3& centerOfRotation);
	void rotate(float pitch, float yaw, float roll, const glm::vec3& centerOfRotation);

	void scale(float s);
	void scale(float a, float b, float c);
	void scale(const glm::vec3& scale);

	void scale(float s, const glm::vec3& centerOfScale);
	void scale(float a, float b, float c, const glm::vec3& centerOfScale);
	void scale(const glm::vec3& scale, const glm::vec3& centerOfScale);

	void reset();

	void fromMatrix(const glm::mat4& m);
	void getPosition(glm::vec3& position);
	void getOrientation(glm::mat4& orientation);
	void getOrientation(glm::quat& orientation);
	void getScale(glm::vec3& scale);
	void apply(const glm::mat4& m);

private:

	glm::mat4 T;
	mutable glm::mat4 invT;
};