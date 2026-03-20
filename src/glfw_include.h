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

#include <epoxy/gl.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <array>
#include <utility>

class GLFW_Interface
{
public:

	GLFW_Interface(int window_width, int window_height)
	{
		const std::string window_title = "random_samples";
		
		glfwSetErrorCallback(error_callback);
		auto glfw_init = glfwInit();

		SetupHints();

		window = glfwCreateWindow(window_width, window_height, window_title.c_str(), nullptr, nullptr);
		glfwSetWindowPos(window, 50, 50);

		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		std::array<float, 4> clear_color{ 0.45f, 0.55f, 0.60f, 1.00 };
		glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
	}

	void SetupHints() const
	{
		const int major_version = 4;
		const int minor_version = 5;

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major_version);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor_version);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);

		glfwWindowHint(GLFW_SAMPLES, 4);

		//glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		//glfwWindowHint(GLFW_MAXIMIZED, true);
	}

	std::string GetGLSLVersion() const
	{
		return std::string("#version 450");
	}

	GLFWwindow* GetWindow()
	{
		return window;
	}

	static void error_callback(int error, const char* description)
	{
		std::cerr << "Glfw Error " << error << ": " << description << '\n';
	}

	static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	std::array<int, 2> GetFrambufferSize()
	{
		int width;
		int height;

		glfwGetFramebufferSize(window, &width, &height);

		return std::array<int, 2>{width, height};
	}

	bool Active()
	{
		bool active = !glfwWindowShouldClose(window);

		if (active)
		{
			glfwPollEvents();
		}

		return active;
	}

	void Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void SwapBuffers() 
	{
		glfwSwapBuffers(window);
	}

	void Terminate()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

private:

	GLFWwindow* window;
};

