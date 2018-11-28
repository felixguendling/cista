#include <cstring>
#include <fstream>
#include <iostream>
#include <set>

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
      if (starts_with(line, include_start)) {
        auto const path =
            include_path + "/" +
            line.substr(include_start_len, line.size() - include_start_len - 1);
        if (included.insert(path).second) {
          write_file(include_path, path, included);
        }
      } else if (starts_with(line, "#pragma once")) {
        // ignore
      } else {
        std::cout << line << "\n";
      }
    }
  } catch (...) {
    std::cerr << "error reading file " << path << "\n";
    throw;
  }
}

int main(int argc, char const** argv) {
  if (argc < 3) {
    printf("usage: %s include_path file0, [file1, [file2, ...]]\n", argv[0]);
    return 1;
  }

  auto const include_path = std::string{argv[1]};
  std::set<std::string> included;
  std::cout << "#pragma once\n\n";
  for (int i = 2; i < argc; ++i) {
    write_file(include_path, argv[i], included);
  }
}