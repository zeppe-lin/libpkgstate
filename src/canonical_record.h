// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <libpkgstate/digest.h>

namespace pkgstate::detail {

inline constexpr std::uint16_t canonical_record_encoding_version = 1;

class canonical_record final {
public:
  explicit canonical_record(std::string_view domain);

  void append_bool(bool value);
  void append_u8(std::uint8_t value);
  void append_u16(std::uint16_t value);
  void append_u32(std::uint32_t value);
  void append_u64(std::uint64_t value);
  void append_bytes(std::string_view value);
  void append_bytes(const digest_bytes& value);

  template<typename Domain>
  void append_digest(const typed_digest<Domain>& value)
  {
    append_u16(value.representation_version());
    append_u16(static_cast<std::uint16_t>(value.algorithm()));
    append_bytes(value.bytes());
  }

  [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept;
  [[nodiscard]] sha256_digest_bytes sha256() const;

private:
  void append_raw(const std::uint8_t* data, std::size_t size);

  std::vector<std::uint8_t> bytes_;
};

} // namespace pkgstate::detail
