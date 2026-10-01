#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>

#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/mmap.h"
#include "cista/targets/file.h"
#endif

TEST_CASE("mmap copy on write preserves the backing file") {
  constexpr auto const FILENAME = "mmap_copy_on_write.bin";
  auto const original = std::string(32769U, 'a');

  std::remove(FILENAME);
  {
    cista::file f{FILENAME, "w+"};
    f.write(original.data(), original.size(), 1U);
  }
  auto const checksum = cista::file{FILENAME, "r"}.checksum();

  for (auto const read_only : {false, true}) {
    if (read_only) {
      std::filesystem::permissions(FILENAME,
                                   std::filesystem::perms::owner_read);
    }

    {
      auto mapped =
          cista::mmap{FILENAME, cista::mmap::protection::COPY_ON_WRITE};
      REQUIRE(mapped.size() == original.size());
      CHECK(mapped.view() == original);
      mapped[0] = 'b';
      mapped.data()[16384U] = 'c';
      mapped[mapped.size() - 1U] = 'd';
      CHECK(mapped[0] == 'b');
      CHECK(mapped[16384U] == 'c');
      CHECK(mapped[mapped.size() - 1U] == 'd');
      CHECK(cista::file{FILENAME, "r"}.checksum() == checksum);

      mapped.sync();
      CHECK(cista::file{FILENAME, "r"}.checksum() == checksum);

      auto independent =
          cista::mmap{FILENAME, cista::mmap::protection::COPY_ON_WRITE};
      CHECK(independent.view() == original);
      independent[0] = 'e';
      CHECK(mapped[0] == 'b');

      auto moved = std::move(mapped);
      CHECK(moved[0] == 'b');
      CHECK(moved[16384U] == 'c');
      CHECK(moved[moved.size() - 1U] == 'd');
    }

    CHECK(cista::file{FILENAME, "r"}.checksum() == checksum);
    CHECK(cista::file{FILENAME, "r"}.size() == original.size());
  }

  std::filesystem::permissions(FILENAME, std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::add);
  CHECK(std::remove(FILENAME) == 0);
}

TEST_CASE("mmap copy on write rejects resize and reserve") {
  constexpr auto const FILENAME = "mmap_copy_on_write_resize.bin";

  std::remove(FILENAME);
  for (auto const& original : {std::string{}, std::string{"file data"}}) {
    {
      cista::file f{FILENAME, "w+"};
      f.write(original.data(), original.size(), 1U);
    }
    auto const checksum = cista::file{FILENAME, "r"}.checksum();

    {
      auto mapped =
          cista::mmap{FILENAME, cista::mmap::protection::COPY_ON_WRITE};
      for (auto const size :
           {std::size_t{0U}, original.size(), original.size() + 1U}) {
        CHECK_THROWS_AS(mapped.resize(size), cista::cista_exception);
        CHECK_THROWS_AS(mapped.reserve(size), cista::cista_exception);
        CHECK(mapped.size() == original.size());
      }
      mapped.sync();
      CHECK(cista::file{FILENAME, "r"}.checksum() == checksum);
    }

    CHECK(cista::file{FILENAME, "r"}.checksum() == checksum);
    CHECK(cista::file{FILENAME, "r"}.size() == original.size());
  }

  CHECK(std::remove(FILENAME) == 0);
}
