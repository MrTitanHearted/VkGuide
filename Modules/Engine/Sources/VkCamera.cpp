#include <VkGuide/VkCamera.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/common.hpp>

const float Camera::YAW = -90.0f;
const float Camera::PITCH = 0.0f;
const float Camera::SPEED = 0.005f;
const float Camera::SENSITIVITY = 0.05f;
const float Camera::ZOOM = 45.0f;
const glm::vec3 Camera::WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);

Camera::Camera() {
    m_Position = glm::vec3(0.0f);
    m_WorldUp = WORLD_UP;
    m_Yaw = YAW;
    m_Pitch = PITCH;
    m_MovementSpeed = SPEED;
    m_MouseSensitivity = SENSITIVITY;
    m_Zoom = ZOOM;

    updateCameraVectors();

    setFront(glm::vec3(0.0f, 0.0f, -1.0f));
}

void Camera::move(CameraMovement direction, float dt) {
    float speed = m_MovementSpeed * dt;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::Forward))
        m_Position += m_Front * speed;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::Backward))
        m_Position -= m_Front * speed;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::Up))
        m_Position += m_Up * speed;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::Down))
        m_Position -= m_Up * speed;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::WorldForward))
        m_Position += glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z)) * speed;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::WorldBackward))
        m_Position -= glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z)) * speed;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::WorldUp))
        m_Position += m_WorldUp * speed;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::WorldDown))
        m_Position -= m_WorldUp * speed;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::Right))
        m_Position += m_Right * speed;
    if (static_cast<int>(direction) & static_cast<int>(CameraMovement::Left))
        m_Position -= m_Right * speed;
}

void Camera::moveMouse(float xOffset, float yOffset, bool constrainPitch, float constrainValue) {
    xOffset *= m_MouseSensitivity;
    yOffset *= m_MouseSensitivity;

    m_Yaw += xOffset;

    if (constrainPitch) {
        if (m_Pitch + yOffset > constrainValue)
            m_Pitch = constrainValue;
        else if (m_Pitch + yOffset < -constrainValue)
            m_Pitch = -constrainValue;
        else
            m_Pitch += yOffset;
    } else {
        m_Pitch += yOffset;
    }

    updateCameraVectors();
}

void Camera::scrollMouse(float yOffset, float zoomMax, float zoomMin) {
    m_Zoom -= yOffset;
    if (m_Zoom > zoomMax)
        m_Zoom = zoomMax;
    if (m_Zoom < zoomMin)
        m_Zoom = zoomMin;
}

void Camera::setPosition(const glm::vec3 &position) {
    m_Position = position;
}

void Camera::setFront(const glm::vec3 &front) {
    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void Camera::setWorldUp(const glm::vec3 &worldUp) {
    m_WorldUp = glm::normalize(worldUp);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

void Camera::setSpeed(float speed) {
    m_MovementSpeed = speed;
}

void Camera::setSensitivity(float sensitivity) {
    m_MouseSensitivity = sensitivity;
}

void Camera::setZoom(float zoom) {
    m_Zoom = zoom;
}

glm::mat4 Camera::getProjection(float aspectRatio, float zNear, float zFar) const {
    return glm::perspective(glm::radians(m_Zoom), aspectRatio, zNear, zFar);
}

glm::mat4 Camera::getView() const {
    return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
}

glm::vec3 Camera::getPosition() const {
    return m_Position;
}

glm::mat4 Camera::getInverseProjection(float aspectRatio, float zNear, float zFar) const {
    return glm::inverse(glm::perspective(glm::radians(m_Zoom), aspectRatio, zNear, zFar));
}

glm::mat4 Camera::getInverseView() const {
    return glm::inverse(glm::lookAt(m_Position, m_Position + m_Front, m_Up));
}

glm::vec3 Camera::getFront() const {
    return m_Front;
}

glm::vec3 Camera::getRight() const {
    return m_Right;
}

glm::vec3 Camera::getUp() const {
    return m_Up;
}

glm::vec3 Camera::getWorldUp() const {
    return m_WorldUp;
}

float Camera::getYaw() const {
    return m_Yaw;
}

float Camera::getPitch() const {
    return m_Pitch;
}

float Camera::getSpeed() const {
    return m_MovementSpeed;
}

float Camera::getSensitivity() const {
    return m_MouseSensitivity;
}

float Camera::getZoom() const {
    return m_Zoom;
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = glm::cos(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));
    front.y = glm::sin(glm::radians(m_Pitch));
    front.z = glm::sin(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));

    m_Front = glm::normalize(front);
    m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
    m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

CameraManager::CameraManager(float aspectRatio)
    : m_Camera{} {
    m_FirstMouse = true;
    m_Far = 100.0f;
    m_Near = 0.01f;
    m_AspectRatio = aspectRatio;
}

void CameraManager::move(CameraMovement direction, float dt) {
    m_Camera.move(direction, dt);
}

void CameraManager::moveMouse(float x, float y, bool constrainPitch, float constrainValue) {
    if (m_FirstMouse) {
        m_LastX = x;
        m_LastY = y;

        m_FirstMouse = false;
    }

    float xOffset = x - m_LastX;
    float yOffset = m_LastY - y;
    m_LastX = x;
    m_LastY = y;

    m_Camera.moveMouse(xOffset, yOffset, constrainPitch, constrainValue);
}

void CameraManager::scrollMouse(float yOffset, float zoomMax, float zoomMin) {
    m_Camera.scrollMouse(yOffset, zoomMax, zoomMin);
}

void CameraManager::setPosition(const glm::vec3 &position) {
    m_Camera.setPosition(position);
}

void CameraManager::setFront(const glm::vec3 &front) {
    m_Camera.setFront(front);
}

void CameraManager::setWorldUp(const glm::vec3 &worldUp) {
    m_Camera.setWorldUp(worldUp);
}

void CameraManager::setSpeed(float speed) {
    m_Camera.setSpeed(speed);
}

void CameraManager::setSensitivity(float sensitivity) {
    m_Camera.setSensitivity(sensitivity);
}

void CameraManager::setZoom(float zoom) {
    m_Camera.setZoom(zoom);
}

void CameraManager::setFirstMouse(bool firstMouse) {
    m_FirstMouse = firstMouse;
}

void CameraManager::setLastX(float lastX) {
    m_LastX = lastX;
}

void CameraManager::setLastY(float lastY) {
    m_LastY = lastY;
}

void CameraManager::setFar(float zFar) {
    m_Far = zFar;
}

void CameraManager::setNear(float zNear) {
    m_Near = zNear;
}

void CameraManager::setAspectRatio(float aspectRatio) {
    m_AspectRatio = aspectRatio;
}

CameraManager &CameraManager::withPosition(const glm::vec3 &position) {
    m_Camera.setPosition(position);
    return *this;
}

CameraManager &CameraManager::withFront(const glm::vec3 &front) {
    m_Camera.setFront(front);
    return *this;
}

CameraManager &CameraManager::withWorldUp(const glm::vec3 &worldUp) {
    m_Camera.setWorldUp(worldUp);
    return *this;
}

CameraManager &CameraManager::withSpeed(float speed) {
    m_Camera.setSpeed(speed);
    return *this;
}

CameraManager &CameraManager::withSensitivity(float sensitivity) {
    m_Camera.setSensitivity(sensitivity);
    return *this;
}

CameraManager &CameraManager::withZoom(float zoom) {
    m_Camera.setZoom(zoom);
    return *this;
}

CameraManager &CameraManager::withFirstMouse(bool firstMouse) {
    m_FirstMouse = firstMouse;
    return *this;
}

CameraManager &CameraManager::withLastX(float lastX) {
    m_LastX = lastX;
    return *this;
}

CameraManager &CameraManager::withLastY(float lastY) {
    m_LastY = lastY;
    return *this;
}

CameraManager &CameraManager::withFar(float zFar) {
    m_Far = zFar;
    return *this;
}

CameraManager &CameraManager::withNear(float zNear) {
    m_Near = zNear;
    return *this;
}

CameraManager &CameraManager::withAspectRatio(float aspectRatio) {
    m_AspectRatio = aspectRatio;
    return *this;
}

glm::mat4 CameraManager::getProjection() const {
    return m_Camera.getProjection(m_AspectRatio, m_Near, m_Far);
}

glm::mat4 CameraManager::getView() const {
    return m_Camera.getView();
}

glm::mat4 CameraManager::getInverseProjection() const {
    return m_Camera.getInverseProjection(m_AspectRatio, m_Near, m_Far);
}

glm::mat4 CameraManager::getInverseView() const {
    return m_Camera.getInverseView();
}

glm::vec3 CameraManager::getPosition() const {
    return m_Camera.getPosition();
}

glm::vec3 CameraManager::getFront() const {
    return m_Camera.getFront();
}

glm::vec3 CameraManager::getRight() const {
    return m_Camera.getRight();
}

glm::vec3 CameraManager::getUp() const {
    return m_Camera.getUp();
}

glm::vec3 CameraManager::getWorldUp() const {
    return m_Camera.getWorldUp();
}

float CameraManager::getYaw() const {
    return m_Camera.getYaw();
}

float CameraManager::getPitch() const {
    return m_Camera.getPitch();
}

float CameraManager::getSpeed() const {
    return m_Camera.getSpeed();
}

float CameraManager::getSensitivity() const {
    return m_Camera.getSensitivity();
}

float CameraManager::getZoom() const {
    return m_Camera.getZoom();
}

bool CameraManager::getFirstMouse() const {
    return m_FirstMouse;
}

float CameraManager::getLastX() const {
    return m_LastX;
}

float CameraManager::getLastY() const {
    return m_LastY;
}

float CameraManager::getFar() const {
    return m_Far;
}

float CameraManager::getNear() const {
    return m_Near;
}

float CameraManager::getAspectRatio() const {
    return m_AspectRatio;
}