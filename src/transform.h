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

#include "rect4.h"

#include <glm/glm.hpp>

#include "glfw_include.h"

#include <imgui.h>
// #include <imgui_impl_glfw.h>
// #include <imgui_impl_opengl3.h>

/// @brief Maps between plot (data) coordinates and canvas (pixel) coordinates.
///
/// The transform is defined by the canvas rectangle, a plot sub-rectangle
/// within the canvas, the axis limits in data space, and the current scroll
/// offset. Call SetAspectRatio() and SetCanvasSizeAndOrigin() before using
/// any of the Transform/Scale methods.
class TransformCoordinateSystem {
public:
  /// @brief Default constructor.
  TransformCoordinateSystem() = default;

  /// @brief Configures the coordinate mapping from data to canvas space.
  /// @param canvas_rect Full canvas rectangle in screen pixels.
  /// @param plot_rect   Plot area rectangle (inside the canvas, excluding axis labels).
  /// @param axes        Axis limits {x_min, x_max, y_min, y_max} in data space.
  void SetAspectRatio(const rect4f &canvas_rect, const rect4f &plot_rect,
                      const val4f &axes)
  {
    this->plot_rect = plot_rect;
    this->canvas_rect = canvas_rect;
    this->axes = axes;

    aspect_ratio =
        plot_rect.size(true) / glm::vec2(axes.Lenght(0, 1), axes.Lenght(2, 3));
  }

  /// @brief Sets the canvas origin and current scroll offset (in pixels).
  /// @param origin    Top-left pixel position of the canvas on screen.
  /// @param scrolling Accumulated pixel-space scroll delta.
  void SetCanvasSizeAndOrigin(const glm::vec2 &origin,
                              const glm::vec2 &scrolling)
  {
    this->origin = origin;
    this->scrolling = scrolling;
  }

  /// @brief Returns the axis limits adjusted for the current scroll offset.
  val4f GetScrolledAxes() const
  {
    glm::vec2 scaled_scrolling = ScaleToPlot(scrolling);

    val4f scrolled_axes;
    scrolled_axes[0] = axes[0] - scaled_scrolling.x;
    scrolled_axes[1] = axes[1] - scaled_scrolling.x;
    scrolled_axes[2] = axes[2] + scaled_scrolling.y;
    scrolled_axes[3] = axes[3] + scaled_scrolling.y;

    return scrolled_axes;
  }

  /// @brief Converts a data-space point to a canvas pixel position.
  /// @param point Data-space {x, y} coordinates.
  /// @return Screen pixel position within the canvas.
  glm::vec2 TransformToCanvas(const glm::vec2 &point) const
  {
    glm::vec2 scaled_point = ScaleToCanvas(point);
    float scaled_x_axis_begin = ScaleXToCanvas(axes[0]);
    float scaled_y_axis_begin = ScaleYToCanvas(axes[2]);

    float axes_x_gap = plot_rect.l() - canvas_rect.l();
    float axes_y_gap = std::abs(canvas_rect.b() - plot_rect.b());

    glm::vec2 result;
    result.x = origin.x + axes_x_gap - scaled_x_axis_begin + scaled_point.x;
    result.y = origin.y + canvas_rect.height(true) - axes_y_gap +
               scaled_y_axis_begin - scaled_point.y;

    return result;
  }

  /// @brief Scales a 2D data-space vector to canvas pixel units.
  glm::vec2 ScaleToCanvas(const glm::vec2 &value) const
  {
    glm::vec2 result;
    result.x = aspect_ratio.x * value.x;
    result.y = aspect_ratio.y * value.y;

    return result;
  }

  /// @brief Scales a single x data value to canvas pixel units.
  float ScaleXToCanvas(const float value) const
  {
    return aspect_ratio.x * value;
  }

  /// @brief Scales a single y data value to canvas pixel units.
  float ScaleYToCanvas(const float value) const
  {
    return aspect_ratio.y * value;
  }

  /// @brief Converts a 2D canvas pixel vector to data-space units.
  glm::vec2 ScaleToPlot(const glm::vec2 &value) const
  {
    glm::vec2 result;
    result.x = (1.0f / aspect_ratio.x) * value.x;
    result.y = (1.0f / aspect_ratio.y) * value.y;
    return result;
  }

  /// @brief Converts a canvas x pixel value to data-space units.
  float ScaleXToPlot(const float value) const
  {
    return (1.0f / aspect_ratio.x) * value;
  }

  /// @brief Converts a canvas y pixel value to data-space units.
  float ScaleYToPlot(const float value) const
  {
    return (1.0f / aspect_ratio.y) * value;
  }

private:
  glm::vec2 aspect_ratio; ///< Pixels-per-data-unit ratio for x and y.

  glm::vec2 origin;    ///< Canvas top-left in screen pixels.
  glm::vec2 scrolling; ///< Accumulated scroll delta in screen pixels.

  rect4f canvas_rect; ///< Full canvas area in screen pixels.
  rect4f plot_rect;   ///< Plot sub-area within the canvas (excludes axis labels).

  val4f axes; ///< Axis limits {x_min, x_max, y_min, y_max} in data space.
};

/// @brief Mixin that exposes a TransformCoordinateSystem to derived classes.
///
/// Classes that need to perform coordinate transforms (e.g. Plot) inherit from
/// this interface to gain a shared transform object and the two accessor methods.
class TransformCoordinateSystemInterface {
public:
  /// @brief Returns the current axis limits adjusted for scroll.
  val4f GetScrolledAxes()
  {
    return transform_coordinate_system.GetScrolledAxes();
  }

  /// @brief Returns a copy of the underlying transform object.
  TransformCoordinateSystem GetTransformedCoordinateSystem()
  {
    return transform_coordinate_system;
  }

protected:
  TransformCoordinateSystem transform_coordinate_system; ///< Owned transform instance.
};
