/*
random_samples
Copyright(c) 2020 Marco Peyer

This program is free software; you can redistribute itand /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110 - 1301 USA.

See <https://www.gnu.org/licenses/gpl-2.0.txt>.
*/

#ifndef RANDOM_SAMPLES_GLFW_INCLUDE_H
#define RANDOM_SAMPLES_GLFW_INCLUDE_H

#include <epoxy/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <array>
#include <iostream>
#include <string>
#include <utility>

/// @brief Manages GLFW window creation, OpenGL context setup and the main loop lifecycle.
///
/// Initialises GLFW, requests an OpenGL 4.5 Core Profile context, sets the
/// clear colour, and provides helpers for polling events, clearing the
/// framebuffer and swapping buffers. Window position is set only on X11 because
/// Wayland does not support glfwSetWindowPos().
class GLFW_Interface {
public:
  /// @brief Creates a GLFW window and initialises the OpenGL context.
  /// @param window_width  Initial window width in screen pixels.
  /// @param window_height Initial window height in screen pixels.
  GLFW_Interface(int window_width, int window_height)
  {
    const std::string window_title = "random_samples";

    glfwSetErrorCallback(error_callback);
    glfwInit();

    SetupHints();

    window = glfwCreateWindow(window_width, window_height, window_title.c_str(),
                              nullptr, nullptr);
    if (glfwGetPlatform() == GLFW_PLATFORM_X11) {
      glfwSetWindowPos(window, kWindowInitX, kWindowInitY);
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    const std::array<float, 4> clear_color{kClearColorR, kClearColorG,
                                           kClearColorB, kClearColorA};
    glClearColor(clear_color.at(0), clear_color.at(1), clear_color.at(2),
                 clear_color.at(3));
  }

  /// @brief Sets GLFW window hints for OpenGL 4.5 Core Profile with 4x MSAA.
  static void SetupHints()
  {
    const int major_version = 4;
    const int minor_version = 5;
    const int msaa_samples = 4;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major_version);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor_version);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, static_cast<int>(true));

    glfwWindowHint(GLFW_SAMPLES, msaa_samples);

    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    // glfwWindowHint(GLFW_MAXIMIZED, true);
  }

  /// @brief Returns the GLSL version string required by the ImGui OpenGL3 backend.
  [[nodiscard]] static std::string GetGLSLVersion() { return {"#version 450"}; }

  /// @brief Returns the underlying GLFWwindow pointer.
  GLFWwindow *GetWindow() { return window; }

  /// @brief GLFW error callback that prints the error to stderr.
  static void error_callback(int error, const char *description)
  {
    std::cerr << "Glfw Error " << error << ": " << description << '\n';
  }

  /// @brief GLFW framebuffer resize callback that updates the OpenGL viewport.
  static void framebuffer_size_callback(GLFWwindow * /*window*/, int width,
                                        int height)
  {
    glViewport(0, 0, width, height);
  }

  /// @brief Returns the current framebuffer size as {width, height} in pixels.
  std::array<int, 2> GetFrambufferSize()
  {
    int width = 0;
    int height = 0;

    glfwGetFramebufferSize(window, &width, &height);

    return std::array<int, 2>{width, height};
  }

  /// @brief Returns true while the window should stay open; also calls glfwPollEvents().
  bool Active()
  {
    const bool active = (glfwWindowShouldClose(window) == 0);

    if (active) {
      glfwPollEvents();
    }

    return active;
  }

  /// @brief Clears the colour buffer.
  static void Clear() { glClear(GL_COLOR_BUFFER_BIT); }

  /// @brief Swaps the front and back buffers (presents the rendered frame).
  void SwapBuffers() { glfwSwapBuffers(window); }

  /// @brief Destroys the window and terminates GLFW.
  void Terminate()
  {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

private:
  static constexpr float kClearColorR{0.45F};
  static constexpr float kClearColorG{0.55F};
  static constexpr float kClearColorB{0.60F};
  static constexpr float kClearColorA{1.00F};
  static constexpr int kWindowInitX{50};
  static constexpr int kWindowInitY{50};

  GLFWwindow *window;
};

#endif // RANDOM_SAMPLES_GLFW_INCLUDE_H
