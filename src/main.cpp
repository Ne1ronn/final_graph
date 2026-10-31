#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstdio>
#include <string>
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "Physics.h"

struct App {
    Camera cam;
    bool lamp = true, sunset = false, reset = false, rmb = false;
    double lx = 0, ly = 0;
} app;

static void onKey(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_L) app.lamp = !app.lamp;
    if (key == GLFW_KEY_N) app.sunset = !app.sunset;
    if (key == GLFW_KEY_R) app.reset = true;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, 1);
}
static void onMouseButton(GLFWwindow*, int b, int a, int) {
    if (b == GLFW_MOUSE_BUTTON_RIGHT) app.rmb = (a == GLFW_PRESS);
}
static void onCursor(GLFWwindow*, double x, double y) {
    if (app.rmb) {
        app.cam.yaw += float(x - app.lx) * 0.3f;
        app.cam.pitch = glm::clamp(app.cam.pitch + float(y - app.ly) * 0.3f, 10.f, 85.f);
    }
    app.lx = x; app.ly = y;
}
static void onScroll(GLFWwindow*, double, double dy) {
    app.cam.dist = glm::clamp(app.cam.dist - float(dy) * 0.8f, 3.f, 25.f);
}

static void run(GLFWwindow* win) {
    Shader sh(std::string(SHADER_DIR) + "phong.vert", std::string(SHADER_DIR) + "phong.frag");
    Mesh cube = Mesh::cube(), plane = Mesh::plane(), sphere = Mesh::sphere(), cyl = Mesh::cylinder();
    Course course;
    Ball ball;

    auto T  = [](glm::vec3 v) { return glm::translate(glm::mat4(1.f), v); };
    auto S  = [](glm::vec3 v) { return glm::scale(glm::mat4(1.f), v); };
    auto Ry = [](float a)     { return glm::rotate(glm::mat4(1.f), a, glm::vec3(0, 1, 0)); };

    auto draw = [&](const Mesh& m, const glm::mat4& M, glm::vec3 color,
                    int type = 0, float shin = 32.f, float spec = 0.3f) {
        sh.setMat4("model", M);
        sh.setMat3("normalMat", glm::transpose(glm::inverse(glm::mat3(M))));
        sh.setVec3("color", color);
        sh.setInt("matType", type);
        sh.setFloat("shininess", shin);
        sh.setFloat("specStrength", spec);
        m.draw();
    };

    float aim = 0.f, power = 0.f, simTime = 0.f, acc = 0.f;
    int strokes = 0;
    bool spaceWas = false;
    const float H = 1.f / 120.f;             

    double prev = glfwGetTime(), fpsTimer = 0; int frames = 0; double fps = 0;

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)std::min(now - prev, 0.1);
        prev = now;
        glfwPollEvents();

        int fw, fh; glfwGetFramebufferSize(win, &fw, &fh);
        if (fh == 0) continue;

        if (app.reset) { ball = Ball(); strokes = 0; power = 0; app.reset = false; }
        bool still = !ball.moving() && ball.sink < 0.f;
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) aim -= 1.5f * dt;
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) aim += 1.5f * dt;
        bool space = glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (still) {
            if (space) power = std::min(1.f, power + 0.8f * dt);
            else if (spaceWas && power > 0.02f) {
                glm::vec2 dir(sinf(aim), -cosf(aim));
                ball.v = dir * power * 10.f;
                strokes++;
                power = 0.f;
            }
        } else power = 0.f;
        spaceWas = space;

        acc += dt;
        while (acc >= H) { course.step(ball, H, simTime); simTime += H; acc -= H; }

        glm::vec3 bp(ball.p.x, 0.f, ball.p.y);
        app.cam.target += (bp - app.cam.target) * (1.f - expf(-6.f * dt));

        // ---------- рендер ----------
        glViewport(0, 0, fw, fh);
        if (app.sunset) glClearColor(0.45f, 0.28f, 0.35f, 1); else glClearColor(0.55f, 0.75f, 0.95f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 P = glm::perspective(glm::radians(50.f), (float)fw / fh, 0.1f, 100.f);
        sh.use();
        sh.setMat4("view", app.cam.view());
        sh.setMat4("projection", P);
        sh.setVec3("viewPos", app.cam.position());

        float t = simTime;
        glm::vec3 sunDir = app.sunset ? glm::normalize(glm::vec3(-0.8f, 0.25f, 0.3f))
                                      : glm::normalize(glm::vec3(0.4f, 1.f, 0.3f));
        glm::vec3 sunCol = app.sunset ? glm::vec3(1.0f, 0.5f, 0.25f) : glm::vec3(1.0f, 0.96f, 0.88f);
        glm::vec3 amb    = app.sunset ? glm::vec3(0.12f, 0.10f, 0.18f) : glm::vec3(0.25f, 0.27f, 0.30f);
        glm::vec3 lampPos(2.f * sinf(t * 0.5f), 1.5f, -2.5f);   // лампа ездит туда-сюда
        glm::vec3 lampCol(1.0f, 0.85f, 0.5f);
        sh.setVec3("sunDir", sunDir);   sh.setVec3("sunColor", sunCol);   sh.setVec3("ambient", amb);
        sh.setVec3("lampPos", lampPos); sh.setVec3("lampColor", lampCol); sh.setInt("lampOn", app.lamp ? 1 : 0);

        draw(plane, S({2 * course.W, 1.f, 2 * course.L}), {1, 1, 1}, 1, 8.f, 0.05f);

        for (const Box& b : course.walls) {
            glm::vec2 c = (b.min + b.max) * 0.5f, s = b.max - b.min;
            draw(cube, T({c.x, 0.15f, c.y}) * S({s.x, 0.3f, s.y}), {0.45f, 0.28f, 0.15f}, 0, 16.f, 0.1f);
        }

        draw(cyl, T({course.hole.x, 0.004f, course.hole.y}) * S({course.holeR, 0.008f, course.holeR}),
             {0.02f, 0.02f, 0.02f}, 0, 8.f, 0.f);

        glm::mat4 poleBase = T({course.hole.x, 0.f, course.hole.y});
        draw(cyl, poleBase * T({0, 0.5f, 0}) * S({0.03f, 1.f, 0.03f}), {0.9f, 0.9f, 0.9f}, 0, 64.f, 0.5f);
        glm::mat4 flagBase = poleBase * T({0, 0.9f, 0}) * Ry(0.25f * sinf(t * 2.f));
        draw(cube, flagBase * T({0.18f, 0, 0}) * S({0.36f, 0.2f, 0.02f}), {0.9f, 0.1f, 0.1f}, 0, 16.f, 0.1f);

        glm::mat4 millBase = T({course.bladeCenter.x, 0.f, course.bladeCenter.y});
        draw(cyl, millBase * T({0, 0.3f, 0}) * S({0.12f, 0.6f, 0.12f}), {0.8f, 0.8f, 0.75f}, 0, 32.f, 0.3f);
        glm::mat4 arm = millBase * T({0, 0.2f, 0}) * Ry(course.bladeSpeed * simTime);
        draw(cube, arm * S({2.4f, 0.3f, 0.2f}), {0.7f, 0.15f, 0.15f}, 0, 32.f, 0.4f);

        float k = ball.sink < 0.f ? 0.f : std::min(ball.sink / 0.5f, 1.f);
        draw(sphere, T({ball.p.x, ball.r - 0.3f * k, ball.p.y}) * S(glm::vec3(ball.r)),
             {1, 1, 1}, 0, 64.f, 0.9f);

        if (still) {
            float len = 0.3f + power * 1.5f;
            glm::mat4 M = T({ball.p.x, 0.03f, ball.p.y}) * Ry(-aim) *
                          T({0, 0, -(0.2f + len / 2)}) * S({0.06f, 0.02f, len});
            draw(cube, M, glm::mix(glm::vec3(0.2f, 0.9f, 0.2f), glm::vec3(0.9f, 0.1f, 0.1f), power), 2);
        }

        if (app.lamp) draw(sphere, T(lampPos) * S(glm::vec3(0.08f)), lampCol, 2);

        glfwSwapBuffers(win);

        frames++; fpsTimer += dt;
        if (fpsTimer >= 0.5) {
            fps = frames / fpsTimer; frames = 0; fpsTimer = 0;
            char title[160];
            std::snprintf(title, sizeof(title), "Mini Golf | FPS: %.0f | Strokes: %d%s",
                          fps, strokes, ball.sink > 0.5f ? " | HOLE! Press R" : "");
            glfwSetWindowTitle(win, title);
        }
    }
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* win = glfwCreateWindow(1280, 720, "Mini Golf", nullptr, nullptr);
    if (!win) { std::fprintf(stderr, "Window creation failed\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(0);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::fprintf(stderr, "GLAD failed\n"); return -1; }

    glfwSetKeyCallback(win, onKey);
    glfwSetMouseButtonCallback(win, onMouseButton);
    glfwSetCursorPosCallback(win, onCursor);
    glfwSetScrollCallback(win, onScroll);

    run(win);
    glfwTerminate();
    return 0;
}