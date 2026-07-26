// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate/owned_entry.h>

#include <tuple>
#include <utility>

#include <libpkgstate/error.h>

namespace pkgstate {
namespace {

void validate_kind(owned_object_kind kind)
{
  switch (kind)
  {
    case owned_object_kind::regular:
    case owned_object_kind::directory:
    case owned_object_kind::symlink:
    case owned_object_kind::fifo:
    case owned_object_kind::character_device:
    case owned_object_kind::block_device:
    case owned_object_kind::socket:
    case owned_object_kind::other:
      return;
  }
  throw state_error("invalid owned object kind");
}

void validate_origin(active_object_origin origin)
{
  switch (origin)
  {
    case active_object_origin::incoming_payload:
    case active_object_origin::retained_existing:
      return;
  }
  throw state_error("invalid active object origin");
}

void validate_side(rejected_object_side side)
{
  switch (side)
  {
    case rejected_object_side::incoming:
    case rejected_object_side::prior_installed:
      return;
  }
  throw state_error("invalid rejected object side");
}

bool text_safe(const std::string& value)
{
  if (value.empty())
    return false;
  for (const unsigned char byte : value)
  {
    if (byte == 0 || byte == '\n' || byte == '\r' || byte < 0x20 ||
        byte == 0x7f)
      return false;
  }
  return true;
}

void validate_object_shape(
    owned_object_kind kind,
    const std::optional<std::uint64_t>& size,
    const std::optional<installed_regular_content_identity>& regular_content,
    const std::optional<std::string>& symlink_target,
    const std::optional<installed_device_number>& device,
    const std::optional<package_path>& hardlink_anchor)
{
  const bool regular = kind == owned_object_kind::regular;
  const bool symlink = kind == owned_object_kind::symlink;
  const bool device_object =
      kind == owned_object_kind::character_device ||
      kind == owned_object_kind::block_device;

  if (regular != size.has_value() || regular != regular_content.has_value())
    throw state_error("regular installed object lacks size or content identity");
  if (!regular && hardlink_anchor.has_value())
    throw state_error("non-regular installed object has a hard-link anchor");
  if (symlink != symlink_target.has_value())
    throw state_error("installed symlink target shape is invalid");
  if (symlink_target && !text_safe(*symlink_target))
    throw state_error("installed symlink target is invalid");
  if (device_object != device.has_value())
    throw state_error("installed device-number shape is invalid");
}

} // namespace

installed_object_timestamp::installed_object_timestamp(
    std::int64_t seconds, std::uint32_t nanoseconds)
    : seconds_(seconds), nanoseconds_(nanoseconds)
{
  if (nanoseconds_ >= 1000000000U)
    throw state_error("installed object nanoseconds are out of range");
}
std::int64_t installed_object_timestamp::seconds() const noexcept
{ return seconds_; }
std::uint32_t installed_object_timestamp::nanoseconds() const noexcept
{ return nanoseconds_; }
bool operator==(const installed_object_timestamp& lhs,
                const installed_object_timestamp& rhs) noexcept
{ return std::tie(lhs.seconds_, lhs.nanoseconds_) ==
         std::tie(rhs.seconds_, rhs.nanoseconds_); }
bool operator!=(const installed_object_timestamp& lhs,
                const installed_object_timestamp& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const installed_object_timestamp& lhs,
               const installed_object_timestamp& rhs) noexcept
{ return std::tie(lhs.seconds_, lhs.nanoseconds_) <
         std::tie(rhs.seconds_, rhs.nanoseconds_); }

installed_device_number::installed_device_number(
    std::uint64_t major, std::uint64_t minor) noexcept
    : major_(major), minor_(minor)
{
}
std::uint64_t installed_device_number::major() const noexcept { return major_; }
std::uint64_t installed_device_number::minor() const noexcept { return minor_; }
bool operator==(const installed_device_number& lhs,
                const installed_device_number& rhs) noexcept
{ return std::tie(lhs.major_, lhs.minor_) == std::tie(rhs.major_, rhs.minor_); }
bool operator!=(const installed_device_number& lhs,
                const installed_device_number& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const installed_device_number& lhs,
               const installed_device_number& rhs) noexcept
{ return std::tie(lhs.major_, lhs.minor_) < std::tie(rhs.major_, rhs.minor_); }

installed_object_metadata::installed_object_metadata(
    owned_object_kind kind,
    std::uint32_t mode,
    std::uint64_t uid,
    std::uint64_t gid,
    installed_object_timestamp mtime,
    std::optional<std::uint64_t> size,
    std::optional<installed_regular_content_identity> regular_content,
    std::optional<std::string> symlink_target,
    std::optional<installed_device_number> device,
    std::optional<package_path> hardlink_anchor)
    : kind_(kind), mode_(mode), uid_(uid), gid_(gid),
      mtime_(std::move(mtime)), size_(std::move(size)),
      regular_content_(std::move(regular_content)),
      symlink_target_(std::move(symlink_target)), device_(std::move(device)),
      hardlink_anchor_(std::move(hardlink_anchor))
{
  validate_kind(kind_);
  if ((mode_ & ~07777U) != 0)
    throw state_error("installed object mode contains type or unknown bits");
  validate_object_shape(kind_, size_, regular_content_, symlink_target_,
                        device_, hardlink_anchor_);
}
owned_object_kind installed_object_metadata::kind() const noexcept
{ return kind_; }
std::uint32_t installed_object_metadata::mode() const noexcept { return mode_; }
std::uint64_t installed_object_metadata::uid() const noexcept { return uid_; }
std::uint64_t installed_object_metadata::gid() const noexcept { return gid_; }
const installed_object_timestamp& installed_object_metadata::mtime() const noexcept
{ return mtime_; }
const std::optional<std::uint64_t>& installed_object_metadata::size() const noexcept
{ return size_; }
const std::optional<installed_regular_content_identity>&
installed_object_metadata::regular_content() const noexcept
{ return regular_content_; }
const std::optional<std::string>&
installed_object_metadata::symlink_target() const noexcept
{ return symlink_target_; }
const std::optional<installed_device_number>&
installed_object_metadata::device() const noexcept
{ return device_; }
const std::optional<package_path>&
installed_object_metadata::hardlink_anchor() const noexcept
{ return hardlink_anchor_; }
bool operator==(const installed_object_metadata& lhs,
                const installed_object_metadata& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.mode_, lhs.uid_, lhs.gid_, lhs.mtime_,
                  lhs.size_, lhs.regular_content_, lhs.symlink_target_,
                  lhs.device_, lhs.hardlink_anchor_) ==
         std::tie(rhs.kind_, rhs.mode_, rhs.uid_, rhs.gid_, rhs.mtime_,
                  rhs.size_, rhs.regular_content_, rhs.symlink_target_,
                  rhs.device_, rhs.hardlink_anchor_);
}
bool operator!=(const installed_object_metadata& lhs,
                const installed_object_metadata& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const installed_object_metadata& lhs,
               const installed_object_metadata& rhs) noexcept
{
  return std::tie(lhs.kind_, lhs.mode_, lhs.uid_, lhs.gid_, lhs.mtime_,
                  lhs.size_, lhs.regular_content_, lhs.symlink_target_,
                  lhs.device_, lhs.hardlink_anchor_) <
         std::tie(rhs.kind_, rhs.mode_, rhs.uid_, rhs.gid_, rhs.mtime_,
                  rhs.size_, rhs.regular_content_, rhs.symlink_target_,
                  rhs.device_, rhs.hardlink_anchor_);
}

rejected_object_reference::rejected_object_reference(
    rejected_object_side side, rejected_object_identity identity)
    : side_(side), identity_(std::move(identity))
{
  validate_side(side_);
}
rejected_object_side rejected_object_reference::side() const noexcept
{ return side_; }
const rejected_object_identity&
rejected_object_reference::identity() const noexcept
{ return identity_; }
bool operator==(const rejected_object_reference& lhs,
                const rejected_object_reference& rhs) noexcept
{ return std::tie(lhs.side_, lhs.identity_) ==
         std::tie(rhs.side_, rhs.identity_); }
bool operator!=(const rejected_object_reference& lhs,
                const rejected_object_reference& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const rejected_object_reference& lhs,
               const rejected_object_reference& rhs) noexcept
{ return std::tie(lhs.side_, lhs.identity_) <
         std::tie(rhs.side_, rhs.identity_); }

owned_entry owned_entry::make(
    package_path path,
    installed_object_metadata object,
    active_object_origin origin,
    std::optional<rejected_object_reference> rejected)
{
  validate_origin(origin);
  return owned_entry(std::move(path), std::move(object), origin,
                     std::move(rejected));
}
owned_entry::owned_entry(
    package_path path, installed_object_metadata object,
    active_object_origin origin,
    std::optional<rejected_object_reference> rejected)
    : path_(std::move(path)), object_(std::move(object)), origin_(origin),
      rejected_(std::move(rejected))
{
}
const package_path& owned_entry::path() const noexcept { return path_; }
owned_object_kind owned_entry::kind() const noexcept { return object_.kind(); }
const installed_object_metadata& owned_entry::object() const noexcept
{ return object_; }
active_object_origin owned_entry::origin() const noexcept { return origin_; }
const std::optional<rejected_object_reference>&
owned_entry::rejected() const noexcept
{ return rejected_; }
bool operator==(const owned_entry& lhs, const owned_entry& rhs) noexcept
{
  return std::tie(lhs.path_, lhs.object_, lhs.origin_, lhs.rejected_) ==
         std::tie(rhs.path_, rhs.object_, rhs.origin_, rhs.rejected_);
}
bool operator!=(const owned_entry& lhs, const owned_entry& rhs) noexcept
{ return !(lhs == rhs); }
bool operator<(const owned_entry& lhs, const owned_entry& rhs) noexcept
{
  return std::tie(lhs.path_, lhs.object_, lhs.origin_, lhs.rejected_) <
         std::tie(rhs.path_, rhs.object_, rhs.origin_, rhs.rejected_);
}

} // namespace pkgstate
