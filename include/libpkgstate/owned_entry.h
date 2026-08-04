// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

/*!
 * \file owned_entry.h
 * \brief Completed installed ownership, object, and reconciliation facts.
 */
#pragma once

#include <libpkgstate/export.h>

#include <cstdint>
#include <optional>
#include <string>

#include <libpkgstate/digest.h>
#include <libpkgstate/package_path.h>

namespace pkgstate {

/*! \brief Durable filesystem object classes represented by installed state. */
enum class owned_object_kind : std::uint8_t {
  regular = 1,         //!< Regular file with size and content identity.
  directory = 2,       //!< Directory.
  symlink = 3,         //!< Symbolic link with exact target text.
  fifo = 4,            //!< Named pipe.
  character_device = 5,//!< Character device with major and minor numbers.
  block_device = 6,    //!< Block device with major and minor numbers.
  socket = 7,          //!< Socket filesystem node.
  other = 8,           //!< Valid object outside the explicit classes above.
};

/*! \brief Authority for the active object retained at an owned path. */
enum class active_object_origin : std::uint8_t {
  incoming_payload = 1, //!< Active object came from the incoming package image.
  retained_existing = 2,//!< Active object was retained from prior target state.
};

/*! \brief Side whose non-active object was retained as rejected evidence. */
enum class rejected_object_side : std::uint8_t {
  incoming = 1,       //!< Incoming payload object was rejected.
  prior_installed = 2,//!< Previously installed object was displaced or rejected.
};

/*! \brief Exact normalized timestamp recorded for an installed object. */
class PKGSTATE_API installed_object_timestamp final {
public:
  /*!
   * \brief Construct a seconds-plus-nanoseconds timestamp.
   * \param seconds Signed seconds component.
   * \param nanoseconds Nanoseconds in the range 0 through 999999999.
   * \throws state_error when \p nanoseconds is out of range.
   */
  installed_object_timestamp(std::int64_t seconds, std::uint32_t nanoseconds);

  /*! \brief Return the signed seconds component. */
  [[nodiscard]] std::int64_t seconds() const noexcept;
  /*! \brief Return the normalized nanoseconds component. */
  [[nodiscard]] std::uint32_t nanoseconds() const noexcept;

  /*! \brief Compare complete timestamps for equality. */
  friend PKGSTATE_API bool operator==(const installed_object_timestamp& lhs,
                                      const installed_object_timestamp& rhs) noexcept;
  /*! \brief Compare complete timestamps for inequality. */
  friend PKGSTATE_API bool operator!=(const installed_object_timestamp& lhs,
                                      const installed_object_timestamp& rhs) noexcept;
  /*! \brief Order timestamps by seconds and nanoseconds. */
  friend PKGSTATE_API bool operator<(const installed_object_timestamp& lhs,
                                     const installed_object_timestamp& rhs) noexcept;

private:
  std::int64_t seconds_;
  std::uint32_t nanoseconds_;
};

/*! \brief Portable major and minor components of one installed device node. */
class PKGSTATE_API installed_device_number final {
public:
  /*! \brief Construct exact device-number components. */
  installed_device_number(std::uint64_t major, std::uint64_t minor) noexcept;

  /*! \brief Return the major device number. */
  [[nodiscard]] std::uint64_t major() const noexcept;
  /*! \brief Return the minor device number. */
  [[nodiscard]] std::uint64_t minor() const noexcept;

  /*! \brief Compare complete device numbers for equality. */
  friend PKGSTATE_API bool operator==(const installed_device_number& lhs,
                                      const installed_device_number& rhs) noexcept;
  /*! \brief Compare complete device numbers for inequality. */
  friend PKGSTATE_API bool operator!=(const installed_device_number& lhs,
                                      const installed_device_number& rhs) noexcept;
  /*! \brief Order device numbers by major and minor components. */
  friend PKGSTATE_API bool operator<(const installed_device_number& lhs,
                                     const installed_device_number& rhs) noexcept;

private:
  std::uint64_t major_;
  std::uint64_t minor_;
};

/*!
 * \brief Complete recorded metadata for one installed active object.
 *
 * Shape is kind-dependent. Regular objects require size and content identity;
 * symbolic links require target text; device nodes require device numbers.
 * Only regular objects may carry a hard-link anchor.
 */
class PKGSTATE_API installed_object_metadata final {
public:
  /*!
   * \brief Validate and construct installed object metadata.
   * \param kind Durable object class.
   * \param mode Permission and special bits only; filesystem type bits are
   * rejected.
   * \param uid Numeric owner identifier.
   * \param gid Numeric group identifier.
   * \param mtime Exact recorded modification time.
   * \param size Required exactly for regular objects.
   * \param regular_content Required exactly for regular objects.
   * \param symlink_target Required exactly for symbolic links.
   * \param device Required exactly for character and block devices.
   * \param hardlink_anchor Optional canonical anchor for a regular hard link.
   * \throws state_error when the kind, mode, or kind-dependent shape is
   * invalid, or when symbolic-link target text is unsafe.
   */
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

  /*! \brief Return the durable object class. */
  [[nodiscard]] owned_object_kind kind() const noexcept;
  /*! \brief Return permission and special mode bits. */
  [[nodiscard]] std::uint32_t mode() const noexcept;
  /*! \brief Return the numeric owner identifier. */
  [[nodiscard]] std::uint64_t uid() const noexcept;
  /*! \brief Return the numeric group identifier. */
  [[nodiscard]] std::uint64_t gid() const noexcept;
  /*! \brief Return the exact recorded modification time. */
  [[nodiscard]] const installed_object_timestamp& mtime() const noexcept;
  /*! \brief Return regular-object size, or an empty value otherwise. */
  [[nodiscard]] const std::optional<std::uint64_t>& size() const noexcept;
  /*! \brief Return regular-content identity, or an empty value otherwise. */
  [[nodiscard]] const std::optional<installed_regular_content_identity>&
  regular_content() const noexcept;
  /*! \brief Return symbolic-link target, or an empty value otherwise. */
  [[nodiscard]] const std::optional<std::string>&
  symlink_target() const noexcept;
  /*! \brief Return device-number components, or an empty value otherwise. */
  [[nodiscard]] const std::optional<installed_device_number>&
  device() const noexcept;
  /*! \brief Return regular hard-link anchor, when this object is a link peer. */
  [[nodiscard]] const std::optional<package_path>&
  hardlink_anchor() const noexcept;

  /*! \brief Compare complete installed object metadata for equality. */
  friend PKGSTATE_API bool operator==(const installed_object_metadata& lhs,
                                      const installed_object_metadata& rhs) noexcept;
  /*! \brief Compare complete installed object metadata for inequality. */
  friend PKGSTATE_API bool operator!=(const installed_object_metadata& lhs,
                                      const installed_object_metadata& rhs) noexcept;
  /*! \brief Order complete installed object metadata canonically. */
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

/*! \brief Exact rejected-object evidence retained beside an active object. */
class PKGSTATE_API rejected_object_reference final {
public:
  /*!
   * \brief Construct a side-qualified rejected-object reference.
   * \param side Side whose object was rejected.
   * \param identity Application-owned identity of the rejected object.
   * \throws state_error when \p side is not a recognized enumerator.
   */
  rejected_object_reference(rejected_object_side side,
                            rejected_object_identity identity);

  /*! \brief Return the rejected side. */
  [[nodiscard]] rejected_object_side side() const noexcept;
  /*! \brief Return the exact rejected-object identity. */
  [[nodiscard]] const rejected_object_identity& identity() const noexcept;

  /*! \brief Compare complete rejected references for equality. */
  friend PKGSTATE_API bool operator==(const rejected_object_reference& lhs,
                                      const rejected_object_reference& rhs) noexcept;
  /*! \brief Compare complete rejected references for inequality. */
  friend PKGSTATE_API bool operator!=(const rejected_object_reference& lhs,
                                      const rejected_object_reference& rhs) noexcept;
  /*! \brief Order rejected references by side and identity. */
  friend PKGSTATE_API bool operator<(const rejected_object_reference& lhs,
                                     const rejected_object_reference& rhs) noexcept;

private:
  rejected_object_side side_;
  rejected_object_identity identity_;
};

/*!
 * \brief One canonical package-owned path and its completed active truth.
 *
 * The entry records what is active, why that object is active, and optional
 * evidence for an object rejected during application. It does not encode
 * package-level source, build, or application policy.
 */
class PKGSTATE_API owned_entry final {
public:
  /*!
   * \brief Validate and construct one owned manifest entry.
   * \param path Canonical absolute package path.
   * \param object Complete active object metadata.
   * \param origin Authority for the active object.
   * \param rejected Optional side-qualified rejected-object evidence.
   * \return Immutable owned entry.
   * \throws state_error when \p origin is not a recognized enumerator.
   */
  [[nodiscard]] static owned_entry make(
      package_path path,
      installed_object_metadata object,
      active_object_origin origin,
      std::optional<rejected_object_reference> rejected = std::nullopt);

  /*! \brief Return the canonical owned path. */
  [[nodiscard]] const package_path& path() const noexcept;
  /*! \brief Return the active object class. */
  [[nodiscard]] owned_object_kind kind() const noexcept;
  /*! \brief Return complete active object metadata. */
  [[nodiscard]] const installed_object_metadata& object() const noexcept;
  /*! \brief Return authority for the active object. */
  [[nodiscard]] active_object_origin origin() const noexcept;
  /*! \brief Return optional rejected-object evidence. */
  [[nodiscard]] const std::optional<rejected_object_reference>&
  rejected() const noexcept;

  /*! \brief Compare complete owned entries for equality. */
  friend PKGSTATE_API bool operator==(const owned_entry& lhs,
                                      const owned_entry& rhs) noexcept;
  /*! \brief Compare complete owned entries for inequality. */
  friend PKGSTATE_API bool operator!=(const owned_entry& lhs,
                                      const owned_entry& rhs) noexcept;
  /*! \brief Order owned entries canonically, beginning with path. */
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
