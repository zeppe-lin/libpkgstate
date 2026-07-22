// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgimage/error.h>
#include <libpkgimage/package_path.h>
#include <libpkgstate/error.h>
#include <libpkgstate/package_path.h>

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "test.h"

namespace {

struct valid_path final {
  std::string_view input;
  std::string_view canonical;
};

template<class Path>
std::optional<std::string>
parent_string(const Path& path)
{
  const auto parent = path.parent();
  if (!parent.has_value())
    return std::nullopt;
  return parent->string();
}

template<class Error, class Path>
bool
rejects(std::string_view input)
{
  try
  {
    static_cast<void>(Path::parse(input));
    return false;
  }
  catch (const Error&)
  {
    return true;
  }
}

} // namespace

int
main()
{
  constexpr std::array valid = {
    valid_path{"./usr//bin/./pkgadd/", "usr/bin/pkgadd"},
    valid_path{"etc", "etc"},
    valid_path{"etc/pkg/pkgadd.conf", "etc/pkg/pkgadd.conf"},
    valid_path{"etcetera/file", "etcetera/file"},
    valid_path{"usr/lib", "usr/lib"},
    valid_path{"usr/lib64", "usr/lib64"},
  };

  for (const valid_path& item : valid)
  {
    const pkgstate::package_path state =
        pkgstate::package_path::parse(item.input);
    const pkgimage::package_path image =
        pkgimage::package_path::parse(item.input);

    CHECK(state.string() == item.canonical);
    CHECK(image.string() == item.canonical);
    CHECK(state.filename() == image.filename());
    CHECK(parent_string(state) == parent_string(image));
  }

  for (const valid_path& lhs : valid)
  {
    for (const valid_path& rhs : valid)
    {
      const pkgstate::package_path state_lhs =
          pkgstate::package_path::parse(lhs.input);
      const pkgstate::package_path state_rhs =
          pkgstate::package_path::parse(rhs.input);
      const pkgimage::package_path image_lhs =
          pkgimage::package_path::parse(lhs.input);
      const pkgimage::package_path image_rhs =
          pkgimage::package_path::parse(rhs.input);

      CHECK((state_lhs == state_rhs) == (image_lhs == image_rhs));
      CHECK((state_lhs < state_rhs) == (image_lhs < image_rhs));
      CHECK(state_lhs.is_ancestor_of(state_rhs) ==
            image_lhs.is_ancestor_of(image_rhs));
    }
  }

  const std::string with_nul("usr\0bin", 7);
  const std::array<std::string_view, 8> invalid = {
    "",
    "/",
    "/etc/passwd",
    ".",
    "../etc/passwd",
    "usr/../../etc/passwd",
    "usr\nlib/lib.so",
    with_nul,
  };

  for (const std::string_view input : invalid)
  {
    CHECK((rejects<pkgstate::path_error, pkgstate::package_path>(input)));
    CHECK((rejects<pkgimage::path_error, pkgimage::package_path>(input)));
  }

  return 0;
}
