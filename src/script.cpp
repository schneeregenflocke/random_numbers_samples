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

#include "glfw_include.h"
#include "histogram.h"
#include "plot.h"
#include "random_samples.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Resource.h"

#include <boost/math/distributions/normal.hpp>
namespace math = boost::math;

#include <epoxy/gl.h>

#include <any>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Finde die Verteilung der Laengen der anhand der t-Verteilung berechneten
// Konfidenzintervalle einer Normalverteilung in % im Verhaeltnis zur Laenge der
// Konfidenzintervalle berechnet anhand der Normalverteilung (bei gegebener
// Varianz) Finde die Verteilung der wahren Werte auf Konfidenzintervallen
// Vergleiche die Verteilung von Mittelwerten der geometrischen und der
// (abhaengigen) bernoullischen Verteilung

namespace {

// Application window
constexpr int kWindowWidth{1280};
constexpr int kWindowHeight{800};
constexpr float kFontSize{18.0F};

// Startup defaults
constexpr int kDefaultDistributionIndex{11};
constexpr int kDefaultNumSamples{1000};
constexpr int kDefaultNumBins{80};

// ImGui layout factors
constexpr float kHalfDivisor{2.0F};
constexpr float kMsPerSecond{1000.0F};
constexpr float kItemWidthFactor{0.2F};
constexpr float kHalfFactor{0.5F};

// Input step sizes
constexpr float kParamInputStep{0.1F};
constexpr float kAxisInputStep{0.5F};
constexpr int kInputFastStep{10};

// Plot margins
constexpr float kAxisGap{50.0F};

// ImGui style values
constexpr float kScrollbarSize{20.0F};
constexpr float kItemSpacing{8.0F};

} // namespace

int main() // NOLINT(readability-function-cognitive-complexity)
try
{
  ////////////////////////////////////////////////////////////////////////////////

  const std::array<std::array<std::string, 2>, 6> licenses{{
      {"random_samples", LOAD_RESOURCE(random_samples_LICENSE).toString()},
      {"boost", LOAD_RESOURCE(boost_LICENSE_1_0).toString()},
      {"glfw", LOAD_RESOURCE(glfw_LICENSE).toString()},
      {"dear imgui", LOAD_RESOURCE(imgui_LICENSE).toString()},
      {"glm", LOAD_RESOURCE(glm_copying).toString()},
      {"embed-resource", LOAD_RESOURCE(embed_resource_LICENSE).toString()},
  }};

  ////////////////////////////////////////////////////////////////////////////////

  GLFW_Interface glfw_interface(kWindowWidth, kWindowHeight);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  auto &imgui_io = ImGui::GetIO();
#ifdef _WIN32
  if (imgui_io.Fonts->AddFontFromFileTTF("c:/Windows/Fonts/Arial.ttf", kFontSize) == nullptr) {
    imgui_io.Fonts->AddFontDefault();
  }
#else
  if (imgui_io.Fonts->AddFontFromFileTTF(
          "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", kFontSize) == nullptr) {
    imgui_io.Fonts->AddFontDefault();
  }
#endif
  imgui_io.ConfigWindowsResizeFromEdges = true;

  ImGui::StyleColorsLight();
  ImGuiStyle &style = ImGui::GetStyle();
  style.FrameRounding = 4;
  style.WindowBorderSize = 0;
  style.FrameBorderSize = 0;
  style.ScrollbarSize = kScrollbarSize;
  style.WindowRounding = 0;
  style.FramePadding = ImVec2(4, 4);
  style.ItemSpacing = ImVec2(kItemSpacing, kItemSpacing);

  ImGui_ImplGlfw_InitForOpenGL(glfw_interface.GetWindow(), true);
  ImGui_ImplOpenGL3_Init(GLFW_Interface::GetGLSLVersion().c_str());

  GLint sample_buffers = 0; // NOLINT(misc-include-cleaner)
  glGetIntegerv(GL_SAMPLES, &sample_buffers); // NOLINT(misc-include-cleaner)

  ////////////////////////////////////////////////////////////////////////////////

  SamplerCollection sampler_collection;
  Plot plot;
  Histogram plot_histogram;

  ////////////////////////////////////////////////////////////////////////////////
  // some provisional global init variables

  int random_distribution_index = kDefaultDistributionIndex;
  int number_samples = kDefaultNumSamples;
  int sample_size = 1;
  int sample_functions_index = 1;
  std::vector<float> current_histogram_data;
  bool single_startup_trigger = true;

  plot_histogram.SetNumberBins(kDefaultNumBins);

  bool open_all = true;

  ////////////////////////////////////////////////////////////////////////////////

  while (glfw_interface.Active() && open_all) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ////////////////////////////////////////////////////////////////////////////////

    const auto [fm_width, fm_height] = glfw_interface.GetFrambufferSize();

    ImGui::SetNextWindowPos(
        glm::vec2(static_cast<float>(fm_width) / kHalfDivisor, 0),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        glm::vec2(static_cast<float>(fm_width) / kHalfDivisor,
                  static_cast<float>(fm_height)),
        ImGuiCond_Always);
    const auto user_window_flags = static_cast<ImGuiWindowFlags>( // NOLINT(hicpp-signed-bitwise)
        static_cast<unsigned>(ImGuiWindowFlags_NoSavedSettings) |
        static_cast<unsigned>(ImGuiWindowFlags_NoMove) |
        static_cast<unsigned>(ImGuiWindowFlags_NoResize));
    ImGui::Begin("User Input", &open_all, user_window_flags);

    ImGui::TextUnformatted(
        std::format("{:.2f} ms/frame ({:.2f} FPS)",
                    kMsPerSecond / imgui_io.Framerate,
                    imgui_io.Framerate).c_str());

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Random Samples")) {
      const float item_width = ImGui::GetContentRegionAvail().x * kItemWidthFactor;
      const float half_avail = ImGui::GetContentRegionAvail().x * kHalfFactor;

      const std::string distribution_combo_label =
          sampler_collection.GetName(random_distribution_index);

      if (ImGui::BeginCombo("Random Distribution",
                            distribution_combo_label.c_str())) {
        for (int index = 0; std::cmp_less(index, sampler_collection.GetSize()); ++index) {
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
      const float input_step = kParamInputStep;

      auto *current_distribution =
          sampler_collection.GetDistribution(random_distribution_index);
      const auto current_parameter_type =
          current_distribution->GetParameterType();

      int parameters_changed = 0;

      if (current_parameter_type == integer_2) {
        auto params = current_distribution->GetParameters();
        auto param0 = std::any_cast<int>(params.at(0));
        auto param1 = std::any_cast<int>(params.at(1));

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += static_cast<int>(ImGui::InputInt(
            current_distribution->GetParameterNames().at(0).c_str(), &param0));

        ImGui::SameLine(half_avail);
        ImGui::SetNextItemWidth(item_width);
        parameters_changed += static_cast<int>(ImGui::InputInt(
            current_distribution->GetParameterNames().at(1).c_str(), &param1));

        current_distribution->SetParameters<int, int>(param0, param1);
      }
      if (current_parameter_type == rationale_2) {
        auto params = current_distribution->GetParameters();
        auto param0 = std::any_cast<float>(params.at(0));
        auto param1 = std::any_cast<float>(params.at(1));

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += static_cast<int>(ImGui::InputFloat(
            current_distribution->GetParameterNames().at(0).c_str(), &param0,
            input_step, input_step, format_string.c_str()));

        ImGui::SameLine(half_avail);
        ImGui::SetNextItemWidth(item_width);
        parameters_changed += static_cast<int>(ImGui::InputFloat(
            current_distribution->GetParameterNames().at(1).c_str(), &param1,
            input_step, input_step, format_string.c_str()));

        current_distribution->SetParameters<float, float>(param0, param1);
      }
      if (current_parameter_type == rationale_1) {
        auto params = current_distribution->GetParameters();
        auto param0 = std::any_cast<float>(params.at(0));

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += static_cast<int>(ImGui::InputFloat(
            current_distribution->GetParameterNames().at(0).c_str(), &param0,
            input_step, input_step, format_string.c_str()));

        current_distribution->SetParameters<float>(param0);
      }

      if (current_parameter_type == double_1) {
        auto params = current_distribution->GetParameters();
        auto param0 = static_cast<float>(std::any_cast<double>(params.at(0)));

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += static_cast<int>(ImGui::InputFloat(
            current_distribution->GetParameterNames().at(0).c_str(), &param0,
            input_step, input_step, format_string.c_str()));

        current_distribution->SetParameters<double>(
            static_cast<double>(param0));
      }

      if (current_parameter_type == integer_1_double_1) {
        auto params = current_distribution->GetParameters();
        auto param0 = std::any_cast<int>(params.at(0));
        auto param1 = static_cast<float>(std::any_cast<double>(params.at(1)));

        ImGui::SetNextItemWidth(item_width);
        parameters_changed += static_cast<int>(ImGui::InputInt(
            current_distribution->GetParameterNames().at(0).c_str(), &param0));

        ImGui::SameLine(half_avail);
        ImGui::SetNextItemWidth(item_width);
        parameters_changed += static_cast<int>(ImGui::InputFloat(
            current_distribution->GetParameterNames().at(1).c_str(), &param1,
            input_step, input_step, format_string.c_str()));

        current_distribution->SetParameters<int, double>(
            param0, static_cast<double>(param1));
      }

      bool sampler_config_changed = false;

      ImGui::SetNextItemWidth(item_width);
      if (ImGui::InputInt("number samples", &number_samples, 1, kInputFastStep)) {
        sampler_config_changed = true;
      }
      ImGui::SameLine(half_avail);
      ImGui::SetNextItemWidth(item_width);
      if (ImGui::InputInt("samples size", &sample_size, 1, kInputFastStep)) {
        sampler_config_changed = true;
      }

      current_distribution->SetSamplerConfig(number_samples, sample_size);

      if (ImGui::Button("(re-)generate samples") || sampler_config_changed ||
          (parameters_changed != 0) || single_startup_trigger) {
        current_distribution->GenerateSamples();
        single_startup_trigger = false;
      }

      auto sample_function_names =
          current_distribution->GetSampleFunctionNames();
      std::string sample_function_combo_label = "none";
      if (!sample_function_names.empty()) {
        sample_function_combo_label =
            sample_function_names.at(static_cast<size_t>(sample_functions_index));
      }

      if (ImGui::BeginCombo("sample function results",
                            sample_function_combo_label.c_str())) {
        for (int index = 0; std::cmp_less(index, sample_function_names.size()); ++index) {
          const bool is_selected = (sample_functions_index == index);

          if (ImGui::Selectable(
                  sample_function_names.at(static_cast<size_t>(index)).c_str(),
                  is_selected)) {
            sample_functions_index = index;
          }
          if (is_selected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }

      if (!sample_function_names.empty()) {
        current_histogram_data = std::any_cast<std::vector<float>>(
            current_distribution->GetSampleFunctionResults(
                sample_function_combo_label));
      }
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Histogram")) {
      const float item_width = ImGui::GetContentRegionAvail().x * kItemWidthFactor;

      auto number_bins = static_cast<int>(plot_histogram.GetNumberBins());

      ImGui::SetNextItemWidth(item_width);
      ImGui::InputInt("Number Bins", &number_bins, 1, 1);

      plot_histogram.SetNumberBins(static_cast<unsigned int>(number_bins));
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Axis Limits")) {
      auto axes = plot.GetAxes();

      const float item_width = ImGui::GetContentRegionAvail().x * kItemWidthFactor;
      const float half_avail = ImGui::GetContentRegionAvail().x * kHalfFactor;

      const float input_step = kAxisInputStep;
      const std::string format_string = "%.2f";

      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("X-Axis Lower Limit", &axes[0], input_step, input_step, // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                        format_string.c_str());

      ImGui::SameLine(half_avail);
      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("Y-Axis Lower Limit", &axes[2], input_step, input_step, // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                        format_string.c_str());

      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("X-Axis Upper Limit", &axes[1], input_step, input_step, // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                        format_string.c_str());

      ImGui::SameLine(half_avail);
      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("Y-Axis Upper Limit", &axes[3], input_step, input_step, // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                        format_string.c_str());

      plot.SetAxes(axes);

      ImGui::Separator();

      auto gaps = plot.GetGridGaps();

      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("X-Axis Grid Gap", gaps.data(), input_step, input_step,
                        format_string.c_str());

      ImGui::SameLine(half_avail);
      ImGui::SetNextItemWidth(item_width);
      ImGui::InputFloat("Y-Axis Grid Gap", &gaps.at(1), input_step, input_step,
                        format_string.c_str());

      plot.SetGridGaps(gaps);
    }

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Colors")) {
      const auto color_edit_flags =
          static_cast<ImGuiColorEditFlags>( // NOLINT(hicpp-signed-bitwise)
              static_cast<unsigned>(ImGuiColorEditFlags_NoInputs) |
              static_cast<unsigned>(ImGuiColorEditFlags_AlphaBar) |
              static_cast<unsigned>(ImGuiColorEditFlags_Float) |
              static_cast<unsigned>(ImGuiColorEditFlags_DisplayRGB) |
              static_cast<unsigned>(ImGuiColorEditFlags_InputRGB));
      ImGui::ColorEdit4("Curve Color", &plot.curve_color.Value.x,
                        color_edit_flags);
      ImGui::ColorEdit4("Histogram Color", &plot.histogram_color.Value.x,
                        color_edit_flags);
    }

    ImGui::SetNextItemOpen(false, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Licenses")) {
      for (const auto &license : licenses) {
        if (ImGui::TreeNode(license.at(0).c_str())) {
          std::vector<char> license_text(license.at(1).cbegin(),
                                         license.at(1).cend());

          ImGui::InputTextMultiline(
              (std::string("##") + license.at(0)).c_str(),
              license_text.data(), license_text.size(), ImVec2(0, 0),
              ImGuiInputTextFlags_ReadOnly);
          ImGui::TreePop();
        }
      }
    }

    ImGui::End();

    ////////////////////////////////////////////////////////////////////////////////

    static const float mean = 0.0F;
    static const float stddev = 1.0F;
    const math::normal_distribution<float> normal_distribution(mean, stddev);

    ////////////////////////////////////////////////////////////////////////////////

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        glm::vec2(static_cast<float>(fm_width) / kHalfDivisor,
                  static_cast<float>(fm_height)),
        ImGuiCond_Always);
    const auto plot_window_flags = static_cast<ImGuiWindowFlags>( // NOLINT(hicpp-signed-bitwise)
        static_cast<unsigned>(ImGuiWindowFlags_NoSavedSettings) |
        static_cast<unsigned>(ImGuiWindowFlags_NoMove) |
        static_cast<unsigned>(ImGuiWindowFlags_NoResize));
    ImGui::Begin("Plot", &open_all, plot_window_flags);

    plot.SetCanvasRect();
    plot.SetPlotRect(kAxisGap, kAxisGap);

    plot.ProceedInteraction();
    plot.ProceedAxes();

    plot.ProceedGrid();
    plot.SetPlotCurve(normal_distribution);

    auto histogram = plot_histogram.SetHistogram(
        current_histogram_data,
        plot.GetScrolledAxes()[0], // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
        plot.GetScrolledAxes()[1]); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

    plot.SetHistogram(histogram);

    plot.Draw();

    ImGui::End();

    ////////////////////////////////////////////////////////////////////////////////

    ImGui::Render();
    GLFW_Interface::Clear();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfw_interface.SwapBuffers();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfw_interface.Terminate();

  return EXIT_SUCCESS;
}
catch (const std::exception &exc)
{
  std::cerr << "Fatal error: " << exc.what() << '\n';
  return EXIT_FAILURE;
}
