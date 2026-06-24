#include "Transform.h"

Transform::Transform() {
	T = glm::mat4(1.0f);
	invT = glm::mat4(1.0f);
}

Transform::Transform(const glm::mat4& m) {
	fromMatrix(m);
}

Transform::Transform(Transform const& rhs) {
	T = rhs.T;
	invT = rhs.invT;
}

Transform::Transform(Transform&& rhs) noexcept{
	T = rhs.T;
	invT = rhs.invT;
}

Transform &Transform::operator=(const Transform& rhs) {
	T = rhs.T;
	invT = rhs.invT;
	return *this;
}

Transform &Transform::operator=(Transform&& rhs) noexcept{
    T = rhs.T;
	invT = rhs.invT;
	return *this;
}

void Transform::reset() {
	T = glm::mat4(1.0f);
	invT = glm::mat4(1.0f);
}

Transform::~Transform() {

}

void Transform::rotate(const glm::vec3& axis, float degrees, bool inPlace) {
	glm::mat4 rotMtx = glm::rotate(glm::radians(degrees), axis);

	if(inPlace){
		rotMtx[3][0] = T[3][0] * (1.0f - rotMtx[0][0]) - T[3][1] * rotMtx[1][0] - T[3][2] * rotMtx[2][0];
		rotMtx[3][1] = T[3][1] * (1.0f - rotMtx[1][1]) - T[3][0] * rotMtx[0][1] - T[3][2] * rotMtx[2][1];
		rotMtx[3][2] = T[3][2] * (1.0f - rotMtx[2][2]) - T[3][0] * rotMtx[0][2] - T[3][1] * rotMtx[1][2];
		rotMtx[3][3] = 1.0f;
	}

	T = rotMtx * T;
}

void Transform::rotate(const glm::quat& quat, bool inPlace) {
    glm::mat4 rotMtx  = glm::toMat4(quat);
	if(inPlace){
		rotMtx[3][0] = T[3][0] * (1.0f - rotMtx[0][0]) - T[3][1] * rotMtx[1][0] - T[3][2] * rotMtx[2][0];
		rotMtx[3][1] = T[3][1] * (1.0f - rotMtx[1][1]) - T[3][0] * rotMtx[0][1] - T[3][2] * rotMtx[2][1];
		rotMtx[3][2] = T[3][2] * (1.0f - rotMtx[2][2]) - T[3][0] * rotMtx[0][2] - T[3][1] * rotMtx[1][2];
		rotMtx[3][3] = 1.0f;
	}

	T = rotMtx * T;
}

void Transform::rotate(float pitch, float yaw, float roll, bool inPlace) {
	glm::mat4 rotMtx = glm::yawPitchRoll(glm::radians(yaw), glm::radians(pitch), glm::radians(roll));
	if(inPlace){
		rotMtx[3][0] = T[3][0] * (1.0f - rotMtx[0][0]) - T[3][1] * rotMtx[1][0] - T[3][2] * rotMtx[2][0];
		rotMtx[3][1] = T[3][1] * (1.0f - rotMtx[1][1]) - T[3][0] * rotMtx[0][1] - T[3][2] * rotMtx[2][1];
		rotMtx[3][2] = T[3][2] * (1.0f - rotMtx[2][2]) - T[3][0] * rotMtx[0][2] - T[3][1] * rotMtx[1][2];
		rotMtx[3][3] = 1.0f;
	}

	T = rotMtx * T ;
}

void Transform::rotate(const glm::vec3& axis, float degrees, const glm::vec3& centerOfRotation) {
	glm::mat4  rotMtx = glm::rotate(glm::radians(degrees), axis);
	rotMtx[3][0] = centerOfRotation[0] * (1.0f - rotMtx[0][0]) - centerOfRotation[1] * rotMtx[1][0] - centerOfRotation[2] * rotMtx[2][0];
	rotMtx[3][1] = centerOfRotation[1] * (1.0f - rotMtx[1][1]) - centerOfRotation[0] * rotMtx[0][1] - centerOfRotation[2] * rotMtx[2][1];
	rotMtx[3][2] = centerOfRotation[2] * (1.0f - rotMtx[2][2]) - centerOfRotation[0] * rotMtx[0][2] - centerOfRotation[1] * rotMtx[1][2];
	T = rotMtx * T;
}

void Transform::rotate(const glm::quat& quat, const glm::vec3& centerOfRotation) {
    glm::mat4 rotMtx =  glm::toMat4(quat);
	rotMtx[3][0] = centerOfRotation[0] * (1.0f - rotMtx[0][0]) - centerOfRotation[1] * rotMtx[1][0] - centerOfRotation[2] * rotMtx[2][0];
	rotMtx[3][1] = centerOfRotation[1] * (1.0f - rotMtx[1][1]) - centerOfRotation[0] * rotMtx[0][1] - centerOfRotation[2] * rotMtx[2][1];
	rotMtx[3][2] = centerOfRotation[2] * (1.0f - rotMtx[2][2]) - centerOfRotation[0] * rotMtx[0][2] - centerOfRotation[1] * rotMtx[1][2];
	T = rotMtx * T;
}

void Transform::rotate(float pitch, float yaw, float roll, const glm::vec3& centerOfRotation) {
	glm::mat4 rotMtx = glm::yawPitchRoll(glm::radians(yaw), glm::radians(pitch), glm::radians(roll));
	rotMtx[3][0] = centerOfRotation[0] * (1.0f - rotMtx[0][0]) - centerOfRotation[1] * rotMtx[1][0] - centerOfRotation[2] * rotMtx[2][0];
	rotMtx[3][1] = centerOfRotation[1] * (1.0f - rotMtx[1][1]) - centerOfRotation[0] * rotMtx[0][1] - centerOfRotation[2] * rotMtx[2][1];
	rotMtx[3][2] = centerOfRotation[2] * (1.0f - rotMtx[2][2]) - centerOfRotation[0] * rotMtx[0][2] - centerOfRotation[1] * rotMtx[1][2];

	T = rotMtx * T;
}

void Transform::translate(float dx, float dy, float dz) {
	T[0][0] = T[0][0] + T[0][3] * dx; T[0][1] = T[0][1] + T[0][3] * dz; T[0][2] = T[0][2] + T[0][3] * dy;
	T[1][0] = T[1][0] + T[1][3] * dx; T[1][1] = T[1][1] + T[1][3] * dz; T[1][2] = T[1][2] + T[1][3] * dy;
	T[2][0] = T[2][0] + T[2][3] * dx; T[2][1] = T[2][1] + T[2][3] * dz; T[2][2] = T[2][2] + T[2][3] * dy;
	T[3][0] = T[3][0] + dx; T[3][1] = T[3][1] + dy; T[3][2] = T[3][2] + dz;
}

void Transform::translate(const glm::vec3& trans) {
	T[0][0] = T[0][0] + T[0][3] * trans[0]; T[0][1] = T[0][1] + T[0][3] * trans[2]; T[0][2] = T[0][2] + T[0][3] * trans[1];
	T[1][0] = T[1][0] + T[1][3] * trans[0]; T[1][1] = T[1][1] + T[1][3] * trans[2]; T[1][2] = T[1][2] + T[1][3] * trans[1];
	T[2][0] = T[2][0] + T[2][3] * trans[0]; T[2][1] = T[2][1] + T[2][3] * trans[2]; T[2][2] = T[2][2] + T[2][3] * trans[1];
	T[3][0] = T[3][0] + trans[0]; T[3][1] = T[3][1] + trans[1]; T[3][2] = T[3][2] + trans[2];
}

void Transform::scale(float s) {
	scale(s, s, s);
}

void Transform::scale(float a, float b, float c) {
	T[0][0] = T[0][0] * a;  T[1][0] = T[1][0] * b; T[2][0] = T[2][0] * c;
	T[0][1] = T[0][1] * a;  T[1][1] = T[1][1] * b; T[2][1] = T[2][1] * c;
	T[0][2] = T[0][2] * a;  T[1][2] = T[1][2] * b; T[2][2] = T[2][2] * c;
}

void Transform::scale(const glm::vec3& scale) {
	T[0][0] = T[0][0] * scale[0];  T[1][0] = T[1][0] * scale[1]; T[2][0] = T[2][0] * scale[2];
	T[0][1] = T[0][1] * scale[0];  T[1][1] = T[1][1] * scale[1]; T[2][1] = T[2][1] * scale[2];
	T[0][2] = T[0][2] * scale[0];  T[1][2] = T[1][2] * scale[1]; T[2][2] = T[2][2] * scale[2];
}

void Transform::scale(float s, const glm::vec3& centerOfScale) {
	scale(s, s, s, centerOfScale);
}

void Transform::scale(float a, float b, float c, const glm::vec3& centerOfScale) {
	T[3][0] = T[0][0] * centerOfScale[0] * (1.0f - a) + T[1][0] * centerOfScale[1] * (1.0f - b) + T[2][0] * centerOfScale[2] * (1.0f - c) + T[3][0];
	T[3][1] = T[0][1] * centerOfScale[0] * (1.0f - a) + T[1][1] * centerOfScale[1] * (1.0f - b) + T[2][1] * centerOfScale[2] * (1.0f - c) + T[3][1];
	T[3][2] = T[0][2] * centerOfScale[0] * (1.0f - a) + T[1][2] * centerOfScale[1] * (1.0f - b) + T[2][2] * centerOfScale[2] * (1.0f - c) + T[3][2];

	T[0][0] = T[0][0] * a;  T[1][0] = T[1][0] * b; T[2][0] = T[2][0] * c;
	T[0][1] = T[0][1] * a;  T[1][1] = T[1][1] * b; T[2][1] = T[2][1] * c;
	T[0][2] = T[0][2] * a;  T[1][2] = T[1][2] * b; T[2][2] = T[2][2] * c;
}

void Transform::scale(const glm::vec3& scale, const glm::vec3& centerOfScale) {
	T[3][0] = T[0][0] * centerOfScale[0] * (1.0f - scale[0]) + T[1][0] * centerOfScale[1] * (1.0f - scale[1]) + T[2][0] * centerOfScale[2] * (1.0f - scale[2]) + T[3][0];
	T[3][1] = T[0][1] * centerOfScale[0] * (1.0f - scale[0]) + T[1][1] * centerOfScale[1] * (1.0f - scale[1]) + T[2][1] * centerOfScale[2] * (1.0f - scale[2]) + T[3][1];
	T[3][2] = T[0][2] * centerOfScale[0] * (1.0f - scale[0]) + T[1][2] * centerOfScale[1] * (1.0f - scale[1]) + T[2][2] * centerOfScale[2] * (1.0f - scale[2]) + T[3][2];

	T[0][0] = T[0][0] * scale[0];  T[1][0] = T[1][0] * scale[1]; T[2][0] = T[2][0] * scale[2];
	T[0][1] = T[0][1] * scale[0];  T[1][1] = T[1][1] * scale[1]; T[2][1] = T[2][1] * scale[2];
	T[0][2] = T[0][2] * scale[0];  T[1][2] = T[1][2] * scale[1]; T[2][2] = T[2][2] * scale[2];
}

const glm::mat4& Transform::getTransformationMatrix() const {
	return T;
}

//this just works with uniform scaling
const glm::mat4& Transform::getInvTransformationMatrix() const {
	float sx =  glm::length(glm::vec3(T[0][0], T[1][0], T[2][0]));
	float sy =  glm::length(glm::vec3(T[0][1], T[1][1], T[2][1]));
	float sz =  glm::length(glm::vec3(T[0][2], T[1][2], T[2][2]));

	float sxx = sx * sx;
	float sxy = sx * sy;
	float sxz = sx * sz;

	float syy = sy * sy;
	float syz = sy * sz;

	float szz = sz * sz;

	invT[0][0] = T[0][0] * (1.0f / sxx); invT[1][0] = T[0][1] * (1.0f / sxy); invT[2][0] = T[0][2] * (1.0f / sxz); invT[3][0] = -(T[3][0] * T[0][0] * (1.0f / sxx) + T[3][1] * T[0][1] * (1.0f / sxy) + T[3][2] * T[0][2] * (1.0f / sxz));
	invT[0][1] = T[1][0] * (1.0f / sxy); invT[1][1] = T[1][1] * (1.0f / syy); invT[2][1] = T[1][2] * (1.0f / syz); invT[3][1] = -(T[3][0] * T[1][0] * (1.0f / sxy) + T[3][1] * T[1][1] * (1.0f / syy) + T[3][2] * T[1][2] * (1.0f / syz));
	invT[0][2] = T[2][0] * (1.0f / sxz); invT[1][2] = T[2][1] * (1.0f / syz); invT[2][2] = T[2][2] * (1.0f / szz); invT[3][2] = -(T[3][0] * T[2][0] * (1.0f / sxz) + T[3][1] * T[2][1] * (1.0f / syz) + T[3][2] * T[2][2] * (1.0f / szz));
	invT[0][3] = 0.0f;					 invT[1][3] = 0.0f;					  invT[2][3] = 0.0f;				   invT[3][3] = 1.0f;

	return invT;
}

void Transform::getScale(glm::vec3& scale) {
	scale = glm::vec3(glm::length(glm::vec3(T[0][0], T[0][1], T[0][2])), glm::length(glm::vec3(T[1][0], T[1][1], T[1][2])), glm::length(glm::vec3(T[2][0], T[2][1], T[2][2])));
}

void Transform::getPosition(glm::vec3& position) {
	position = glm::vec3(T[3][0], T[3][1], T[3][2]);
}

void Transform::setPosition(float x, float y, float z) {
	T[3][0] = x; T[3][1] = y; T[3][2] = z;
}

void Transform::setPosition(const glm::vec3& position) {
	T[3][0] = position[0]; T[3][1] = position[1]; T[3][2] = position[2];
}

void Transform::getOrientation(glm::mat4& orientation) {
	float sx = glm::length(glm::vec3(T[0][0], T[1][0], T[2][0]));
	float sy = glm::length(glm::vec3(T[0][1], T[1][1], T[2][1]));
	float sz = glm::length(glm::vec3(T[0][2], T[1][2], T[2][2]));

	orientation[0][0] = T[0][0] * (1.0f / sx); orientation[0][1] = T[0][1] * (1.0f / sx); orientation[0][2] = T[0][2] * (1.0f / sx); orientation[0][3] = 0.0f;
	orientation[1][0] = T[1][0] * (1.0f / sy); orientation[1][1] = T[1][1] * (1.0f / sy); orientation[1][2] = T[1][2] * (1.0f / sy); orientation[1][3] = 0.0f;
	orientation[2][0] = T[2][0] * (1.0f / sz); orientation[2][1] = T[2][1] * (1.0f / sz); orientation[2][2] = T[2][2] * (1.0f / sz); orientation[2][3] = 0.0f;
	orientation[3][0] = 0.0f;				   orientation[3][1] = 0.0f;				  orientation[3][2] = 0.0f;				     orientation[3][3] = 1.0f;
}

void Transform::getOrientation(glm::quat& orientation) {
	float sx = glm::length(glm::vec3(T[0][0], T[1][0], T[2][0]));
	float sy = glm::length(glm::vec3(T[0][1], T[1][1], T[2][1]));
	float sz = glm::length(glm::vec3(T[0][2], T[1][2], T[2][2]));

	glm::mat3 rot;
	rot[0][0] = T[0][0] * (1.0f / sx); rot[0][1] = T[0][1] * (1.0f / sx); rot[0][2] = T[0][2] * (1.0f / sx);
	rot[1][0] = T[1][0] * (1.0f / sy); rot[1][1] = T[1][1] * (1.0f / sy); rot[1][2] = T[1][2] * (1.0f / sy);
	rot[2][0] = T[2][0] * (1.0f / sz); rot[2][1] = T[2][1] * (1.0f / sz); rot[2][2] = T[2][2] * (1.0f / sz);


	orientation = glm::quat_cast(rot);
}

void Transform::fromMatrix(const glm::mat4& m) {
	T[0][0] = m[0][0]; T[0][1] = m[0][1]; T[0][2] = m[0][2]; T[0][3] = m[0][3];
	T[1][0] = m[1][0]; T[1][1] = m[1][1]; T[1][2] = m[1][2]; T[1][3] = m[1][3];
	T[2][0] = m[2][0]; T[2][1] = m[2][1]; T[2][2] = m[2][2]; T[2][3] = m[2][3];
	T[3][0] = m[3][0]; T[3][1] = m[3][1]; T[3][2] = m[3][2]; T[3][3] = m[3][3];	
}

void Transform::apply(const glm::mat4& m) {
	T = m * T;
}