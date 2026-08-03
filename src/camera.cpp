#include "camera.hpp"
#include <iostream>

using namespace std;

vec3 Camera::lookDir(){
    vec2 radAngles = vec2(radians(m_angles.x), radians(m_angles.y));
    vec3 lookDir = vec3(- sin(radAngles.x) * cos(radAngles.y), sin(radAngles.y), - cos(radAngles.x) * cos(radAngles.y));
    return normalize(lookDir);
}

void Camera::move(const CameraMoveInputs& inputs, float dt){
    vec3 step = vec3(
        (int)inputs.right - (int)inputs.left,
        (int)inputs.up - (int)inputs.down,
        (int)inputs.forward - (int)inputs.backward
    );
    vec3 moveForward = step.z * lookDir();
    vec3 moveUp = vec3(0, step.y, 0);
    vec3 moveRight = step.x * normalize(cross(lookDir(), vec3(0,1,0)));
    m_isMoving = length(step) > 0;
    if (m_isMoving){
        float speedFactor = dt * 60.0f;
        if (inputs.sprinting) speedFactor *= 2;
        if (inputs.slowing) speedFactor /= 6;
        m_pos += speedFactor * m_moveSensitivity * normalize(moveForward + moveUp + moveRight);
    }
}

void Camera::resetMousePos(float mouseX, float mouseY){
    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;
}

void Camera::rotate(float mouseX, float mouseY){
    float deltaMouseX = mouseX - m_lastMouseX;
    float deltaMouseY = mouseY - m_lastMouseY;
    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;
    m_angles += vec2(-deltaMouseX, -deltaMouseY) * m_lookSensitivity;
    m_angles.y = std::clamp(m_angles.y, -80.0f, 80.0f);
    m_isLooking = length(vec2(-deltaMouseX, -deltaMouseY)) > 0;
}

bool Camera::getIsMoving(int frame){
    bool result = m_isMoving || m_isLooking;
    if (result) m_lastMovingFrame = frame;
    return frame - m_lastMovingFrame < 10;
}

void Camera::updateGPU(){
    glUniform3f(ShaderProgram::getVarLoc("camera.pos"), m_pos.x, m_pos.y, m_pos.z);
    glUniform3f(ShaderProgram::getVarLoc("camera.lookDir"), lookDir().x, lookDir().y, lookDir().z);
    
    glUniform1f(ShaderProgram::getVarLoc("cameraFov"), m_camProps.fov);
    glUniform1f(ShaderProgram::getVarLoc("cameraAperture"), m_camProps.aperture);
    glUniform1f(ShaderProgram::getVarLoc("cameraFocalLength"), m_camProps.focalLength);
}

mat4 Camera::viewMatrix(){
    vec3 forward = lookDir();
    vec3 worldUp = abs(forward.y) < 0.999f ? vec3(0,1,0) : vec3(0,0,1);
    return glm::lookAt(m_pos, m_pos + forward, worldUp);
}