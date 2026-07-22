// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include "generation_codec.h"

#include <libpkgstate/canonical_generation_store.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <libpkgstate/error.h>
#include <libpkgstate/installed_control.h>
#include <libpkgstate/installed_package.h>
#include <libpkgstate/owned_entry.h>
#include <libpkgstate/package_path.h>
#include <libpkgstate/package_release.h>

namespace pkgstate::detail {
namespace {

constexpr std::array<std::uint8_t, 17> binding_magic = {
    'p', 'k', 'g', 's', 't', 'a', 't', 'e', '-',
    'b', 'i', 'n', 'd', 'i', 'n', 'g', 0,
};
constexpr std::array<std::uint8_t, 18> snapshot_magic = {
    'p', 'k', 'g', 's', 't', 'a', 't', 'e', '-',
    's', 'n', 'a', 'p', 's', 'h', 'o', 't', 0,
};

class writer final {
public:
  template<std::size_t Size>
  void raw(const std::array<std::uint8_t, Size>& value)
  {
    bytes_.insert(bytes_.end(), value.begin(), value.end());
  }

  void u8(std::uint8_t value)
  {
    bytes_.push_back(value);
  }

  void u16(std::uint16_t value)
  {
    bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes_.push_back(static_cast<std::uint8_t>(value & 0xffU));
  }

  void u64(std::uint64_t value)
  {
    for (int shift = 56; shift >= 0; shift -= 8)
    {
      bytes_.push_back(static_cast<std::uint8_t>(
          (value >> static_cast<unsigned>(shift)) & 0xffU));
    }
  }

  void bytes(std::string_view value)
  {
    u64(static_cast<std::uint64_t>(value.size()));
    if (!value.empty())
    {
      const auto* first =
          reinterpret_cast<const std::uint8_t*>(value.data());
      bytes_.insert(bytes_.end(), first, first + value.size());
    }
  }

  template<typename Identity>
  void digest(const Identity& identity)
  {
    bytes(identity.string());
  }

  [[nodiscard]] std::vector<std::uint8_t> take()
  {
    return std::move(bytes_);
  }

private:
  std::vector<std::uint8_t> bytes_;
};

class reader final {
public:
  explicit reader(std::string_view bytes)
      : bytes_(bytes)
  {
  }

  template<std::size_t Size>
  void expect(const std::array<std::uint8_t, Size>& expected,
              const char* label)
  {
    require(Size, label);
    const auto* actual = reinterpret_cast<const std::uint8_t*>(
        bytes_.data() + position_);
    if (!std::equal(expected.begin(), expected.end(), actual))
      fail(std::string("invalid ") + label);
    position_ += Size;
  }

  [[nodiscard]] std::uint8_t u8(const char* label)
  {
    require(1, label);
    return static_cast<std::uint8_t>(bytes_[position_++]);
  }

  [[nodiscard]] std::uint16_t u16(const char* label)
  {
    require(2, label);
    const auto first = static_cast<std::uint8_t>(bytes_[position_]);
    const auto second = static_cast<std::uint8_t>(bytes_[position_ + 1]);
    position_ += 2;
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(first) << 8U) | second);
  }

  [[nodiscard]] std::uint64_t u64(const char* label)
  {
    require(8, label);
    std::uint64_t result = 0;
    for (std::size_t index = 0; index < 8; ++index)
    {
      result = (result << 8U) |
               static_cast<std::uint8_t>(bytes_[position_ + index]);
    }
    position_ += 8;
    return result;
  }

  [[nodiscard]] std::string bytes(const char* label)
  {
    const std::uint64_t size64 = u64(label);
    if (size64 > static_cast<std::uint64_t>(remaining()))
      fail(std::string(label) + " length exceeds generation record");
    if (size64 > static_cast<std::uint64_t>(
                     std::numeric_limits<std::size_t>::max()))
      fail(std::string(label) + " length is not representable");

    const std::size_t size = static_cast<std::size_t>(size64);
    std::string result(bytes_.substr(position_, size));
    position_ += size;
    return result;
  }

  [[nodiscard]] std::size_t count(const char* label)
  {
    const std::uint64_t value = u64(label);
    if (value > static_cast<std::uint64_t>(remaining()))
      fail(std::string(label) + " is not plausible for remaining bytes");
    if (value > static_cast<std::uint64_t>(
                    std::numeric_limits<std::size_t>::max()))
      fail(std::string(label) + " is not representable");
    return static_cast<std::size_t>(value);
  }

  void finish()
  {
    if (position_ != bytes_.size())
      fail("trailing bytes in canonical generation record");
  }

private:
  [[nodiscard]] std::size_t remaining() const noexcept
  {
    return bytes_.size() - position_;
  }

  void require(std::size_t size, const char* label)
  {
    if (size > remaining())
      fail(std::string("truncated ") + label);
  }

  [[noreturn]] static void fail(const std::string& message)
  {
    throw store_error(message);
  }

  std::string_view bytes_;
  std::size_t position_ = 0;
};

[[nodiscard]] std::uint8_t
encode_fact_state(installed_control_fact_state value)
{
  switch (value)
  {
    case installed_control_fact_state::recorded_at_installation:
      return 1;
    case installed_control_fact_state::recorded_in_compatibility_storage:
      return 2;
    case installed_control_fact_state::supplied_by_migration:
      return 3;
    case installed_control_fact_state::historically_unavailable:
      return 4;
  }
  throw state_error("invalid installed-control fact state");
}

[[nodiscard]] installed_control_fact_state
decode_fact_state(std::uint8_t value)
{
  switch (value)
  {
    case 1:
      return installed_control_fact_state::recorded_at_installation;
    case 2:
      return installed_control_fact_state::recorded_in_compatibility_storage;
    case 3:
      return installed_control_fact_state::supplied_by_migration;
    case 4:
      return installed_control_fact_state::historically_unavailable;
  }
  throw store_error("invalid installed-control fact-state value");
}

[[nodiscard]] std::uint8_t
encode_lifecycle_phase(removal_lifecycle_phase value)
{
  switch (value)
  {
    case removal_lifecycle_phase::pre_remove:
      return 1;
    case removal_lifecycle_phase::post_remove:
      return 2;
  }
  throw state_error("invalid removal lifecycle phase");
}

[[nodiscard]] removal_lifecycle_phase
decode_lifecycle_phase(std::uint8_t value)
{
  switch (value)
  {
    case 1:
      return removal_lifecycle_phase::pre_remove;
    case 2:
      return removal_lifecycle_phase::post_remove;
  }
  throw store_error("invalid removal lifecycle phase value");
}

[[nodiscard]] std::uint8_t
encode_provenance_kind(control_provenance_kind value)
{
  switch (value)
  {
    case control_provenance_kind::candidate_control:
      return 1;
    case control_provenance_kind::artifact:
      return 2;
    case control_provenance_kind::artifact_manifest:
      return 3;
    case control_provenance_kind::application_evidence:
      return 4;
    case control_provenance_kind::transaction_evidence:
      return 5;
    case control_provenance_kind::legacy_package_observation:
      return 6;
    case control_provenance_kind::legacy_snapshot_observation:
      return 7;
    case control_provenance_kind::legacy_migration_evidence:
      return 8;
  }
  throw state_error("invalid installed-control provenance kind");
}

[[nodiscard]] control_provenance_kind
decode_provenance_kind(std::uint8_t value)
{
  switch (value)
  {
    case 1:
      return control_provenance_kind::candidate_control;
    case 2:
      return control_provenance_kind::artifact;
    case 3:
      return control_provenance_kind::artifact_manifest;
    case 4:
      return control_provenance_kind::application_evidence;
    case 5:
      return control_provenance_kind::transaction_evidence;
    case 6:
      return control_provenance_kind::legacy_package_observation;
    case 7:
      return control_provenance_kind::legacy_snapshot_observation;
    case 8:
      return control_provenance_kind::legacy_migration_evidence;
  }
  throw store_error("invalid installed-control provenance-kind value");
}

[[nodiscard]] std::uint8_t
encode_entry_type(owned_entry_type value)
{
  switch (value)
  {
    case owned_entry_type::non_directory:
      return 1;
    case owned_entry_type::directory:
      return 2;
  }
  throw state_error("invalid ownership entry type");
}

[[nodiscard]] owned_entry_type
decode_entry_type(std::uint8_t value)
{
  switch (value)
  {
    case 1:
      return owned_entry_type::non_directory;
    case 2:
      return owned_entry_type::directory;
  }
  throw store_error("invalid ownership entry-type value");
}

void
encode_binding_fields(writer& output, const state_target_binding& binding)
{
  output.digest(binding.managed_target());
  output.digest(binding.state_store());
  output.digest(binding.root_view());
  output.digest(binding.state_backend());
  output.digest(binding.publication_domain());
}

[[nodiscard]] state_target_binding
decode_binding_fields(reader& input)
{
  const managed_target_identity managed_target =
      managed_target_identity::parse(input.bytes("managed-target identity"));
  const state_store_identity state_store =
      state_store_identity::parse(input.bytes("state-store identity"));
  const root_view_identity root_view =
      root_view_identity::parse(input.bytes("root-view identity"));
  const state_backend_identity state_backend =
      state_backend_identity::parse(input.bytes("state-backend identity"));
  const publication_domain_identity publication_domain =
      publication_domain_identity::parse(
          input.bytes("publication-domain identity"));
  return state_target_binding::make(managed_target,
                                    state_store,
                                    root_view,
                                    state_backend,
                                    publication_domain);
}

void
encode_release(writer& output, const package_release& release)
{
  output.bytes(release.name());
  output.bytes(release.version());
  output.bytes(release.release());
}

[[nodiscard]] package_release
decode_release(reader& input)
{
  const std::string name = input.bytes("package name");
  const std::string version = input.bytes("package version");
  const std::string release = input.bytes("package release");
  return package_release::make(name, version, release);
}

void
encode_control(writer& output, const installed_control& control)
{
  const installed_control_completeness& completeness = control.completeness();
  output.u8(encode_fact_state(completeness.runtime_dependencies));
  output.u8(encode_fact_state(completeness.removal_lifecycle));
  output.u8(encode_fact_state(completeness.target_profile));
  output.u8(encode_fact_state(completeness.provenance));

  output.u64(control.runtime_dependencies().size());
  for (const runtime_dependency_declaration& dependency :
       control.runtime_dependencies())
  {
    output.bytes(dependency.expression());
  }

  output.u64(control.removal_lifecycle().size());
  for (const removal_lifecycle_declaration& declaration :
       control.removal_lifecycle())
  {
    output.u8(encode_lifecycle_phase(declaration.phase()));
    output.bytes(declaration.format());
    output.bytes(declaration.material());
  }

  output.u64(control.target_profile().size());
  for (const target_profile_fact& fact : control.target_profile())
  {
    output.bytes(fact.name());
    output.bytes(fact.value());
  }

  output.u64(control.provenance().size());
  for (const control_provenance& provenance : control.provenance())
  {
    output.u8(encode_provenance_kind(provenance.kind()));
    output.bytes(provenance.identity());
  }
}

[[nodiscard]] installed_control
decode_control(reader& input, const package_release& release)
{
  installed_control_completeness completeness;
  completeness.runtime_dependencies =
      decode_fact_state(input.u8("runtime-dependency completeness"));
  completeness.removal_lifecycle =
      decode_fact_state(input.u8("removal-lifecycle completeness"));
  completeness.target_profile =
      decode_fact_state(input.u8("target-profile completeness"));
  completeness.provenance =
      decode_fact_state(input.u8("provenance completeness"));

  std::vector<runtime_dependency_declaration> dependencies;
  const std::size_t dependency_count =
      input.count("runtime-dependency count");
  dependencies.reserve(dependency_count);
  for (std::size_t index = 0; index < dependency_count; ++index)
  {
    dependencies.push_back(runtime_dependency_declaration::make(
        input.bytes("runtime-dependency declaration")));
  }

  std::vector<removal_lifecycle_declaration> lifecycle;
  const std::size_t lifecycle_count = input.count("removal-lifecycle count");
  lifecycle.reserve(lifecycle_count);
  for (std::size_t index = 0; index < lifecycle_count; ++index)
  {
    const removal_lifecycle_phase phase =
        decode_lifecycle_phase(input.u8("removal-lifecycle phase"));
    const std::string format = input.bytes("removal-lifecycle format");
    std::string material = input.bytes("removal-lifecycle material");
    lifecycle.push_back(removal_lifecycle_declaration::make(
        phase, format, std::move(material)));
  }

  std::vector<target_profile_fact> profile;
  const std::size_t profile_count = input.count("target-profile count");
  profile.reserve(profile_count);
  for (std::size_t index = 0; index < profile_count; ++index)
  {
    const std::string name = input.bytes("target-profile name");
    const std::string value = input.bytes("target-profile value");
    profile.push_back(target_profile_fact::make(name, value));
  }

  std::vector<control_provenance> provenance;
  const std::size_t provenance_count = input.count("provenance count");
  provenance.reserve(provenance_count);
  for (std::size_t index = 0; index < provenance_count; ++index)
  {
    const control_provenance_kind kind =
        decode_provenance_kind(input.u8("provenance kind"));
    const std::string identity = input.bytes("provenance identity");
    provenance.push_back(control_provenance::make(kind, identity));
  }

  return installed_control::make(release,
                                 completeness,
                                 std::move(dependencies),
                                 std::move(lifecycle),
                                 std::move(profile),
                                 std::move(provenance));
}

[[nodiscard]] std::string_view
as_string_view(const std::vector<std::uint8_t>& bytes)
{
  return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

void
require_canonical_encoding(const std::vector<std::uint8_t>& canonical,
                           std::string_view original,
                           const char* label)
{
  if (as_string_view(canonical) != original)
    throw store_error(std::string(label) + " is not canonically encoded");
}

} // namespace

std::vector<std::uint8_t>
encode_generation_binding(const state_target_binding& binding)
{
  writer output;
  output.raw(binding_magic);
  output.u16(canonical_generation_storage_version);
  encode_binding_fields(output, binding);
  return output.take();
}

state_target_binding
decode_generation_binding(std::string_view bytes)
{
  try
  {
    reader input(bytes);
    input.expect(binding_magic, "canonical binding magic");
    if (input.u16("canonical binding version") !=
        canonical_generation_storage_version)
      throw store_error("unsupported canonical binding encoding version");
    state_target_binding result = decode_binding_fields(input);
    input.finish();
    require_canonical_encoding(encode_generation_binding(result),
                               bytes,
                               "canonical binding record");
    return result;
  }
  catch (const store_error&)
  {
    throw;
  }
  catch (const error& failure)
  {
    throw store_error(std::string("invalid canonical binding record: ") +
                      failure.what());
  }
}

std::vector<std::uint8_t>
encode_generation_snapshot(const snapshot& value)
{
  writer output;
  output.raw(snapshot_magic);
  output.u16(canonical_generation_storage_version);
  output.u16(value.schema_version());
  encode_binding_fields(output, value.target_binding());
  output.u64(value.packages().size());

  for (const installed_package& package : value.packages())
  {
    encode_release(output, package.release());
    encode_control(output, package.control());
    output.u64(package.manifest().size());
    for (const owned_entry& entry : package.manifest())
    {
      output.bytes(entry.path.string());
      output.u8(encode_entry_type(entry.type));
    }
  }

  return output.take();
}

snapshot
decode_generation_snapshot(std::string_view bytes)
{
  try
  {
    reader input(bytes);
    input.expect(snapshot_magic, "canonical generation magic");
    if (input.u16("canonical generation version") !=
        canonical_generation_storage_version)
    {
      throw store_error("unsupported canonical generation encoding version");
    }
    if (input.u16("installed-state schema version") !=
        installed_state_schema_version)
    {
      throw store_error("unsupported installed-state snapshot schema version");
    }

    state_target_binding binding = decode_binding_fields(input);
    std::vector<installed_package> packages;
    const std::size_t package_count = input.count("installed-package count");
    packages.reserve(package_count);

    for (std::size_t index = 0; index < package_count; ++index)
    {
      package_release release = decode_release(input);
      installed_control control = decode_control(input, release);
      std::vector<owned_entry> manifest;
      const std::size_t manifest_count =
          input.count("ownership manifest count");
      manifest.reserve(manifest_count);
      for (std::size_t entry = 0; entry < manifest_count; ++entry)
      {
        manifest.push_back({
            package_path::parse(input.bytes("owned package path")),
            decode_entry_type(input.u8("owned entry type")),
        });
      }

      packages.push_back(installed_package::make(
          release, std::move(control), binding, std::move(manifest)));
    }

    input.finish();
    snapshot result = snapshot::make(std::move(binding), std::move(packages));
    require_canonical_encoding(encode_generation_snapshot(result),
                               bytes,
                               "canonical generation snapshot");
    return result;
  }
  catch (const store_error&)
  {
    throw;
  }
  catch (const error& failure)
  {
    throw store_error(std::string("invalid canonical generation snapshot: ") +
                      failure.what());
  }
}

} // namespace pkgstate::detail
