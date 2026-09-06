#pragma once
#include <stack>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// A GLM-backed matrix stack to replace OpenGL 1.x glPushMatrix, glPopMatrix, glRotatef, etc.
class MatrixStack {
public:
    MatrixStack() {
        _stack.push(glm::mat4(1.0f));
    }

    void push() {
        _stack.push(_stack.top());
    }

    void pop() {
        if (_stack.size() > 1) {
            _stack.pop();
        }
    }

    void translate(float x, float y, float z) {
        _stack.top() = glm::translate(_stack.top(), glm::vec3(x, y, z));
    }

    void rotate(float angleDeg, float x, float y, float z) {
        _stack.top() = glm::rotate(_stack.top(), glm::radians(angleDeg), glm::vec3(x, y, z));
    }

    void scale(float x, float y, float z) {
        _stack.top() = glm::scale(_stack.top(), glm::vec3(x, y, z));
    }

    void loadIdentity() {
        _stack.top() = glm::mat4(1.0f);
    }

    void multiply(const glm::mat4& mat) {
        _stack.top() = _stack.top() * mat;
    }

    void ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
        _stack.top() = glm::ortho(left, right, bottom, top, zNear, zFar);
    }

    void perspective(float fovyDeg, float aspect, float zNear, float zFar) {
        // Vulkan typically prefers LH coordinate system or RH with Y down
        // If we use right-handed + zero-to-one clip space:
        glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(fovyDeg), aspect, zNear, zFar);
        proj[1][1] *= -1.0f; // Flip Y for Vulkan
        _stack.top() = proj;
    }

    const glm::mat4& get() const {
        return _stack.top();
    }

    const float* valuePtr() const {
        return glm::value_ptr(_stack.top());
    }

private:
    std::stack<glm::mat4> _stack;
};
