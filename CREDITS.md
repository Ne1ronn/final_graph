# Credits

The following third-party libraries are used by Mini Golf. GLFW and GLM are
external build dependencies; the other listed source components are bundled in
the repository.

## Libraries

- **Dear ImGui** — created by Omar Cornut; MIT License. The bundled copy and
  license are in `third_party/imgui/` and `third_party/imgui/LICENSE.txt`.
  [Project](https://github.com/ocornut/imgui) ·
  [License](https://github.com/ocornut/imgui/blob/master/LICENSE.txt)
- **GLAD 0.1.36** — generated OpenGL loader by David Herberth; MIT License.
  [Source and license](https://github.com/Dav1dde/glad/tree/v0.1.36)
- **GLFW** — created by Marcus Geelnard and Camilla Löwy; zlib/libpng License.
  [Project](https://github.com/glfw/glfw) ·
  [License](https://www.glfw.org/license.html)
- **GLM (OpenGL Mathematics)** — created by G-Truc Creation; available under
  the MIT License or Happy Bunny License. [Project](https://github.com/g-truc/glm) ·
  [License terms](https://github.com/g-truc/glm/blob/master/copying.txt)

## Included Supporting Code

- **Khronos `khrplatform.h`** — copyright The Khronos Group Inc.; the bundled
  header contains its permissive license notice.
  [Source and notice](https://github.com/KhronosGroup/EGL-Registry/blob/main/api/KHR/khrplatform.h)
- **Dear ImGui `imstb_rectpack`, `imstb_textedit`, and `imstb_truetype` headers**
  — based on libraries by Sean Barrett, with modifications in the bundled Dear
  ImGui copies. These headers offer MIT and public-domain licensing terms;
  their notices are included in each file.
  [Upstream stb project](https://github.com/nothings/stb)

## Project Assets

- No third-party models, image textures, fonts, or tutorial assets are used.
- Mesh geometry, GLSL shaders, and grass/wood textures are project content;
  the textures are generated procedurally at runtime.
