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

#include "random_numbers.h"

#include <variant>
#include <thread>



// Ty0 sample variables, 
// Ty1 sample function results variables
template<typename Ty0, typename Ty1>
class SampleFunctions
{
public:

	Ty1 Sum(const std::vector<Ty0>& sample_variables) const
	{
		const Ty1 sum = static_cast<Ty1>(std::accumulate(sample_variables.cbegin(), sample_variables.cend(), static_cast<Ty0>(0)));
		return sum;
	}

	Ty1 Mean(const std::vector<Ty0>& sample_variables) const
	{
		const Ty1 size = static_cast<Ty1>(sample_variables.size());
		const Ty1 mean = Sum(sample_variables) / size;
		return mean;
	}

	Ty1 TotalSumOfSquares(const std::vector<Ty0>& sample_variables, Ty1 sample_mean) const
	{
		Ty1 totalsumofsquares = 0;
		for (const auto& variable : sample_variables)
		{
			totalsumofsquares += std::pow(variable - sample_mean, 2);
		}

		return totalsumofsquares;
	}

	Ty1 Variance(Ty1 totalsumofsquares, Ty1 sample_size) const
	{
		return totalsumofsquares / sample_size;
	}

	Ty1 StandardDeviation(Ty1 sample_variance) const
	{
		return std::sqrt(sample_variance);
	}
};



/*template<typename Ty>
struct VisitDataTable
{
	template<typename = typename std::enable_if<!std::is_same<Ty, double>::value>::type>
	double operator()(const double& variable) const
	{
		return variable;
	}

	Ty operator()(const Ty& variable) const
	{
		return variable;
	}

	std::string operator()(const std::string& variable) const
	{
		return variable;
	}
};*/


// Ty0 sample variables type,
// Ty1 sample function results variables type
template<typename Ty0, typename Ty1>
class DataTable
{
public:

	// avoid type duplicate
	// for variant template arguments
	using VariantType = typename std::conditional<
		std::is_same<Ty0, Ty1>::value,
		std::variant<Ty0, std::string>,
		std::variant<Ty0, Ty1, std::string>
	>::type;

	DataTable() :
		sample_function_results_columns(5),
		number_name_rows(1),
		number_columns(0),
		number_rows(0),
		number_samples(0),
		sample_size(0)
	{
		//std::cout << "variable type " << typeid(T).name() << '\n';

		
	}

	template<typename V>
	void GenerateSamples(const V& random_distribution, size_t number_samples, size_t sample_size)
	{
		SetSize(number_samples, sample_size);


		// Check std::execution::parallel_policy with algorithm
		// Compare with 
		// const auto t1 = std::chrono::high_resolution_clock::now();
		// const auto t2 = std::chrono::high_resolution_clock::now();
		// const std::chrono::duration<double, std::milli> ms = t2 - t1;

		const size_t number_threads = 12;

		const size_t rest = number_samples % number_threads;
		const size_t slice_size = (number_samples - rest) / number_threads;

		std::vector<std::pair<std::size_t, std::size_t>> slice_indexes(number_threads);

		for (size_t index = 0; index < slice_indexes.size(); ++index)
		{
			if (index < slice_indexes.size() - 1)
			{
				slice_indexes[index].first = number_name_rows + slice_size * index;
				slice_indexes[index].second = slice_indexes[index].first + slice_size;
			}
			else
			{
				slice_indexes[index].first = number_name_rows + slice_size * index;
				slice_indexes[index].second = slice_indexes[index].first + slice_size + rest;
			}
		}

		std::vector<std::thread> thread_array;

		for (const auto& slice : slice_indexes)
		{
			std::thread current_thread(&DataTable::GenerateSamplesSubset<V>, this, random_distribution, slice.first, slice.second);
			thread_array.push_back(std::move(current_thread));
			//std::cout << "Started thread " << thread_array.size() << "\n";

			//GenerateSamplesSubset(random_distribution, slice.first, slice.second);
		}

		for (size_t index = 0; index < thread_array.size(); ++index)
		{
			thread_array[index].join();
			//std::cout << "Joined thread " << index << "\n";
		}
	}

	template<typename V>
	void GenerateSamplesSubset(const V& random_distribution, size_t row_begin_index, size_t row_end_index)
	{
		RandomNumberGenerator<V> generator(random_distribution);

		for (size_t row_index = row_begin_index; row_index < row_end_index; ++row_index)
		{
			for (size_t col_index = 0; col_index < sample_size; ++col_index)
			{
				GetVariantRef(col_index, row_index) = generator.GenerateRandomNumber();
			}
		}
	}

	std::vector<std::string> GetSampleFunctionNames() const
	{
		return sample_function_names;
	}

	void CalculateSampleFunctionResults()
	{
		const size_t first_column = sample_size;
		const size_t first_row = number_name_rows;

		sample_function_names.resize(sample_function_results_columns);
		sample_function_names[0] = std::string("sum");
		sample_function_names[1] = std::string("mean");
		sample_function_names[2] = std::string("tts");
		sample_function_names[3] = std::string("variance1");
		sample_function_names[4] = std::string("variance2");

		

		for (size_t index = 0; index < sample_function_results_columns; ++index)
		{
			data[first_column + index] = sample_function_names[index];
		}

		const Ty1 double_sample_size = static_cast<Ty1>(sample_size);

		for (size_t index = 0; index < number_samples; ++index)
		{
			auto sample = GetSample(index);
			auto sum = sample_functions.Sum(sample);
			auto mean = sample_functions.Mean(sample);
			auto tts = sample_functions.TotalSumOfSquares(sample, mean);

			GetVariantRef(sample_size + 0, index + first_row) = sum;
			GetVariantRef(sample_size + 1, index + first_row) = mean;
			GetVariantRef(sample_size + 2, index + first_row) = tts;
			GetVariantRef(sample_size + 3, index + first_row) = sample_functions.Variance(tts, double_sample_size);
			GetVariantRef(sample_size + 4, index + first_row) = sample_functions.Variance(tts, double_sample_size - static_cast<Ty1>(1));
		}
	}

	size_t GetColumnByName(const std::string& name) const
	{
		size_t column = 0;
		bool found = false;

		for (size_t index = 0; index < number_columns; ++index)
		{
			if (std::get<std::string>(data[index]) == name)
			{
				column = index;
				found = true;
			}
		}

		found == false ? throw std::logic_error("data table: column name not found") : false;

		return column;
	}

	std::vector<Ty1> GetColumnData(const std::string& name) const
	{
		size_t column = GetColumnByName(name);

		std::vector<Ty1> column_data(number_samples);

		for (size_t index = 0; index < number_samples; ++index)
		{
			column_data[index] = std::get<Ty1>(GetVariantRef(column, index + number_name_rows));
		}

		return column_data;
	}

	std::vector<Ty0> GetSample(size_t number) const
	{
		number += number_name_rows;

		std::vector<Ty0> sample(sample_size);

		for (size_t index = 0; index < sample_size; ++index)
		{
			sample[index] = std::get<Ty0>(GetVariantRef(index, number));
		}

		return sample;
	}

	VariantType& GetVariantRef(size_t column, size_t row)
	{
		return data[row * number_columns + column];
	}

	const VariantType& GetVariantRef(size_t column, size_t row) const
	{
		return data[row * number_columns + column];
	}

	std::string GetString(size_t column, size_t row) const
	{
		std::stringstream stream;

		auto variant = GetVariantRef(column, row);

		std::visit([&stream](auto& variable)
			{
				stream << variable;
			}, variant);

		return stream.str();
	}

	size_t GetNumberRows() const
	{
		return number_rows;
	}

	size_t GetNumberColumns() const
	{
		return number_columns;
	}

private:

	void SetSize(size_t number_samples, size_t sample_size)
	{
		this->number_samples = number_samples;
		this->sample_size = sample_size;

		number_columns = sample_size + sample_function_results_columns;
		number_rows = number_samples + number_name_rows;

		data.resize(number_columns * number_rows);

		NameSampleColumns();
	}

	void NameSampleColumns()
	{
		for (size_t index = 0; index < sample_size; ++index)
		{
			GetVariantRef(index, 0) = std::to_string(index + 1);
		}
	}

	const size_t sample_function_results_columns;
	const size_t number_name_rows;

	size_t number_samples;
	size_t sample_size;

	size_t number_columns;
	size_t number_rows;

	std::vector<VariantType> data;
	SampleFunctions<Ty0, Ty1> sample_functions;
	std::vector<std::string> sample_function_names;
};
