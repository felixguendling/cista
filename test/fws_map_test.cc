#include "doctest.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/fws_multimap.h"
#endif

using cista::offset::fws_multimap;

template <typename T>
using entry_t = typename cista::offset::fws_multimap<unsigned, T>::entry_t;

template <typename Ref, typename Result>
void check_result(Ref const& ref, Result const& result) {
  using value_t = typename Ref::value_type;

  if (ref.size() != result.size() && result.size() < 10) {
    std::cout << "Invalid result:\n  Expected: ";
    std::copy(begin(ref), end(ref),
              std::ostream_iterator<value_t>(std::cout, " "));
    std::cout << "\n  Result:   ";
    std::copy(begin(result), end(result),
              std::ostream_iterator<value_t>(std::cout, " "));
    std::cout << std::endl;
  }
  CHECK(ref.size() == result.size());
  for (auto i = 0U; i < ref.size(); ++i) {
    CHECK(ref[i] == result[i]);
  }
}

TEST_CASE("fws_multimap_test, multimap_simple") {
  fws_multimap<unsigned, int> m;

  m.push_back(1);
  m.push_back(2);
  m.finish_key();

  m.push_back(3);
  m.finish_key();

  m.push_back(4);
  m.push_back(5);
  m.push_back(6);
  m.finish_key();

  m.finish_map();

  CHECK(3 + 1 == m.index_size());
  check_result(cista::offset::vector<int>({1, 2}), m[0]);
  check_result(cista::offset::vector<int>({3}), m[1]);
  check_result(cista::offset::vector<int>({4, 5, 6}), m[2]);

  check_result(cista::offset::vector<int>({1, 2}), *begin(m));
  check_result(cista::offset::vector<int>({3}), *(begin(m) + 1));
  check_result(cista::offset::vector<int>({4, 5, 6}), *(begin(m) + 2));
  CHECK(end(m) == begin(m) + 3);
}

TEST_CASE("fws_multimap_test, multimap_empty_key") {
  fws_multimap<unsigned, int> m;

  m.push_back(1);
  m.push_back(2);
  m.finish_key();

  m.finish_key();

  m.push_back(4);
  m.push_back(5);
  m.push_back(6);
  m.finish_key();

  m.finish_map();

  CHECK(3 + 1 == m.index_size());
  check_result(cista::offset::vector<int>({1, 2}), m[0]);
  check_result(cista::offset::vector<int>({}), m[1]);
  check_result(cista::offset::vector<int>({4, 5, 6}), m[2]);

  check_result(cista::offset::vector<int>({1, 2}), *begin(m));
  check_result(cista::offset::vector<int>({}), *(begin(m) + 1));
  check_result(cista::offset::vector<int>({4, 5, 6}), *(begin(m) + 2));
  CHECK(end(m) == begin(m) + 3);
}
