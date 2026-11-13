#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <string>
#include "Camera.h"
#include "Mesh.h"
#include "Particles.h"
#include "Physics.h"
#include "Shader.h"
#include "Shadow.h"
#include "Texture.h"

class Scene {
public:
    explicit Scene(const std::string& shaderDir)
        : sh(shaderDir + "phong.vert", shaderDir + "phong.frag"),
          depthSh(shaderDir + "depth.vert", shaderDir + "depth.frag"),
          cube(Mesh::cube()), plane(Mesh::plane()), sphere(Mesh::sphere()),
          cyl(Mesh::cylinder()), shadow(2048),
          grassTex(Texture::GRASS), woodTex(Texture::WOOD) {}

    void render(int width, int height, const Course& course, const Ball& ball,
                const Camera& camera, const Confetti& confetti, float aim, float power,
                float simTime, bool lamp, bool sunset, bool shadows, bool preview) {
        const bool still = !ball.moving() && ball.sink < 0.f;
        const float t = simTime;
        const glm::vec3 sunDir = sunset ? glm::normalize(glm::vec3(-0.8f, 0.25f, 0.3f))
                                        : glm::normalize(glm::vec3(0.4f, 1.f, 0.3f));
        const glm::vec3 sunCol = sunset ? glm::vec3(1.0f, 0.5f, 0.25f)
                                        : glm::vec3(1.0f, 0.96f, 0.88f);
        const glm::vec3 amb = sunset ? glm::vec3(0.12f, 0.10f, 0.18f)
                                     : glm::vec3(0.25f, 0.27f, 0.30f);
        const glm::vec3 skyTop = sunset ? glm::vec3(0.18f, 0.15f, 0.35f)
                                        : glm::vec3(0.20f, 0.45f, 0.85f);
        const glm::vec3 skyHor = sunset ? glm::vec3(0.95f, 0.50f, 0.30f)
                                        : glm::vec3(0.75f, 0.88f, 1.00f);
        const glm::vec3 lampPos(course.hole.x + 2.f * sinf(t * 0.5f),
                                1.5f, course.hole.y + 2.f);
        const glm::vec3 lampCol(1.0f, 0.85f, 0.5f);

        const glm::mat4 lightView =
            glm::lookAt(sunDir * 18.f, glm::vec3(0.f), glm::vec3(0, 1, 0));
        const glm::mat4 lightProj = glm::ortho(-9.f, 9.f, -9.f, 9.f, 1.f, 40.f);
        const glm::mat4 lightSpace = lightProj * lightView;

        auto drawScene = [&](bool shadowPass) {
            draw(plane, scale({2 * course.W, 1.f, 2 * course.L}),
                 {1, 1, 1}, 1, 8.f, 0.05f, false, shadowPass);

            for (const Box& b : course.walls) {
                const glm::vec2 c = (b.min + b.max) * 0.5f;
                const glm::vec2 s = b.max - b.min;
                draw(cube, translate({c.x, 0.15f, c.y}) * scale({s.x, 0.3f, s.y}),
                     {0.55f, 0.34f, 0.18f}, 4, 16.f, 0.1f, true, shadowPass);
            }

            draw(cyl, translate({course.hole.x, 0.004f, course.hole.y}) *
                 scale({course.holeR, 0.008f, course.holeR}),
                 {0.02f, 0.02f, 0.02f}, 0, 8.f, 0.f, false, shadowPass);

            const glm::mat4 poleBase = translate({course.hole.x, 0.f, course.hole.y});
            draw(cyl, poleBase * translate({0, 0.5f, 0}) * scale({0.03f, 1.f, 0.03f}),
                 {0.9f, 0.9f, 0.9f}, 0, 64.f, 0.5f, true, shadowPass);
            const glm::mat4 flagBase =
                poleBase * translate({0, 0.9f, 0}) * rotateY(0.25f * sinf(t * 2.f));
            draw(cube, flagBase * translate({0.18f, 0, 0}) * scale({0.36f, 0.2f, 0.02f}),
                 {0.9f, 0.1f, 0.1f}, 0, 16.f, 0.1f, true, shadowPass);

            for (const Blade& bl : course.blades) {
                const glm::mat4 base = translate({bl.center.x, 0.f, bl.center.y});
                draw(cyl, base * translate({0, 0.3f, 0}) * scale({0.12f, 0.6f, 0.12f}),
                     {0.8f, 0.8f, 0.75f}, 0, 32.f, 0.3f, true, shadowPass);
                const glm::mat4 arm =
                    base * translate({0, 0.2f, 0}) * rotateY(bl.speed * simTime);
                draw(cube, arm * scale({2.f * bl.halfLen, 0.3f, 0.2f}),
                     {0.7f, 0.15f, 0.15f}, 0, 32.f, 0.4f, true, shadowPass);
            }

            const float k = ball.sink < 0.f ? 0.f : std::min(ball.sink / 0.5f, 1.f);
            draw(sphere, translate({ball.p.x, ball.r - 0.3f * k, ball.p.y}) *
                 scale(glm::vec3(ball.r)), {1, 1, 1}, 0, 64.f, 0.9f, true, shadowPass);

            if (shadowPass) return;

            if (still) {
                const float len = 0.3f + power * 1.5f;
                const glm::mat4 aimModel =
                    translate({ball.p.x, 0.03f, ball.p.y}) * rotateY(-aim) *
                    translate({0, 0, -(0.2f + len / 2)}) * scale({0.06f, 0.02f, len});
                draw(cube, aimModel,
                     glm::mix(glm::vec3(0.2f, 0.9f, 0.2f),
                              glm::vec3(0.9f, 0.1f, 0.1f), power),
                     2, 1, 0, false, shadowPass);

                if (preview) {
                    const float pw = std::max(power, 0.35f);
                    const glm::vec2 d(sinf(aim), -cosf(aim));
                    const auto path = predictPath(course, ball, d * pw * 10.f, simTime);
                    for (size_t i = 0; i < path.size(); i++) {
                        const glm::vec3 color = glm::mix(
                            glm::vec3(1.f), glm::vec3(1.f, 0.8f, 0.2f),
                            i / static_cast<float>(path.size()));
                        draw(sphere, translate({path[i].x, 0.05f, path[i].y}) *
                             scale(glm::vec3(0.035f)), color, 2, 1, 0, false, shadowPass);
                    }
                }
            }

            for (const Particle& p : confetti.ps)
                draw(cube, translate(p.p) * rotateY(p.life * 8.f) *
                     scale({0.06f, 0.012f, 0.04f}), p.color, 2, 1, 0, false, shadowPass);

            if (lamp)
                draw(sphere, translate(lampPos) * scale(glm::vec3(0.08f)),
                     lampCol, 2, 1, 0, false, shadowPass);
        };

        if (shadows) {
            shadow.begin();
            depthSh.use();
            depthSh.setMat4("lightSpace", lightSpace);
            drawScene(true);
            shadow.end(width, height);
        }

        glClearColor(skyHor.r, skyHor.g, skyHor.b, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 projection =
            glm::perspective(glm::radians(50.f), static_cast<float>(width) / height, 0.1f, 100.f);
        const glm::vec3 camPos = camera.position();
        sh.use();
        sh.setMat4("view", camera.view());
        sh.setMat4("projection", projection);
        sh.setMat4("lightSpace", lightSpace);
        sh.setVec3("viewPos", camPos);
        sh.setVec3("sunDir", sunDir);
        sh.setVec3("sunColor", sunCol);
        sh.setVec3("ambient", amb);
        sh.setVec3("lampPos", lampPos);
        sh.setVec3("lampColor", lampCol);
        sh.setInt("lampOn", lamp ? 1 : 0);
        sh.setVec3("skyTop", skyTop);
        sh.setVec3("skyHorizon", skyHor);
        sh.setInt("shadowsOn", shadows ? 1 : 0);
        sh.setInt("shadowMap", 0);
        sh.setInt("grassTex", 1);
        sh.setInt("woodTex", 2);
        grassTex.bind(1);
        woodTex.bind(2);
        shadow.bind(0);

        glDepthMask(GL_FALSE);
        draw(cube, translate(camPos) * scale(glm::vec3(80.f)),
             {1, 1, 1}, 3, 1, 0, false, false);
        glDepthMask(GL_TRUE);
        drawScene(false);
    }

private:
    Shader sh;
    Shader depthSh;
    Mesh cube;
    Mesh plane;
    Mesh sphere;
    Mesh cyl;
    ShadowMap shadow;
    Texture grassTex;
    Texture woodTex;

    static glm::mat4 translate(glm::vec3 v) {
        return glm::translate(glm::mat4(1.f), v);
    }
    static glm::mat4 scale(glm::vec3 v) {
        return glm::scale(glm::mat4(1.f), v);
    }
    static glm::mat4 rotateY(float angle) {
        return glm::rotate(glm::mat4(1.f), angle, glm::vec3(0, 1, 0));
    }

    void draw(const Mesh& mesh, const glm::mat4& model, glm::vec3 color,
              int type, float shininess, float specular, bool castsShadow,
              bool shadowPass) const {
        if (shadowPass) {
            if (castsShadow) {
                depthSh.setMat4("model", model);
                mesh.draw();
            }
            return;
        }
        sh.setMat4("model", model);
        sh.setMat3("normalMat",
                   glm::transpose(glm::inverse(glm::mat3(model))));
        sh.setVec3("color", color);
        sh.setInt("matType", type);
        sh.setFloat("shininess", shininess);
        sh.setFloat("specStrength", specular);
        mesh.draw();
    }
};
