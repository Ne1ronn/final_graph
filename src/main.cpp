#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "Physics.h"
#include "Shadow.h"
#include "Particles.h"

struct App {
    Camera cam;
    bool lamp = true, sunset = false, shadows = true, preview = true;
    bool reset = false, next = false, rmb = false;
    double lx = 0, ly = 0;
} app;

static void onKey(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    switch (key) {
        case GLFW_KEY_L: app.lamp = !app.lamp; break;
        case GLFW_KEY_N: app.sunset = !app.sunset; break;
        case GLFW_KEY_T: app.shadows = !app.shadows; break;
        case GLFW_KEY_P: app.preview = !app.preview; break;
        case GLFW_KEY_R: app.reset = true; break;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER: app.next = true; break;
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, 1); break;
    }
}
static void onMouseButton(GLFWwindow*, int b, int a, int) {
    if (b != GLFW_MOUSE_BUTTON_RIGHT) return;
    if (a == GLFW_PRESS && ImGui::GetIO().WantCaptureMouse) return;
    app.rmb = (a == GLFW_PRESS);
}
static void onCursor(GLFWwindow*, double x, double y) {
    if (app.rmb) {
        app.cam.yaw += float(x - app.lx) * 0.3f;
        app.cam.pitch = glm::clamp(app.cam.pitch + float(y - app.ly) * 0.3f, 10.f, 85.f);
    }
    app.lx = x; app.ly = y;
}
static void onScroll(GLFWwindow*, double, double dy) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    app.cam.dist = glm::clamp(app.cam.dist - float(dy) * 0.8f, 3.f, 25.f);
}

static void run(GLFWwindow* win) {
    const std::string dir = SHADER_DIR;
    Shader sh(dir + "phong.vert", dir + "phong.frag");
    Shader depthSh(dir + "depth.vert", dir + "depth.frag");
    Mesh cube = Mesh::cube(), plane = Mesh::plane(), sphere = Mesh::sphere(), cyl = Mesh::cylinder();
    ShadowMap shadow(2048);
    Confetti confetti;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    const int LEVELS = 3;
    int parTotal = 0;
    for (int i = 0; i < LEVELS; i++) parTotal += Course::make(i).par;

    Course course;
    Ball ball;
    int level = 0, strokes = 0, scores[LEVELS] = {0, 0, 0};
    float aim = 0.f, power = 0.f, simTime = 0.f, acc = 0.f;
    bool holeDone = false, spaceWas = false;

    auto loadLevel = [&](int i) {
        level = i;
        course = Course::make(i);
        ball = Ball();
        ball.p = course.start;
        strokes = 0; power = 0.f; holeDone = false;
        if (i == 0) for (int& s : scores) s = 0;
        scores[i] = 0;
        aim = atan2f(course.hole.x - course.start.x, -(course.hole.y - course.start.y));
        confetti.clear();
        app.cam.target = glm::vec3(ball.p.x, 0.f, ball.p.y);
    };
    loadLevel(0);

    auto T  = [](glm::vec3 v) { return glm::translate(glm::mat4(1.f), v); };
    auto S  = [](glm::vec3 v) { return glm::scale(glm::mat4(1.f), v); };
    auto Ry = [](float a)     { return glm::rotate(glm::mat4(1.f), a, glm::vec3(0, 1, 0)); };

    bool shadowPass = false;
    auto draw = [&](const Mesh& m, const glm::mat4& M, glm::vec3 color, int type = 0,
                    float shin = 32.f, float spec = 0.3f, bool casts = true) {
        if (shadowPass) {
            if (casts) { depthSh.setMat4("model", M); m.draw(); }
            return;
        }
        sh.setMat4("model", M);
        sh.setMat3("normalMat", glm::transpose(glm::inverse(glm::mat3(M))));
        sh.setVec3("color", color);
        sh.setInt("matType", type);
        sh.setFloat("shininess", shin);
        sh.setFloat("specStrength", spec);
        m.draw();
    };

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

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (app.reset) { app.reset = false; loadLevel(level); }
        if (app.next) {
            app.next = false;
            if (holeDone) loadLevel(level + 1 < LEVELS ? level + 1 : 0);
        }

        bool still = !ball.moving() && ball.sink < 0.f;
        if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) aim -= 1.5f * dt;
        if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) aim += 1.5f * dt;
        bool space = glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS;
        if (still) {
            if (space) power = std::min(1.f, power + 0.8f * dt);
            else if (spaceWas && power > 0.02f) {
                ball.v = glm::vec2(sinf(aim), -cosf(aim)) * power * 10.f;
                strokes++;
                power = 0.f;
            }
        } else power = 0.f;
        spaceWas = space;

        acc += dt;
        while (acc >= H) { course.step(ball, H, simTime); simTime += H; acc -= H; }
        confetti.update(dt);
        if (ball.sink > 0.5f && !holeDone) {
            holeDone = true;
            scores[level] = strokes;
            confetti.burst(glm::vec3(course.hole.x, 0.3f, course.hole.y));
        }

        glm::vec3 bp(ball.p.x, 0.f, ball.p.y);
        app.cam.target += (bp - app.cam.target) * (1.f - expf(-6.f * dt));

        float t = simTime;
        glm::vec3 sunDir = app.sunset ? glm::normalize(glm::vec3(-0.8f, 0.25f, 0.3f))
                                      : glm::normalize(glm::vec3(0.4f, 1.f, 0.3f));
        glm::vec3 sunCol = app.sunset ? glm::vec3(1.0f, 0.5f, 0.25f) : glm::vec3(1.0f, 0.96f, 0.88f);
        glm::vec3 amb    = app.sunset ? glm::vec3(0.12f, 0.10f, 0.18f) : glm::vec3(0.25f, 0.27f, 0.30f);
        glm::vec3 skyTop = app.sunset ? glm::vec3(0.18f, 0.15f, 0.35f) : glm::vec3(0.20f, 0.45f, 0.85f);
        glm::vec3 skyHor = app.sunset ? glm::vec3(0.95f, 0.50f, 0.30f) : glm::vec3(0.75f, 0.88f, 1.00f);
        glm::vec3 lampPos(course.hole.x + 2.f * sinf(t * 0.5f), 1.5f, course.hole.y + 2.f);
        glm::vec3 lampCol(1.0f, 0.85f, 0.5f);

        glm::mat4 lightView = glm::lookAt(sunDir * 18.f, glm::vec3(0.f), glm::vec3(0, 1, 0));
        glm::mat4 lightProj = glm::ortho(-9.f, 9.f, -9.f, 9.f, 1.f, 40.f);
        glm::mat4 lightSpace = lightProj * lightView;

        auto drawScene = [&]() {
            draw(plane, S({2 * course.W, 1.f, 2 * course.L}), {1, 1, 1}, 1, 8.f, 0.05f, false);

            for (const Box& b : course.walls) {
                glm::vec2 c = (b.min + b.max) * 0.5f, s = b.max - b.min;
                draw(cube, T({c.x, 0.15f, c.y}) * S({s.x, 0.3f, s.y}), {0.55f, 0.34f, 0.18f}, 4, 16.f, 0.1f);
            }

            draw(cyl, T({course.hole.x, 0.004f, course.hole.y}) * S({course.holeR, 0.008f, course.holeR}),
                 {0.02f, 0.02f, 0.02f}, 0, 8.f, 0.f, false);

            glm::mat4 poleBase = T({course.hole.x, 0.f, course.hole.y});
            draw(cyl, poleBase * T({0, 0.5f, 0}) * S({0.03f, 1.f, 0.03f}), {0.9f, 0.9f, 0.9f}, 0, 64.f, 0.5f);
            glm::mat4 flagBase = poleBase * T({0, 0.9f, 0}) * Ry(0.25f * sinf(t * 2.f));
            draw(cube, flagBase * T({0.18f, 0, 0}) * S({0.36f, 0.2f, 0.02f}), {0.9f, 0.1f, 0.1f}, 0, 16.f, 0.1f);

            for (const Blade& bl : course.blades) {
                glm::mat4 base = T({bl.center.x, 0.f, bl.center.y});
                draw(cyl, base * T({0, 0.3f, 0}) * S({0.12f, 0.6f, 0.12f}), {0.8f, 0.8f, 0.75f}, 0, 32.f, 0.3f);
                glm::mat4 arm = base * T({0, 0.2f, 0}) * Ry(bl.speed * simTime);
                draw(cube, arm * S({2.f * bl.halfLen, 0.3f, 0.2f}), {0.7f, 0.15f, 0.15f}, 0, 32.f, 0.4f);
            }

            float k = ball.sink < 0.f ? 0.f : std::min(ball.sink / 0.5f, 1.f);
            draw(sphere, T({ball.p.x, ball.r - 0.3f * k, ball.p.y}) * S(glm::vec3(ball.r)),
                 {1, 1, 1}, 0, 64.f, 0.9f);

            if (shadowPass) return;

            if (still) {
                float len = 0.3f + power * 1.5f;
                glm::mat4 M = T({ball.p.x, 0.03f, ball.p.y}) * Ry(-aim) *
                              T({0, 0, -(0.2f + len / 2)}) * S({0.06f, 0.02f, len});
                draw(cube, M, glm::mix(glm::vec3(0.2f, 0.9f, 0.2f), glm::vec3(0.9f, 0.1f, 0.1f), power), 2, 1, 0, false);

                if (app.preview) {
                    float pw = std::max(power, 0.35f);
                    glm::vec2 d(sinf(aim), -cosf(aim));
                    auto path = predictPath(course, ball, d * pw * 10.f, simTime);
                    for (size_t i = 0; i < path.size(); i++) {
                        glm::vec3 c = glm::mix(glm::vec3(1.f), glm::vec3(1.f, 0.8f, 0.2f), i / (float)path.size());
                        draw(sphere, T({path[i].x, 0.05f, path[i].y}) * S(glm::vec3(0.035f)), c, 2, 1, 0, false);
                    }
                }
            }

            for (const Particle& p : confetti.ps)
                draw(cube, T(p.p) * Ry(p.life * 8.f) * S({0.06f, 0.012f, 0.04f}), p.color, 2, 1, 0, false);

            if (app.lamp) draw(sphere, T(lampPos) * S(glm::vec3(0.08f)), lampCol, 2, 1, 0, false);
        };

        if (app.shadows) {
            shadowPass = true;
            shadow.begin();
            depthSh.use();
            depthSh.setMat4("lightSpace", lightSpace);
            drawScene();
            shadow.end(fw, fh);
            shadowPass = false;
        }

        glClearColor(skyHor.r, skyHor.g, skyHor.b, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 P = glm::perspective(glm::radians(50.f), (float)fw / fh, 0.1f, 100.f);
        glm::vec3 camPos = app.cam.position();
        sh.use();
        sh.setMat4("view", app.cam.view());
        sh.setMat4("projection", P);
        sh.setMat4("lightSpace", lightSpace);
        sh.setVec3("viewPos", camPos);
        sh.setVec3("sunDir", sunDir);   sh.setVec3("sunColor", sunCol);   sh.setVec3("ambient", amb);
        sh.setVec3("lampPos", lampPos); sh.setVec3("lampColor", lampCol); sh.setInt("lampOn", app.lamp ? 1 : 0);
        sh.setVec3("skyTop", skyTop);   sh.setVec3("skyHorizon", skyHor);
        sh.setInt("shadowsOn", app.shadows ? 1 : 0);
        sh.setInt("shadowMap", 0);
        shadow.bind(0);

        glDepthMask(GL_FALSE);
        draw(cube, T(camPos) * S(glm::vec3(80.f)), {1, 1, 1}, 3, 1, 0, false);
        glDepthMask(GL_TRUE);

        drawScene();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
        ImGui::Begin("Mini Golf", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Hole %d/%d   Par %d", level + 1, LEVELS, course.par);
        ImGui::Text("Strokes: %d   FPS: %.0f", strokes, fps);
        ImGui::ProgressBar(power, ImVec2(180, 0), "Power");
        ImGui::Separator();
        ImGui::Checkbox("Lamp (L)", &app.lamp);
        ImGui::Checkbox("Sunset (N)", &app.sunset);
        ImGui::Checkbox("Shadows (T)", &app.shadows);
        ImGui::Checkbox("Trajectory preview (P)", &app.preview);
        ImGui::Separator();
        ImGui::TextUnformatted("A/D: aim\nSPACE: hold and release to shoot\nRMB drag: orbit camera\nMouse wheel: zoom\nR: restart hole\nEnter: next hole");
        ImGui::End();

        if (holeDone) {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.4f),
                                    ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::Begin("Hole complete!", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Text("Hole %d: %d strokes (par %d, %+d)", level + 1, strokes, course.par, strokes - course.par);
            if (level + 1 == LEVELS) {
                int total = 0; for (int s : scores) total += s;
                ImGui::Separator();
                for (int i = 0; i < LEVELS; i++) ImGui::Text("Hole %d: %d", i + 1, scores[i]);
                ImGui::Text("Total: %d (par %d, %+d)", total, parTotal, total - parTotal);
            }
            if (ImGui::Button(level + 1 < LEVELS ? "Next hole (Enter)" : "Play again (Enter)")) app.next = true;
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);

        frames++; fpsTimer += dt;
        if (fpsTimer >= 0.5) {
            fps = frames / fpsTimer; frames = 0; fpsTimer = 0;
            char title[160];
            std::snprintf(title, sizeof(title), "Mini Golf | FPS: %.0f | Hole %d | Strokes: %d",
                          fps, level + 1, strokes);
            glfwSetWindowTitle(win, title);
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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