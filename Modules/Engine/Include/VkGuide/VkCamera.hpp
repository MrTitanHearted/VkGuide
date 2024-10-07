#pragma once

#include <VkGuide/Defines.hpp>

enum class CameraMovement : std::uint32_t {
    Forward = 1 << 0,
    Backward = 1 << 1,
    Up = 1 << 2,
    Down = 1 << 3,
    WorldForward = 1 << 4,
    WorldBackward = 1 << 5,
    WorldUp = 1 << 6,
    WorldDown = 1 << 7,
    Right = 1 << 8,
    Left = 1 << 9,
};

class Camera {
   public:
    static const float YAW;
    static const float PITCH;
    static const float SPEED;
    static const float SENSITIVITY;
    static const float ZOOM;
    static const glm::vec3 WORLD_UP;

   public:
    Camera();
    ~Camera() = default;

    void move(CameraMovement direction, float dt);
    void moveMouse(float xOffset, float yOffset, bool constrainPitch, float constrainValue);
    void scrollMouse(float yOffset, float zoomMax, float zoomMin);

    void setPosition(const glm::vec3 &position);
    void setFront(const glm::vec3 &front);
    void setWorldUp(const glm::vec3 &worldUp);
    void setSpeed(float speed);
    void setSensitivity(float sensitivity);
    void setZoom(float zoom);

    glm::mat4 getProjection(float aspectRatio, float zNear, float zFar) const;
    glm::mat4 getView() const;
    glm::mat4 getInverseProjection(float aspectRatio, float zNear, float zFar) const;
    glm::mat4 getInverseView() const;

    glm::vec3 getPosition() const;
    glm::vec3 getFront() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;
    glm::vec3 getWorldUp() const;

    float getYaw() const;
    float getPitch() const;
    float getSpeed() const;
    float getSensitivity() const;
    float getZoom() const;

   private:
    glm::vec3 m_Position;
    glm::vec3 m_Front;
    glm::vec3 m_Right;
    glm::vec3 m_Up;
    glm::vec3 m_WorldUp;

    float m_Yaw;
    float m_Pitch;

    float m_MovementSpeed;
    float m_MouseSensitivity;
    float m_Zoom;

    void updateCameraVectors();
};

class CameraManager {
   public:
    CameraManager(float aspectRatio = 1.0f);
    ~CameraManager() = default;

    void move(CameraMovement direction, float dt);
    void moveMouse(float x, float y, bool constrainPitch = true, float constrainValue = 90.0f);
    void scrollMouse(float yOffset, float zoomMax = 45.0f, float zoomMin = 1.0f);

    void setPosition(const glm::vec3 &position);
    void setFront(const glm::vec3 &front);
    void setWorldUp(const glm::vec3 &worldUp);
    void setSpeed(float speed);
    void setSensitivity(float sensitivity);
    void setZoom(float zoom);
    void setFirstMouse(bool firstMouse);
    void setLastX(float lastX);
    void setLastY(float lastY);
    void setFar(float zFar);
    void setNear(float zNear);
    void setAspectRatio(float aspectRatio);

    CameraManager &withPosition(const glm::vec3 &position);
    CameraManager &withFront(const glm::vec3 &front);
    CameraManager &withWorldUp(const glm::vec3 &worldUp);
    CameraManager &withSpeed(float speed);
    CameraManager &withSensitivity(float sensitivity);
    CameraManager &withZoom(float zoom);
    CameraManager &withFirstMouse(bool firstMouse);
    CameraManager &withLastX(float lastX);
    CameraManager &withLastY(float lastY);
    CameraManager &withFar(float zFar);
    CameraManager &withNear(float zNear);
    CameraManager &withAspectRatio(float aspectRatio);

    glm::mat4 getProjection() const;
    glm::mat4 getView() const;
    glm::mat4 getInverseProjection() const;
    glm::mat4 getInverseView() const;

    glm::vec3 getPosition() const;
    glm::vec3 getFront() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;
    glm::vec3 getWorldUp() const;
    float getYaw() const;
    float getPitch() const;
    float getSpeed() const;
    float getSensitivity() const;
    float getZoom() const;

    bool getFirstMouse() const;
    float getLastX() const;
    float getLastY() const;
    float getFar() const;
    float getNear() const;
    float getAspectRatio() const;

   private:
    Camera m_Camera;

    bool m_FirstMouse;

    float m_LastX;
    float m_LastY;
    float m_Far;
    float m_Near;
    float m_AspectRatio;
};