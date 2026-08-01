// SPDX-FileCopyrightText: 2026 Alexandr Savca
// SPDX-License-Identifier: GPL-3.0-or-later

#include <libpkgstate-apply/state_projection.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <openssl/evp.h>

#include <libpkgplan/precondition.h>
#include <libpkgstate/package_path.h>

namespace pkgstate::apply_adapter {
namespace {

[[noreturn]] void refuse(application_state_projection_error_code code,
                         const char* message)
{
  throw application_state_projection_error(code, message);
}

template<typename Destination, typename Source>
Destination translate_identity(const Source& source)
{
  try
  {
    return Destination::parse(source.string());
  }
  catch (const std::exception& error)
  {
    throw application_state_projection_error(
        application_state_projection_error_code::identity_translation,
        std::string("application identity vocabulary rejected canonical state: ") +
            error.what());
  }
}

package_path translate_path(const pkgplan::package_path& source)
{
  try
  {
    return package_path::parse(source.string());
  }
  catch (const std::exception& error)
  {
    throw application_state_projection_error(
        application_state_projection_error_code::path_translation,
        std::string("state path vocabulary rejected application path: ") +
            error.what());
  }
}

const pkgplan::operation_preconditions& preconditions(
    const pkgapply::package_application_request& request)
{
  if (const auto* install = request.installation())
    return install->plan().preconditions();
  if (const auto* upgrade = request.upgrade())
    return upgrade->plan().preconditions();
  if (const auto* removal = request.removal())
    return removal->plan().preconditions();
  refuse(application_state_projection_error_code::request_binding_mismatch,
         "application request has no operation body");
}

class evidence_record final {
public:
  explicit evidence_record(std::string_view domain)
  {
    static constexpr std::array<std::uint8_t, 9> magic = {
        'p', 'k', 'g', 's', 't', 'a', 't', 'e', 0,
    };
    append_raw(magic.data(), magic.size());
    append_u16(1);
    append_bytes(domain);
  }

  void append_u16(std::uint16_t value)
  {
    bytes_.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes_.push_back(static_cast<std::uint8_t>(value & 0xffU));
  }

  void append_u64(std::uint64_t value)
  {
    for (int shift = 56; shift >= 0; shift -= 8)
    {
      bytes_.push_back(static_cast<std::uint8_t>(
          (value >> static_cast<unsigned>(shift)) & 0xffU));
    }
  }

  void append_bytes(std::string_view value)
  {
    append_u64(static_cast<std::uint64_t>(value.size()));
    append_raw(reinterpret_cast<const std::uint8_t*>(value.data()),
               value.size());
  }

  [[nodiscard]] std::array<std::uint8_t, 32> sha256() const
  {
    using context_ptr =
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
    context_ptr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!context)
      refuse(application_state_projection_error_code::evidence_construction,
             "could not allocate application state evidence digest");

    if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1 ||
        EVP_DigestUpdate(context.get(), bytes_.data(), bytes_.size()) != 1)
    {
      refuse(application_state_projection_error_code::evidence_construction,
             "could not hash application state projection evidence");
    }

    std::array<std::uint8_t, 32> result{};
    unsigned int size = 0;
    if (EVP_DigestFinal_ex(context.get(), result.data(), &size) != 1 ||
        size != result.size())
    {
      refuse(application_state_projection_error_code::evidence_construction,
             "could not finalize application state projection evidence");
    }
    return result;
  }

private:
  void append_raw(const std::uint8_t* data, std::size_t size)
  {
    if (size != 0)
      bytes_.insert(bytes_.end(), data, data + size);
  }

  std::vector<std::uint8_t> bytes_;
};

pkgapply::state_projection_evidence_identity identify_evidence(
    const pkgapply::package_application_request& request,
    const pkgapply::target_mutation_lease& lease,
    const snapshot& state,
    const std::vector<pkgapply::projected_path_owners>& paths)
{
  evidence_record record("pkgstate/application-state-projection-evidence/1");
  record.append_u16(application_state_projection_evidence_schema_version);
  record.append_bytes(request.identity().string());
  record.append_bytes(request.target().identity().string());
  record.append_bytes(lease.identity().string());
  record.append_bytes(state.target_binding().identity().string());
  record.append_bytes(state.identity().string());
  record.append_bytes(state.ownership_identity().string());
  record.append_u64(static_cast<std::uint64_t>(paths.size()));
  for (const auto& item : paths)
  {
    record.append_bytes(item.path().string());
    record.append_u64(static_cast<std::uint64_t>(item.owners().size()));
    for (const auto& owner : item.owners())
      record.append_bytes(owner.string());
  }
  const auto digest = record.sha256();
  static constexpr char hexadecimal[] = "0123456789abcdef";
  std::string value = "v1:sha256:";
  value.reserve(value.size() + digest.size() * 2);
  for (const std::uint8_t byte : digest)
  {
    value.push_back(hexadecimal[(byte >> 4U) & 0x0fU]);
    value.push_back(hexadecimal[byte & 0x0fU]);
  }
  return pkgapply::state_projection_evidence_identity::parse(value);
}

void validate_lease_before_read(
    const pkgapply::package_application_request& request,
    const pkgapply::target_mutation_lease& lease)
{
  if (!lease.held())
    refuse(application_state_projection_error_code::lease_not_held,
           "target mutation lease is not held before state read");
  if (lease.target() != request.target().identity())
    refuse(application_state_projection_error_code::lease_target_mismatch,
           "target mutation lease belongs to another application target");
  if (lease.exclusion_domain() != request.target().mutation_exclusion_domain())
    refuse(application_state_projection_error_code::lease_domain_mismatch,
           "target mutation lease belongs to another exclusion domain");
}

void require_lease_still_held(const pkgapply::target_mutation_lease& lease)
{
  if (!lease.held())
    refuse(application_state_projection_error_code::lease_lost,
           "target mutation lease was lost during state projection");
}

} // namespace

application_state_projection_error::application_state_projection_error(
    application_state_projection_error_code code,
    std::string message)
    : std::invalid_argument(std::move(message)), code_(code)
{
}

application_state_projection_error_code
application_state_projection_error::code() const noexcept
{
  return code_;
}

lease_bound_application_state::lease_bound_application_state(
    snapshot state,
    pkgapply::lease_bound_state_projection projection)
    : state_(std::move(state)), projection_(std::move(projection))
{
}

const snapshot& lease_bound_application_state::state() const noexcept
{
  return state_;
}

const pkgapply::lease_bound_state_projection&
lease_bound_application_state::projection() const noexcept
{
  return projection_;
}

lease_bound_application_state read_application_state(
    const pkgapply::package_application_request& request,
    const pkgapply::target_mutation_lease& lease,
    const canonical_store& store)
{
  validate_lease_before_read(request, lease);
  snapshot current = store.read();
  require_lease_still_held(lease);

  const pkgplan::operation_preconditions& required = preconditions(request);
  if (required.target() != request.target().target())
  {
    refuse(application_state_projection_error_code::request_binding_mismatch,
           "application request target differs from accepted plan");
  }

  const state_target_binding& binding = current.target_binding();
  if (binding.managed_target().string() !=
          request.target().managed_target().string() ||
      binding.root_view().string() != request.target().root_view().string())
  {
    refuse(application_state_projection_error_code::target_binding_mismatch,
           "canonical state belongs to another application target");
  }

  const auto planner_snapshot =
      translate_identity<pkgplan::installed_state_snapshot_identity>(
          current.identity());
  if (planner_snapshot != required.installed_snapshot())
  {
    refuse(application_state_projection_error_code::expected_snapshot_mismatch,
           "canonical state differs from the accepted installed snapshot");
  }

  const auto planner_ownership =
      translate_identity<pkgplan::ownership_inventory_identity>(
          current.ownership_identity());
  if (planner_ownership != required.ownership_inventory())
  {
    refuse(application_state_projection_error_code::ownership_inventory_mismatch,
           "canonical ownership differs from the accepted inventory");
  }

  std::vector<pkgapply::projected_path_owners> paths;
  paths.reserve(required.paths().size());
  for (const auto& required_path : required.paths())
  {
    std::vector<pkgplan::installed_package_identity> owners;
    for (const installed_package* owner :
         current.owners(translate_path(required_path.path())))
    {
      owners.push_back(
          translate_identity<pkgplan::installed_package_identity>(
              owner->identity()));
    }
    std::sort(owners.begin(), owners.end());
    if (owners != required_path.owners())
    {
      refuse(application_state_projection_error_code::path_owners_mismatch,
             "canonical path owners differ from accepted plan");
    }
    paths.emplace_back(required_path.path(), std::move(owners));
  }

  const auto evidence = identify_evidence(request, lease, current, paths);
  auto projection = pkgapply::lease_bound_state_projection::make(
      lease.identity(), planner_snapshot, planner_ownership,
      pkgapply::state_projection_completeness::complete,
      std::move(paths), evidence);

  require_lease_still_held(lease);
  return lease_bound_application_state(
      std::move(current), std::move(projection));
}

} // namespace pkgstate::apply_adapter
