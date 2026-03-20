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

/// @file script.cpp
/// @brief Application entry point.
///
/// Initialises GLFW/OpenGL, sets up the ImGui context, and runs the main render
/// loop. The loop is split into two ImGui windows:
///  - **Plot** (left half): renders the PDF curve and the histogram of the
///    currently selected sample function results via Plot.
///  - **User Input** (right half): exposes controls for the random distribution,
///    its parameters, sample count/size, histogram bin count, axis limits and
///    colours, plus a collapsible licence viewer.

#include "plot.h"
#include "random_samples.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Resource.h"

#include <boost/math/distributions.hpp>
namespace math = boost::math;

#include <algorithm>
#include <random>
#include <sstream>
#include <string>

// Finde die Verteilung der L�ngen der anhand der t-Verteilung berechneten
// Konfidenzintervalle einer Normalverteilung in % im Verh�ltnis zur L�nge der
// Konfidenzintervalle berechnet anhand der Normalverteilung (bei gegebener
// Varianz) Finde die Verteilung der wahren Werte auf Konfidenzintervallen
// Vergleiche die Verteilung von Mittelwerten der geometrischen und der
// (abh�ngigen) bernoullischen Verteilung

int main(int argc, char *argv[])
{
  ////////////////////////////////////////////////////////////////////////////////

  std::vector<std::array<std::string, 2>> licenses(6);

  licenses[0][0] = "random_samples";
  licenses[0][1] = LOAD_RESOURCE(random_samples_LICENSE).toString();

  licenses[1][0] = "boost";
  licenses[1][1] = LOAD_RESOURCE(boost_LICENSE_1_0).toString();

  licenses[2][0] = "glfw";
  licenses[2][1] = LOAD_RESOURCE(glfw_LICENSE).toString();

  licenses[3][0] = "dear imgui";
  licenses[3][1] = LOAD_RESOURCE(imgui_LICENSE).toString();

  licenses[4][0] = "glm";
  licenses[4][1] = LOAD_RESOURCE(glm_copying).toString();

  licenses[5][0] = "embed-resource";
  licenses[5][1] = LOAD_RESOURCE(embed_resource_LICENSE).toString();

  ////////////////////////////////////////////////////////////////////////////////

  GLFW_Interface glfw_interface(1280, 800);

  bool check_imgui = IMGUI_CHECKVERSION();
  ImGuiContext *context = ImGui::CreateContext();

  auto &io = ImGui::GetIO();
#if defined(_WIN32)
  ImFont *font =
      io.Fonts->AddFontFromFileTTF("c:/Windows/Fonts/Arial.ttf", 18.0f);
#else
  ImFont *font = io.Fonts->AddFontFromFileTTF(
      "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 18.0f);
#endif
  if (!font)
    font = io.Fonts->AddFontDefault();
  io.ConfigWindowsResizeFromEdges = true;

  ImGui::StyleColorsLight();
  ImGuiStyle &style = ImGui::GetStyle();
  style.FrameRounding = 4;
  style.WindowBorderSize = 0;
  style.FrameBorderSize = 0;
  style.ScrollbarSize = 20;
  style.WindowRounding = 0;
  style.FramePadding = ImVec2(4, 4);
  style.ItemSpacing = ImVec2(8, 8);

  ImGui_ImplGlfw_InitForOpenGL(glfw_interface.GetWindow(), true);
  ImGui_ImplOpenGL3_Init(glfw_interface.GetGLSLVersion().c_str());

  GLint sample_buffers = 0;
  glGetIntegerv(GL_SAMPLES, &sample_buffers);
  // std::cout << "sample_buffers " << sample_buffers << '\n';

  ////////////////////////////////////////////////////////////////////////////////

  SamplerCollection sampler_collection;
  Plot plot;
  Histogram plot_histogram;

  ////////////////////////////////////////////////////////////////////////////////
  // some provisional global init variables

  const float axis_gap = 50;
  int random_distribution_index = 11;
  int number_samples = 1000;
  int sample_size = 1;
  int sample_functions_index = 1;
  std::vector<float> current_histogram_data;
  bool single_startup_trigger = true;

  plot_histogram.SetNumberBins(80);

  bool open_all = true;

  ////////////////////////////////////////////////////////////////////////////////

  while (glfw_interface.Active() && open_all == true) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ////////////////////////////////////////////////////////////////////////////////

    const auto fm_size = glfw_interface.GetFrambufferSize();

    ImGui::SetNextWindowPos(glm::vec2(static_cast<float>(fm_size[0]) / 2.f, 0),
                            ImGuiCond_Always);
    ImGui::SetNextWindowSize(glm::vec2(static_cast<float>(fm_size[0]) / 2.f,
                                       static_cast<float>(fm_size[1])),
                             ImGuiCond_Always);
    ImGui::Begin("User Input", &open_all,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize);

    ImGui::Text("%.2f ms/frame (%.2f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Random Samples")) {
      const float item_width = ImGui::GetContentRegionAvail().x * 0.2f;
      const float half_avail = ImGui::GetContentRegionAvail().x * 0.5f;

      std::string distribution_combo_label =
          sampler_collection.GetName(random_distribution_index).c_str();

      if (ImGui::BeginCombo("Random Distribution",
                            distribution_combo_label.c_str())) {
        for (int index = 0; index < sampler_collection.GetSize(); ++index) {
          const bool is_selected = (random_distribution_index == index);

          if (ImGui::Selectable(sampler_collection.GetName(index).c_str(),
                                is_selected)) {
            random_distribution_index = index;
          }
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      const std::string format_string = "%.2f";
      const float input_step = 0.1f;

      auto current_distribution =
          sampler_collection.GetDistribution(random_distribution_index);
      const auto current_parameter_type =
          current_distribution->GetParameterType();

      int parameters_changed = 0;

      if (current_parameter_type == integer_2) {
        auto params = current_distribution->GetParameters();
        auto param0 = std::any_cast<int>(params[0]);
        auto param1 = std::any_cast<int>(params[1]);

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += ImGui::InputInt(
            current_distribution->GetParameterNames()[0].c_str(), &param0);

        ImGui::SameLine(half_avail);
        ImGui::SetNextItemWidth(item_width);
        parameters_changed += ImGui::InputInt(
            current_distribution->GetParameterNames()[1].c_str(), &param1);

        current_distribution->SetParameters<int, int>(param0, param1);
      }
      if (current_parameter_type == rationale_2) {
        auto params = current_distribution->GetParameters();
        auto param0 = std::any_cast<float>(params[0]);
        auto param1 = std::any_cast<float>(params[1]);

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += ImGui::InputFloat(
            current_distribution->GetParameterNames()[0].c_str(), &param0,
            input_step, input_step, format_string.c_str());

        ImGui::SameLine(half_avail);
        ImGui::SetNextItemWidth(item_width);
        parameters_changed += ImGui::InputFloat(
            current_distribution->GetParameterNames()[1].c_str(), &param1,
            input_step, input_step, format_string.c_str());

        current_distribution->SetParameters<float, float>(param0, param1);
      }
      if (current_parameter_type == rationale_1) {
        auto params = current_distribution->GetParameters();
        auto param0 = std::any_cast<float>(params[0]);

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += ImGui::InputFloat(
            current_distribution->GetParameterNames()[0].c_str(), &param0,
            input_step, input_step, format_string.c_str());

        current_distribution->SetParameters<float>(param0);
      }

      if (current_parameter_type == double_1) {
        auto params = current_distribution->GetParameters();
        auto param0 = static_cast<float>(std::any_cast<double>(params[0]));

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += ImGui::InputFloat(
            current_distribution->GetParameterNames()[0].c_str(), &param0,
            input_step, input_step, format_string.c_str());

        current_distribution->SetParameters<double>(
            static_cast<double>(param0));
      }

      if (current_parameter_type == integer_1_double_1) {
        auto params = current_distribution->GetParameters();
        auto param0 = std::any_cast<int>(params[0]);
        auto param1 = static_cast<float>(std::any_cast<double>(params[1]));

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += ImGui::InputInt(
            current_distribution->GetParameterNames()[0].c_str(), &param0);

        ImGui::SameLine(half_avail);
        ImGui::SetNextItemWidth(item_width);
        parameters_changed += ImGui::InputFloat(
            current_distribution->GetParameterNames()[1].c_str(), &param1,
            input_step, input_step, format_string.c_str());

        current_distribution->SetParameters<int, double>(
            param0, static_cast<double>(param1));
      }

      /*auto sampler_config = current_distribution->GetSamplerConfig();
      int number_samples = static_cast<int>(sampler_config[0]);
      int sample_size = static_cast<int>(sampler_config[1]);*/

      bool sampler_config_changed = false;

      ImGui::SetNextItemWidth(item_width);
      sampler_config_changed =
          ImGui::InputInt("number samples", &number_samples, 1, 10);
      ImGui::SameLine(half_avail);
      ImGui::SetNextItemWidth(item_width);
      sampler_config_changed =
          ImGui::InputInt("samples size", &sample_size, 1, 10);

      current_distribution->SetSamplerConfig(number_samples, sample_size);

      if (ImGui::Button("(re-)generate samples") || sampler_config_changed ||
          parameters_changed || single_startup_trigger) {
        current_distribution->GenerateSamples();
        single_startup_trigger = false;
      }

      auto sample_function_names =
          current_distribution->GetSampleFunctionNames();
      std::string sample_function_combo_label = "none";
      if (sample_function_names.size() > 0) {
        sample_function_combo_label =
            sample_function_names[sample_functions_index];
      }

      if (ImGui::BeginCombo("sample function results",
                            sample_function_combo_label.c_str())) {
        for (int index = 0; index < sample_function_names.size(); ++index) {
          const bool is_selected = (sample_functions_index == index);

          if (ImGui::Selectable(sample_function_names[index].c_str(),
                                is_selected)) {
            sample_functions_index = index;
          }
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      if (sample_function_names.size() > 0) {
        current_histogram_data = std::any_cast<std::vector<float>>(
            current_distribution->GetSampleFunctionResults(
                sample_function_combo_label));
      }
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Histogram")) {
      const float item_width = ImGui::GetContentRegionAvail().x * 0.2f;
      const float half_avail = ImGui::GetContentRegionAvail().x * 0.5f;

      int number_bins = plot_histogram.GetNumberBins();

      ImGui::SetNextItemWidth(item_width);
      ImGui::InputInt("Number Bins", &number_bins, 1, 1);

      plot_histogram.SetNumberBins(number_bins);
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Axis Limits")) {
      auto axes = plot.GetAxes();

      const float item_width = ImGui::GetContentRegionAvail().x * 0.2f;
      const float half_avail = ImGui::GetContentRegionAvail().x * 0.5f;

      const float input_step = 0.5f;
      const std::string format_string = "%.2f";

      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("X-Axis Lower Limit", &axes[0], input_step, input_step,
                        format_string.c_str());

      ImGui::SameLine(half_avail);
      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("Y-Axis Lower Limit", &axes[2], input_step, input_step,
                        format_string.c_str());

      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("X-Axis Upper Limit", &axes[1], input_step, input_step,
                        format_string.c_str());

      ImGui::SameLine(half_avail);
      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("Y-Axis Upper Limit", &axes[3], input_step, input_step,
                        format_string.c_str());

      plot.SetAxes(axes);

      ImGui::Separator();

      auto gaps = plot.GetGridGaps();

      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("X-Axis Grid Gap", &gaps[0], input_step, input_step,
                        format_string.c_str());

      ImGui::SameLine(half_avail);
      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("Y-Axis Grid Gap", &gaps[1], input_step, input_step,
                        format_string.c_str());

      plot.SetGridGaps(gaps);
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Colors")) {
      ImGuiColorEditFlags color_edit_flags =
          ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar |
          ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB |
          ImGuiColorEditFlags_InputRGB;
      ImGui::ColorEdit4("Curve Color", &plot.curve_color.Value.x,
                        color_edit_flags);
      ImGui::ColorEdit4("Histogram Color", &plot.histogram_color.Value.x,
                        color_edit_flags);
    }

    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Licenses")) {
      const float item_width = ImGui::GetContentRegionAvail().x * 0.2f;
      const float half_avail = ImGui::GetContentRegionAvail().x * 0.5f;

      for (size_t index = 0; index < licenses.size(); ++index) {
        if (ImGui::TreeNode(licenses[index][0].c_str())) {
          std::vector<char> license_text(licenses[index][1].cbegin(),
                                         licenses[index][1].cend());

          ImGui::InputTextMultiline(
              (std::string("##") + licenses[index][0]).c_str(),
              license_text.data(), license_text.size(), ImVec2(0, 0),
              ImGuiInputTextFlags_ReadOnly); // ImGuiInputTextFlags_Multiline
          ImGui::TreePop();
        }
      }
    }

    ImGui::End();

    ////////////////////////////////////////////////////////////////////////////////

    static float mean = 0.f;
    static float stddev = 1.f;
    math::normal_distribution<float> normal_distribution(mean, stddev);

    ////////////////////////////////////////////////////////////////////////////////

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(glm::vec2(static_cast<float>(fm_size[0]) / 2.f,
                                       static_cast<float>(fm_size[1])),
                             ImGuiCond_Always);
    ImGui::Begin("Plot", &open_all,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize);

    plot.SetCanvasRect();
    plot.SetPlotRect(axis_gap, 50);

    plot.ProceedInteraction();
    plot.ProceedAxes();

    plot.ProceedGrid();
    plot.SetPlotCurve(normal_distribution);

    auto histogram = plot_histogram.SetHistogram(current_histogram_data,
                                                 plot.GetScrolledAxes()[0],
                                                 plot.GetScrolledAxes()[1]);

    plot.SetHistogram(histogram);

    plot.Draw();

    ImGui::End();

    ////////////////////////////////////////////////////////////////////////////////

    // ImGui::ShowDemoWindow();

    ////////////////////////////////////////////////////////////////////////////////

    ImGui::Render();
    glfw_interface.Clear();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfw_interface.SwapBuffers();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfw_interface.Terminate();

  // system("pause");

  return EXIT_SUCCESS;
}
