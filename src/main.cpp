#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <algorithm>
#include <cstdio>
#include "Camera.h"
#include "Physics.h"
#include "Particles.h"
#include "Scene.h"

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
    Scene scene(SHADER_DIR);
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

    const float H = 1.f / 120.f;
    double prev = glfwGetTime(), fpsTimer = prev;
    int frames = 0;
    double fps = 0;
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

        if (glfwGetKey(win, GLFW_KEY_LEFT)  == GLFW_PRESS) app.cam.yaw   -= 90.f * dt;
        if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) app.cam.yaw   += 90.f * dt;
        if (glfwGetKey(win, GLFW_KEY_UP)    == GLFW_PRESS) app.cam.pitch  = std::min(85.f, app.cam.pitch + 60.f * dt);
        if (glfwGetKey(win, GLFW_KEY_DOWN)  == GLFW_PRESS) app.cam.pitch  = std::max(10.f, app.cam.pitch - 60.f * dt);
        if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) app.cam.dist = std::min(25.f, app.cam.dist + 6.f * dt);
        if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) app.cam.dist = std::max(3.f,  app.cam.dist - 6.f * dt);

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

        scene.render(fw, fh, course, ball, app.cam, confetti, aim, power,
                     simTime, app.lamp, app.sunset, app.shadows, app.preview);

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
        ImGui::TextUnformatted("A/D: aim\nSPACE: hold and release to shoot\nArrows: orbit camera\nQ/E: zoom\nRMB drag: orbit camera\nMouse wheel: zoom\nR: restart hole\nEnter: next hole");
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

        frames++;
        double fpsNow = glfwGetTime();
        double fpsElapsed = fpsNow - fpsTimer;
        if (fpsElapsed >= 0.5) {
            fps = frames / fpsElapsed;
            frames = 0;
            fpsTimer = fpsNow;
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
    glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::fprintf(stderr, "GLAD failed\n"); return -1; }

    glfwSetKeyCallback(win, onKey);
    glfwSetMouseButtonCallback(win, onMouseButton);
    glfwSetCursorPosCallback(win, onCursor);
    glfwSetScrollCallback(win, onScroll);

    run(win);
    glfwTerminate();
    return 0;
}