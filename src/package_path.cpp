// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/error.h>
#include <libpkgstate/package_path.h>

#include <ostream>
#include <sstream>
#include <utility>
#include <vector>

namespace pkgstate {
namespace {

[[noreturn]] void
invalid_path(std::string_view input, const char* reason)
{
  std::ostringstream message;
  message << "invalid installed package path: " << reason;
  if (input.find('\0') == std::string_view::npos)
    message << ": '" << input << "'";
  throw path_error(message.str());
}

} // namespace

package_path::package_path(std::string normalized)
    : value_(std::move(normalized))
{
}

package_path
package_path::parse(std::string_view input)
{
  if (input.empty())
    invalid_path(input, "path is empty");

  if (input.front() == '/')
    invalid_path(input, "absolute paths are forbidden");

  for (const char ch : input)
  {
    if (ch == '\0')
      invalid_path(input, "NUL byte is forbidden");
    if (ch == '\n' || ch == '\r')
      invalid_path(input, "line separators are forbidden");
  }

  std::vector<std::string_view> components;
  std::size_t begin = 0;

  while (begin <= input.size())
  {
    const std::size_t end = input.find('/', begin);
    const std::size_t count =
        end == std::string_view::npos ? input.size() - begin : end - begin;
    const std::string_view component = input.substr(begin, count);

    if (!component.empty() && component != ".")
    {
      if (component == "..")
        invalid_path(input, "parent traversal is forbidden");
      components.push_back(component);
    }

    if (end == std::string_view::npos)
      break;
    begin = end + 1;
  }

  if (components.empty())
    invalid_path(input, "path has no object component");

  std::string normalized;
  std::size_t length = components.size() - 1;
  for (const std::string_view component : components)
    length += component.size();
  normalized.reserve(length);

  for (std::size_t i = 0; i < components.size(); ++i)
  {
    if (i != 0)
      normalized.push_back('/');
    normalized.append(components[i].data(), components[i].size());
  }

  return package_path(std::move(normalized));
}

const std::string&
package_path::string() const noexcept
{
  return value_;
}

std::string_view
package_path::filename() const noexcept
{
  const std::size_t separator = value_.rfind('/');
  if (separator == std::string::npos)
    return value_;
  return std::string_view(value_).substr(separator + 1);
}

std::optional<package_path>
package_path::parent() const
{
  const std::size_t separator = value_.rfind('/');
  if (separator == std::string::npos)
    return std::nullopt;
  return package_path(value_.substr(0, separator));
}

bool
package_path::is_ancestor_of(const package_path& other) const noexcept
{
  return other.value_.size() > value_.size()
      && other.value_.compare(0, value_.size(), value_) == 0
      && other.value_[value_.size()] == '/';
}

bool
operator==(const package_path& lhs, const package_path& rhs) noexcept
{
  return lhs.value_ == rhs.value_;
}

bool
operator!=(const package_path& lhs, const package_path& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const package_path& lhs, const package_path& rhs) noexcept
{
  return lhs.value_ < rhs.value_;
}

std::ostream&
operator<<(std::ostream& out, const package_path& path)
{
  return out << path.string();
}

} // namespace pkgstate
