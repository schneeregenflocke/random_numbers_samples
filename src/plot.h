#ifndef HOME_TITAN99_CODE_RANDOM_NUMBERS_SAMPLES_SRC_PLOT_H
#define HOME_TITAN99_CODE_RANDOM_NUMBERS_SAMPLES_SRC_PLOT_H

#include "transform.h"
#include <array>
#include <boost/histogram/axis/regular.hpp>
#include <boost/histogram/fwd.hpp>
#include <boost/histogram/histogram.hpp>
#include <boost/math/distributions.hpp> // NOLINT(misc-include-cleaner)
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <imgui.h>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace math = boost::math;
namespace histogram = boost::histogram;

/// @brief ImGui-based 2D plot canvas with a scrollable grid, axes, PDF curve,
/// and histogram.
///
/// Plot inherits TransformCoordinateSystemInterface to manage the mapping
/// between data coordinates and screen pixels. A typical frame sequence is:
///
/// 1. SetCanvasRect()        — read ImGui cursor position
/// 2. SetPlotRect()          — compute plot sub-area with axis label margins
/// 3. ProceedInteraction()   — handle mouse drag for scrolling
/// 4. ProceedAxes()          — recompute axis line endpoints
/// 5. ProceedGrid()          — recompute grid lines, ticks and labels
/// 6. SetPlotCurve()         — evaluate PDF at 1000 points
/// 7. SetHistogram()         — project histogram bins to canvas
/// 8. Draw()                 — render everything via ImDrawList
class Plot : public TransformCoordinateSystemInterface {
public:
  /// @brief Constructs with default axis limits [-5, 5] x [-0.25, 1] and grid
  /// gaps {1, 0.25}.
  Plot()
      : scrolling(0, 0), axes({-5.0F, 5.0F, -0.25F, 1.0F}),
        grid_gaps({1.0F, 0.25F}),
        curve_color(ImColor(.191F, .526F, .805F, 1.F)),
        histogram_color(ImColor(.944F, .791F, .663F, .534F))
  {
  }

  /// @brief Returns the current axis limits {x_min, x_max, y_min, y_max}.
  [[nodiscard]] val4f GetAxes() const { return this->axes; }

  /// @brief Sets the axis limits.
  void SetAxes(const val4f &axes) { this->axes = axes; }

  /// @brief Returns the current grid step sizes {x_gap, y_gap} in data units.
  [[nodiscard]] std::array<float, 2> GetGridGaps() const
  {
    return this->grid_gaps;
  }

  /// @brief Sets the grid step sizes.
  void SetGridGaps(const std::array<float, 2> &grid_gaps)
  {
    this->grid_gaps = grid_gaps;
  }

  /// @brief Updates the canvas rectangle from the current ImGui cursor position
  /// and available content region.
  void SetCanvasRect()
  {
    const glm::vec2 cursor_screen_pos = ImGui::GetCursorScreenPos();
    const glm::vec2 content_region_avail = ImGui::GetContentRegionAvail();

    canvas_rect =
        rect4f(cursor_screen_pos, cursor_screen_pos + content_region_avail);
  }

  /// @brief Computes the plot sub-rectangle by shrinking the canvas by the
  /// given axis label margins.
  /// @param axis_gap_back  Margin on the left/top side (for axis labels).
  /// @param axis_gap_front Margin on the right/bottom side.
  void SetPlotRect(const float axis_gap_back, const float axis_gap_front)
  {
    this->axis_gap_back = axis_gap_back;
    this->axis_gap_front = axis_gap_front;

    plot_rect = rect4f(
        canvas_rect.lt() + glm::vec2((axis_gap_back * 2), axis_gap_back),
        canvas_rect.rb() + glm::vec2(-axis_gap_back, -(axis_gap_back * 2)));
  }

  /// @brief Registers an invisible ImGui button over the canvas and updates the
  /// scroll offset on right-drag.
  void ProceedInteraction()
  {
    ImGui::InvisibleButton("canvas", canvas_rect.size(true),
                           ImGuiButtonFlags_MouseButtonLeft |
                               ImGuiButtonFlags_MouseButtonRight);

    is_hovered = ImGui::IsItemHovered();
    is_active = ImGui::IsItemActive();

    if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
      scrolling.x += ImGui::GetIO().MouseDelta.x;
      scrolling.y += ImGui::GetIO().MouseDelta.y;
    }

    origin =
        glm::vec2(canvas_rect.l() + scrolling.x, canvas_rect.t() + scrolling.y);
  }

  /// @brief Reconfigures the coordinate transform and recomputes the x/y axis
  /// line endpoints.
  void ProceedAxes()
  {
    transform_coordinate_system.SetAspectRatio(canvas_rect, plot_rect, axes);

    transform_coordinate_system.SetCanvasSizeAndOrigin(origin, scrolling);

    x_axisline_p0 = glm::vec2(canvas_rect.l() + (axis_gap_back * 2),
                              canvas_rect.b() - axis_gap_back);
    x_axisline_p1 = glm::vec2(canvas_rect.r() - axis_gap_back,
                              canvas_rect.b() - axis_gap_back);

    y_axisline_p0 = glm::vec2(canvas_rect.l() + axis_gap_back,
                              canvas_rect.b() - (axis_gap_back * 2));
    y_axisline_p1 = glm::vec2(canvas_rect.l() + axis_gap_back,
                              canvas_rect.t() + axis_gap_back);
  }

  /// @brief Regenerates vertical/horizontal grid lines, axis ticks, and numeric
  /// labels for the current scroll position.
  void ProceedGrid()
  {
    vertical_grid.clear();
    horizontal_grid.clear();

    x_axis_ticks.clear();
    y_axis_ticks.clear();

    x_axis_labels.clear();
    y_axis_labels.clear();

    const float tick_length = 10;
    const int decimal_places = 2;

    const glm::vec2 grid_step = transform_coordinate_system.ScaleToCanvas(
        glm::vec2(grid_gaps.at(0), grid_gaps.at(1)));
    const float magic_x = fmodf(scrolling.x, grid_step.x);

    const val4f scrolled_axes = transform_coordinate_system.GetScrolledAxes();

    float x = plot_rect.l() + magic_x;

    if (magic_x < 0) {
      x += grid_step.x;
    }

    for (; x <= plot_rect.r(); x += grid_step.x) { // NOLINT(bugprone-float-loop-counter,cert-flp30-c,clang-analyzer-security.FloatLoopCounter)
      const std::array<glm::vec2, 2> current_line = {glm::vec2(x, plot_rect.t()),
                                               glm::vec2(x, plot_rect.b())};
      vertical_grid.push_back(current_line);

      const std::array<glm::vec2, 2> current_tick = {
          glm::vec2(x, x_axisline_p0.y),
          glm::vec2(x, x_axisline_p0.y + tick_length)};
      x_axis_ticks.push_back(current_tick);

      const float x_value =
          transform_coordinate_system.ScaleXToPlot(x - plot_rect.l()) +
          scrolled_axes.at(0);
      std::stringstream stream;
      stream << std::fixed << std::setprecision(decimal_places) << x_value;
      const std::tuple<glm::vec2, std::string> current_label =
          std::make_tuple(current_tick.at(1), stream.str());
      x_axis_labels.push_back(current_label);
    }

    const float magic_y = fmodf(-scrolling.y, grid_step.y);

    float y = plot_rect.b() - magic_y;

    if (magic_y < 0) {
      y -= grid_step.y;
    }

    for (; y >= plot_rect.t(); y -= grid_step.y) { // NOLINT(bugprone-float-loop-counter,cert-flp30-c,clang-analyzer-security.FloatLoopCounter)
      const std::array<glm::vec2, 2> current_line = {glm::vec2(plot_rect.l(), y),
                                               glm::vec2(plot_rect.r(), y)};
      horizontal_grid.push_back(current_line);

      const std::array<glm::vec2, 2> current_tick = {
          glm::vec2(y_axisline_p0.x, y),
          glm::vec2(y_axisline_p0.x - tick_length, y)};
      y_axis_ticks.push_back(current_tick);

      const float y_value =
          scrolled_axes.at(2) -
          transform_coordinate_system.ScaleYToPlot(y - plot_rect.b());
      std::stringstream stream;
      stream << std::fixed << std::setprecision(decimal_places) << y_value;
      const std::tuple<glm::vec2, std::string> current_label =
          std::make_tuple(current_tick.at(1), stream.str());
      y_axis_labels.push_back(current_label);
    }
  }

  /// @brief Evaluates the PDF of the given Boost.Math distribution at 1000
  /// evenly-spaced points
  ///        across the visible x axis range and projects them to canvas
  ///        coordinates.
  /// @param distribution A Boost.Math distribution that supports
  /// boost::math::pdf().
  template <typename Ty> void SetPlotCurve(const Ty distribution)
  {
    const val4f scrolled_axes = transform_coordinate_system.GetScrolledAxes();

    const size_t steps = 1000;
    const float step_size =
        scrolled_axes.Lenght(0, 1) / static_cast<float>(steps);

    transformed_curve.resize(steps);

    for (size_t index = 0; index < steps; ++index) {
      const auto float_index = static_cast<float>(index);

      const float current_x_value = scrolled_axes.at(0) + (float_index * step_size);
      const float current_y_value = math::pdf(distribution, current_x_value);

      transformed_curve.at(index) = transform_coordinate_system.TransformToCanvas(
          glm::vec2(current_x_value, current_y_value));
    }
  }

  /// @brief Projects Boost.Histogram bins to canvas pixel rectangles ready for
  /// drawing.
  /// @param histogram A normalised Boost.Histogram with a single regular axis.
  void SetHistogram(
      const histogram::histogram<std::tuple<histogram::axis::regular<>>>
          &histogram)
  {
    bin_array.clear();

    for (histogram::axis::index_type index = 1; index < histogram.size() - 1;
         ++index) {
      const float bin_lower = static_cast<float>(histogram.axis(0).bin(index).lower());
      const float bin_upper = static_cast<float>(histogram.axis(0).bin(index).upper());
      const float histogram_width = bin_upper - bin_lower;
      const float bin_height = static_cast<float>(histogram[index]) / histogram_width; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access,cppcoreguidelines-init-variables)

      const glm::vec2 p0 = transform_coordinate_system.TransformToCanvas(
          glm::vec2(bin_lower, bin_height));
      const glm::vec2 p1 = transform_coordinate_system.TransformToCanvas(
          glm::vec2(bin_upper, 0.0F));

      bin_array.push_back({p0, p1});
    }
  }

  /// @brief Renders all plot elements (background, axes, grid, histogram bars,
  /// PDF curve) via ImDrawList.
  void Draw()
  {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(canvas_rect.lt(), canvas_rect.rb(),
                             ImColor(1.0F, 1.0F, 1.0F, 1.0F));
    draw_list->AddRect(canvas_rect.lt(), canvas_rect.rb(),
                       ImColor(1.0F, 1.0F, 1.0F, 1.0F));
    draw_list->AddRect(plot_rect.lt(), plot_rect.rb(),
                       ImColor(0.0F, 0.0F, 0.0F, 0.5F));

    const ImColor axis_color(0.4F, 0.4F, 1.0F, 1.0F);

    ImFont *font = ImGui::GetIO().Fonts->Fonts[0]; // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

    const float font_size = 14.0F;
    const float tick_gap = 2.0F;

    for (const auto &label : x_axis_labels) {
      auto pos = std::get<0>(label);
      auto text = std::get<1>(label);

      const glm::vec2 font_size_vec =
          font->CalcTextSizeA(font_size, FLT_MAX, 0.0F, text.c_str());
      pos.x = pos.x - (font_size_vec.x * 0.5F);
      pos.y = pos.y + tick_gap;

      draw_list->AddText(font, font_size, pos, axis_color, text.c_str());
    }

    for (const auto &label : y_axis_labels) {
      auto pos = std::get<0>(label);
      auto text = std::get<1>(label);

      const glm::vec2 font_size_vec =
          font->CalcTextSizeA(font_size, FLT_MAX, 0.0F, text.c_str());
      pos.x = pos.x - (font_size_vec.x + tick_gap);
      pos.y = pos.y - (font_size_vec.y * 0.5F);

      draw_list->AddText(font, font_size, pos, axis_color, text.c_str());
    }

    draw_list->PushClipRect(canvas_rect.lt(), canvas_rect.rb(), false);

    draw_list->AddLine(x_axisline_p0, x_axisline_p1, axis_color, 2.0F);
    draw_list->AddLine(y_axisline_p0, y_axisline_p1, axis_color, 2.0F);

    for (const auto &line : x_axis_ticks) {
      draw_list->AddLine(line[0], line[1], axis_color, 2); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    }

    for (const auto &line : y_axis_ticks) {
      draw_list->AddLine(line[0], line[1], axis_color, 2); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    }

    draw_list->PopClipRect();

    draw_list->PushClipRect(plot_rect.lt(), plot_rect.rb(), false);

    const ImColor grid_lines_color(0.8F, 0.8F, 0.8F, 0.5F);

    for (const auto &line : vertical_grid) {
      draw_list->AddLine(line[0], line[1], grid_lines_color); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    }

    for (const auto &line : horizontal_grid) {
      draw_list->AddLine(line[0], line[1], grid_lines_color); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    }

    for (const auto &bin : bin_array) {
      draw_list->AddRect(bin[0], bin[1], histogram_color); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
      draw_list->AddRectFilled(bin[0], bin[1], histogram_color); // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    }

    draw_list->AddPolyline(transformed_curve.data(),
                           static_cast<int>(transformed_curve.size()),
                           curve_color, ImDrawFlags_None, 2.0F);

    draw_list->PopClipRect();
  }

  ImColor curve_color;     ///< Colour used for the PDF curve polyline.
  ImColor histogram_color; ///< Colour used to fill and outline histogram bars.

private:
  rect4f canvas_rect;
  rect4f plot_rect;

  float axis_gap_back = 0.0F;
  float axis_gap_front = 0.0F;

  bool is_hovered = false;
  bool is_active = false;

  glm::vec2 scrolling;
  glm::vec2 origin{};

  val4f axes;
  std::array<float, 2> grid_gaps;

  glm::vec2 x_axisline_p0{};
  glm::vec2 x_axisline_p1{};

  glm::vec2 y_axisline_p0{};
  glm::vec2 y_axisline_p1{};


  std::vector<std::array<glm::vec2, 2>> vertical_grid;
  std::vector<std::array<glm::vec2, 2>> horizontal_grid;

  std::vector<std::array<glm::vec2, 2>> x_axis_ticks;
  std::vector<std::array<glm::vec2, 2>> y_axis_ticks;

  std::vector<std::tuple<glm::vec2, std::string>> x_axis_labels;
  std::vector<std::tuple<glm::vec2, std::string>> y_axis_labels;

  std::vector<ImVec2> transformed_curve;
  std::vector<std::array<glm::vec2, 2>> bin_array;
};

#endif // HOME_TITAN99_CODE_RANDOM_NUMBERS_SAMPLES_SRC_PLOT_H
