# Mini Golf

## 1. Project Overview

Mini Golf is a small interactive 3D mini-golf game built with C++ and OpenGL.
Players aim and shoot a ball through three courses with walls and moving
obstacles. The game includes orbit-camera controls, lighting and shadow options,
trajectory preview, scoring, and a simple ImGui interface.

## 2. Team Members

| Name | Group | Responsibilities |
|---|---|---|
| Yesset Kozhakhmet | SE-2427 | OpenGL rendering pipeline, shaders and lighting, camera and input, core gameplay, scoring and UI |
| Nurtore Dauylbay | SE-2427 | Course physics and moving obstacles, shadow mapping, procedural textures, and third-party dependency integration |

## 3. Features

- Real-time 3D rendering with an OpenGL 3.3 Core Profile pipeline
- GLSL vertex and fragment shaders
- Per-fragment Blinn-Phong lighting with ambient, diffuse, and specular terms
- Directional sunlight and a moving, toggleable lamp
- Shadow mapping with a toggleable shadow pass
- Procedural grass and wood textures with UV mapping and mipmaps
- Orbit camera controlled by keyboard and mouse
- Three mini-golf courses, ball physics, moving obstacles, and score tracking
- Aim and trajectory preview, power control, and confetti on hole completion
- ImGui controls, on-screen hints, and an FPS counter

## 4. Technologies and Libraries

- C++17
- OpenGL 3.3 Core Profile
- GLFW
- GLAD (bundled)
- GLM
- Dear ImGui (bundled)
- CMake

## 5. Build Instructions

### Requirements

- macOS with Xcode Command Line Tools
- Homebrew
- CMake 3.16 or newer
- GLFW and GLM
- A graphics driver that supports OpenGL 3.3

Install the required tools and libraries:

```sh
xcode-select --install
brew install cmake glfw glm
```

### Building

Run from the project root:

```sh
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Running

Run the application from the project root:

```sh
./build/minigolf
```

The window is created at 1280x720. Shader paths are configured by CMake to use
the project's `shaders/` directory.

## 6. Controls

| Key / Input | Action |
|---|---|
| Right mouse drag | Orbit camera |
| Mouse wheel | Zoom camera |
| Arrow keys | Orbit camera |
| Q / E | Zoom camera |
| A / D | Aim |
| Hold and release Space | Shoot |
| L | Toggle lamp |
| N | Toggle sunset lighting |
| T | Toggle shadows |
| P | Toggle trajectory preview |
| R | Restart current hole |
| Enter | Advance after completing a hole |
| Escape | Exit |

## 7. Project Structure

- `src/main.cpp` — application setup, input, game loop, and UI
- `src/Scene.h` — scene resources, lighting, shadow pass, and rendering
- `src/Camera.h` — orbit camera
- `src/Mesh.h` — vertex format, VAO/VBO/EBO setup, and procedural meshes
- `src/Physics.h` — courses, ball physics, collisions, and trajectory prediction
- `src/Shader.h` — shader compilation, linking, and uniform helpers
- `src/Shadow.h` — shadow-map framebuffer and texture
- `src/Texture.h` — procedural texture generation and mipmaps
- `src/Particles.h` — confetti particle effects
- `shaders/` — Phong and depth-pass GLSL shaders
- `third_party/` — bundled GLAD and Dear ImGui sources
- `CMakeLists.txt` — build configuration

## 8. AI Usage Note

An AI assistant using Claude and Copilot SDK in VS Code assisted with code review,
refactoring, and preparing project documentation. The project team is responsible for
reviewing the resulting code and for the final project submission.

## 9. Credits

See [CREDITS.md](./CREDITS.md) for third-party components and asset information.
