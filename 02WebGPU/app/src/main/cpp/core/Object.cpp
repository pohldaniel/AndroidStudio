#include <utility>
#include "Object.h"

thread_local glm::mat4 Object2D::Transformation;

Object2D::Object2D() {
	m_position = glm::vec2(0.0f, 0.0f);
	m_scale = glm::vec2(1.0f, 1.0f);
	m_orientation = 0.0f;
}

Object2D::Object2D(Object2D const& rhs) {
	m_position = rhs.m_position;
	m_scale = rhs.m_scale;
	m_orientation = rhs.m_orientation;
}

Object2D& Object2D::operator=(const Object2D& rhs) {
	m_position = rhs.m_position;
	m_scale = rhs.m_scale;
	m_orientation = rhs.m_orientation;
	return *this;
}

Object2D::Object2D(Object2D&& rhs) noexcept : Object2D(rhs) {
	m_position = std::move(rhs.m_position);
	m_scale = std::move(rhs.m_scale);
	m_orientation = std::move(rhs.m_orientation);
}

Object2D& Object2D::operator=(Object2D&& rhs) noexcept {
	m_position = std::move(rhs.m_position);
	m_scale = std::move(rhs.m_scale);
	m_orientation = std::move(rhs.m_orientation);
	return *this;
}

Object2D::~Object2D() {

}

void Object2D::setScale(float sx, float sy) {
	m_scale[0] = sx;
	m_scale[1] = sy;
}

void Object2D::setScale(const glm::vec2& scale) {
	m_scale = scale;
}

void Object2D::setScale(float s) {
	setScale(s, s);
}

void Object2D::setPosition(float x, float y) {
	m_position[0] = x;
	m_position[1] = y;
}

void Object2D::setPosition(const glm::vec2& position) {
	m_position = position;
}

void Object2D::setOrientation(float degrees) {
	m_orientation = degrees;
}

void Object2D::translate(const glm::vec2& trans) {
	m_position[0] += trans[0];
	m_position[1] += trans[1];
}

void Object2D::translate(float dx, float dy) {
	m_position[0] += dx;
	m_position[1] += dy;
}

void Object2D::rotate(float degrees) {
	m_orientation += degrees;
}

void Object2D::translateRelative(float _dx, float _dy) {

	float angle = glm::radians(m_orientation);
	float cos = cosf(angle);
	float sin = sinf(angle);

	float dx = _dx * cos - _dy * sin;
	float dy = _dx * sin + _dy * cos;

	m_position[0] += dx;
	m_position[1] += dy;
}

void Object2D::translateRelative(const glm::vec2& trans) {
	translateRelative(trans[0], trans[1]);
}

void Object2D::scale(const glm::vec2& scale) {
	m_scale[0] *= scale[0];
	m_scale[1] *= scale[1];
}

void Object2D::scale(float sx, float sy) {
	m_scale[0] *= sx;
	m_scale[1] *= sy;
}

void Object2D::scale(float s) {
	m_scale[0] *= s;
	m_scale[1] *= s;
}

const glm::vec2& Object2D::getPosition() const {
	return m_position;
}

const glm::vec2& Object2D::getScale() const {
	return m_scale;
}

float Object2D::getOrientation() const {
	return m_orientation;
}

const glm::mat4& Object2D::getTransformationSOP() const {
	Transformation = glm::translate(glm::vec3(m_position[0], m_position[1], 0.0f)) * glm::rotate(glm::radians(m_orientation), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::vec3(m_scale[0], m_scale[1], 1.0f));
	return Transformation;
}

const glm::mat4& Object2D::getTransformationSO() const {
	Transformation = glm::rotate(glm::radians(m_orientation), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::vec3(m_scale[0], m_scale[1], 1.0f));
	return Transformation;
}

const glm::mat4& Object2D::getTransformationSP() const {
	Transformation = glm::translate(glm::vec3(m_position[0], m_position[1], 0.0f)) * glm::scale(glm::vec3(m_scale[0], m_scale[1], 1.0f));
	return Transformation;
}

const glm::mat4& Object2D::getTransformationOP()  const {
	Transformation = glm::translate(glm::vec3(m_position[0], m_position[1], 0.0f)) * glm::rotate(glm::radians(m_orientation), glm::vec3(0.0f, 0.0f, 1.0f));
	return Transformation;
}

const glm::mat4& Object2D::getTransformationP() const {
	Transformation = glm::translate(glm::vec3(m_position[0], m_position[1], 0.0f));
	return Transformation;
}

const glm::mat4& Object2D::getTransformationO()  const {
	Transformation = glm::rotate(glm::radians(m_orientation), glm::vec3(0.0f, 0.0f, 1.0f));
	return Transformation;
}

const glm::mat4& Object2D::getTransformationS() const {
	Transformation = glm::scale(glm::vec3(m_scale[0], m_scale[1], 1.0f));
	return Transformation;
}

const glm::mat4& Object2D::GetTransformation() {
	return Transformation;
}

////////////////////////////////////////////////////////////////////////////////////
thread_local glm::mat4 Object::Transformation;

Object::Object() {
	m_position = glm::vec3(0.0f, 0.0f, 0.0f);
	m_scale = glm::vec3(1.0f, 1.0f, 1.0f);	
	m_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
}

Object::Object(Object const& rhs) {
	m_position = rhs.m_position;
	m_scale = rhs.m_scale;
	m_orientation = rhs.m_orientation;	
}

Object& Object::operator=(const Object& rhs) {
	m_position = rhs.m_position;
	m_scale = rhs.m_scale;
	m_orientation = rhs.m_orientation;
	return *this;
}

Object::Object(Object&& rhs) noexcept : Object(rhs) {
	m_position = std::move(rhs.m_position);
	m_scale = std::move(rhs.m_scale);
	m_orientation = std::move(rhs.m_orientation);
}

Object& Object::operator=(Object&& rhs) noexcept {
	m_position = std::move(rhs.m_position);
	m_scale = std::move(rhs.m_scale);
	m_orientation = std::move(rhs.m_orientation);
	return *this;
}

void Object::setScale(float sx, float sy, float sz) const {
	m_scale[0] = sx;
	m_scale[1] = sy;
	m_scale[2] = sz;
}

void Object::setScale(const glm::vec3& scale) const {
	m_scale = scale;
}

void Object::setScale(float s) const {
	m_scale[0] = s;
	m_scale[1] = s;
	m_scale[2] = s;
}

void Object::setPosition(float x, float y, float z) const {
	m_position[0] = x;
	m_position[1] = y;
	m_position[2] = z;
}

void Object::setPosition(const glm::vec3& position) const {
	m_position = position;
}

void Object::setOrientation(const glm::vec3& axis, float degrees) const {
	m_orientation = glm::angleAxis(glm::radians(degrees), axis);
}

void Object::setOrientation(float pitch, float yaw, float roll) const {
    m_orientation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
}

void Object::setOrientation(const glm::vec3& eulerAngle) const {
	m_orientation = glm::quat(glm::vec3(glm::radians(eulerAngle[0]), glm::radians(eulerAngle[1]), glm::radians(eulerAngle[2])));
}

void Object::setOrientation(const glm::quat& orientation) const {
	m_orientation = orientation;
}

void Object::setOrientation(float x, float y, float z, float w) const {
	m_orientation[0] = x;
	m_orientation[1] = y;
	m_orientation[2] = z;
	m_orientation[3] = w;
}

void Object::translate(const glm::vec3& trans) {
	m_position += trans;
}

void Object::translate(float dx, float dy, float dz) {
	m_position[0] += dx;
	m_position[1] += dy;
	m_position[2] += dz;
}

void Object::translateRelative(const glm::vec3& trans) {
	m_position += m_orientation * trans;
}

void Object::translateRelative(float dx, float dy, float dz) {
	 m_position += m_orientation * glm::vec3(dx, dy, dz);
}

void Object::scale(const glm::vec3& scale) {
	m_scale[0] *= scale[0];
	m_scale[1] *= scale[1];
	m_scale[2] *= scale[2];
}

void Object::scale(float sx, float sy, float sz) {
	m_scale[0] *= sx;
	m_scale[1] *= sy;
	m_scale[2] *= sz;
}

void Object::scale(float s) {
	m_scale[0] *= s;
	m_scale[1] *= s;
	m_scale[2] *= s;
}

void Object::rotate(float pitch, float yaw, float roll) {
    m_orientation = m_orientation * glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
}

void Object::rotate(const glm::vec3& eulerAngle) {
	m_orientation = m_orientation * glm::quat(glm::vec3(glm::radians(eulerAngle[0]), glm::radians(eulerAngle[1]), glm::radians(eulerAngle[2])));
}

void Object::rotate(const glm::vec3& axis, float degrees) {
	m_orientation = m_orientation * glm::angleAxis(glm::radians(degrees), axis);
}

void Object::rotate(const glm::quat& orientation) {
	m_orientation *= orientation;
}

void Object::rotate(float x, float y, float z, float w) {
	m_orientation = m_orientation * glm::quat(x, y, z, w);
}

const glm::vec3& Object::getPosition() const {
	return m_position;
}

const glm::vec3& Object::getScale() const {
	return m_scale;
}

const glm::quat& Object::getOrientation() const {
	return m_orientation;
}

glm::vec3& Object::getPosition() {
	return m_position;
}

glm::vec3& Object::getScale() {
	return m_scale;
}

glm::quat& Object::getOrientation() {
	return m_orientation;
}

const glm::mat4& Object::getTransformationSOP() const {
	Transformation = glm::translate(m_position) * glm::toMat4(m_orientation) * glm::scale(m_scale);
	return Transformation;
}

const glm::mat4& Object::getTransformationSO() const {
	Transformation = glm::toMat4(m_orientation) * glm::scale(m_scale);
	return Transformation;
}

const glm::mat4& Object::getTransformationSP() const {
	Transformation = glm::translate(m_position) * glm::scale(m_scale);
	return Transformation;
}

const glm::mat4& Object::getTransformationOP()  const {
	Transformation = glm::translate(m_position) * glm::toMat4(m_orientation);
	return Transformation;
}

const glm::mat4& Object::getTransformationP() const {
	Transformation = glm::translate(m_position);
	return Transformation;
}

const glm::mat4& Object::getTransformationO()  const {
	Transformation = glm::toMat4(m_orientation);
	return Transformation;
}

const glm::mat4& Object::getTransformationS() const {
	Transformation = glm::scale(m_scale);
	return Transformation;
}

const glm::mat4& Object::GetTransformation() {
	return Transformation;
}