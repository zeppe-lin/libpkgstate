// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/digest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace pkgstate {
namespace {

constexpr std::string_view representation_prefix = "v1:sha256:";
constexpr char lowercase_hex[] = "0123456789abcdef";

[[nodiscard]] std::uint8_t
parse_hex_digit(char value)
{
  if (value >= '0' && value <= '9')
    return static_cast<std::uint8_t>(value - '0');
  if (value >= 'a' && value <= 'f')
    return static_cast<std::uint8_t>(10 + value - 'a');

  throw digest_error(digest_error_code::invalid_hex,
                     "digest must use lowercase hexadecimal digits");
}

} // namespace

digest_error::digest_error(digest_error_code code, std::string message)
    : identity_error(std::move(message)), code_(code)
{
}

digest_error_code
digest_error::code() const noexcept
{
  return code_;
}

namespace detail {

digest_value::digest_value(std::uint16_t representation_version,
                           digest_algorithm algorithm,
                           digest_bytes bytes)
    : representation_version_(representation_version),
      algorithm_(algorithm),
      bytes_(std::move(bytes))
{
}

digest_value
digest_value::from_sha256(sha256_digest_bytes bytes)
{
  return digest_value(digest_representation_version,
                      digest_algorithm::sha256,
                      digest_bytes(bytes.begin(), bytes.end()));
}

digest_value
digest_value::parse(std::string_view input)
{
  const std::size_t first_separator = input.find(':');
  const std::size_t second_separator =
      first_separator == std::string_view::npos
          ? std::string_view::npos
          : input.find(':', first_separator + 1);

  if (first_separator == std::string_view::npos ||
      second_separator == std::string_view::npos ||
      input.find(':', second_separator + 1) != std::string_view::npos)
  {
    throw digest_error(digest_error_code::invalid_format,
                       "digest must have version, algorithm, and value fields");
  }

  const std::string_view version = input.substr(0, first_separator);
  const std::string_view algorithm = input.substr(
      first_separator + 1, second_separator - first_separator - 1);
  const std::string_view hexadecimal = input.substr(second_separator + 1);

  if (version != "v1")
  {
    throw digest_error(digest_error_code::unsupported_version,
                       "unsupported digest representation version");
  }
  if (algorithm != "sha256")
  {
    throw digest_error(digest_error_code::unsupported_algorithm,
                       "unsupported digest algorithm");
  }
  if (hexadecimal.size() != sha256_digest_size * 2)
  {
    throw digest_error(digest_error_code::invalid_length,
                       "SHA-256 digest must contain 64 hexadecimal digits");
  }

  sha256_digest_bytes bytes{};
  for (std::size_t index = 0; index < bytes.size(); ++index)
  {
    const std::uint8_t high = parse_hex_digit(hexadecimal[index * 2]);
    const std::uint8_t low = parse_hex_digit(hexadecimal[index * 2 + 1]);
    bytes[index] = static_cast<std::uint8_t>((high << 4U) | low);
  }

  return from_sha256(bytes);
}

std::uint16_t
digest_value::representation_version() const noexcept
{
  return representation_version_;
}

digest_algorithm
digest_value::algorithm() const noexcept
{
  return algorithm_;
}

const digest_bytes&
digest_value::bytes() const noexcept
{
  return bytes_;
}

std::string
digest_value::string() const
{
  std::string result;
  result.reserve(representation_prefix.size() + bytes_.size() * 2);
  result.append(representation_prefix);

  for (const std::uint8_t byte : bytes_)
  {
    result.push_back(lowercase_hex[(byte >> 4U) & 0x0fU]);
    result.push_back(lowercase_hex[byte & 0x0fU]);
  }

  return result;
}

bool
operator==(const digest_value& lhs, const digest_value& rhs) noexcept
{
  return lhs.representation_version_ == rhs.representation_version_ &&
         lhs.algorithm_ == rhs.algorithm_ && lhs.bytes_ == rhs.bytes_;
}

bool
operator!=(const digest_value& lhs, const digest_value& rhs) noexcept
{
  return !(lhs == rhs);
}

bool
operator<(const digest_value& lhs, const digest_value& rhs) noexcept
{
  if (lhs.representation_version_ != rhs.representation_version_)
    return lhs.representation_version_ < rhs.representation_version_;
  if (lhs.algorithm_ != rhs.algorithm_)
  {
    return static_cast<std::uint16_t>(lhs.algorithm_) <
           static_cast<std::uint16_t>(rhs.algorithm_);
  }

  return std::lexicographical_compare(lhs.bytes_.begin(), lhs.bytes_.end(),
                                      rhs.bytes_.begin(), rhs.bytes_.end());
}

} // namespace detail
} // namespace pkgstate
