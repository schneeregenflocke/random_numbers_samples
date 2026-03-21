#ifndef RANDOM_SAMPLES_FILE_IO_H
#define RANDOM_SAMPLES_FILE_IO_H

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

/// @brief RAII wrapper around std::ofstream for writing tab-delimited output.
///
/// The file is opened in truncate mode with maximum double precision on
/// construction and closed automatically on destruction. The stream insertion
/// operator forwards to the underlying file stream.
class FileOutput {
public:
  /// @brief Opens the file at the given path for writing.
  /// @param name Path to the output file.
  explicit FileOutput(const std::string &name) : filepath(name)
  {
    file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    OpenFile();
  }

  /// @brief Closes the file on destruction.
  ~FileOutput() { Close(); }

  FileOutput(const FileOutput &) = delete;
  FileOutput(FileOutput &&) = delete;
  FileOutput &operator=(const FileOutput &) = delete;
  FileOutput &operator=(FileOutput &&) = delete;

  /// @brief Writes a value to the file and returns the underlying stream.
  template <typename T> std::ofstream &operator<<(const T &value)
  {
    file << value;
    return file;
  }

  /// @brief Explicitly closes the file (also called by the destructor).
  void Close()
  {
    try {
      file.close();
    } catch (std::ofstream::failure &e) {
      std::cerr << e.what() << '\n';
    }
  }

  /// @brief Returns the filename (without directory) of the opened file.
  std::filesystem::path GetFilepath() { return filepath.filename(); }

private:
  template <typename T = double> void OpenFile()
  {
    try {
      file.open(filepath, std::ios::trunc);

      file << std::fixed;
      file << std::setprecision(std::numeric_limits<T>::digits10);
    } catch (std::ofstream::failure &e) {
      std::cerr << e.what() << '\n';
    }
  }

  std::filesystem::path filepath;
  std::ofstream file;
};

#endif // RANDOM_SAMPLES_FILE_IO_H
