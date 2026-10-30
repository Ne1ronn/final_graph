#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

struct Camera {
    glm::vec3 target{0.f};
    float yaw = 90.f, pitch = 45.f, dist = 9.f;

    glm::vec3 position() const {
        float y = glm::radians(yaw), p = glm::radians(pitch);
        return target + dist * glm::vec3(cosf(p) * cosf(y), sinf(p), cosf(p) * sinf(y));
    }
    glm::mat4 view() const { return glm::lookAt(position(), target, glm::vec3(0, 1, 0)); }
};