#pragma once

#include <immintrin.h>

/// @brief Uniform random bit generator that wraps the x86 RDRAND hardware
/// instruction.
///
/// Satisfies the C++ UniformRandomBitGenerator concept, so it can be used
/// directly with any `<random>` distribution.
/// Requires the `-mrdrnd` compiler flag on GCC/Clang.
class rdrand32_Engine {
public:
  /// @brief Default constructor.
  rdrand32_Engine() = default;

  /// @brief The generated value type (unsigned 32-bit integer).
  using result_type = unsigned int;

  /// @brief Returns the minimum value produced (0).
  static constexpr result_type(min)() { return 0; }

  /// @brief Returns the maximum value produced (UINT32_MAX).
  static constexpr result_type(max)() { return static_cast<result_type>(-1); }

  /// @brief Invokes the RDRAND instruction and returns the result.
  // use -mrdrnd compiler option for g++
  result_type operator()()
  {
    result_type random = 0;
    _rdrand32_step(&random);
    return random;
  }

  rdrand32_Engine(const rdrand32_Engine &) = delete;
  rdrand32_Engine &operator=(const rdrand32_Engine &) = delete;
};

/// @brief Combines an rdrand32_Engine with a `<random>` distribution.
///
/// Provides a single GenerateRandomNumber() call that draws from the
/// hardware-backed random engine through the given distribution.
///
/// @tparam T A `<random>` distribution type (e.g.
/// std::normal_distribution<float>).
template <typename T> class RandomNumberGenerator {
public:
  /// @brief The distribution's result type.
  using U = typename T::result_type;

  /// @brief Constructs the generator with a copy of the given distribution.
  /// @param distribution Distribution to sample from.
  explicit RandomNumberGenerator(const T &distribution)
      : distribution(distribution)
  {
  }

  /// @brief Draws one random number from the distribution using RDRAND.
  U GenerateRandomNumber() { return distribution(rdrand); }

private:
  rdrand32_Engine rdrand; ///< Hardware entropy source.
  T distribution;         ///< Probability distribution applied to raw bits.
};
