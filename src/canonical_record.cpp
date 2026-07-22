// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "canonical_record.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

#include <openssl/evp.h>

#include <libpkgstate/error.h>

namespace pkgstate::detail {
namespace {

constexpr std::array<std::uint8_t, 9> record_magic = {
    'p', 'k', 'g', 's', 't', 'a', 't', 'e', 0,
};

void
validate_domain(std::string_view domain)
{
  if (domain.empty())
    throw state_error("canonical record domain must not be empty");

  for (const char byte : domain)
  {
    if (byte == '\0' || byte == '\n' || byte == '\r')
      throw state_error("canonical record domain is not line-safe");
  }
}

} // namespace

canonical_record::canonical_record(std::string_view domain)
{
  validate_domain(domain);
  append_raw(record_magic.data(), record_magic.size());
  append_u16(canonical_record_encoding_version);
  append_bytes(domain);
}

void
canonical_record::append_bool(bool value)
{
  append_u8(value ? 1 : 0);
}

void
canonical_record::append_u8(std::uint8_t value)
{
  bytes_.push_back(value);
}

void
canonical_record::append_u16(std::uint16_t value)
{
  bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
  bytes_.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

void
canonical_record::append_u32(std::uint32_t value)
{
  for (int shift = 24; shift >= 0; shift -= 8)
  {
    bytes_.push_back(
        static_cast<std::uint8_t>((value >> static_cast<unsigned>(shift)) &
                                  0xffU));
  }
}

void
canonical_record::append_u64(std::uint64_t value)
{
  for (int shift = 56; shift >= 0; shift -= 8)
  {
    bytes_.push_back(
        static_cast<std::uint8_t>((value >> static_cast<unsigned>(shift)) &
                                  0xffU));
  }
}

void
canonical_record::append_bytes(std::string_view value)
{
  append_u64(static_cast<std::uint64_t>(value.size()));
  append_raw(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
}

void
canonical_record::append_bytes(const digest_bytes& value)
{
  append_u64(static_cast<std::uint64_t>(value.size()));
  append_raw(value.data(), value.size());
}

const std::vector<std::uint8_t>&
canonical_record::bytes() const noexcept
{
  return bytes_;
}

sha256_digest_bytes
canonical_record::sha256() const
{
  using context_ptr =
      std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
  context_ptr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
  if (!context)
    throw state_error("could not allocate canonical digest context");

  if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1 ||
      EVP_DigestUpdate(context.get(), bytes_.data(), bytes_.size()) != 1)
  {
    throw state_error("could not compute canonical SHA-256 digest");
  }

  sha256_digest_bytes result{};
  unsigned int size = 0;
  if (EVP_DigestFinal_ex(context.get(), result.data(), &size) != 1 ||
      size != result.size())
  {
    throw state_error("could not finalize canonical SHA-256 digest");
  }

  return result;
}

void
canonical_record::append_raw(const std::uint8_t* data, std::size_t size)
{
  if (size == 0)
    return;
  bytes_.insert(bytes_.end(), data, data + size);
}

} // namespace pkgstate::detail
