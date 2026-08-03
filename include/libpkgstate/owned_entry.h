// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*! \file owned_entry.h
 *  \brief Completed installed ownership, object, and reconciliation facts.
 */
#pragma once

#include <libpkgstate/export.h>

#include <cstdint>
#include <optional>
#include <string>

#include <libpkgstate/digest.h>
#include <libpkgstate/package_path.h>

namespace pkgstate {

enum class owned_object_kind : std::uint8_t {
  regular = 1,
  directory = 2,
  symlink = 3,
  fifo = 4,
  character_device = 5,
  block_device = 6,
  socket = 7,
  other = 8,
};

enum class active_object_origin : std::uint8_t {
  incoming_payload = 1,
  retained_existing = 2,
};

enum class rejected_object_side : std::uint8_t {
  incoming = 1,
  prior_installed = 2,
};

class PKGSTATE_API installed_object_timestamp final {
public:
  installed_object_timestamp(std::int64_t seconds, std::uint32_t nanoseconds);
  [[nodiscard]] std::int64_t seconds() const noexcept;
  [[nodiscard]] std::uint32_t nanoseconds() const noexcept;
  friend PKGSTATE_API bool operator==(const installed_object_timestamp& lhs,
                         const installed_object_timestamp& rhs) noexcept;
  friend PKGSTATE_API bool operator!=(const installed_object_timestamp& lhs,
                         const installed_object_timestamp& rhs) noexcept;
  friend PKGSTATE_API bool operator<(const installed_object_timestamp& lhs,
                        const installed_object_timestamp& rhs) noexcept;
private:
  std::int64_t seconds_;
  std::uint32_t nanoseconds_;
};

class PKGSTATE_API installed_device_number final {
public:
  installed_device_number(std::uint64_t major, std::uint64_t minor) noexcept;
  [[nodiscard]] std::uint64_t major() const noexcept;
  [[nodiscard]] std::uint64_t minor() const noexcept;
  friend PKGSTATE_API bool operator==(const installed_device_number& lhs,
                         const installed_device_number& rhs) noexcept;
  friend PKGSTATE_API bool operator!=(const installed_device_number& lhs,
                         const installed_device_number& rhs) noexcept;
  friend PKGSTATE_API bool operator<(const installed_device_number& lhs,
                        const installed_device_number& rhs) noexcept;
private:
  std::uint64_t major_;
  std::uint64_t minor_;
};

/*! \brief Complete recorded metadata for one installed active object. */
class PKGSTATE_API installed_object_metadata final {
public:
  installed_object_metadata(
      owned_object_kind kind,
      std::uint32_t mode,
      std::uint64_t uid,
      std::uint64_t gid,
      installed_object_timestamp mtime,
      std::optional<std::uint64_t> size = std::nullopt,
      std::optional<installed_regular_content_identity> regular_content =
          std::nullopt,
      std::optional<std::string> symlink_target = std::nullopt,
      std::optional<installed_device_number> device = std::nullopt,
      std::optional<package_path> hardlink_anchor = std::nullopt);

  [[nodiscard]] owned_object_kind kind() const noexcept;
  [[nodiscard]] std::uint32_t mode() const noexcept;
  [[nodiscard]] std::uint64_t uid() const noexcept;
  [[nodiscard]] std::uint64_t gid() const noexcept;
  [[nodiscard]] const installed_object_timestamp& mtime() const noexcept;
  [[nodiscard]] const std::optional<std::uint64_t>& size() const noexcept;
  [[nodiscard]] const std::optional<installed_regular_content_identity>&
  regular_content() const noexcept;
  [[nodiscard]] const std::optional<std::string>&
  symlink_target() const noexcept;
  [[nodiscard]] const std::optional<installed_device_number>&
  device() const noexcept;
  [[nodiscard]] const std::optional<package_path>&
  hardlink_anchor() const noexcept;

  friend PKGSTATE_API bool operator==(const installed_object_metadata& lhs,
                         const installed_object_metadata& rhs) noexcept;
  friend PKGSTATE_API bool operator!=(const installed_object_metadata& lhs,
                         const installed_object_metadata& rhs) noexcept;
  friend PKGSTATE_API bool operator<(const installed_object_metadata& lhs,
                        const installed_object_metadata& rhs) noexcept;

private:
  owned_object_kind kind_;
  std::uint32_t mode_;
  std::uint64_t uid_;
  std::uint64_t gid_;
  installed_object_timestamp mtime_;
  std::optional<std::uint64_t> size_;
  std::optional<installed_regular_content_identity> regular_content_;
  std::optional<std::string> symlink_target_;
  std::optional<installed_device_number> device_;
  std::optional<package_path> hardlink_anchor_;
};

class PKGSTATE_API rejected_object_reference final {
public:
  rejected_object_reference(rejected_object_side side,
                            rejected_object_identity identity);
  [[nodiscard]] rejected_object_side side() const noexcept;
  [[nodiscard]] const rejected_object_identity& identity() const noexcept;
  friend PKGSTATE_API bool operator==(const rejected_object_reference& lhs,
                         const rejected_object_reference& rhs) noexcept;
  friend PKGSTATE_API bool operator!=(const rejected_object_reference& lhs,
                         const rejected_object_reference& rhs) noexcept;
  friend PKGSTATE_API bool operator<(const rejected_object_reference& lhs,
                        const rejected_object_reference& rhs) noexcept;
private:
  rejected_object_side side_;
  rejected_object_identity identity_;
};

class PKGSTATE_API owned_entry final {
public:
  [[nodiscard]] static owned_entry make(
      package_path path,
      installed_object_metadata object,
      active_object_origin origin,
      std::optional<rejected_object_reference> rejected = std::nullopt);
  [[nodiscard]] const package_path& path() const noexcept;
  [[nodiscard]] owned_object_kind kind() const noexcept;
  [[nodiscard]] const installed_object_metadata& object() const noexcept;
  [[nodiscard]] active_object_origin origin() const noexcept;
  [[nodiscard]] const std::optional<rejected_object_reference>&
  rejected() const noexcept;
  friend PKGSTATE_API bool operator==(const owned_entry& lhs,
                         const owned_entry& rhs) noexcept;
  friend PKGSTATE_API bool operator!=(const owned_entry& lhs,
                         const owned_entry& rhs) noexcept;
  friend PKGSTATE_API bool operator<(const owned_entry& lhs,
                        const owned_entry& rhs) noexcept;
private:
  owned_entry(package_path path,
              installed_object_metadata object,
              active_object_origin origin,
              std::optional<rejected_object_reference> rejected);
  package_path path_;
  installed_object_metadata object_;
  active_object_origin origin_;
  std::optional<rejected_object_reference> rejected_;
};

} // namespace pkgstate
