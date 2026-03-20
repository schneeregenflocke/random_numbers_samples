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

#ifndef RANDOM_SAMPLES_HISTOGRAM_H
#define RANDOM_SAMPLES_HISTOGRAM_H

#include "transform.h"

#include <boost/histogram.hpp>
#include <boost/histogram/ostream.hpp>
namespace histogram = boost::histogram;

#include <vector>

/// @brief Wraps Boost.Histogram to produce a normalised frequency histogram.
///
/// Data is filled with per-entry weight = 1/N so that bin heights integrate
/// to 1 and can be directly compared with a probability density curve.
class Histogram {
public:
  /// @brief Constructs with a default bin count of 20.
  Histogram() = default;

  /// @brief Builds and returns a Boost.Histogram from the given data vector.
  ///
  /// Each entry is weighted by 1/N so the histogram approximates the PDF.
  ///
  /// @param data        Sample data values to histogram.
  /// @param lower_limit Left edge of the first bin.
  /// @param upper_limit Right edge of the last bin.
  /// @return A boost::histogram::histogram with a single regular axis.
  template <typename Ty0>
  auto SetHistogram(std::vector<Ty0> &data, const float lower_limit,
                    const float upper_limit)
  {
    const auto axis =
        histogram::axis::regular<>(number_bins, lower_limit, upper_limit);
    auto hist = histogram::make_histogram(axis);
    const float weight_value =
        static_cast<float>(1) / static_cast<float>(data.size());
    hist.fill(data, histogram::weight(weight_value));

    return hist;
  }

  /// @brief Returns the current number of histogram bins.
  [[nodiscard]] unsigned int GetNumberBins() const { return number_bins; }

  /// @brief Sets the number of histogram bins used in the next SetHistogram() call.
  void SetNumberBins(unsigned int bins) { number_bins = bins; }

private:
  static constexpr unsigned int kDefaultBins{20};
  unsigned int number_bins{kDefaultBins}; ///< Number of equally-spaced bins.
};

#endif // RANDOM_SAMPLES_HISTOGRAM_H
