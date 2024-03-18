#include "CSVParserUtil.h"
#include "rapidcsv.h"
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace VDMS {
class CSVParser {
public:
  CSVParser(std::string filename, size_t num_threads, std::string server,
            int port)
      : m_filename(filename), m_num_threads(num_threads), vdms_server(server),
        vdms_port(port) {}
  ~CSVParser() {}

  std::vector<VDMS::Response>
  parse_csv_lines(const std::string &filename, int start_line, int end_line,
                  std::vector<VDMS::Response> &local_results,
                  const size_t thread_id) {

    rapidcsv::Document csv(filename);
    std::vector<std::string> columnNames = csv.GetColumnNames();
    VDMS::CSVParserUtil csv_util(vdms_server, vdms_port, columnNames,
                                 static_cast<int>(thread_id));
    for (int i = start_line; i <= end_line - 1; ++i) {
      std::vector<std::string> row = csv.GetRow<std::string>(i);
      VDMS::Response result = csv_util.parse_row(row);
      if (local_results.empty()) {
        // If local_results is empty, resize it to have at least one element
        local_results.resize(1);
      } else if (i - start_line >= static_cast<int>(local_results.size())) {
        // If the index is beyond the current size, resize to accommodate the
        // index
        local_results.resize(i - start_line + 1);
      }

      // Replace the value at the specified index in local_results
      local_results[i - start_line] = result;
    }

    return local_results;
  }

  std::vector<VDMS::Response> parse() {
    std::ifstream file(m_filename);
    if (!file.is_open()) {
      std::cerr << "Error opening file: " << m_filename << std::endl;
    }
    int num_lines = std::count(std::istreambuf_iterator<char>(file),
                               std::istreambuf_iterator<char>(), '\n');
    file.close();

    std::size_t lines_per_thread = num_lines / m_num_threads;

    std::mutex mutex;
    std::vector<std::thread> threads;
    std::vector<std::vector<VDMS::Response>> all_local_results(m_num_threads);
    std::vector<VDMS::Response> all_results; // Local vector for each thread
    all_results.reserve(num_lines);

    for (size_t i = 0; i < m_num_threads; i++) {
      size_t start_line = i * lines_per_thread;
      size_t end_line =
          (i == m_num_threads - 1) ? num_lines - 1 : (i + 1) * lines_per_thread;

      threads.emplace_back(&CSVParser::parse_csv_lines, this,
                           std::ref(m_filename), start_line, end_line,
                           std::ref(all_local_results[i]), i);
    }

    for (auto &thread : threads) {
      thread.join();
    }
    size_t allResultsSizeBefore = all_results.size();
    for (const auto &local_results : all_local_results) {

      // Extend the size of all_results to accommodate local_results
      all_results.resize(all_results.size() + local_results.size());

      // Copy elements from local_results to the appropriate positions in
      // all_results
      std::copy(local_results.begin(), local_results.end(),
                all_results.begin() + allResultsSizeBefore);
    }

    return all_results;
  }

private:
  std::string m_filename;
  size_t m_num_threads;
  std::string vdms_server;
  int vdms_port;
};
}; // namespace VDMS