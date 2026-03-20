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

#include "file_io.h"
#include "random_data_table.h"
#include "random_numbers.h"

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

/// @brief Classifies the parameter signature of a probability distribution.
///
/// Used by the UI to decide which ImGui input widgets to render.
enum ParameterTypes {
  integer_2,          ///< Two integer parameters (e.g. uniform_int min/max).
  rationale_2,        ///< Two floating-point parameters (e.g. normal mean/sigma).
  rationale_1,        ///< One floating-point parameter (e.g. chi-squared n).
  double_1,           ///< One double parameter (e.g. bernoulli p).
  integer_1_double_1  ///< One integer and one double parameter (e.g. binomial t/p).
};

/// @brief Stores the name, parameter type tag, parameter names, and current parameter values
///        for a probability distribution.
///
/// Parameters are held as std::any so the same class can represent distributions
/// with different numeric types (int, float, double).
class DistributionParameters {
public:
  /// @brief Constructs the descriptor for a named distribution.
  /// @param distribution_name Human-readable name shown in the UI combo box.
  /// @param parameter_types   Enum tag describing the parameter signature.
  /// @param parameter_names   Labels for up to two parameters (shown in the UI).
  DistributionParameters(const std::string &distribution_name,
                         const ParameterTypes parameter_types,
                         const std::vector<std::string> &parameter_names)
      : distribution_name(distribution_name), parameter_types(parameter_types),
        parameter_names(parameter_names)
  {
  }

  /// @brief Updates stored parameters; nullopt arguments leave the current value unchanged.
  /// @tparam Ty0 Type of the first parameter.
  /// @tparam Ty1 Type of the second parameter (defaults to Ty0).
  template <typename Ty0, typename Ty1 = Ty0>
  void SetParameters(std::optional<Ty0> parameter0 = std::nullopt,
                     std::optional<Ty1> parameter1 = std::nullopt)
  {
    if (parameter0.has_value()) {
      parameters[0] = parameter0.value();
    }

    if (parameter1.has_value()) {
      parameters[1] = parameter1.value();
    }
  }

  /// @brief Returns the two stored parameters as a 2-element std::any array.
  std::array<std::any, 2> GetParameters() { return parameters; }

  /// @brief Returns the parameter type tag for this distribution.
  ParameterTypes GetParameterType() { return parameter_types; }

  /// @brief Returns the human-readable distribution name.
  std::string GetName() { return distribution_name; }

  /// @brief Returns the parameter label strings.
  std::vector<std::string> GetParameterNames() { return parameter_names; }

private:
  std::string distribution_name;
  std::vector<std::string> parameter_names;
  ParameterTypes parameter_types;
  std::array<std::any, 2> parameters;
};

/// @brief Abstract interface for a typed sampling manager.
///
/// Extends DistributionParameters with the operations needed by the UI:
/// configuring sample count / size, triggering generation, and retrieving
/// results without exposing the concrete distribution type.
class SamplingManagerInterface : public DistributionParameters {
public:
  /// @brief Constructs the interface, forwarding arguments to DistributionParameters.
  SamplingManagerInterface(const std::string &distribution_name,
                           const ParameterTypes parameter_types,
                           const std::vector<std::string> &parameter_names)
      : DistributionParameters(distribution_name, parameter_types,
                               parameter_names)
  {
  }

  /// @brief Sets the number of samples and sample size for the next GenerateSamples() call.
  virtual void SetSamplerConfig(const size_t number_samples,
                                const size_t sample_size) = 0;
  /// @brief Returns the current {number_samples, sample_size} configuration.
  virtual std::array<size_t, 2> GetSamplerConfig() const = 0;
  /// @brief Generates all samples using the current parameters and configuration.
  virtual void GenerateSamples() = 0;

  /// @brief Returns the sample at the given index wrapped in std::any.
  virtual std::any GetSample(const size_t index) const = 0;
  /// @brief Returns the names of the computed statistic columns.
  virtual std::vector<std::string> GetSampleFunctionNames() const = 0;
  /// @brief Returns the values of the named statistic column as std::any.
  virtual std::any GetSampleFunctionResults(const std::string &name) const = 0;
};

/// @brief Concrete sampling manager for a specific `<random>` distribution type.
///
/// Holds a DataTable that stores the raw sample values and their statistics.
/// UpdateParameterPackage() has one specialisation per supported distribution
/// and is selected via SFINAE based on DistributionTy.
///
/// @tparam DistributionTy A C++ standard `<random>` distribution (e.g. std::normal_distribution<float>).
/// @tparam RationalTy     Floating-point type used for computed statistics results.
template <typename DistributionTy, typename RationalTy>
class SamplingManager : public SamplingManagerInterface {
public:
  /// @brief Constructs the manager and reads default parameters from the distribution.
  SamplingManager(const std::string &distribution_name,
                  const ParameterTypes parameter_types,
                  const std::vector<std::string> &parameter_names)
      : SamplingManagerInterface(distribution_name, parameter_types,
                                 parameter_names),
        sampler_config({1000, 30})
  {
    UpdateParameterPackage(true);
  }

  using ResultTy = typename DistributionTy::result_type;
  using ParametersTy = typename DistributionTy::param_type;

  virtual void SetSamplerConfig(const size_t number_samples,
                                const size_t sample_size) override
  {
    sampler_config[0] = number_samples;
    sampler_config[1] = sample_size;
  }
  virtual std::array<size_t, 2> GetSamplerConfig() const override
  {
    return sampler_config;
  }

  void GenerateSamples() override
  {
    random_distribution.reset();
    UpdateParameterPackage();

    data_table.GenerateSamples(random_distribution, sampler_config[0],
                               sampler_config[1]);
    data_table.CalculateSampleFunctionResults();
  }

  virtual std::any GetSample(const size_t index) const override
  {
    return data_table.GetSample(index);
  }

  virtual std::vector<std::string> GetSampleFunctionNames() const override
  {
    return data_table.GetSampleFunctionNames();
  }

  virtual std::any
  GetSampleFunctionResults(const std::string &name) const override
  {
    return data_table.GetColumnData(name);
  }

  /// @brief Writes the full data table (headers + data) to the given FileOutput in tab-delimited format.
  void WriteToFile(FileOutput &file_output) const
  {
    for (size_t row_index = 0; row_index < data_table.GetNumberRows();
         ++row_index) {
      for (size_t col_index = 0; col_index < data_table.GetNumberColumns();
           ++col_index) {
        file_output << data_table.GetString(col_index, row_index) << '\t';
      }
      file_output << '\n';
    }
  }

private:
  DistributionTy random_distribution;
  std::array<size_t, 2> sampler_config;
  DataTable<ResultTy, RationalTy> data_table;

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::uniform_int_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<int, int>(param_package.a(), param_package.b());
    } else {
      const auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<ResultTy>(parameters[1]);

      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    // param1 = std::clamp(param1, param0 + 1,
    // std::numeric_limits<result_t>::max()); _STL_ASSERT(_Min0 <= _Max0,
    //"invalid min and max arguments for uniform_int");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::uniform_real_distribution<ResultTy>>::value,
      void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, ResultTy>(param_package.a(), param_package.b());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<ResultTy>(parameters[1]);

      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    // float add = std::nextafter(param0, std::numeric_limits<float>::max());
    //_STL_ASSERT(_Min0 <= _Max0 && (0 <= _Min0 || _Max0 <= _Min0 +
    //(numeric_limits<_Ty>::max)()), 	"invalid min and max arguments for
    //uniform_real");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<std::is_same<Dummy, std::bernoulli_distribution>::value,
                   void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<double>(param_package.p());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<double>(parameters[0]);
      random_distribution.param(typename Dummy::param_type(param0));
    }
    //_STL_ASSERT(0.0 <= _P0 && _P0 <= 1.0, "invalid probability argument for
    //bernoulli_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::binomial_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, double>(param_package.p(), param_package.t());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<double>(parameters[1]);
      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    //_STL_ASSERT(0.0 <= _T0, "invalid max argument for binomial_distribution");
    //_STL_ASSERT(0.0 <= _P0 && _P0 <= 1.0, "invalid probability argument for
    //binomial_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::negative_binomial_distribution<ResultTy>>::value,
      void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, double>(param_package.p(), param_package.k());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<double>(parameters[1]);
      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    //_STL_ASSERT(0.0 < _K0, "invalid max argument for "
    //	"negative_binomial_distribution");
    //_STL_ASSERT(0.0 < _P0 && _P0 <= 1.0, "invalid probability argument for "
    //	"negative_binomial_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::geometric_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<double>(param_package.p());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<double>(parameters[0]);
      random_distribution.param(typename Dummy::param_type(param0));
    }
    //_STL_ASSERT(0.0 < _P0 && _P0 < 1.0, "invalid probability argument for
    //geometric_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::poisson_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<double>(param_package.mean());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<double>(parameters[0]);
      random_distribution.param(typename Dummy::param_type(param0));
    }
    //_STL_ASSERT(0.0 < _Mean0, "invalid mean argument for
    //poisson_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::exponential_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy>(param_package.lambda());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      random_distribution.param(typename Dummy::param_type(param0));
    }
    //_STL_ASSERT(0.0 < _Lambda0, "invalid lambda argument for
    //exponential_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::gamma_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, ResultTy>(param_package.alpha(),
                                        param_package.beta());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<ResultTy>(parameters[1]);
      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    //_STL_ASSERT(0.0 < _Alpha0, "invalid alpha argument for
    //gamma_distribution"); _STL_ASSERT(0.0 < _Beta0, "invalid beta argument for
    //gamma_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::weibull_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, ResultTy>(param_package.a(), param_package.b());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<ResultTy>(parameters[1]);
      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    //_STL_ASSERT(0.0 < _A0, "invalid a argument for weibull_distribution");
    //_STL_ASSERT(0.0 < _B0, "invalid b argument for weibull_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::extreme_value_distribution<ResultTy>>::value,
      void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, ResultTy>(param_package.a(), param_package.b());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<ResultTy>(parameters[1]);
      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    //_STL_ASSERT(0.0 < _B0, "invalid b argument for
    //extreme_value_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::normal_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, ResultTy>(param_package.mean(),
                                        param_package.stddev());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<ResultTy>(parameters[1]);
      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    //_STL_ASSERT(0.0 < _Sigma0, "invalid sigma argument for
    //normal_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::lognormal_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, ResultTy>(param_package.m(), param_package.s());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<ResultTy>(parameters[1]);
      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    //_STL_ASSERT(0.0 < _S0, "invalid s argument for lognormal_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::chi_squared_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy>(param_package.n());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      random_distribution.param(typename Dummy::param_type(param0));
    }
    // _STL_ASSERT(0 < _N0, "invalid n argument for chi_squared_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::cauchy_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, ResultTy>(param_package.a(), param_package.b());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<ResultTy>(parameters[1]);
      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    //_STL_ASSERT(0.0 < _B0, "invalid b argument for cauchy_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::fisher_f_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy, ResultTy>(param_package.m(), param_package.n());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      auto param1 = std::any_cast<ResultTy>(parameters[1]);
      random_distribution.param(typename Dummy::param_type(param0, param1));
    }
    //_STL_ASSERT(0 < _M0, "invalid m argument for fisher_f_distribution");
    //_STL_ASSERT(0 < _N0, "invalid n argument for fisher_f_distribution");
  }

  template <typename Dummy = DistributionTy>
  std::enable_if_t<
      std::is_same<Dummy, std::student_t_distribution<ResultTy>>::value, void>
  UpdateParameterPackage(const bool init = false)
  {
    if (init) {
      const auto param_package = random_distribution.param();
      SetParameters<ResultTy>(param_package.n());
    } else {
      auto parameters = GetParameters();
      auto param0 = std::any_cast<ResultTy>(parameters[0]);
      random_distribution.param(typename Dummy::param_type(param0));
    }
    //_STL_ASSERT(0 < _N0, "invalid n argument for student_t_distribution");
  }
};

/// @brief Owns all 17 supported distribution managers and provides uniform access by index.
///
/// The distributions cover uniform, Bernoulli, Poisson, and normal-family
/// distributions from the C++ standard library, instantiated with float or int
/// result types as appropriate.
class SamplerCollection {
public:
  using RationalTy = float; ///< Floating-point type for continuous distributions.
  using IntegerTy = int;    ///< Integer type for discrete distributions.

  using sampler_t00 =
      SamplingManager<std::uniform_int_distribution<IntegerTy>, RationalTy>;
  using sampler_t01 =
      SamplingManager<std::uniform_real_distribution<RationalTy>, RationalTy>;

  using sampler_t02 = SamplingManager<std::bernoulli_distribution, RationalTy>;
  using sampler_t03 =
      SamplingManager<std::binomial_distribution<IntegerTy>, RationalTy>;
  using sampler_t04 =
      SamplingManager<std::negative_binomial_distribution<IntegerTy>,
                      RationalTy>;
  using sampler_t05 =
      SamplingManager<std::geometric_distribution<IntegerTy>, RationalTy>;

  using sampler_t06 =
      SamplingManager<std::poisson_distribution<IntegerTy>, RationalTy>;
  using sampler_t07 =
      SamplingManager<std::exponential_distribution<RationalTy>, RationalTy>;
  using sampler_t08 =
      SamplingManager<std::gamma_distribution<RationalTy>, RationalTy>;
  using sampler_t09 =
      SamplingManager<std::weibull_distribution<RationalTy>, RationalTy>;
  using sampler_t10 =
      SamplingManager<std::extreme_value_distribution<RationalTy>, RationalTy>;

  using sampler_t11 =
      SamplingManager<std::normal_distribution<RationalTy>, RationalTy>;
  using sampler_t12 =
      SamplingManager<std::lognormal_distribution<RationalTy>, RationalTy>;
  using sampler_t13 =
      SamplingManager<std::chi_squared_distribution<RationalTy>, RationalTy>;
  using sampler_t14 =
      SamplingManager<std::cauchy_distribution<RationalTy>, RationalTy>;
  using sampler_t15 =
      SamplingManager<std::fisher_f_distribution<RationalTy>, RationalTy>;
  using sampler_t16 =
      SamplingManager<std::student_t_distribution<RationalTy>, RationalTy>;

  SamplerCollection()
  {
    auto ptr_00 = std::make_unique<sampler_t00>(
        "uniform int", integer_2, std::vector<std::string>{"min", "max"});
    auto ptr_01 = std::make_unique<sampler_t01>(
        "uniform real", rationale_2, std::vector<std::string>{"min", "max"});

    auto ptr_02 = std::make_unique<sampler_t02>("bernoulli", double_1,
                                                std::vector<std::string>{"p"});
    auto ptr_03 = std::make_unique<sampler_t03>(
        "binomial", integer_1_double_1, std::vector<std::string>{"t", "p"});
    auto ptr_04 =
        std::make_unique<sampler_t04>("negativ binomial", integer_1_double_1,
                                      std::vector<std::string>{"k", "p"});
    auto ptr_05 = std::make_unique<sampler_t05>("geometric", double_1,
                                                std::vector<std::string>{"p"});

    auto ptr_06 = std::make_unique<sampler_t06>("poisson", double_1,
                                                std::vector<std::string>{"mu"});
    auto ptr_07 = std::make_unique<sampler_t07>(
        "exponential", rationale_1, std::vector<std::string>{"lambda"});
    auto ptr_08 = std::make_unique<sampler_t08>(
        "gamma", rationale_2, std::vector<std::string>{"alpha", "beta"});
    auto ptr_09 = std::make_unique<sampler_t09>(
        "weibull", rationale_2, std::vector<std::string>{"a", "b"});
    auto ptr_10 = std::make_unique<sampler_t10>(
        "extreme value", rationale_2, std::vector<std::string>{"a", "b"});

    auto ptr_11 = std::make_unique<sampler_t11>(
        "normal", rationale_2, std::vector<std::string>{"mu", "sigma"});
    auto ptr_12 = std::make_unique<sampler_t12>(
        "log normal", rationale_2, std::vector<std::string>{"m", "s"});
    auto ptr_13 = std::make_unique<sampler_t13>("chi squared", rationale_1,
                                                std::vector<std::string>{"n"});
    auto ptr_14 = std::make_unique<sampler_t14>(
        "cauchy", rationale_2, std::vector<std::string>{"a", "b"});
    auto ptr_15 = std::make_unique<sampler_t15>(
        "fisher f", rationale_2, std::vector<std::string>{"m", "n"});
    auto ptr_16 = std::make_unique<sampler_t16>("student t", rationale_1,
                                                std::vector<std::string>{"n"});

    distribution_array[0] = std::move(ptr_00);
    distribution_array[1] = std::move(ptr_01);
    distribution_array[2] = std::move(ptr_02);
    distribution_array[3] = std::move(ptr_03);
    distribution_array[4] = std::move(ptr_04);
    distribution_array[5] = std::move(ptr_05);
    distribution_array[6] = std::move(ptr_06);
    distribution_array[7] = std::move(ptr_07);
    distribution_array[8] = std::move(ptr_08);
    distribution_array[9] = std::move(ptr_09);
    distribution_array[10] = std::move(ptr_10);
    distribution_array[11] = std::move(ptr_11);
    distribution_array[12] = std::move(ptr_12);
    distribution_array[13] = std::move(ptr_13);
    distribution_array[14] = std::move(ptr_14);
    distribution_array[15] = std::move(ptr_15);
    distribution_array[16] = std::move(ptr_16);
  }

  /// @brief Returns the number of available distributions (17).
  size_t GetSize() { return distribution_array.size(); }

  /// @brief Returns a raw pointer to the SamplingManagerInterface at the given index.
  auto GetDistribution(size_t index) { return distribution_array[index].get(); }

  /// @brief Returns the human-readable name of the distribution at the given index.
  std::string GetName(size_t index)
  {
    return distribution_array[index]->GetName();
  }

private:
  std::array<std::unique_ptr<SamplingManagerInterface>, 17> distribution_array;
};

class ClampParameters {
public:
};
