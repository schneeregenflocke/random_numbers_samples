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

#pragma once

#include <GLFW/glfw3.h>
#include <epoxy/gl.h>

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
    auto glfw_init = glfwInit();

    SetupHints();

    window = glfwCreateWindow(window_width, window_height, window_title.c_str(),
                              nullptr, nullptr);
    if (glfwGetPlatform() == GLFW_PLATFORM_X11)
      glfwSetWindowPos(window, 50, 50);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    std::array<float, 4> clear_color{0.45f, 0.55f, 0.60f, 1.00};
    glClearColor(clear_color[0], clear_color[1], clear_color[2],
                 clear_color[3]);
  }

  /// @brief Sets GLFW window hints for OpenGL 4.5 Core Profile with 4x MSAA.
  void SetupHints() const
  {
    const int major_version = 4;
    const int minor_version = 5;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major_version);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor_version);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

    glfwWindowHint(GLFW_SAMPLES, 4);

    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    // glfwWindowHint(GLFW_MAXIMIZED, true);
  }

  /// @brief Returns the GLSL version string required by the ImGui OpenGL3 backend.
  std::string GetGLSLVersion() const { return std::string("#version 450"); }

  /// @brief Returns the underlying GLFWwindow pointer.
  GLFWwindow *GetWindow() { return window; }

  /// @brief GLFW error callback that prints the error to stderr.
  static void error_callback(int error, const char *description)
  {
    std::cerr << "Glfw Error " << error << ": " << description << '\n';
  }

  /// @brief GLFW framebuffer resize callback that updates the OpenGL viewport.
  static void framebuffer_size_callback(GLFWwindow *window, int width,
                                        int height)
  {
    glViewport(0, 0, width, height);
  }

  /// @brief Returns the current framebuffer size as {width, height} in pixels.
  std::array<int, 2> GetFrambufferSize()
  {
    int width;
    int height;

    glfwGetFramebufferSize(window, &width, &height);

    return std::array<int, 2>{width, height};
  }

  /// @brief Returns true while the window should stay open; also calls glfwPollEvents().
  bool Active()
  {
    bool active = !glfwWindowShouldClose(window);

    if (active) {
      glfwPollEvents();
    }

    return active;
  }

  /// @brief Clears the colour buffer.
  void Clear() { glClear(GL_COLOR_BUFFER_BIT); }

  /// @brief Swaps the front and back buffers (presents the rendered frame).
  void SwapBuffers() { glfwSwapBuffers(window); }

  /// @brief Destroys the window and terminates GLFW.
  void Terminate()
  {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

private:
  GLFWwindow *window;
};
