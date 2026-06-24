#include "Camera.h"

const glm::mat4 Camera::BIAS_SHIFT_Z(0.5f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 0.5f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 0.5f, 0.0f,
                                     0.5f, 0.5f, 0.5f, 1.0f);

const glm::mat4 Camera::BIAS(0.5f,  0.0f, 0.0f, 0.0f,
                             0.0f, -0.5f, 0.0f, 0.0f,
                             0.0f,  0.0f, 1.0f, 0.0f,
                             0.5f,  0.5f, 0.0f, 1.0f);

Camera::Camera(){
	
	WORLD_XAXIS = glm::vec3(1.0f, 0.0f, 0.0f);
	WORLD_YAXIS = glm::vec3(0.0f, 1.0f, 0.0f);
	WORLD_ZAXIS = glm::vec3(0.0f, 0.0f, 1.0f);

	m_accumPitchDegrees = 0.0f;
	m_accumYawDegrees = 0.0f;
	m_rotationSpeed = 1.0f;
	m_movingSpeed = 1.0f;
	m_distance = 0.0f;

    m_xAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    m_yAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    m_zAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    m_viewDir = glm::vec3(0.0f, 0.0f, -1.0f);

	m_eye = glm::vec3(0.0f, 0.0f, 0.0f) ;
    m_persMatrix = glm::mat4(1.0f);
	m_invPersMatrix = glm::mat4(1.0f);
	m_orthMatrix = glm::mat4(1.0f);
	m_invOrthMatrix = glm::mat4(1.0f);

	orthogonalize();
	fillTranslationPart();
}

Camera::Camera(const glm::vec3 &eye, const glm::vec3& target, const glm::vec3& up) {

	WORLD_XAXIS = glm::vec3(1.0f, 0.0f, 0.0f);
	WORLD_YAXIS = glm::vec3(0.0f, 1.0f, 0.0f);
	WORLD_ZAXIS = glm::vec3(0.0f, 0.0f, 1.0f);

	m_accumPitchDegrees = 0.0f;
	m_accumYawDegrees = 0.0f;
	m_rotationSpeed = 1.0f;
	m_movingSpeed = 1.0f;
	m_distance = 0.0f;

	m_persMatrix = glm::mat4(1.0f);
	m_invPersMatrix = glm::mat4(1.0f);
	m_orthMatrix = glm::mat4(1.0f);
	m_invOrthMatrix = glm::mat4(1.0f);

	lookAt(eye, target, up);
}

Camera::~Camera() {}

void Camera::perspective(float fovx, float aspect, float znear, float zfar){
    float e = tanf(fovx * 0.5f);
	float xScale = (1.0f / (e * aspect));
	float yScale = (1.0f / e);

	m_persMatrix[0][0] = xScale;
	m_persMatrix[0][1] = 0.0f;
	m_persMatrix[0][2] = 0.0f;
	m_persMatrix[0][3] = 0.0f;

	m_persMatrix[1][0] = 0.0f;
	m_persMatrix[1][1] = yScale;
	m_persMatrix[1][2] = 0.0f;
	m_persMatrix[1][3] = 0.0f;

	m_persMatrix[2][0] = 0.0f;
	m_persMatrix[2][1] = 0.0f;
	m_persMatrix[2][2] = zfar / (znear - zfar);
	m_persMatrix[2][3] = -1.0f;

	m_persMatrix[3][0] = 0.0f;
	m_persMatrix[3][1] = 0.0f;
	m_persMatrix[3][2] = (zfar * znear) / (znear - zfar);
	m_persMatrix[3][3] = 0.0f;	

	m_invPersMatrix[0][0] = e * aspect;
	m_invPersMatrix[0][1] = 0.0f;
	m_invPersMatrix[0][2] = 0.0f;
	m_invPersMatrix[0][3] = 0.0f;

	m_invPersMatrix[1][0] = 0.0f;
	m_invPersMatrix[1][1] = e;
	m_invPersMatrix[1][2] = 0.0f;
	m_invPersMatrix[1][3] = 0.0f;

	m_invPersMatrix[2][0] = 0.0f;
	m_invPersMatrix[2][1] = 0.0f;
	m_invPersMatrix[2][2] = 0.0;
	m_invPersMatrix[2][3] = (znear - zfar) / (zfar * znear);

	m_invPersMatrix[3][0] = 0.0f;
	m_invPersMatrix[3][1] = 0.0f;
	m_invPersMatrix[3][2] = -1.0f;
	m_invPersMatrix[3][3] = 1.0 / znear;
}

void Camera::orthographic(float left, float right, float bottom, float top, float znear, float zfar){
    m_orthMatrix = glm::ortho(left, right, bottom, top, znear, zfar);
	
	m_invOrthMatrix[0][0] = (right - left) * 0.5f;
	m_invOrthMatrix[0][1] = 0.0f;
	m_invOrthMatrix[0][2] = 0.0f;
	m_invOrthMatrix[0][3] = 0.0f;

	m_invOrthMatrix[1][0] = 0.0f;
	m_invOrthMatrix[1][1] = (top - bottom) * 0.5f;
	m_invOrthMatrix[1][2] = 0.0f;
	m_invOrthMatrix[1][3] = 0.0f;

	m_invOrthMatrix[2][0] = 0.0f;
	m_invOrthMatrix[2][1] = 0.0f;
	m_invOrthMatrix[2][2] = (znear - zfar) * 0.5f;
	m_invOrthMatrix[2][3] = 0.0f;

	m_invOrthMatrix[3][0] = (right + left) * 0.5f;
	m_invOrthMatrix[3][1] = (top + bottom) * 0.5f;
	m_invOrthMatrix[3][2] = -(zfar + znear) * 0.5f;
	m_invOrthMatrix[3][3] = 1.0f;
}

void Camera::lookAt(const glm::vec3& eye, const glm::vec3& target, const glm::vec3& up){
	m_eye = eye;
	m_target = target;
	m_distance = (m_target - m_eye).length();

	m_zAxis = glm::normalize(m_eye - target);
	m_xAxis = glm::normalize(glm::cross(up, m_zAxis));
	m_yAxis = glm::normalize(glm::cross(m_zAxis, m_xAxis));
	m_viewDir = -m_zAxis;
	m_accumPitchDegrees = glm::degrees(-asinf(m_yAxis[2]));

	fillRotationPart();
	fillTranslationPart();
}

void Camera::lookAt(float distance, float pitch, float yaw){
	m_distance = distance;
	m_accumPitchDegrees = pitch;
	m_accumYawDegrees = yaw;

	pitch = glm::radians(pitch);
	yaw = glm::radians(yaw);

	float cosY = cosf(yaw);
	float cosP = cosf(pitch);
	float sinY = sinf(yaw);
	float sinP = sinf(pitch);

	m_xAxis[0] = cosY; m_xAxis[2] = sinY;
	m_yAxis[0] = sinP * sinY; m_yAxis[1] = cosP; m_yAxis[2] = -sinP * cosY;
	m_zAxis[0] = -cosP * sinY; m_zAxis[1] = sinP; m_zAxis[2] = cosP * cosY;
	m_viewDir = -m_zAxis;
	m_eye = m_zAxis * distance;

	fillRotationPart();
	fillTranslationPart();
}

void Camera::lookAt(float distance, float pitch, float yaw, float roll) {
	m_accumPitchDegrees = pitch;
	m_accumYawDegrees = yaw;
	m_distance = distance;

	pitch = glm::radians(pitch);
	yaw = glm::radians(yaw);
	roll = glm::radians(roll);

	float cosY = cosf(yaw);
	float cosP = cosf(pitch);
	float cosR = cosf(roll);
	float sinY = sinf(yaw);
	float sinP = sinf(pitch);
	float sinR = sinf(roll);

	m_xAxis[0] = cosR * cosY - sinR * sinP * sinY; m_xAxis[1] = -sinR * cosP; m_xAxis[2] = cosR * sinY + sinR * sinP * cosY;
	m_yAxis[0] = sinR * cosY + cosR * sinP * sinY; m_yAxis[1] = cosR * cosP; m_yAxis[2] = sinR * sinY - cosR * sinP * cosY;
	m_zAxis[0] = -cosP * sinY; m_zAxis[1] = sinP; m_zAxis[2] = cosP * cosY;
	m_viewDir = -m_zAxis;

	m_eye = m_zAxis * distance;

	fillRotationPart();
	fillTranslationPart();
}

void Camera::fillRotationPart() {
	m_viewMatrix[0][0] = m_xAxis[0];
	m_viewMatrix[0][1] = m_yAxis[0];
	m_viewMatrix[0][2] = m_zAxis[0];
	m_viewMatrix[0][3] = 0.0f;

	m_viewMatrix[1][0] = m_xAxis[1];
	m_viewMatrix[1][1] = m_yAxis[1];
	m_viewMatrix[1][2] = m_zAxis[1];
	m_viewMatrix[1][3] = 0.0f;

	m_viewMatrix[2][0] = m_xAxis[2];
	m_viewMatrix[2][1] = m_yAxis[2];
	m_viewMatrix[2][2] = m_zAxis[2];
	m_viewMatrix[2][3] = 0.0f;

	m_invViewMatrix[0][0] = m_xAxis[0];
	m_invViewMatrix[0][1] = m_xAxis[1];
	m_invViewMatrix[0][2] = m_xAxis[2];
	m_invViewMatrix[0][3] = 0.0f;

	m_invViewMatrix[1][0] = m_yAxis[0];
	m_invViewMatrix[1][1] = m_yAxis[1];
	m_invViewMatrix[1][2] = m_yAxis[2];
	m_invViewMatrix[1][3] = 0.0f;

	m_invViewMatrix[2][0] = m_zAxis[0];
	m_invViewMatrix[2][1] = m_zAxis[1];
	m_invViewMatrix[2][2] = m_zAxis[2];
	m_invViewMatrix[2][3] = 0.0f;
}

void Camera::fillTranslationPart() {
	m_viewMatrix[3][0] = -glm::dot(m_xAxis, m_eye);
	m_viewMatrix[3][1] = -glm::dot(m_yAxis, m_eye);
	m_viewMatrix[3][2] = -glm::dot(m_zAxis, m_eye);
	m_viewMatrix[3][3] = 1.0f;

	m_invViewMatrix[3][0] = m_eye[0];
	m_invViewMatrix[3][1] = m_eye[1];
	m_invViewMatrix[3][2] = m_eye[2];
	m_invViewMatrix[3][3] = 1.0f;
}

void Camera::rotateY(float degrees) {
	glm::mat4 rotMtx = glm::rotate(glm::radians(degrees), WORLD_YAXIS);

	m_accumYawDegrees += degrees;
	m_xAxis = rotMtx * glm::vec4(m_xAxis, 0.0f);
	m_yAxis = rotMtx * glm::vec4(m_yAxis, 0.0f);
	m_zAxis = rotMtx * glm::vec4(m_zAxis, 0.0f);
	fillRotationPart();
	
	m_eye = rotMtx * glm::vec4(m_eye, 0.0f);
	fillTranslationPart();
}

void Camera::rotate(float yaw, float pitch) {
	rotateFirstPerson(yaw * m_rotationSpeed, pitch * m_rotationSpeed);
	orthogonalize();
	fillTranslationPart();
}

void Camera::rotateFirstPerson(float yaw, float pitch){

	m_accumPitchDegrees += pitch;
	m_accumYawDegrees += yaw;
	
	if (m_accumPitchDegrees > 90.0f){
		pitch = 90.0f - (m_accumPitchDegrees - pitch);
		m_accumPitchDegrees = 90.0f;
	}

	if (m_accumPitchDegrees < -90.0f){
		pitch = -90.0f - (m_accumPitchDegrees - pitch);
		m_accumPitchDegrees = -90.0f;
	}
	
	glm::mat4 rotMtx;

	// Rotate camera's existing x and z axes about the world y axis.
	if (yaw != 0.0f){
		rotMtx = glm::rotate(rotMtx, glm::radians(yaw), WORLD_YAXIS);
		m_xAxis = rotMtx * glm::vec4(m_xAxis, 0.0f);
		m_zAxis = rotMtx * glm::vec4(m_zAxis, 0.0f);
	}

	// Rotate camera's existing y and z axes about its existing x axis.
	if (pitch != 0.0f){
		rotMtx = glm::mat4(1.0f);
		rotMtx = glm::rotate(rotMtx, glm::radians(pitch), m_xAxis);
		m_yAxis = rotMtx * glm::vec4(m_yAxis, 0.0f);
		m_zAxis = rotMtx * glm::vec4(m_zAxis, 0.0f);
	}
}

void Camera::orthogonalize() {
    m_zAxis = glm::normalize(m_zAxis);
    m_yAxis = glm::normalize(glm::cross(m_zAxis, m_xAxis));
	m_xAxis = glm::normalize(glm::cross(m_yAxis, m_zAxis));
	m_viewDir = -m_zAxis;
	fillRotationPart();
}

void Camera::move(const glm::vec3& direction) {
	m_eye += m_xAxis * direction[0] * m_movingSpeed;
	m_eye += WORLD_YAXIS * direction[1] * m_movingSpeed;
	m_eye += m_viewDir * direction[2] * m_movingSpeed;
	fillTranslationPart();
}

void Camera::move(float distance) {
	m_eye += m_zAxis * distance;
	fillTranslationPart();
}

void Camera::setPosition(float x, float y, float z, bool observe){
	setPosition(glm::vec3(x, y, z), observe);
	fillTranslationPart();
}

void Camera::setPosition(const glm::vec3& position, bool observe){
    m_eye = position;
	if (observe) {		
		m_zAxis = glm::normalize(m_eye - m_target);
		m_xAxis = glm::normalize(glm::cross(WORLD_YAXIS, m_zAxis));
		m_yAxis = glm::normalize(glm::cross(m_zAxis, m_xAxis));
		m_viewDir = -m_zAxis;
		fillRotationPart();
	}
    fillTranslationPart();
}

void Camera::setRotationSpeed(float rotationSpeed){
	m_rotationSpeed = rotationSpeed;
}

void Camera::setMovingSpeed(float movingSpeed){
	m_movingSpeed = movingSpeed;
}

const glm::mat4& Camera::getPerspectiveMatrix() const{
	return m_persMatrix;
}

const glm::mat4& Camera::getInvPerspectiveMatrix() const{
	return  m_invPersMatrix;
}

const glm::mat4& Camera::getOrthographicMatrix() const{
	return m_orthMatrix;
}

const glm::mat4& Camera::getInvOrthographicMatrix() const {
	return m_invOrthMatrix;
}

const glm::mat4& Camera::getViewMatrix() const{
	return m_viewMatrix;
}

const glm::mat4& Camera::getInvViewMatrix() const{
	return m_invViewMatrix;
}

const glm::mat4 Camera::getRotationMatrix() const{
	return glm::mat4(m_viewMatrix[0][0], m_viewMatrix[0][1], m_viewMatrix[0][2], 0.0f,
                     m_viewMatrix[1][0], m_viewMatrix[1][1], m_viewMatrix[1][2], 0.0f,
                     m_viewMatrix[2][0], m_viewMatrix[2][1], m_viewMatrix[2][2], 0.0f,
                     0.0f,0.0f, 0.0f, 1.0f);
}

const glm::vec3& Camera::getPosition() const{
	return m_eye;
}

const glm::vec3& Camera::getCamX() const{
	return m_xAxis;
}

const glm::vec3& Camera::getCamY() const{
	return m_yAxis;
}

const glm::vec3& Camera::getCamZ() const{
	return m_zAxis;
}

glm::mat4 Camera::GetNormalMatrix(const glm::mat4& m) {

	glm::mat4 normalMatrix;
	float det;
	float invDet;

	det = m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) +
		  m[0][1] * (m[1][2] * m[2][0] - m[2][2] * m[1][0]) +
		  m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

	invDet = 1.0f / det;

	normalMatrix[0][0] = (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invDet;
	normalMatrix[1][0] = (m[2][1] * m[0][2] - m[2][2] * m[0][1]) * invDet;
	normalMatrix[2][0] = (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * invDet;
	normalMatrix[3][0] = 0.0f;

	normalMatrix[0][1] = (m[2][0] * m[1][2] - m[1][0] * m[2][2]) * invDet;
	normalMatrix[1][1] = (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * invDet;
	normalMatrix[2][1] = (m[1][0] * m[0][2] - m[1][2] * m[0][0]) * invDet;
	normalMatrix[3][1] = 0.0f;

	normalMatrix[0][2] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
	normalMatrix[1][2] = (m[2][0] * m[0][1] - m[0][0] * m[2][1]) * invDet;
	normalMatrix[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * invDet;
	normalMatrix[3][2] = 0.0f;

	normalMatrix[0][3] = 0.0f;
	normalMatrix[1][3] = 0.0f;
	normalMatrix[2][3] = 0.0f;
	normalMatrix[3][3] = 1.0f;

	return normalMatrix;
}

glm::mat4 Camera::GetRotationMatrix(const glm::mat4& viewMatrix){
	return glm::mat4(viewMatrix[0][0], viewMatrix[0][1], viewMatrix[0][2], 0.0f,
                 	 viewMatrix[1][0], viewMatrix[1][1], viewMatrix[1][2], 0.0f,
                 	 viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2], 0.0f,
                 	 0.0f, 0.0f, 0.0f, 1.0);
}