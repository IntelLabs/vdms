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
  std::vector<VDMS::Response>
  parse_csv_lines(const std::string &filename, int start_line, int end_line,
                  std::mutex &mutex, std::vector<VDMS::Response> &all_results,
                  const size_t thread_id) {
    rapidcsv::Document csv(filename);

    size_t rowCount = csv.GetRowCount();
    std::vector<std::string> columnNames = csv.GetColumnNames();
    VDMS::CSVParserUtil csv_util(vdms_server, vdms_port, columnNames,
                                 static_cast<int>(thread_id));
    for (int i = start_line; i < end_line; ++i) {
      std::vector<std::string> row = csv.GetRow<std::string>(i);
      VDMS::Response result = csv_util.parse_row(row);

      std::lock_guard<std::mutex> lock(mutex);
      all_results.push_back(result);
    }

    return all_results;
  }
  std::vector<VDMS::Response> parse() {
    auto start = std::chrono::high_resolution_clock::now();

    std::ifstream file(m_filename);
    if (!file) {
      std::cerr << "Error opening file\n";
    }

    int num_lines = std::count(std::istreambuf_iterator<char>(file),
                               std::istreambuf_iterator<char>(), '\n');
    std::size_t lines_per_thread = num_lines / m_num_threads;

    std::mutex mutex;
    std::vector<std::thread> threads;
    std::vector<VDMS::Response> all_results;

    for (size_t i = 0; i < m_num_threads; i++) {
      size_t start_line = i * lines_per_thread;
      size_t end_line =
          (i == m_num_threads - 1) ? num_lines - 1 : (i + 1) * lines_per_thread;

      threads.emplace_back(&CSVParser::parse_csv_lines, this,
                           std::ref(m_filename), start_line, end_line,
                           std::ref(mutex), std::ref(all_results), i);
    }

    for (auto &thread : threads) {
      thread.join();
    }

    auto finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = finish - start;
    // std::cout << "Elapsed time: " << elapsed.count() << " s\n";
    return all_results;
  }

private:
  std::string m_filename;
  size_t m_num_threads;
  std::string vdms_server;
  int vdms_port;
};
}; // namespace VDMS