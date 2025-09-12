#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#define starts_with(s, start) (s.substr(0, std::strlen(start)) == start)

void write_file(std::string const& include_path, std::string const& path,
                std::set<std::string>& included) {
  try {
    std::ifstream f{path.c_str()};
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::string line;
    while (!f.eof() && f.peek() != EOF && std::getline(f, line)) {
      constexpr auto const include_start = R"(#include ")";
      auto const include_start_len = std::strlen(include_start);
      if (starts_with(line, include_start) &&
          line.find("fmt/") == std::string::npos) {
        auto const include_end = line.find_last_of("\"");
        auto const include_file =
            line.substr(include_start_len, include_end - include_start_len);

        auto const path = include_path + "/" + include_file;
        if (included.insert(path).second) {
          try {
            write_file(include_path, path, included);
          } catch (std::exception const& e) {
            std::cout << "// " << path << ": " << e.what() << "\n";
          }
        }
      } else if (starts_with(line, "#pragma once")) {
        // ignore
      } else {
        std::cout << line << "\n";
      }
    }
  } catch (...) {
    throw;
  }
}

int main(int argc, char const** argv) {
  if (argc < 4) {
    printf("usage: %s license include_path file0, [file1, [file2, ...]]\n",
           argv[0]);
    return 1;
  }

  auto const license_path = std::string{argv[1]};
  auto const include_path = std::string{argv[2]};

  std::cout << "/*\n";
  std::ifstream f{license_path.c_str()};
  f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  std::string line;
  while (!f.eof() && f.peek() != EOF && std::getline(f, line)) {
    std::cout << line << "\n";
  }
  std::cout << "*/\n\n";

  std::cout << "#pragma once\n\n";
  std::set<std::string> included;
  for (int i = 3; i < argc; ++i) {
    write_file(include_path, argv[i], included);
  }
}