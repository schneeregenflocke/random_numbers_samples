#ifndef HOME_TITAN99_CODE_RANDOM_NUMBERS_SAMPLES_SRC_RECT4_H
#define HOME_TITAN99_CODE_RANDOM_NUMBERS_SAMPLES_SRC_RECT4_H

#include <array>
#include <cmath>
#include <cstddef>
#include <glm/fwd.hpp>
#include <glm/glm.hpp> // NOLINT(misc-include-cleaner)
#include <stdexcept>

/// @brief Axis-aligned 2D rectangle stored as four edge values.
///
/// The edges are ordered as left, bottom, right, top.
/// Helper methods compute width, height, size, and corner vectors,
/// and support expand/reduce/scale operations.
///
/// @tparam Ty Numeric type for the edge values (e.g. float).
template <typename Ty> class rect4 {
public:
  /// @brief Default-constructs a zero rectangle.
  rect4() : edges{0, 0, 0, 0} {}

  /// @brief Constructs from a 4-element array {l, b, r, t}.
  explicit rect4(const std::array<Ty, 4> &edges) : edges(edges) {}

  /// @brief Constructs from explicit left, bottom, right, top values.
  // left, bottom, right, top
  rect4(const Ty l, const Ty b, const Ty r, const Ty t) : edges{l, b, r, t} {}

  /// @brief Constructs a centred rectangle from width and height.
  /// @param width  Total width; extends from -width/2 to +width/2.
  /// @param height Total height; extends from -height/2 to +height/2.
  rect4(const Ty width, const Ty height)
  {
    Ty op = static_cast<Ty>(2);
    edges[0] = -width / op;
    edges[1] = -height / op;
    edges[2] = width / op;
    edges[3] = height / op;
  }

  /// @brief Constructs from a left-top and right-bottom corner vector.
  /// @param lt Left-top corner (x = left, y = top).
  /// @param rb Right-bottom corner (x = right, y = bottom).
  rect4(const glm::vec<2, Ty> &lt, const glm::vec<2, Ty> &rb)
  {
    edges[0] = lt.x;
    edges[1] = rb.y;
    edges[2] = rb.x;
    edges[3] = lt.y;
  }

  /// @brief Returns the left edge.
  [[nodiscard]] Ty l() const { return edges[0]; }

  /// @brief Returns the bottom edge.
  [[nodiscard]] Ty b() const { return edges[1]; }

  /// @brief Returns the right edge.
  [[nodiscard]] Ty r() const { return edges[2]; }

  /// @brief Returns the top edge.
  [[nodiscard]] Ty t() const { return edges[3]; }

  /// @brief Sets the left edge.
  void l(const Ty l) { edges[0] = l; }

  /// @brief Sets the bottom edge.
  void b(const Ty b) { edges[1] = b; }

  /// @brief Sets the right edge.
  void r(const Ty r) { edges[2] = r; }

  /// @brief Sets the top edge.
  void t(const Ty t) { edges[3] = t; }

  /// @brief Returns true when all edges are zero.
  [[nodiscard]] bool is_empty() const
  {
    bool empty = false;

    if (l() == 0 && b() == 0 && r() == 0 && t() == 0) {
      empty = true;
    }

    return empty;
  }

  /// @brief Returns the rectangle width (r - l).
  /// @param use_abs If true, returns the absolute value; otherwise throws if l
  /// > r.
  [[nodiscard]] Ty width(const bool use_abs = false) const
  {
    Ty width = 0;

    if (!use_abs) {
      if (l() > r()) {
        throw std::logic_error("left > right");
      }
      width = r() - l();
    } else {
      width = std::abs(r() - l());
    }

    return width;
  }

  /// @brief Returns the rectangle height (t - b).
  /// @param use_abs If true, returns the absolute value; otherwise throws if b
  /// > t.
  [[nodiscard]] Ty height(const bool use_abs = false) const
  {
    Ty height = 0;

    if (!use_abs) {
      if (b() > t()) {
        throw std::logic_error("bottom > top");
      }
      height = t() - b();
    } else {
      height = std::abs(t() - b());
    }

    return height;
  }

  /// @brief Returns {width, height} as a 2D vector.
  /// @param use_abs Passed to width() and height().
  [[nodiscard]] glm::vec<2, Ty> size(const bool use_abs = false) const
  {
    return glm::vec<2, Ty>(width(use_abs), height(use_abs));
  }

  /// @brief Returns a new rectangle expanded outward by the given margins.
  /// @param expand Margins to add: l/b expand left/bottom outward, r/t expand
  /// right/top outward.
  [[nodiscard]] rect4 expand(const rect4 &expand) const
  {
    rect4 expanded;
    expanded.l(l() - expand.l());
    expanded.b(b() - expand.b());
    expanded.r(r() + expand.r());
    expanded.t(t() + expand.t());
    return expanded;
  }

  /// @brief Returns a new rectangle shrunk inward by the given margins.
  /// @param reduce Margins to remove from each side.
  [[nodiscard]] rect4 reduce(const rect4 &reduce) const
  {
    rect4 reduced;
    reduced.l(l() + reduce.l());
    reduced.b(b() + reduce.b());
    reduced.r(r() - reduce.r());
    reduced.t(t() - reduce.t());
    return reduced;
  }

  /// @brief Returns a new rectangle uniformly scaled about its centre.
  /// @param factor Scaling factor (> 1 enlarges, < 1 shrinks).
  [[nodiscard]] rect4 scale(const Ty factor) const
  {
    float half_width_diff = ((width() * factor) - width()) / static_cast<Ty>(2);
    float half_height_diff =
        ((height() * factor) - height()) / static_cast<Ty>(2);

    rect4 scaled = expand(rect4(half_width_diff, half_width_diff,
                                half_height_diff, half_height_diff));
    return scaled;
  }

  /// @brief Returns the centre point of the rectangle.
  [[nodiscard]] glm::vec<2, Ty> center() const
  {
    return glm::vec<2, Ty>(l() + (width() / static_cast<Ty>(2)),
                           b() + (height() / static_cast<Ty>(2)));
  }

  /// @brief Returns the left-bottom corner vector.
  [[nodiscard]] glm::vec<2, Ty> lb() const { return glm::vec<2, Ty>(l(), b()); }

  /// @brief Returns the right-bottom corner vector.
  [[nodiscard]] glm::vec<2, Ty> rb() const { return glm::vec<2, Ty>(r(), b()); }

  /// @brief Returns the left-top corner vector.
  [[nodiscard]] glm::vec<2, Ty> lt() const { return glm::vec<2, Ty>(l(), t()); }

  /// @brief Returns the right-top corner vector.
  [[nodiscard]] glm::vec<2, Ty> rt() const { return glm::vec<2, Ty>(r(), t()); }

private:
  std::array<Ty, 4> edges; ///< Internal storage: {left, bottom, right, top}.
};

/// @brief Convenience alias for a float-typed rect4.
using rect4f = rect4<float>;

/// @brief Generic 4-element value array with a range-length helper.
///
/// Used to represent axis limits (x_min, x_max, y_min, y_max) and similar
/// ordered quadruples that are not geometrically oriented like rect4.
///
/// @tparam Ty Numeric type for the stored values.
template <typename Ty> class val4 {
public:
  /// @brief Default-constructs with all values zero.
  val4() : val{0, 0, 0, 0} {}

  /// @brief Constructs from a 4-element array.
  explicit val4(const std::array<Ty, 4> &val) : val(val) {}

  /// @brief Returns the span between two indexed elements (val[to] -
  /// val[from]).
  /// @param from Index of the start element.
  /// @param to   Index of the end element.
  [[nodiscard]] Ty Lenght(const size_t from, const size_t to) const
  {
    return val.at(to) - val.at(from);
  }

  /// @brief Mutable element access.
  Ty &operator[](size_t index) { return val.at(index); }

  /// @brief Const element access.
  Ty operator[](size_t index) const { return val.at(index); }

  /// @brief Bounds-checked mutable element access.
  Ty &at(size_t index) { return val.at(index); }

  /// @brief Bounds-checked const element access.
  [[nodiscard]] const Ty &at(size_t index) const { return val.at(index); }

private:
  std::array<Ty, 4> val; ///< Internal 4-element storage.
};

/// @brief Convenience alias for a float-typed val4.
using val4f = val4<float>;

#endif // HOME_TITAN99_CODE_RANDOM_NUMBERS_SAMPLES_SRC_RECT4_H
